//-[stdhead]-------------------------------------------------------
//Project:
//File   :stmt.cpp
//Started:17.09.02 14:48:01
//Updated:24.09.02 16:41:53
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
#include "stdhead.h"
#include "stmt.h"
#include "excpt.h"

typedef struct
{
    CGOdbcStmt *pStmt;
    int iTimeOut;
    HANDLE hStopEvent;
}SQLTIMEOUT;

DWORD WINAPI SQLTimeoutProc(void *_pParam)
{
    SQLTIMEOUT *pTimeOut = (SQLTIMEOUT *)_pParam;
    if (WaitForSingleObject(pTimeOut->hStopEvent, pTimeOut->iTimeOut) == WAIT_TIMEOUT)
        pTimeOut->pStmt->cancel();
    return 0;
}

//Stmt constructor
CGOdbcStmt::CGOdbcStmt(HDBC hDbc)
{
    m_hStmt = 0;
    SQLAllocStmt(hDbc, &m_hStmt);
    SQLSetStmtOption(m_hStmt, SQL_CONCURRENCY, SQL_CONCUR_VALUES);
    SQLSetStmtOption(m_hStmt, SQL_ASYNC_ENABLE, SQL_ASYNC_ENABLE_OFF);
    SQLSetStmtOption(m_hStmt, SQL_CURSOR_TYPE, SQL_CURSOR_STATIC);
    SQLSetStmtOption(m_hStmt, SQL_ROWSET_SIZE, 1);

    m_sError = "";
    m_iColCount = 0;
    m_pColumns = 0;
    m_pData = 0;
    m_iParamsCount = 0;
    m_pParams = 0;
    m_pParamData = 0;
    m_iTimeOut = 0;
}

//destructor
CGOdbcStmt::~CGOdbcStmt()
{
    _beforeMove();
    SQLFreeStmt(m_hStmt, SQL_CLOSE);
    _free();
}

//free internal buffer
void CGOdbcStmt::_free()
{
    m_iColCount = 0;
    m_iParamsCount = 0;
    m_sError = "";
    if (m_pColumns)
    {
        delete m_pColumns;
        m_pColumns = 0;
    }
    if (m_pData)
    {
        delete m_pData;
        m_pData = 0;
    }
    if (m_pParams)
    {
        delete m_pParams;
        m_pParams = 0;
    }
    if (m_pParamData)
    {
        delete m_pParamData;
        m_pParamData = 0;
    }
}


//--------------------------------------------------//
//execution methods                                 //
//--------------------------------------------------//

//set timeout for next execute operation
void CGOdbcStmt::setTimeOut(int iMSec)
{
    m_iTimeOut = iMSec;
}


//execute query
void CGOdbcStmt::execute(const char *szQuery)
{
    RETCODE rc;

    SQLTIMEOUT t = {this, m_iTimeOut, 0};
    DWORD dwThreadID;
    HANDLE hThread;


    if (m_iTimeOut)
    {
        t.hStopEvent = CreateEvent(0, FALSE, FALSE, 0);
        hThread = CreateThread(0, 0, SQLTimeoutProc, &t, 0, &dwThreadID);
        Sleep(1);
    }

    rc = SQLExecDirect(m_hStmt, (unsigned char *)szQuery, SQL_NTS);

    if (m_iTimeOut)
    {
        SetEvent(t.hStopEvent);
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(t.hStopEvent);
    }

    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
        m_sError = "";
        return ;
    }
    else
    {
        _throwError();
        return ;
    }
}

//execute prepared query
void CGOdbcStmt::execute()
{
    RETCODE rc;
    SQLTIMEOUT t = {this, m_iTimeOut, 0};
    DWORD dwThreadID;
    HANDLE hThread;

    if (m_iTimeOut)
    {
        t.hStopEvent = CreateEvent(0, FALSE, FALSE, 0);
        hThread = CreateThread(0, 0, SQLTimeoutProc, &t, 0, &dwThreadID);
        Sleep(1);
    }


    rc = SQLExecute(m_hStmt);

    if (m_iTimeOut)
    {
        SetEvent(t.hStopEvent);
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(t.hStopEvent);
    }

    while(rc == SQL_NEED_DATA)
    {
        //data is needed
        SQLPOINTER pi = 0;
        rc = SQLParamData(m_hStmt, &pi);
        if (rc == SQL_NEED_DATA)
        {
            PARAM *pParam = (PARAM *)pi;

            int iBlockSize = 4096;
            int iWasSent = 0;
            int iToSend;

            while (iWasSent < pParam->iLen)
            {
                iToSend = pParam->iLen - iWasSent;
                if (iToSend > iBlockSize)
                    iToSend = iBlockSize;
                SQLPutData(m_hStmt, pParam->pData + iWasSent, iToSend);
                iWasSent += iToSend;
            }
        }
    }

    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
        m_sError = "";
        return ;
    }
    else
    {
        _throwError();
        return ;
    }
}

//prepare query
void CGOdbcStmt::prepare(const char *szQuery)
{
    RETCODE rc;
    _free();
    rc = SQLPrepare(m_hStmt, (unsigned char *)szQuery, SQL_NTS);
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
        m_sError = "";
        return ;
    }
    else
    {
        _throwError();
        return ;
    }
}

