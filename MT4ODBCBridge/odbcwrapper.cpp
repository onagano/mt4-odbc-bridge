#include "odbcwrapper.h"
#include <stdio.h>
#include <stdlib.h>

using namespace cppodbc;

ODBCWrapper::ODBCWrapper()
{
    environment = NULL;
    connection = NULL;
    nextStmtId = 1;
    lastErrorNo = 0;
    _tcsncpy_s(lastErrorMesg, _T(""), _TRUNCATE);
}

ODBCWrapper::~ODBCWrapper()
{
    close();
}

void ODBCWrapper::handleException(ODBCException *e)
{
    lastErrorNo = e->getNativeError();
    _stprintf_s(lastErrorMesg, _T("%s(%s)"), e->getSQLState(), e->getErrorMesg());
    delete e;
}

int ODBCWrapper::open(const TCHAR *dsn, const TCHAR *username, const TCHAR *password)
{
    try
    {
        // typical sequence to open a connection
        environment = new Environment();
        environment->setAttribute(SQL_ATTR_ODBC_VERSION, SQL_OV_ODBC3);

        connection = new Connection(environment);
        connection->setAttribute(SQL_ATTR_LOGIN_TIMEOUT, 5);
        connection->connect(dsn, username, password);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }
    return MOB_OK;
}

void ODBCWrapper::close()
{
    for (std::map<int, Statement*>::iterator ite = stmts.begin(); ite != stmts.end(); ite++) {
        Statement *s = ite->second;
        if (s) delete s;
    }
    stmts.clear();
    if (connection) delete connection; // Disconnected in destructor.
    if (environment) delete environment;
    connection = NULL;
    environment = NULL;
}

int ODBCWrapper::isDead()
{
    int dead = MOB_UNDEFINED;
    try
    {
        dead = connection->isDead();
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }
    return dead;
}

int ODBCWrapper::commit()
{
    try
    {
        connection->commit();
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }
    return MOB_OK;
}

int ODBCWrapper::rollback()
{
    try
    {
        connection->rollback();
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }
    return MOB_OK;
}

int ODBCWrapper::getAutoCommit()
{
    int autoCommit = MOB_UNDEFINED;
    try
    {
        autoCommit = connection->getAutoCommit();
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }
    return autoCommit;
}

int ODBCWrapper::setAutoCommit(int autoCommit)
{
    try
    {
        connection->setAutoCommit(autoCommit);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }
    return MOB_OK;
}

int ODBCWrapper::execute(const TCHAR *sql)
{
    int rowNum = MOB_UNDEFINED;
    Statement stmt(connection);
    try
    {
        rowNum = stmt.execute(sql);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }
    return rowNum;
}

int ODBCWrapper::getLastErrorNo()
{
    int temp = lastErrorNo;
    lastErrorNo = 0;
    return temp;
}

TCHAR *ODBCWrapper::getLastErrorMesg()
{
    return lastErrorMesg;
}

int ODBCWrapper::registerStatement(const TCHAR *sql)
{
    int stmtId = nextStmtId++;
    Statement *stmt = new Statement(connection);
    try
    {
        stmt->prepare(sql);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        delete stmt;
        return MOB_NG;
    }
    stmts[stmtId] = stmt;
    return stmtId;
}

int ODBCWrapper::unregisterStatement(int stmtId)
{
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;
    delete stmt;
    stmts.erase(stmtId);
    return MOB_OK;
}

int ODBCWrapper::bindIntParameter(int stmtId, int slot, int *intp)
{
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;

    try
    {
        stmt->bindIntParameter(slot, intp);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }

    return MOB_OK;
}

int ODBCWrapper::bindDoubleParameter(int stmtId, int slot, double *dblp)
{
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;

    try
    {
        stmt->bindDoubleParameter(slot, dblp);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }

    return MOB_OK;
}

int ODBCWrapper::bindStringParameter(int stmtId, int slot, TCHAR *strp)
{
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;

    try
    {
        stmt->bindStringParameter(slot, strp);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }

    return MOB_OK;
}

int ODBCWrapper::bindDatetimeParameter(int stmtId, int slot, unsigned int *timep)
{
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;

    try
    {
        stmt->bindUIntTimetParameter(slot, timep);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }

    return MOB_OK;
}

int ODBCWrapper::bindIntColumn(int stmtId, int slot, int *intp)
{
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;

    try
    {
        stmt->bindIntColumn(slot, intp);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }

    return MOB_OK;
}

int ODBCWrapper::bindDoubleColumn(int stmtId, int slot, double *dblp)
{
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;

    try
    {
        stmt->bindDoubleColumn(slot, dblp);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }

    return MOB_OK;
}

int ODBCWrapper::bindStringColumn(int stmtId, int slot, TCHAR *strp)
{
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;

    try
    {
        stmt->bindStringColumn(slot, strp);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }

    return MOB_OK;
}

int ODBCWrapper::bindDatetimeColumn(int stmtId, int slot, unsigned int *timep)
    {
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;

    try
    {
        stmt->bindUIntTimetColumn(slot, timep);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }

    return MOB_OK;
}

int ODBCWrapper::executeStatement(int stmtId)
{
    int rv = 0;
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;

    try
    {
        rv = stmt->execute();
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }

    return rv;
}

int ODBCWrapper::selectToFetch(int stmtId)
{
    int rv = 0;
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;

    try
    {
        rv = stmt->execute(false);
    }
    catch (ODBCException *e)
    {
        handleException(e);
        return MOB_NG;
    }

    return rv;
}

int ODBCWrapper::fetch(int stmtId)
{
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;
    int rc = stmt->fetch();
    return (rc);
}

int ODBCWrapper::fetchedAll(int stmtId)
{
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;
    stmt->fetchedAll();
    return MOB_OK;
}

