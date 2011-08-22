#include "cppodbc.h"

using namespace cppodbc;

// ======================================================================
// Implementation of class ODBCException
// ======================================================================

ODBCException::ODBCException(int nError, const TCHAR *sqlState, const TCHAR *errorMesg)
{
    this->nativeError = nError;
    _tcsncpy_s(this->sqlState, sqlState, _TRUNCATE);
    _tcsncpy_s(this->errorMesg, errorMesg, _TRUNCATE);
}

ODBCException::~ODBCException()
{
}

// ======================================================================
// Implementation of class Handler
// ======================================================================

/**
 * Constructor of Handler class.
 * This allocate a handle by SQLAllocHandle.
 * The type of handle is distinguished by hType.
 */
Handler::Handler(SQLSMALLINT hType, SQLHANDLE parent)
{
    this->hType = hType;
    process(SQLAllocHandle(hType, parent, &handle));
};

/**
 * Destructor of Handler class.
 * This makes the instance be free by SQLFreeHandle, corresponding to
 * the constructor.
 */
Handler::~Handler()
{
    retcode = SQLFreeHandle(hType, handle);
}

/**
 * Checks every ODBC API call and throws an exception if necessary.
 */
SQLRETURN Handler::process(SQLRETURN rc)
{
    retcode = rc;
    //TODO: Need to clarify when messages are available, even in non-error situation.
    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        break;
    case SQL_ERROR:
    case SQL_INVALID_HANDLE:
        throw makeException();
    default:
        break;
    }
    return rc;
}

ODBCException *Handler::makeException()
{
    //BUG: Seems to work in only MBCS mode, not in UNICODE mode.
    // Maybe http://stackoverflow.com/questions/829311/sqlgetdiagrec-causes-crash-in-unicode-release-build
    char        sqlState[EX_SQLSTATE_LEN];
    SQLINTEGER  nativeError;
    char        errorMesg[EX_ERRMESG_LEN];
    SQLSMALLINT errorMesgLen;
    retcode = SQLGetDiagRecA(hType, handle, 1, (SQLCHAR *) sqlState, &nativeError,
        (SQLCHAR *) errorMesg, sizeof(errorMesg), &errorMesgLen);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
    {
    //WORKAROUND: force to use MBCS even in UNICODE mode.
#ifdef UNICODE
        wchar_t wSqlState[EX_SQLSTATE_LEN];
        wchar_t wErrorMesg[EX_ERRMESG_LEN];
        size_t dummy;
        mbstowcs_s(&dummy, wSqlState, sqlState, _TRUNCATE);
        if (errorMesgLen < EX_ERRMESG_LEN)
            mbstowcs_s(&dummy, wErrorMesg, errorMesg, _TRUNCATE);
        else
            mbstowcs_s(&dummy, wErrorMesg, "Error message buffer is not enough.", _TRUNCATE);
        return new ODBCException(nativeError, (TCHAR *) wSqlState, (TCHAR *) wErrorMesg);
#else
        if (errorMesgLen >= EX_ERRMESG_LEN)
            strncpy_s(errorMesg, "Error message buffer is not enough.", _TRUNCATE);
        return new ODBCException(nativeError, (TCHAR *) sqlState, (TCHAR *) errorMesg);
#endif
    }
    else
    {
        return new ODBCException(-1, _T(""), _T("Cannot get message."));
    }
}

// ======================================================================
// Implementation of class Environment
// ======================================================================

Environment::Environment()
    : Handler(SQL_HANDLE_ENV, SQL_NULL_HANDLE)
{
    // allocated at the superclass
}

Environment::~Environment()
{
    // freed at the superclass
}

void Environment::setAttribute(int aType, int value)
{
    process(SQLSetEnvAttr(handle, aType, (SQLPOINTER) value, 0));
}

int Environment::getAttribute(int aType)
{
    int value;
    process(SQLGetEnvAttr(handle, aType, &value, 0, NULL));
    return value;
}

// ======================================================================
// Implementation of class Connection
// ======================================================================

Connection::Connection(Environment *env)
    : Handler(SQL_HANDLE_DBC, env->handle)
{
    // allocated at the superclass
}

Connection::~Connection()
{
    disconnect();
    // freed at the suuperclass
}