//bind parameters
bool CGOdbcStmt::bindParamsAuto()
{
    RETCODE rc;

    m_iParamsCount = 0;
    rc = SQLNumParams(m_hStmt, &m_iParamsCount);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
        _throwError();
        return false;
    }

    if (m_iParamsCount <= 0)
        return false;

    m_pParams = new PARAM[m_iParamsCount];
    memset(m_pParams, 0, sizeof(PARAM) * m_iParamsCount);

    //get parameters description
    long lBuffSize = 0;
    short iNullable;
    int i;
    for (i = 0; i < m_iParamsCount; i++)
    {
        rc = SQLDescribeParam(m_hStmt, i + 1,
                              &m_pParams[i].iSqlType,
                              &m_pParams[i].iSqlSize,
                              &m_pParams[i].iScale,
                              &iNullable);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        {
            _throwError();
            return false;
        }
        //convert SQL type to SQL-C type
        m_pParams[i].lOffset = lBuffSize;
        switch (m_pParams[i].iSqlType)
        {
        case    SQL_BIT:
        case    SQL_TINYINT:
        case    SQL_SMALLINT:
        case    SQL_INTEGER:
                m_pParams[i].iSqlCType = SQL_C_LONG;
                m_pParams[i].iLen = 4;
                break;
        case    SQL_DOUBLE:
        case    SQL_FLOAT:
        case    SQL_NUMERIC:
        case    SQL_DECIMAL:
        case    SQL_REAL:
                m_pParams[i].iSqlCType = SQL_C_DOUBLE;
                m_pParams[i].iLen = 8;
                break;
        case    SQL_DATE:
        case    SQL_TYPE_DATE:
                m_pParams[i].iSqlCType = SQL_C_DATE;
                m_pParams[i].iLen = sizeof(CGOdbcStmt::DATE);
                break;
        case    SQL_TIMESTAMP:
        case    SQL_TYPE_TIMESTAMP:
                m_pParams[i].iSqlCType = SQL_C_TIMESTAMP;
                m_pParams[i].iLen = sizeof(CGOdbcStmt::TIMESTAMP);
                break;
        case    SQL_CHAR:
        case    SQL_VARCHAR:
                m_pParams[i].iSqlCType = SQL_C_CHAR;
                m_pParams[i].cbData = SQL_NTS;
                m_pParams[i].iLen = m_pParams[i].iSqlSize + 1;
                break;
        case    SQL_LONGVARCHAR:
        case    SQL_TYPE_ORACLE_CLOB:
                m_pParams[i].iSqlCType = SQL_C_CHAR;
                m_pParams[i].bLOB = true;
                m_pParams[i].iLen = 0;
                break;
        case    SQL_BINARY:
        case    SQL_VARBINARY:
                m_pParams[i].iSqlCType = SQL_C_BINARY;
                m_pParams[i].iLen = m_pParams[i].iSqlSize;
                break;
        case    SQL_LONGVARBINARY:
        case    SQL_TYPE_ORACLE_BLOB:
                m_pParams[i].iSqlCType = SQL_C_BINARY;
                m_pParams[i].bLOB = true;
                m_pParams[i].iLen = 0;
                break;
        case    SQL_GUID:
                m_pParams[i].iSqlCType = SQL_C_GUID;
                m_pParams[i].iLen = sizeof(GUID);
                break;
        default:
                m_pParams[i].iSqlCType = SQL_C_CHAR;
                m_pParams[i].iLen = 128;
                break;
        }
        lBuffSize += m_pParams[i].iLen;
    }
    m_pParamData = new unsigned char[lBuffSize];
    for (i = 0; i < m_iParamsCount; i++)
    {
        if (m_pParams[i].bLOB)
        {
            m_pParams[i].pData = 0;
            m_pParams[i].iLen = 0;
            m_pParams[i].cbData = SQL_DATA_AT_EXEC;

            rc = SQLBindParameter(m_hStmt, i + 1, SQL_PARAM_INPUT, m_pParams[i].iSqlCType,
                                  m_pParams[i].iSqlType, m_pParams[i].iSqlSize,
                                  m_pParams[i].iScale, (unsigned char *)&m_pParams[i],
                                  0, &m_pParams[i].cbData);
        }
        else
        {
            m_pParams[i].pData = m_pParamData + m_pParams[i].lOffset;
            m_pParams[i].cbData = m_pParams[i].iLen;
            rc = SQLBindParameter(m_hStmt, i + 1, SQL_PARAM_INPUT, m_pParams[i].iSqlCType,
                                  m_pParams[i].iSqlType, m_pParams[i].iSqlSize,
                                  m_pParams[i].iScale, m_pParams[i].pData,
                                  m_pParams[i].iLen, &m_pParams[i].cbData);
        }
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        {
            _throwError();
            return false;
        }
    }
    return true;
}

//set number of parameters manually
void CGOdbcStmt::setParamNumber(int iParam)
{
    m_iParamsCount = iParam;
    m_iBinded = 0;
    m_pParams = new PARAM[m_iParamsCount];
    memset(m_pParams, 0, sizeof(PARAM) * m_iParamsCount);
}


