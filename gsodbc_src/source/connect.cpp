//-[stdhead]-------------------------------------------------------
//Project:
//File   :connect.cpp
//Started:17.09.02 13:56:41
//Updated:25.09.02 10:36:32
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
#include "stdhead.h"
#include "connect.h"
#include "stmt.h"
#include "excpt.h"

CGOdbcConnect::CGOdbcConnect()
{
    SQLAllocEnv(&m_hEnv);
    m_hConn = 0;
    m_sError = "";
    m_sDriver = "";
}

//destructor
CGOdbcConnect::~CGOdbcConnect()
{
    close();
    SQLFreeEnv(m_hEnv);
}


//establish connect by DSN name, user identifier and password
void CGOdbcConnect::connect(const char *szDSN, const char *szUID, const char *szPWD)
{
    close();

    SQLAllocConnect(m_hEnv, &m_hConn);
    SQLSetConnectOption(m_hConn, SQL_ODBC_CURSORS, SQL_CUR_USE_ODBC);
    m_bShared = false;

    RETCODE rc;
    rc = SQLConnect(m_hConn, (unsigned char *)szDSN, SQL_NTS,
                             (unsigned char *)szUID, SQL_NTS,
                             (unsigned char *)szPWD, SQL_NTS);

    if (rc == SQL_ERROR)
    {
        try
        {
            _throwError();
        }
        catch(...)
        {
            SQLFreeConnect(m_hConn);
            m_hConn = 0;
            throw;
        }
        return;
    }
    _getDriver();
    m_sError = "";
    return ;
}

//establish connect by DSN name, user identifier and password
void CGOdbcConnect::connect(const char *szStr, HWND hWnd)
{
    close();

    SQLAllocConnect(m_hEnv, &m_hConn);
    SQLSetConnectOption(m_hConn, SQL_ODBC_CURSORS, SQL_CUR_USE_ODBC);

    RETCODE rc;
    unsigned char szOutStr[1024];
    short cchOutStr = 1024;
    rc = SQLDriverConnect(m_hConn, 0, (unsigned char *)szStr, SQL_NTS,
                                   szOutStr, 1024, &cchOutStr,
                                   SQL_DRIVER_COMPLETE_REQUIRED);
    m_bShared = false;
    if (rc == SQL_ERROR)
    {
        try
        {
            _throwError();
        }
        catch(...)
        {
            SQLFreeConnect(m_hConn);
            m_hConn = 0;
            throw;
        }
        return;
    }

    _getDriver();
    m_sError = "";
    return ;
}

//close connection
void CGOdbcConnect::close()
{
    if (m_hConn)
    {
        if (!m_bShared)
        {
            SQLDisconnect(m_hConn);
            SQLFreeConnect(m_hConn);
        }
        m_hConn = 0;
    }
    m_sError = "";
    m_sDriver = "";
}


//get error description
void CGOdbcConnect::_throwError()
{
    char szState[MAX_PATH] = "", szError[MAX_PATH * 2] ="";
    char szErrorText[MAX_PATH * 4];
    long lNative = 0;
    short cb = 0;

    SQLError(SQL_NULL_HENV, m_hConn, SQL_NULL_HSTMT, (unsigned char *)szState, &lNative,
             (unsigned char *)szError, MAX_PATH * 2, &cb);
    sprintf(szErrorText, "Error = %i\nSQL state = %s\n%s", lNative, szState, szError);
    m_sError = szErrorText;
    throw new CGOdbcEx(false, lNative, szErrorText);
}

//get database driver name
void CGOdbcConnect::_getDriver()
{
    char szDriverName[MAX_PATH] = "";
    short iLen = MAX_PATH;
    RETCODE rc;
    rc = SQLGetInfo(m_hConn, SQL_DRIVER_NAME, szDriverName, MAX_PATH, &iLen);
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
        m_sDriver = szDriverName;
    else
        m_sDriver = "unknown";
}


//returns last error during connection
const char *CGOdbcConnect::getError()
{
    return m_sError;
}

//return name of the driver
const char *CGOdbcConnect::getDriver()
{
    return m_sDriver;
}

CGOdbcStmt *CGOdbcConnect::createStatement()
{
    return new CGOdbcStmt(m_hConn);
}

void CGOdbcConnect::freeStatement(CGOdbcStmt *pStmt)
{
    delete pStmt;
}