void Connection::setAttribute(int aType, int value)
{
    process(SQLSetConnectAttr(handle, aType, (SQLPOINTER) value, 0));
}

int Connection::getAttribute(int aType)
{
    int value;
    process(SQLGetConnectAttr(handle, aType, &value, 0, NULL));
    return value;
}

void Connection::connect(const TCHAR *dsn, const TCHAR *username, const TCHAR *password)
{
    if (!username && !password)
        process(SQLConnect(handle, (SQLTCHAR *) dsn, SQL_NTS,
            (SQLTCHAR *) username, SQL_NTS,
            (SQLTCHAR *) password, SQL_NTS));
    else
        process(SQLConnect(handle, (SQLTCHAR *) dsn, SQL_NTS,
            (SQLTCHAR *) NULL, 0,
            (SQLTCHAR *) NULL, 0));
}

void Connection::disconnect()
{
    retcode = SQLDisconnect(handle);
}

int Connection::isDead()
{
    return getAttribute(SQL_ATTR_CONNECTION_DEAD) == SQL_CD_TRUE;
}

void Connection::commit()
{
    process(SQLEndTran(SQL_HANDLE_DBC, handle, SQL_COMMIT));
}

void Connection::rollback()
{
    process(SQLEndTran(SQL_HANDLE_DBC, handle, SQL_ROLLBACK));
}

int Connection::getAutoCommit()
{
    return getAttribute(SQL_ATTR_AUTOCOMMIT) == SQL_AUTOCOMMIT_ON;
}

void Connection::setAutoCommit(int autoCommit)
{
    int val = autoCommit == 0
        ? SQL_AUTOCOMMIT_OFF
        : SQL_AUTOCOMMIT_ON;
    setAttribute(SQL_ATTR_AUTOCOMMIT, val);
}

// ======================================================================
// Implementation of class Statement
// ======================================================================

Statement::Statement(Connection *dbc) : Handler(SQL_HANDLE_STMT, dbc->handle)
{
    // allocated at the superclass
    this->parameters    = NULL;
    this->paramNum        = 0;
    this->columns        = NULL;
    this->colNum        = 0;
    this->rowHandler    = NULL;
    this->anyPointer    = NULL;
}

Statement::~Statement()
{
    free(); // about cursor, parameters, columns this may hold
    // freed at the superclass
}

void Statement::setAttribute(int aType, int value)
{
    process(SQLSetStmtAttr(handle, aType, (SQLPOINTER) value, 0));
}

int Statement::getAttribute(int aType)
{
    int value;
    process(SQLGetStmtAttr(handle, aType, &value, 0, NULL));
    return value;
}

void Statement::free()
{
    for (std::vector<DataPointer*>::iterator ite = parameterHolder.begin(); ite != parameterHolder.end(); ite++) {
        if (*ite) { delete *ite; *ite = NULL; }
    }
    for (std::vector<DataPointer*>::iterator ite = columnHolder.begin(); ite != columnHolder.end(); ite++) {
        if (*ite) { delete *ite; *ite = NULL; }
    }
    // indirectDataPointers oesn't need to delete because thoese were deleted in one of aboves.
    retcode = SQLFreeStmt(handle, SQL_CLOSE);
    retcode = SQLFreeStmt(handle, SQL_UNBIND);
    retcode = SQLFreeStmt(handle, SQL_RESET_PARAMS);
}

int Statement::execute(const TCHAR *sql)
{
    process(SQLExecDirect(handle, (SQLTCHAR *) sql, SQL_NTS));
    return afterExecution();
}

void Statement::prepare(const TCHAR *sql)
{
    process(SQLPrepare(handle, (SQLTCHAR *) sql, SQL_NTS));
}


void Statement::bindIntParameter(int slot, int *intp)
{
    IntPointer *p = new IntPointer(intp);
    parameterHolder.push_back(p); // deleted at Statement:free
    bindParameter(slot, p);
}

void Statement::bindDoubleParameter(int slot, double *dblp)
{
    DoublePointer *p = new DoublePointer(dblp);
    parameterHolder.push_back(p); // deleted at Statement:free
    bindParameter(slot, p);
}

void Statement::bindStringParameter(int slot, TCHAR *strp)
{
    StringPointer *p = new StringPointer(strp, _tcslen(strp));
    parameterHolder.push_back(p); // deleted at Statement:free
    bindParameter(slot, p);
}