void CGOdbcStmt::bindParam(int iIndex, short iSqlType, short iSqlCType, int iLength, unsigned long iSqlSize, short iScale)
{
    if (iIndex < 0 || iIndex >= m_iParamsCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);
    if (m_iBinded == m_iParamsCount)
        throw new CGOdbcEx(true, CGOdbcEx::eAllParameterAlreadyBound);

    m_pParams[iIndex].iSqlType = iSqlType;
    m_pParams[iIndex].iSqlCType = iSqlCType;
    m_pParams[iIndex].iLen = iLength;
    m_pParams[iIndex].iSqlSize = iSqlSize;
    m_pParams[iIndex].iScale = iScale;
    m_iBinded++;
    if (m_iBinded != m_iParamsCount)
        return ;

    //get parameters description
    long lBuffSize = 0;
    int i;
    for (i = 0; i < m_iParamsCount; i++)
    {
        //convert SQL type to SQL-C type
        m_pParams[i].lOffset = lBuffSize;
        switch (m_pParams[i].iSqlType)
        {
        case    SQL_BIT:
        case    SQL_TINYINT:
        case    SQL_SMALLINT:
        case    SQL_INTEGER:
                m_pParams[i].iLen = 4;
                break;
        case    SQL_DOUBLE:
        case    SQL_FLOAT:
        case    SQL_NUMERIC:
        case    SQL_DECIMAL:
        case    SQL_REAL:
                m_pParams[i].iLen = 8;
                break;
        case    SQL_DATE:
        case    SQL_TYPE_DATE:
                m_pParams[i].iLen = sizeof(CGOdbcStmt::DATE);
                break;
        case    SQL_TIMESTAMP:
        case    SQL_TYPE_TIMESTAMP:
                m_pParams[i].iLen = sizeof(CGOdbcStmt::TIMESTAMP);
                break;
        case    SQL_CHAR:
        case    SQL_VARCHAR:
                m_pParams[i].iLen = m_pParams[i].iSqlSize + 1;
                break;
        case    SQL_LONGVARCHAR:
        case    SQL_TYPE_ORACLE_CLOB:
                m_pParams[i].iSqlCType = SQL_C_CHAR;
                m_pParams[i].bLOB = true;
                m_pParams[i].iLen = 0;
                break;
        case    SQL_BINARY:
        case    SQL_VARBINARY:
                m_pParams[i].iLen = m_pParams[i].iSqlSize;
                break;
        case    SQL_LONGVARBINARY:
        case    SQL_TYPE_ORACLE_BLOB:
                m_pParams[i].iSqlCType = SQL_C_BINARY;
                m_pParams[i].bLOB = true;
                m_pParams[i].iLen = 0;
                break;
        case    SQL_GUID:
                m_pParams[i].iLen = sizeof(GUID);
                break;
        default:
                break;
        }
        lBuffSize += m_pParams[i].iLen;
    }

    m_pParamData = new unsigned char[lBuffSize];
    RETCODE rc;

    for (i = 0; i < m_iParamsCount; i++)
    {
        if (m_pParams[i].bLOB)
        {
            m_pParams[i].pData = 0;
            m_pParams[i].iLen = 0;
            m_pParams[i].cbData = SQL_DATA_AT_EXEC;

            rc = SQLBindParameter(m_hStmt, i + 1, SQL_PARAM_INPUT, m_pParams[i].iSqlCType,
                                  m_pParams[i].iSqlType, m_pParams[i].iSqlSize,
                                  m_pParams[i].iScale, (unsigned char *)&m_pParams[i],
                                  0, &m_pParams[i].cbData);
        }
        else
        {
            m_pParams[i].pData = m_pParamData + m_pParams[i].lOffset;
            m_pParams[i].cbData = m_pParams[i].iLen;
            rc = SQLBindParameter(m_hStmt, i + 1, SQL_PARAM_INPUT, m_pParams[i].iSqlCType,
                                  m_pParams[i].iSqlType, m_pParams[i].iSqlSize,
                                  m_pParams[i].iScale, m_pParams[i].pData,
                                  m_pParams[i].iLen, &m_pParams[i].cbData);
        }
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        {
            _throwError();
            return ;
        }
    }
    return ;
}


//switch to next result
bool CGOdbcStmt::moreResults()
{
    RETCODE rc;
    rc = SQLMoreResults(m_hStmt);
    return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}

//returns number of rows
long CGOdbcStmt::getRowCount()
{
    long iRetVal = -1;
    SQLRowCount(m_hStmt, &iRetVal);
    return iRetVal;
}