//execute and bind select query
CGOdbcStmt *CGOdbcConnect::executeSelect(const char *szSql)
{
    CGOdbcStmt *pStmt = new CGOdbcStmt(m_hConn);
    try
    {
        pStmt->execute(szSql);
        if (!pStmt->bindAuto())
        {
            delete pStmt;
            return 0;
        }
        return pStmt;
    }
    catch(...)
    {
        delete pStmt;
        throw;
    }
}

//execute query and returns number of affected rows
int CGOdbcConnect::executeUpdate(const char *szSql)
{
    CGOdbcStmt *pStmt = new CGOdbcStmt(m_hConn);
    try
    {
        pStmt->execute(szSql);
        int iRet;
        iRet = pStmt->getRowCount();
        delete pStmt;
        return iRet;
    }
    catch(...)
    {
        delete pStmt;
        throw;
    }
}

//set transaction processing mode
void CGOdbcConnect::setTransMode(bool bAutoCommit)
{
    RETCODE rc;
    if (bAutoCommit)
        rc = SQLSetConnectOption(m_hConn, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_ON);
    else
        rc = SQLSetConnectOption(m_hConn, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);

    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        _throwError();
}

//commit transaction
void CGOdbcConnect::commit()
{
    RETCODE rc;
    rc = SQLEndTran(SQL_HANDLE_DBC, m_hConn, SQL_COMMIT);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        _throwError();
}

//rollback transaction
void CGOdbcConnect::rollback()
{
    RETCODE rc;
    rc = SQLEndTran(SQL_HANDLE_DBC, m_hConn, SQL_ROLLBACK);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        _throwError();
}

//get connection handle
HDBC CGOdbcConnect::getConnect()
{
    return m_hConn;
}

//set shared connection handle
void CGOdbcConnect::setConnect(HDBC hConn)
{
    close();
    m_bShared = true;
    m_hConn = hConn;
}


//select list of datatypes
CGOdbcStmt * CGOdbcConnect::getListOfTypes()
{
    CGOdbcStmt *pStmt = new CGOdbcStmt(m_hConn);
    try
    {
        RETCODE rc;

        rc = SQLGetTypeInfo(pStmt->m_hStmt, SQL_ALL_TYPES);

        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            pStmt->_throwError();

        if (!pStmt->bindAuto())
        {
            delete pStmt;
            return 0;
        }
        return pStmt;
    }
    catch(...)
    {
        delete pStmt;
        throw;
    }
}

//select list of tables
CGOdbcStmt * CGOdbcConnect::getListOfTables(const char * szTableMask)
{
    CGOdbcStmt *pStmt = new CGOdbcStmt(m_hConn);
    try
    {
        RETCODE rc;
        rc = SQLTables(pStmt->m_hStmt, NULL, SQL_NTS, NULL, SQL_NTS, (unsigned char *)szTableMask, SQL_NTS, NULL, SQL_NTS);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            pStmt->_throwError();

        if (!pStmt->bindAuto())
        {
            delete pStmt;
            return 0;
        }
        return pStmt;
    }
    catch(...)
    {
        delete pStmt;
        throw;
    }
}

//select list of tables
CGOdbcStmt * CGOdbcConnect::getListOfColumns(const char * szTableMask)
{
    CGOdbcStmt *pStmt = new CGOdbcStmt(m_hConn);
    try
    {
        RETCODE rc;
        rc = SQLColumns(pStmt->m_hStmt, NULL, SQL_NTS, NULL, SQL_NTS, (unsigned char *)szTableMask, SQL_NTS, NULL, SQL_NTS);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            pStmt->_throwError();

        if (!pStmt->bindAuto())
        {
            delete pStmt;
            return 0;
        }
        return pStmt;
    }
    catch(...)
    {
        delete pStmt;
        throw;
    }
}

//information about DSN
bool CGOdbcConnect::firstDSN(char *pszDSN, int lMax)
{
    RETCODE rc;
    short cbData, cbData1;
    char szDesc[256];

    rc = SQLDataSources(m_hEnv, SQL_FETCH_FIRST, (unsigned char *)pszDSN, lMax, &cbData,
                                                 (unsigned char *)szDesc, 256, &cbData1);
    return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}



bool CGOdbcConnect::nextDSN(char *pszDSN, int lMax)
{
    RETCODE rc;
    short cbData, cbData1;
    char szDesc[256];

    rc = SQLDataSources(m_hEnv, SQL_FETCH_NEXT, (unsigned char *)pszDSN, lMax, &cbData,
                                                (unsigned char *)szDesc, 256, &cbData1);
    return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}