void Statement::bindTimestampParameter(int slot, SQL_TIMESTAMP_STRUCT *tsp)
{
    TimestampPointer *p = new TimestampPointer(tsp);
    parameterHolder.push_back(p); // deleted at Statement:free
    bindParameter(slot, p);
}

//void Statement::bindTimetParameter(int slot, time_t *tp)
//{
//    TimetPointer *p = new TimetPointer(tp);
//    parameterHolder.push_back(p); // deleted at Statement:free
//    indirectDataPointers.push_back(p);
//    bindParameter(slot, p);
//}

void Statement::bindUIntTimetParameter(int slot, unsigned int *tp)
{
    UIntTimetPointer *p = new UIntTimetPointer(tp);
    parameterHolder.push_back(p); // deleted at Statement:free
    indirectDataPointers.push_back(p);
    bindParameter(slot, p);
}

void Statement::bindParameter(int slot, DataPointer *dp)
{
    process(SQLBindParameter(handle, slot, SQL_PARAM_INPUT,
        dp->getCType(), dp->getSQLType(), dp->getColumnSize(), dp->getScale(),
        dp->getDataPointer(), dp->getDataSize(), dp->getLenOrInd()));
}

void Statement::bindParameters(DataPointer *dps, int size)
{
    for (int i = 0; i < size; i++)
        bindParameter(i + 1, &dps[i]);
    parameters = dps;
    paramNum = size;
}

void Statement::bindIntColumn(int slot, int *intp)
{
    IntPointer *p = new IntPointer(intp);
    columnHolder.push_back(p); // deleted at Statement:free
    bindColumn(slot, p);
}

void Statement::bindDoubleColumn(int slot, double *dblp)
{
    DoublePointer *p = new DoublePointer(dblp);
    columnHolder.push_back(p); // deleted at Statement:free
    bindColumn(slot, p);
}

void Statement::bindStringColumn(int slot, TCHAR *strp)
{
    StringPointer *p = new StringPointer(strp, _tcslen(strp));
    columnHolder.push_back(p); // deleted at Statement:free
    bindColumn(slot, p);
}

void Statement::bindTimestampColumn(int slot, SQL_TIMESTAMP_STRUCT *tsp)
{
    TimestampPointer *p = new TimestampPointer(tsp);
    columnHolder.push_back(p); // deleted at Statement:free
    bindColumn(slot, p);
}

//void Statement::bindTimetColumn(int slot, time_t *tp)
//{
//    TimetPointer *p = new TimetPointer(tp);
//    columnHolder.push_back(p); // deleted at Statement:free
//    indirectDataPointers.push_back(p);
//    bindColumn(slot, p);
//}

void Statement::bindUIntTimetColumn(int slot, unsigned int *tp)
{
    UIntTimetPointer *p = new UIntTimetPointer(tp);
    columnHolder.push_back(p); // deleted at Statement:free
    indirectDataPointers.push_back(p);
    bindColumn(slot, p);
}

void Statement::bindColumn(int slot, DataPointer *dp)
{
    process(SQLBindCol(handle, slot, dp->getCType(),
        dp->getDataPointer(), dp->getDataSize(), dp->getLenOrInd()));
}

void Statement::bindColumns(DataPointer *dps, int size)
{
    for (int i = 0; i < size; i++)
        bindColumn(i + 1, &dps[i]);
    columns = dps;
    colNum = size;
}

void Statement::setRowHandler(void (*rowHandler)(int rowCnt, DataPointer *dps, int size, void *anyPointer), void *anyPointer)
{
    this->rowHandler = rowHandler;
    this->anyPointer = anyPointer;
}

int Statement::execute()
{
    return execute(true);
}

int Statement::execute(bool fetchImmediately)
{
    // rebinding for indirectDataPointers to reflect the new value
    for (int i = 0, n = indirectDataPointers.size(); i < n; i++) {
        UIntTimetPointer *p = indirectDataPointers[i]; //TODO: use polymorphism!
        if (p) p->rebind();
    }

    process(SQLExecute(handle));
    if (fetchImmediately)
    {
        return afterExecution();
    }
    else
    {
        SQLSMALLINT colNum;
        process(SQLNumResultCols(handle, &colNum));
        return (int) colNum;
    }
}