//bind data
bool CGOdbcStmt::bindAuto()
{
    _free();
    //get number of columns in result set
    m_iColCount = 0;
    RETCODE rc = SQLNumResultCols(m_hStmt, &m_iColCount);

    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        _throwError();
    if (m_iColCount < 1)
        return false;

    m_pColumns = new COLUMN[m_iColCount];
    memset(m_pColumns, 0, sizeof(COLUMN) * m_iColCount);

    //build description of columns and calculate size of the buffer
    int i;
    short iLen;
    short iNullable;
    long lBuffSize = 0;
    for (i = 0; i < m_iColCount; i++)
    {
        rc = SQLDescribeCol(m_hStmt, i + 1,
                            (unsigned char *)m_pColumns[i].szName, MAX_PATH, &iLen,
                            &m_pColumns[i].iSqlType,
                            &m_pColumns[i].iSqlSize,
                            &m_pColumns[i].iScale,
                            &iNullable);

        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            _throwError();

        m_pColumns[i].lOffset = lBuffSize;
        switch (m_pColumns[i].iSqlType)
        {
        case    SQL_BIT:
        case    SQL_TINYINT:
        case    SQL_SMALLINT:
        case    SQL_INTEGER:
                m_pColumns[i].iSqlCType = SQL_C_LONG;
                m_pColumns[i].iLen = 4;
                break;
        case    SQL_DOUBLE:
        case    SQL_FLOAT:
        case    SQL_NUMERIC:
        case    SQL_DECIMAL:
        case    SQL_REAL:
                m_pColumns[i].iSqlCType = SQL_C_DOUBLE;
                m_pColumns[i].iLen = 8;
                break;
        case    SQL_DATE:
        case    SQL_TYPE_DATE:
                m_pColumns[i].iSqlCType = SQL_C_DATE;
                m_pColumns[i].iLen = sizeof(CGOdbcStmt::DATE);
                break;
        case    SQL_TIMESTAMP:
        case    SQL_TYPE_TIMESTAMP:
                m_pColumns[i].iSqlCType = SQL_C_TIMESTAMP;
                m_pColumns[i].iLen = sizeof(CGOdbcStmt::TIMESTAMP);
                break;
        case    SQL_CHAR:
        case    SQL_VARCHAR:
                m_pColumns[i].iSqlCType = SQL_C_CHAR;
                m_pColumns[i].iLen = m_pColumns[i].iSqlSize + 1;
                break;
        case    SQL_LONGVARCHAR:
        case    SQL_TYPE_ORACLE_CLOB:
                m_pColumns[i].iSqlCType = SQL_C_CHAR;
                m_pColumns[i].bLOB = true;
                m_pColumns[i].iLen = 0;
                break;
        case    SQL_BINARY:
        case    SQL_VARBINARY:
                m_pColumns[i].iSqlCType = SQL_C_BINARY;
                m_pColumns[i].iLen = m_pColumns[i].iSqlSize;
                break;
        case    SQL_LONGVARBINARY:
        case    SQL_TYPE_ORACLE_BLOB:
                m_pColumns[i].iSqlCType = SQL_C_BINARY;
                m_pColumns[i].bLOB = true;
                m_pColumns[i].iLen = 0;
                break;
        case    SQL_GUID:
                m_pColumns[i].iSqlCType = SQL_C_GUID;
                m_pColumns[i].iLen = sizeof(GUID);
                break;
        default:
                m_pColumns[i].iSqlCType = SQL_C_CHAR;
                m_pColumns[i].iLen = 128;
                break;
        }
        lBuffSize += m_pColumns[i].iLen;
    }

    //allocate data buffer and bind columns
    m_pData = new unsigned char[lBuffSize];
    for (i = 0; i < m_iColCount; i++)
    {
        if (m_pColumns[i].bLOB)
        {
            m_pColumns[i].pData = 0;
            rc = SQLBindCol(m_hStmt, i + 1, m_pColumns[i].iSqlCType,
                            0, 0, &m_pColumns[i].cbData);
        }
        else
        {
            m_pColumns[i].pData = m_pData + m_pColumns[i].lOffset;
            rc = SQLBindCol(m_hStmt, i + 1, m_pColumns[i].iSqlCType,
                            m_pColumns[i].pData, m_pColumns[i].iLen, &m_pColumns[i].cbData);
        }
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            _throwError();
    }
    return true;
}

//returns last error
const char *CGOdbcStmt::getError()
{
    return m_sError;
}

//--------------------------------------------------//
//moving methods                                    //
//--------------------------------------------------//
//go to first row
bool CGOdbcStmt::first()
{
    RETCODE rc;
    unsigned long lRow;
    unsigned short lStatus;
    _beforeMove();
    rc = SQLExtendedFetch(m_hStmt, SQL_FETCH_FIRST, 0, &lRow, &lStatus);
    return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}

//go to last row
bool CGOdbcStmt::last()
{
    RETCODE rc;
    unsigned long lRow;
    unsigned short lStatus;
    _beforeMove();
    rc = SQLExtendedFetch(m_hStmt, SQL_FETCH_LAST, 0, &lRow, &lStatus);
    return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}

//go to next row
bool CGOdbcStmt::next()
{
    RETCODE rc;
    unsigned long lRow;
    unsigned short lStatus;
    _beforeMove();
    rc = SQLExtendedFetch(m_hStmt, SQL_FETCH_NEXT, 0, &lRow, &lStatus);
    return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}

//go to previous row
bool CGOdbcStmt::prev()
{
    RETCODE rc;
    unsigned long lRow;
    unsigned short lStatus;
    _beforeMove();
    rc = SQLExtendedFetch(m_hStmt, SQL_FETCH_PRIOR, 0, &lRow, &lStatus);
    return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}

//go to row
bool CGOdbcStmt::go(int iRow)
{
    RETCODE rc;
    unsigned long lRow;
    unsigned short lStatus;
    _beforeMove();
    rc = SQLExtendedFetch(m_hStmt, SQL_FETCH_ABSOLUTE, iRow, &lRow, &lStatus);
    return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;

}