int ODBCWrapper::insertTick(const int stmtId, unsigned int datetime, int fraction, double *vals, const int size)
{
    int rv = 0;
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;
    //TODO: rewrite to use type-specific parameter binding method
    DataPointer *dps = (DataPointer *) malloc(sizeof(DataPointer) * (size + 2));
    
    SQL_TIMESTAMP_STRUCT ts;
    time_t tt = (time_t) datetime;
    TimestampPointer::timet2timestamp(&tt, &ts);
    dps[0] = TimestampPointer(&ts);
    dps[1] = IntPointer(&fraction);
    for(int i = 0; i < size; i++)
        dps[i + 2] = DoublePointer(&(vals[i]));
    try
    {
        stmt->bindParameters(dps, size + 2);
        rv = stmt->execute();
    }
    catch (ODBCException *e)
    {
        handleException(e);
        free(dps);
        return MOB_NG;
    }

    free(dps);
    return rv;
}

int ODBCWrapper::insertBar(const int stmtId, unsigned int datetime, double *vals, const int size)
{
    int rv = 0;
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;
    //TODO: rewrite to use type-specific parameter binding method
    DataPointer *dps = (DataPointer *) malloc(sizeof(DataPointer) * (size + 1));

    SQL_TIMESTAMP_STRUCT ts;
    time_t tt = (time_t) datetime;
    TimestampPointer::timet2timestamp(&tt, &ts);
    dps[0] = TimestampPointer(&ts);
    for(int i = 0; i < size; i++)
        dps[i + 1] = DoublePointer(&vals[i]);
    try
    {
        stmt->bindParameters(dps, size + 1);
        rv = stmt->execute();
    }
    catch (ODBCException *e)
    {
        handleException(e);
        free(dps);
        return MOB_NG;
    }

    free(dps);
    return rv;
}

int ODBCWrapper::copyRates(const int stmtId, RateInfo *rates, const int size, const int start, const int end)
{
    int rv = 0;
    Statement *stmt = stmts[stmtId];
    if (!stmt) return MOB_INVALID_STMTID;
    if (start < 0 || end > size || start > end) return MOB_NG;

    for (int i = start; i < end; i++)
    {
        SQL_TIMESTAMP_STRUCT ts;
        time_t tt = (time_t) rates[i].ctm;
        TimestampPointer::timet2timestamp(&tt, &ts);
        DataPointer dps[6] =
        {
            TimestampPointer(&ts),
            DoublePointer(&rates[i].open),
            DoublePointer(&rates[i].high),
            DoublePointer(&rates[i].low),
            DoublePointer(&rates[i].close),
            DoublePointer(&rates[i].vol)
        };
        try
        {
            //TODO: rewrite to use type-specific parameter binding method
            stmt->bindParameters(dps, 6);
            rv += stmt->execute();
        }
        catch (ODBCException *e)
        {
            handleException(e);
            return MOB_NG;
        }
    }

    return rv;
}

// by some reason, some code are skipped when optimization...
#pragma optimize ("", off)

static void selectDoubleHandler(int rowCnt, DataPointer *values, int size, void *ap)
{
    if (rowCnt == 1)
    {
        double *rv = (double *) ap;
        *rv = *((double *) values[0].getDataPointer());
    }
}

double ODBCWrapper::selectDouble(const TCHAR *sql, const double defaultVal)
{
    double rv;
    double dummy;
    DataPointer dps[1] = {DoublePointer(&dummy)};
    try
    {
        Statement stmt(connection);
        stmt.bindColumns(dps, 1);
        stmt.setRowHandler(selectDoubleHandler, &rv);
        if (stmt.execute(sql) == 0 || dps[0].isNull()) // No result or null
            rv = defaultVal;
    }
    catch (...)
    {
        return defaultVal;
    }
    return rv;
}

static void selectIntHandler(int rowCnt, DataPointer *values, int size, void *ap)
{
    if (rowCnt == 1)
    {
        int *rv = (int *) ap;
        *rv = *((int *) values[0].getDataPointer());
    }
}

int ODBCWrapper::selectInt(const TCHAR *sql, const int defaultVal)
{
    int rv;
    int dummy;
    DataPointer dps[1] = {IntPointer(&dummy)};
    try
    {
        Statement stmt(connection);
        stmt.bindColumns(dps, 1);
        stmt.setRowHandler(selectIntHandler, &rv);
        if (stmt.execute(sql) == 0 || dps[0].isNull()) // No result or null
            rv = defaultVal;
    }
    catch (...)
    {
        return defaultVal;
    }
    return rv;
}

static void selectDatetimeHandler(int rowCnt, DataPointer *values, int size, void *ap)
{
    if (rowCnt == 1)
    {
        SQL_TIMESTAMP_STRUCT *rv = (SQL_TIMESTAMP_STRUCT *) ap;
        *rv = *((SQL_TIMESTAMP_STRUCT *) values[0].getDataPointer());
    }
}

unsigned int ODBCWrapper::selectDatetime(const TCHAR *sql, const unsigned int defaultVal)
{
    unsigned int rv;
    SQL_TIMESTAMP_STRUCT dummy;
    DataPointer dps[1] = {TimestampPointer(&dummy)};
    SQL_TIMESTAMP_STRUCT ts;
    time_t tt;
    try
    {
        Statement stmt(connection);
        stmt.bindColumns(dps, 1);
        stmt.setRowHandler(selectDatetimeHandler, &ts);
        if (stmt.execute(sql) == 0 || dps[0].isNull()) // No result or null
            return defaultVal;
        TimestampPointer::timestamp2timet(&ts, &tt);
        rv = (unsigned int) tt;
    }
    catch (...)
    {
        return defaultVal;
    }
    return rv;
}

#pragma optimize ("", on)