int Statement::afterExecution()
{
    int rc = -1;
    SQLSMALLINT colNum;
    process(SQLNumResultCols(handle, &colNum));
    if (colNum == 0) {
        // Non-SELECT statement
        SQLINTEGER affectedRows;
        process(SQLRowCount(handle, &affectedRows));
        rc = affectedRows;
    } else {
        // SELECT statement
        int rowCount = 0;
        try
        {
            while ((retcode = SQLFetch(handle)) != SQL_NO_DATA)
            {
                switch (retcode)
                {
                case SQL_SUCCESS:
                case SQL_SUCCESS_WITH_INFO:
                    rowCount++;
                    if (rowHandler)
                        (*rowHandler)(rowCount, columns, colNum, anyPointer);
                    break;
                default:
                    throw makeException();
                }
            }
        }
        catch (ODBCException *e)
        {
            retcode = SQLCloseCursor(handle);
            throw e;
        }
        process(SQLCloseCursor(handle));
        rc = rowCount;
    }
    return rc;
}

int Statement::fetch()
{
    retcode = SQLFetch(handle);

    // refetching for indirectDataPointers to reflect the new value
    for (int i = 0, n = indirectDataPointers.size(); i < n; i++) {
        UIntTimetPointer *p = indirectDataPointers[i]; //TODO: use polymorphism!
        if (p) p->refetch();
    }

    switch (retcode)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        return 1;
    case SQL_NO_DATA:
    default:
        return 0;
    }
}

void Statement::fetchedAll()
{
    SQLCloseCursor(handle);
}

// ======================================================================
// Implementation of class DataPointer
// ======================================================================

DataPointer::DataPointer(SQLSMALLINT cType, SQLSMALLINT sqlType, SQLULEN columnSize,
    SQLSMALLINT scale, SQLPOINTER  dataPointer, SQLLEN dataSize)
{
    this->cType         = cType;
    this->sqlType       = sqlType;
    this->columnSize    = columnSize;
    this->scale         = scale;
    this->dataPointer   = dataPointer;
    this->dataSize      = dataSize;
    this->lenOrInd      = &lengthOrIndicator;
}

DataPointer::~DataPointer()
{
}

void DataPointer::setNull()
{
    *lenOrInd = SQL_NULL_DATA;
}

int DataPointer::isNull()
{
    return *lenOrInd == SQL_NULL_DATA;
}

int DataPointer::length()
{
    return *lenOrInd;
}

// ======================================================================
// Implementation of class IntPointer
// ======================================================================

IntPointer::IntPointer(int *intp)
    : DataPointer(SQL_C_SLONG, SQL_INTEGER, 0, 0, intp, 0)
{
    lengthOrIndicator = NULL;
}

IntPointer::~IntPointer()
{
}

// ======================================================================
// Implementation of class DoublePointer
// ======================================================================

DoublePointer::DoublePointer(double *doublep)
    : DataPointer(SQL_C_DOUBLE, SQL_DOUBLE, 0, 0, doublep, 0)
{
    lengthOrIndicator = NULL;
}

DoublePointer::~DoublePointer()
{
}

// ======================================================================
// Implementation of class StringPointer
// ======================================================================

StringPointer::StringPointer(TCHAR *stringp, int size)
    : DataPointer(SQL_C_TCHAR, SQL_VARCHAR, size, 0, stringp, size)
{
    lengthOrIndicator = SQL_NTS;
}

StringPointer::~StringPointer()
{
}

// ======================================================================
// Implementation of class TimestampPointer
// ======================================================================

TimestampPointer::TimestampPointer(SQL_TIMESTAMP_STRUCT *timestamp)
    : DataPointer(SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 19, 0, timestamp, 0)
{
    lengthOrIndicator = NULL;
}

TimestampPointer::TimestampPointer(SQL_TIMESTAMP_STRUCT *timestamp, int fractionScale)
    : DataPointer(SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 20 + fractionScale, fractionScale, timestamp, 0)
{
    lengthOrIndicator = NULL;
}

TimestampPointer::~TimestampPointer()
{
}

void TimestampPointer::structtm2timestamp(struct tm *in, SQL_TIMESTAMP_STRUCT *out)
{
    out->year    = in->tm_year + 1900;
    out->month    = in->tm_mon + 1;
    out->day    = in->tm_mday;
    out->hour    = in->tm_hour;
    out->minute    = in->tm_min;
    out->second    = in->tm_sec;
    out->fraction = 0;
}