//skip number of rows
bool CGOdbcStmt::skip(int iRows)
{
    RETCODE rc;
    unsigned long lRow;
    unsigned short lStatus;
    _beforeMove();
    rc = SQLExtendedFetch(m_hStmt, SQL_FETCH_RELATIVE, iRows, &lRow, &lStatus);
    return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}

//returns current row index
int CGOdbcStmt::getRowNo()
{
    int lRetVal;
    SQLGetStmtOption(m_hStmt, SQL_ROW_NUMBER, (unsigned long *)&lRetVal);
    return lRetVal;
}

//--------------------------------------------------//
//data access methods                               //
//--------------------------------------------------//
//returns integer value
int CGOdbcStmt::getInt(int iCol)
{
    if (iCol < 0 || iCol >= m_iColCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    switch (m_pColumns[iCol].iSqlCType)
    {
    case    SQL_C_LONG:
            return *(int *)m_pColumns[iCol].pData;
    case    SQL_C_DOUBLE:
            return (int)*(double *)m_pColumns[iCol].pData;
    case    SQL_C_CHAR:
            if (!m_pColumns[iCol].bLOB)
                return atoi((char *)m_pColumns[iCol].pData);
    default:
            throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
    }
}

//returns zero-terminated character value
const char *CGOdbcStmt::getChar(int iCol)
{
    if (iCol < 0 || iCol >= m_iColCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    char szBuff[1024];
    char szFrmt[32];
    SYSTEMTIME st;
    int i;
    GUID guid;

    if (isNull(iCol))
        return (const char *)"(null)";

    switch(m_pColumns[iCol].iSqlCType)
    {
    case    SQL_C_CHAR:
            if (m_pColumns[iCol].bLOB)
                _loadBlob(iCol);
            return (const char *)m_pColumns[iCol].pData;
    case    SQL_C_LONG:
            sprintf(szBuff, "%i", *(int *)m_pColumns[iCol].pData);
            m_sLastStr = szBuff;
            return m_sLastStr;
    case    SQL_C_DOUBLE:
            sprintf(szFrmt, "%%.%if", m_pColumns[iCol].iScale);
            sprintf(szBuff, szFrmt, *(double *)m_pColumns[iCol].pData);
            m_sLastStr = szBuff;
            return m_sLastStr;
    case    SQL_C_DATE:
            memset(&st, 0, sizeof(st));
            st.wYear = ((CGOdbcStmt::DATE *)(m_pColumns[iCol].pData))->iYear;
            st.wMonth = ((CGOdbcStmt::DATE *)(m_pColumns[iCol].pData))->iMonth;
            st.wDay = ((CGOdbcStmt::DATE *)(m_pColumns[iCol].pData))->iDay;
            GetDateFormat(LOCALE_USER_DEFAULT,
                          DATE_SHORTDATE,
                          &st, NULL, szBuff, 1024);
            m_sLastStr = szBuff;
            return m_sLastStr;
    case    SQL_C_TIMESTAMP:
            memset(&st, 0, sizeof(st));
            st.wYear = ((TIMESTAMP *)(m_pColumns[iCol].pData))->iYear;
            st.wMonth = ((TIMESTAMP  *)(m_pColumns[iCol].pData))->iMonth;
            st.wDay = ((TIMESTAMP  *)(m_pColumns[iCol].pData))->iDay;
            st.wHour = ((TIMESTAMP  *)(m_pColumns[iCol].pData))->iHour;
            st.wMinute = ((TIMESTAMP  *)(m_pColumns[iCol].pData))->iMin;
            st.wSecond = ((TIMESTAMP  *)(m_pColumns[iCol].pData))->iSec;
            GetDateFormat(LOCALE_USER_DEFAULT,
                          DATE_SHORTDATE,
                          &st, NULL, szBuff, 1024);
            i = strlen(szBuff);
            szBuff[i++] = ' ';
            GetTimeFormat(LOCALE_USER_DEFAULT,
                          0, &st, NULL, szBuff + i, 1024 - i);
            m_sLastStr = szBuff;
            return m_sLastStr;
    case    SQL_C_GUID:
            memcpy(&guid, m_pColumns[iCol].pData, sizeof(GUID));
            sprintf(szBuff, "%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X",
                             guid.Data1,guid.Data2,guid.Data3,
                             ((WORD)guid.Data4[0]<<8) + guid.Data4[1], guid.Data4[2], guid.Data4[3],
                             guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
            m_sLastStr = szBuff;
            return m_sLastStr;
    }

    throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
}

//returns length of value
int CGOdbcStmt::getLength(int iCol)
{
    if (iCol < 0 || iCol >= m_iColCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    if (m_pColumns[iCol].bLOB)
    {
        _loadBlob(iCol);
        return m_pColumns[iCol].iLen;
    }
    else
    {
        return (int)m_pColumns[iCol].cbData;
    }
}

//returns pointer to value
const void *CGOdbcStmt::getPtr(int iCol)
{
    if (iCol < 0 || iCol >= m_iColCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    if (m_pColumns[iCol].bLOB)
        _loadBlob(iCol);
    return (void *)m_pColumns[iCol].pData;
}

//returns true if value is null
bool CGOdbcStmt::isNull(int iCol)
{
    if (iCol < 0 || iCol >= m_iColCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    return m_pColumns[iCol].cbData == SQL_NULL_DATA;
}

//returns number value
double CGOdbcStmt::getNumber(int iCol)
{
    if (iCol < 0 || iCol >= m_iColCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    switch (m_pColumns[iCol].iSqlCType)
    {
    case    SQL_C_LONG:
            return (double)*(int *)m_pColumns[iCol].pData;
    case    SQL_C_DOUBLE:
            return *(double *)m_pColumns[iCol].pData;
    case    SQL_C_CHAR:
            if (!m_pColumns[iCol].bLOB)
                return atof((char *)m_pColumns[iCol].pData);
    default:
            throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
    }
}

//returns date-only value
const CGOdbcStmt::DATE *CGOdbcStmt::getDate(int iCol)
{
    if (iCol < 0 || iCol >= m_iColCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    if (m_pColumns[iCol].iSqlCType == SQL_C_DATE ||
        m_pColumns[iCol].iSqlCType == SQL_C_TIMESTAMP)
        return (const CGOdbcStmt::DATE *)m_pColumns[iCol].pData;
    else
        throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
}

//returns timestamp value
const CGOdbcStmt::TIMESTAMP *CGOdbcStmt::getTimeStamp(int iCol)
{
    if (iCol < 0 || iCol >= m_iColCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    if (m_pColumns[iCol].iSqlCType == SQL_C_TIMESTAMP)
        return (const CGOdbcStmt::TIMESTAMP *)m_pColumns[iCol].pData;
    else
        throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
}

//returns value as GUID
const GUID * CGOdbcStmt::getGUID(int iCol)
{
    if (iCol < 0 || iCol >= m_iColCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    if (m_pColumns[iCol].iSqlCType == SQL_C_GUID)
        return (const GUID *)m_pColumns[iCol].pData;
    else
        throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
}


//get description of error
void CGOdbcStmt::_throwError()
{
    char szState[MAX_PATH], szError[MAX_PATH * 2];
    long lNative;
    short   cb;
    char szStr[MAX_PATH * 4];

    SQLError (SQL_NULL_HENV, SQL_NULL_HDBC, m_hStmt, (unsigned char *)szState, &lNative,
                  (unsigned char *)szError, MAX_PATH * 2, &cb);
    sprintf (szStr, "Error:=%i\nSql state:=%s\n%s", lNative, szState, szError);
    m_sError = szStr;
    throw new CGOdbcEx(false, lNative, szStr);
}

//clear buffers before move
void CGOdbcStmt::_beforeMove()
{
    int i;
    for (i = 0; i < m_iColCount; i++)
    {
        m_pColumns[i].cbData = 0;
        if (m_pColumns[i].bLOB && m_pColumns[i].pData)
        {
            delete m_pColumns[i].pData;
            m_pColumns[i].pData = 0;
            m_pColumns[i].iLen = 0;
        }
    }
}

//load blob data
void CGOdbcStmt::_loadBlob(int iCol)
{
    if (m_pColumns[iCol].pData != 0)
        return;

    unsigned char *szBuff = 0, *_szBuff;
    long nFetched;
    long lSize = 0;
    unsigned char pcBuffer[4096];
    RETCODE rc;

    while (true)
    {
        nFetched = 4096;
        rc = SQLGetData(m_hStmt, iCol + 1, m_pColumns[iCol].iSqlCType,
                        pcBuffer, 4096, &nFetched);

        if (rc == SQL_NO_DATA)
            break;

        if (rc != SQL_SUCCESS_WITH_INFO && rc != SQL_SUCCESS)
        {
            if (szBuff)
                delete szBuff;
            _throwError();
            return;
        }

        if (rc == 1)
            nFetched = 4096;

        if (nFetched > 0)
        {
            if (m_pColumns[iCol].iSqlCType == SQL_C_CHAR)
                nFetched--;

            if (nFetched > 0)
            {
                _szBuff = new unsigned char[lSize + nFetched + 32];
                if (szBuff)
                {
                    memcpy(_szBuff, szBuff, lSize);
                    delete szBuff;
                }
                memcpy(_szBuff + lSize, pcBuffer, nFetched + 1);
                szBuff = _szBuff;
                lSize += nFetched;
            }
        }
    }
    if (szBuff != 0)
    {
        if (m_pColumns[iCol].iSqlCType == SQL_C_CHAR)
        {
            lSize++;
            szBuff[lSize] = 0;
        }
        m_pColumns[iCol].iLen = lSize;
        m_pColumns[iCol].pData = szBuff;
    }
    else
    {
        m_pColumns[iCol].iLen = 0;
        m_pColumns[iCol].pData = 0;
        m_pColumns[iCol].cbData = SQL_NULL_DATA;
    }
}

//--------------------------------------------------//
//parameter set method                              //
//--------------------------------------------------//
//set integer value
void CGOdbcStmt::setInt(int iParam, int iValue)
{
    if (iParam < 0 || iParam >= m_iParamsCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    switch (m_pParams[iParam].iSqlCType)
    {
    case    SQL_C_LONG:
            *(int *)m_pParams[iParam].pData = iValue;
            break;
    case    SQL_C_DOUBLE:
            *(double *)m_pParams[iParam].pData = iValue;
            break;
    default:
            throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
            break;
    }
}


//set character value
void CGOdbcStmt::setChar(int iParam, const char *pParam)
{
    if (iParam < 0 || iParam >= m_iParamsCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    switch (m_pParams[iParam].iSqlCType)
    {
    case    SQL_C_LONG:
            *(int *)m_pParams[iParam].pData = atoi(pParam);
            break;
    case    SQL_C_DOUBLE:
            *(double *)m_pParams[iParam].pData = atof(pParam);
            break;
    case    SQL_C_CHAR:
            if (m_pParams[iParam].bLOB)
            {
                m_pParams[iParam].pData = (unsigned char *)pParam;
                m_pParams[iParam].iLen = strlen(pParam);
                m_pParams[iParam].cbData = m_pParams[iParam].iLen;
                SQLBindParameter(m_hStmt, iParam + 1, SQL_PARAM_INPUT, m_pParams[iParam].iSqlCType,
                                 m_pParams[iParam].iSqlType, m_pParams[iParam].iSqlSize,
                                 m_pParams[iParam].iScale, m_pParams[iParam].pData,
                                 m_pParams[iParam].iLen, &m_pParams[iParam].cbData);
            }
            else
            {
                strncpy((char *)m_pParams[iParam].pData, pParam, m_pParams[iParam].iLen - 1);
                m_pParams[iParam].cbData = min(strlen(pParam), m_pParams[iParam].iLen);
            }
            break;
    case    SQL_C_BINARY:
            if (m_pParams[iParam].bLOB)
            {
                m_pParams[iParam].pData = (unsigned char *)pParam;
                m_pParams[iParam].iLen = strlen(pParam);
                m_pParams[iParam].cbData = m_pParams[iParam].iLen;
                SQLBindParameter(m_hStmt, iParam + 1, SQL_PARAM_INPUT, m_pParams[iParam].iSqlCType,
                                 m_pParams[iParam].iSqlType, m_pParams[iParam].iSqlSize,
                                 m_pParams[iParam].iScale, m_pParams[iParam].pData,
                                 m_pParams[iParam].iLen, &m_pParams[iParam].cbData);
            }
            else
            {
                strncpy((char *)m_pParams[iParam].pData, pParam, m_pParams[iParam].iLen);
                m_pParams[iParam].cbData = min(strlen(pParam), m_pParams[iParam].iLen);
            }
            break;
    default:
            throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
            break;
    }
}

//set binary value
void CGOdbcStmt::setBin(int iParam, const void *pBin, int iLen)
{
    if (iParam < 0 || iParam >= m_iParamsCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    switch (m_pParams[iParam].iSqlCType)
    {
    case    SQL_C_BINARY:
            if (m_pParams[iParam].bLOB)
            {
                m_pParams[iParam].pData = (unsigned char *)pBin;
                m_pParams[iParam].iLen = iLen;
                m_pParams[iParam].cbData = m_pParams[iParam].iLen;
                SQLBindParameter(m_hStmt, iParam + 1, SQL_PARAM_INPUT, m_pParams[iParam].iSqlCType,
                                 m_pParams[iParam].iSqlType, m_pParams[iParam].iSqlSize,
                                 m_pParams[iParam].iScale, m_pParams[iParam].pData,
                                 m_pParams[iParam].iLen, &m_pParams[iParam].cbData);
            }
            else
            {
                memcpy(m_pParams[iParam].pData, pBin, min(m_pParams[iParam].iLen, iLen));
                m_pParams[iParam].cbData = min(iLen, m_pParams[iParam].iLen);
            }
            break;
    default:
            throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
            break;
    }
}

//set double value
void CGOdbcStmt::setNumber(int iParam, double dblVal)
{
    if (iParam < 0 || iParam >= m_iParamsCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    switch (m_pParams[iParam].iSqlCType)
    {
    case    SQL_C_LONG:
            *(int *)m_pParams[iParam].pData = dblVal;
            break;
    case    SQL_C_DOUBLE:
            *(double *)m_pParams[iParam].pData = dblVal;
            break;
    default:
            throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
            break;
    }
}

//set date value
void CGOdbcStmt::setDate(int iParam, const CGOdbcStmt::DATE *pDate)
{
    if (iParam < 0 || iParam >= m_iParamsCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    if (m_pParams[iParam].iSqlCType == SQL_C_DATE)
    {
        memcpy(m_pParams[iParam].pData, pDate, sizeof(CGOdbcStmt::DATE));
    }
    else
    if (m_pParams[iParam].iSqlCType == SQL_C_TIMESTAMP)
    {
        memset(m_pParams[iParam].pData, 0, m_pParams[iParam].iLen);
        memcpy(m_pParams[iParam].pData, pDate, sizeof(CGOdbcStmt::DATE));
    }
    else
        throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);

}

//set timestamp value
void CGOdbcStmt::setTimeStamp(int iParam, const CGOdbcStmt::TIMESTAMP *pDate)
{
    if (iParam < 0 || iParam >= m_iParamsCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    if (m_pParams[iParam].iSqlCType == SQL_C_DATE)
    {
        memcpy(m_pParams[iParam].pData, pDate, sizeof(CGOdbcStmt::DATE));
    }
    else
    if (m_pParams[iParam].iSqlCType == SQL_C_TIMESTAMP)
    {
        memcpy(m_pParams[iParam].pData, pDate, sizeof(CGOdbcStmt::TIMESTAMP));
    }
    else
        throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
}

//set guid value
void CGOdbcStmt::setGUID(int iParam, const GUID *pGuid)
{
    if (iParam < 0 || iParam >= m_iParamsCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    if (m_pParams[iParam].iSqlCType == SQL_C_GUID)
    {
        memcpy(m_pParams[iParam].pData, pGuid, sizeof(GUID));
    }
    else
        throw new CGOdbcEx(true, CGOdbcEx::eTypeIsIncompatible);
}

//set parameter value to null
void CGOdbcStmt::setNull(int iParam)
{
    if (iParam < 0 || iParam >= m_iParamsCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    m_pParams[iParam].cbData = SQL_NULL_DATA;
}

//--------------------------------------------------//
//work with fields                                  //
//--------------------------------------------------//
//returns number of columns
int CGOdbcStmt::getColCount()
{
    return m_iColCount;
}

//return columns description
const CGOdbcStmt::COLUMN * CGOdbcStmt::getColumn(int iCol)
{
    if (iCol < 0 || iCol >= m_iColCount)
        throw new CGOdbcEx(true, CGOdbcEx::eIndexOutOfRange);

    return &m_pColumns[iCol];
}

//finds column by name
int CGOdbcStmt::findColumn(const char *szName)
{
    for (int i = 0; i < m_iColCount; i++)
        if (!stricmp(szName, m_pColumns[i].szName))
            return i;
    return -1;
}

//returns integer value
int CGOdbcStmt::getInt(const char *szCol)
{
    int iCol;
    iCol = findColumn(szCol);
    if (iCol == -1)
        throw new CGOdbcEx(true, CGOdbcEx::eFieldNotFound);
    return getInt(iCol);
}

//returns zero-terminated character value
const char * CGOdbcStmt::getChar(const char *szCol)
{
    int iCol;
    iCol = findColumn(szCol);
    if (iCol == -1)
        throw new CGOdbcEx(true, CGOdbcEx::eFieldNotFound);
    return getChar(iCol);
}

//returns length of value
int CGOdbcStmt::getLength(const char *szCol)
{
    int iCol;
    iCol = findColumn(szCol);
    if (iCol == -1)
        throw new CGOdbcEx(true, CGOdbcEx::eFieldNotFound);
    return getLength(iCol);
}

//returns pointer to value
const void * CGOdbcStmt::getPtr(const char *szCol)
{
    int iCol;
    iCol = findColumn(szCol);
    if (iCol == -1)
        throw new CGOdbcEx(true, CGOdbcEx::eFieldNotFound);
    return getPtr(iCol);
}

//returns true if value is null
bool CGOdbcStmt::isNull(const char *szCol)
{
    int iCol;
    iCol = findColumn(szCol);
    if (iCol == -1)
        throw new CGOdbcEx(true, CGOdbcEx::eFieldNotFound);
    return isNull(iCol);
}

//returns number value
double CGOdbcStmt::getNumber(const char *szCol)
{
    int iCol;
    iCol = findColumn(szCol);
    if (iCol == -1)
        throw new CGOdbcEx(true, CGOdbcEx::eFieldNotFound);
    return getNumber(iCol);
}

//returns date-only value
const CGOdbcStmt::DATE * CGOdbcStmt::getDate(const char *szCol)
{
    int iCol;
    iCol = findColumn(szCol);
    if (iCol == -1)
        throw new CGOdbcEx(true, CGOdbcEx::eFieldNotFound);
    return getDate(iCol);
}

//returns timestamp value
const CGOdbcStmt::TIMESTAMP * CGOdbcStmt::getTimeStamp(const char *szCol)
{
    int iCol;
    iCol = findColumn(szCol);
    if (iCol == -1)
        throw new CGOdbcEx(true, CGOdbcEx::eFieldNotFound);
    return getTimeStamp(iCol);
}

//returns value as GUID
const GUID * CGOdbcStmt::getGUID(const char *szCol)
{
    int iCol;
    iCol = findColumn(szCol);
    if (iCol == -1)
        throw new CGOdbcEx(true, CGOdbcEx::eFieldNotFound);
    return getGUID(iCol);
}

//cancel statement
void CGOdbcStmt::cancel()
{
    RETCODE rc = SQLCancel(m_hStmt);
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
        m_sError = "";
        return ;
    }
    else
    {
        _throwError();
        return ;
    }
}

//convert to native SQL
void CGOdbcStmt::toNative(const char *szSQL, char *szNativeSQL, int lMaxLen)
{
    RETCODE rc;
    long lRealLen;
    rc = SQLNativeSql(m_hStmt, (unsigned char *)szSQL, SQL_NTS, (unsigned char *)szNativeSQL, lMaxLen, &lRealLen);
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
        m_sError = "";
        return ;
    }
    else
    {
        _throwError();
        return ;
    }
}

//--------------------------------------------------//
//returns ODBC statement
HSTMT CGOdbcStmt::getStmt()
{
    return m_hStmt;
}