void TimestampPointer::timestamp2structtm(SQL_TIMESTAMP_STRUCT *in, struct tm *out)
{
    out->tm_year    = in->year - 1900;
    out->tm_mon        = in->month - 1;
    out->tm_mday    = in->day;
    out->tm_hour    = in->hour;
    out->tm_min        = in->minute;
    out->tm_sec        = in->second;
    out->tm_isdst    = -1;
    _mkgmtime(out);
}

void TimestampPointer::timet2timestamp(time_t *in, SQL_TIMESTAMP_STRUCT *out)
{
    struct tm temp;
    gmtime_s(&temp, in);
    structtm2timestamp(&temp, out);
}

void TimestampPointer::timestamp2timet(SQL_TIMESTAMP_STRUCT *in, time_t *out)
{
    struct tm temp;
    timestamp2structtm(in, &temp);
    *out = _mkgmtime(&temp);
}

// ======================================================================
// Implementation of class TimetPointer
// ======================================================================

TimetPointer::TimetPointer(time_t *timet)
    : TimestampPointer(&timestamp)
{
    timeType = timet;
    rebind();
}

TimetPointer::~TimetPointer()
{
}

void TimetPointer::rebind()
{
    TimestampPointer::timet2timestamp(timeType, &timestamp);
}

void TimetPointer::refetch()
{
    TimestampPointer::timestamp2timet(&timestamp, timeType);
}

// ======================================================================
// Implementation of class UIntTimetPointer
// ======================================================================

UIntTimetPointer::UIntTimetPointer(unsigned int *uintTimet)
    : TimetPointer(&timet)
{
    uintp = uintTimet;
    rebind();
}

UIntTimetPointer::~UIntTimetPointer()
{
}

void UIntTimetPointer::rebind()
{
    timet = (time_t) *uintp;
    TimetPointer::rebind();
}

void UIntTimetPointer::refetch()
{
    TimetPointer::refetch();
    *uintp = (unsigned int) timet;
}

// ======================================================================
// Implementation of class DatePointer
// ======================================================================

DatePointer::DatePointer(SQL_DATE_STRUCT *datep)
    : DataPointer(SQL_C_TYPE_DATE, SQL_TYPE_DATE, 10, 0, datep, 0)
{
    lengthOrIndicator = NULL;
}

DatePointer::~DatePointer()
{
}

// ======================================================================
// Implementation of class TimePointer
// ======================================================================

TimePointer::TimePointer(SQL_TIME_STRUCT *timep)
    : DataPointer(SQL_C_TYPE_TIME, SQL_TYPE_TIME, 8, 0, timep, 0)
{
    lengthOrIndicator = NULL;
}

TimePointer::~TimePointer()
{
}

// ======================================================================
// Implementation of class NumericPointer
// ======================================================================

NumericPointer::NumericPointer(SQL_NUMERIC_STRUCT *numericp, int precision, int scale)
    : DataPointer(SQL_C_NUMERIC, SQL_NUMERIC, precision, scale, numericp, 0)
{
    lengthOrIndicator = NULL;
}

NumericPointer::~NumericPointer()
{
}

// http://support.microsoft.com/kb/222831
static long strtohextoval(const unsigned char *arr)
{
    long val=0,value=0;
    int i=1,last=1,current;
    int a=0,b=0;

    for(i=0;i<16;i++)
    {
        current = (int) arr[i];
        a= current % 16; //Obtain LSD
        b= current / 16; //Obtain MSD

        value += last* a;    
        last = last * 16;    
        value += last* b;
        last = last * 16;    
    }
    return value;
}

void NumericPointer::numstruct2double(SQL_NUMERIC_STRUCT *nums, double *dbl)
{
    int divisor = 1;
    for (int i = 0; i < nums->scale; i++)
        divisor *= 10;
    int sign = nums->sign == 0 ? -1 : 1;
    double dnum = sign * strtohextoval(nums->val) / (double) divisor;
    *dbl = dnum;
}

//TODO: complete writing double2numstruct()

double NumericPointer::doubleValue()
{
    double dbl;
    NumericPointer::numstruct2double((SQL_NUMERIC_STRUCT *) getDataPointer(), &dbl);
    return dbl;
}
