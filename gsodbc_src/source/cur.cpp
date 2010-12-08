//-[stdhead]-------------------------------------------------------
//Project:
//File   :curex.cpp
//Started:23.09.02 18:52:17
//Updated:25.09.02 14:56:41
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
#include "stdhead.h"
#include "stmt.h"
#include "cur.h"
#include "curidx.h"
#include "curex.h"

//initialize variables
void CGCursor::_init()
{
    m_pMainFile = 0;
    m_pLobFile = 0;
    m_pColumns = 0;
    m_pColBufs = 0;
    m_pData = 0;
    m_bPersist = false;
    *m_szFileName = 0;
    *m_szLobFileName = 0;
    m_lRowCount = 0;
    m_lColCount = 0;
}


//constructor for statement
CGCursor::CGCursor(CGOdbcStmt *pStmt, const char *szFileName)
{
    _init();

    //create files
    if (szFileName)
    {
        strcpy(m_szFileName, szFileName);
        strcpy(m_szLobFileName, szFileName);
        strcat(m_szLobFileName, ".lob");
        m_bPersist = true;
    }
    else
    {
        char szTemp[MAX_PATH];
        GetTempPath(MAX_PATH, szTemp);
        GetTempFileName(szTemp, "curex", 0, m_szFileName);
        strcpy(m_szLobFileName, m_szFileName);
        strcat(m_szLobFileName, ".lob");
        m_bPersist = false;
    }

    m_pMainFile = fopen(m_szFileName, "w+b");
    if (!m_pMainFile)
        throw new CGCursorEx(CGCursorEx::eFileOpen, m_szFileName);

    m_pLobFile = fopen(m_szLobFileName, "w+b");
    m_lLobSize = 0;

    if (!m_pLobFile)
        throw new CGCursorEx(CGCursorEx::eFileOpen, m_szLobFileName);

    //retrieve column's descriptions
    m_lColCount = pStmt->getColCount();

    if (m_lColCount < 1)
        throw new CGCursorEx(CGCursorEx::eNoColumns);

    m_pColumns = new COLUMN[m_lColCount];
    memset(m_pColumns, 0, sizeof(COLUMN) * m_lColCount);
    m_pColBufs = new COLBUF[m_lColCount];
    memset(m_pColBufs, 0, sizeof(COLBUF) * m_lColCount);

    int i;
    long lBufSize = 0;

    lBufSize += 1;          //marker of row status
    for (i = 0; i < m_lColCount; i++)
    {
        const CGOdbcStmt::COLUMN *pCol = pStmt->getColumn(i);

        strcpy(m_pColumns[i].szName, pCol->szName);

        switch (pCol->iSqlCType)
        {
        case    SQL_C_CHAR:
                if (pCol->bLOB)
                {
                    m_pColumns[i].eType = eCLOB;
                    m_pColumns[i].lSize = 8;
                }
                else
                {
                    m_pColumns[i].eType = eChar;
                    m_pColumns[i].lSize = pCol->iLen;
                }
                break;
        case    SQL_C_LONG:
                m_pColumns[i].eType = eInt;
                m_pColumns[i].lSize = 4;
                break;
        case    SQL_C_DOUBLE:
                m_pColumns[i].eType = eDouble;
                m_pColumns[i].lSize = 8;
                m_pColumns[i].lScale = pCol->iScale;

                break;
        case    SQL_C_BINARY:
                if (pCol->bLOB)
                {
                    m_pColumns[i].eType = eBLOB;
                    m_pColumns[i].lSize = 8;
                }
                else
                {
                    m_pColumns[i].eType = eBinary;
                    m_pColumns[i].lSize = pCol->iLen;
                }
                break;
        case    SQL_C_DATE:
                m_pColumns[i].eType = eDate;
                m_pColumns[i].lSize = sizeof(CGOdbcStmt::DATE);
                break;
        case    SQL_C_TIMESTAMP:
                m_pColumns[i].eType = eTimeStamp;
                m_pColumns[i].lSize = sizeof(CGOdbcStmt::TIMESTAMP);
                break;
        case    SQL_C_GUID:
                m_pColumns[i].eType = eGUID;
                m_pColumns[i].lSize = sizeof(GUID);
                break;
        default:
                throw new CGCursorEx(CGCursorEx::eUnknownType, pCol->iSqlCType);
                break;
        }
        lBufSize += 1;                      //null flag
        lBufSize += m_pColumns[i].lSize;    //row data
    }

    int lWritten;
    m_lRowCount = 0;
    lWritten = fwrite(&m_lColCount, 1, sizeof(m_lColCount), m_pMainFile);
    if (lWritten != sizeof(m_lColCount))
        throw new CGCursorEx(CGCursorEx::eWriteError);

    lWritten = fwrite(&lBufSize, 1, sizeof(m_lColCount), m_pMainFile);
    if (lWritten != sizeof(m_lColCount))
        throw new CGCursorEx(CGCursorEx::eWriteError);

    i = 0;
    lWritten = fwrite(&i, 1, sizeof(m_lColCount), m_pMainFile);
    if (lWritten != sizeof(m_lColCount))
        throw new CGCursorEx(CGCursorEx::eWriteError);

    lWritten = fwrite(m_pColumns, 1, sizeof(COLUMN) * m_lColCount, m_pMainFile);
    if (lWritten != sizeof(COLUMN) * m_lColCount)
        throw new CGCursorEx(CGCursorEx::eWriteError);

    m_lDataOffset = ftell(m_pMainFile);

    m_pData = new unsigned char[lBufSize];
    memset(m_pData, 0, sizeof(lBufSize));

    m_lBufSize = lBufSize;

    unsigned char *pData = m_pData + 1;
    for (i = 0; i < m_lColCount; i++)
    {
        m_pColBufs[i].pData = pData;
        m_pColBufs[i].pLOB = 0;
        pData += (m_pColumns[i].lSize + 1);
    }
    //copy data from cursor to file
    bool bRC;

    for (bRC = pStmt->first(); bRC; bRC = pStmt->next())
    {
        memset(m_pData, 0, m_lBufSize);
        *m_pData = eMirror;
        for (i = 0; i < m_lColCount; i++)
        {
            if (pStmt->isNull(i))
                *m_pColBufs[i].pData = 0;
            else
            {
                *m_pColBufs[i].pData = 1;
                switch(m_pColumns[i].eType)
                {
                case    eInt:
                        *(long *)(m_pColBufs[i].pData + 1) = pStmt->getInt(i);
                        break;
                case    eChar:
                        strcpy((char *)(m_pColBufs[i].pData + 1), pStmt->getChar(i));
                        break;
                case    eBinary:
                        memcpy((void *)(m_pColBufs[i].pData + 1), pStmt->getPtr(i), pStmt->getLength(i));
                        break;
                case    eDouble:
                        *(double *)(m_pColBufs[i].pData + 1) = pStmt->getNumber(i);
                        break;
                case    eDate:
                        memcpy((void *)(m_pColBufs[i].pData + 1), pStmt->getPtr(i), sizeof(CGOdbcStmt::DATE));
                        break;
                case    eTimeStamp:
                        memcpy((void *)(m_pColBufs[i].pData + 1), pStmt->getPtr(i), sizeof(CGOdbcStmt::TIMESTAMP));
                        break;
                case    eGUID:
                        memcpy((void *)(m_pColBufs[i].pData + 1), pStmt->getPtr(i), sizeof(GUID));
                        break;
                case    eCLOB:
                case    eBLOB:
                        const void *pData;
                        int lData;
                        pData = pStmt->getPtr(i);
                        lData = pStmt->getLength(i);
                        *(long *)(m_pColBufs[i].pData + 1) = m_lLobSize;
                        *(long *)(m_pColBufs[i].pData + 1 + 4) = lData;
                        lWritten = fwrite(pData, 1, lData, m_pLobFile);
                        if (lWritten != lData)
                            throw new CGCursorEx(CGCursorEx::eWriteError);
                        m_lLobSize += lWritten;
                        break;
                }
            }
        }
        //buffer is ready
        lWritten = fwrite(m_pData, 1, m_lBufSize, m_pMainFile);
        if (lWritten != m_lBufSize)
            throw new CGCursorEx(CGCursorEx::eWriteError);
        m_lRowCount++;
    }
    //cursor is constructed!
    if (m_lRowCount)
        go(1);
    else
        m_lCurRow = 0;
}

//constructor for loading from persistent file
CGCursor::CGCursor(const char *szFileName)
{
    _init();

    strcpy(m_szFileName, szFileName);
    strcpy(m_szLobFileName, szFileName);
    strcat(m_szLobFileName, ".lob");
    m_bPersist = true;

    m_pMainFile = fopen(m_szFileName, "r+b");
    if (!m_pMainFile)
        throw new CGCursorEx(CGCursorEx::eFileOpen, m_szFileName);

    m_pLobFile = fopen(m_szLobFileName, "r+b");

    if (!m_pLobFile)
        throw new CGCursorEx(CGCursorEx::eFileOpen, m_szLobFileName);

    fseek(m_pLobFile, 0, SEEK_END);
    m_lLobSize = ftell(m_pLobFile);
    fseek(m_pLobFile, 0, SEEK_SET);

    int lRead;

    lRead = fread(&m_lColCount, 1, sizeof(m_lColCount), m_pMainFile);
    if (lRead != sizeof(int))
        throw new CGCursorEx(CGCursorEx::eReadError);

    lRead = fread(&m_lBufSize, 1, sizeof(m_lBufSize), m_pMainFile);
    if (lRead != sizeof(int))
        throw new CGCursorEx(CGCursorEx::eReadError);

    lRead = fread(&m_lRowCount, 1, sizeof(m_lRowCount), m_pMainFile);
    if (lRead != sizeof(int))
        throw new CGCursorEx(CGCursorEx::eReadError);

    m_pColumns = new COLUMN[m_lColCount];
    memset(m_pColumns, 0, sizeof(COLUMN) * m_lColCount);
    m_pColBufs = new COLBUF[m_lColCount];
    memset(m_pColBufs, 0, sizeof(COLBUF) * m_lColCount);

    lRead = fread(m_pColumns, 1, sizeof(COLUMN) * m_lColCount, m_pMainFile);
    if (lRead != sizeof(COLUMN) * m_lColCount)
        throw new CGCursorEx(CGCursorEx::eReadError);

    m_lDataOffset = ftell(m_pMainFile);

    m_pData = new unsigned char[m_lBufSize];
    memset(m_pData, 0, sizeof(m_lBufSize));

    int i;
    unsigned char *pData = m_pData + 1;
    for (i = 0; i < m_lColCount; i++)
    {
        m_pColBufs[i].pData = pData;
        m_pColBufs[i].pLOB = 0;
        pData += (m_pColumns[i].lSize + 1);
    }

    //cursor is constructed!
    if (m_lRowCount)
        go(1);
    else
        m_lCurRow = 0;
}

//constructor for new persistent file
CGCursor::CGCursor(int iColumn, CGCursor::COLUMN * pColumn, const char * szFileName)
{
    _init();

    //create files
    if (szFileName)
    {
        strcpy(m_szFileName, szFileName);
        strcpy(m_szLobFileName, szFileName);
        strcat(m_szLobFileName, ".lob");
        m_bPersist = true;
    }
    else
    {
        char szTemp[MAX_PATH];
        GetTempPath(MAX_PATH, szTemp);
        GetTempFileName(szTemp, "curex", 0, m_szFileName);
        strcpy(m_szLobFileName, m_szFileName);
        strcat(m_szLobFileName, ".lob");
        m_bPersist = false;
    }

    m_pMainFile = fopen(m_szFileName, "w+b");
    if (!m_pMainFile)
        throw new CGCursorEx(CGCursorEx::eFileOpen, m_szFileName);

    m_pLobFile = fopen(m_szLobFileName, "w+b");
    m_lLobSize = 0;

    if (!m_pLobFile)
        throw new CGCursorEx(CGCursorEx::eFileOpen, m_szLobFileName);

    //retrieve column's descriptions
    m_lColCount = iColumn;

    if (m_lColCount < 1)
        throw new CGCursorEx(CGCursorEx::eNoColumns);

    m_pColumns = new COLUMN[m_lColCount];
    memset(m_pColumns, 0, sizeof(COLUMN) * m_lColCount);
    m_pColBufs = new COLBUF[m_lColCount];
    memset(m_pColBufs, 0, sizeof(COLBUF) * m_lColCount);

    int i;
    long lBufSize = 0;

    lBufSize += 1;          //marker of row status
    for (i = 0; i < m_lColCount; i++)
    {
        memcpy(&m_pColumns[i], &pColumn[i], sizeof(COLUMN));
        switch (m_pColumns[i].eType)
        {
        case    eCLOB:
                m_pColumns[i].lSize = 8;
                break;
        case    eChar:
                break;
        case    eInt:
                m_pColumns[i].lSize = 4;
                break;
        case    eDouble:
                m_pColumns[i].lSize = 8;
                break;
        case    eBLOB:
                m_pColumns[i].lSize = 8;
                break;
        case    eBinary:
                break;
        case    eDate:
                m_pColumns[i].lSize = sizeof(CGOdbcStmt::DATE);
                break;
        case    eTimeStamp:
                m_pColumns[i].lSize = sizeof(CGOdbcStmt::TIMESTAMP);
                break;
        case    eGUID:
                m_pColumns[i].lSize = sizeof(GUID);
                break;
        default:
                throw new CGCursorEx(CGCursorEx::eUnknownType, m_pColumns[i].eType);
                break;
        }
        lBufSize += 1;                      //null flag
        lBufSize += m_pColumns[i].lSize;    //row data
    }

    int lWritten;
    m_lRowCount = 0;
    lWritten = fwrite(&m_lColCount, 1, sizeof(m_lColCount), m_pMainFile);
    if (lWritten != sizeof(m_lColCount))
        throw new CGCursorEx(CGCursorEx::eWriteError);

    lWritten = fwrite(&lBufSize, 1, sizeof(m_lColCount), m_pMainFile);
    if (lWritten != sizeof(m_lColCount))
        throw new CGCursorEx(CGCursorEx::eWriteError);

    i = 0;
    lWritten = fwrite(&i, 1, sizeof(m_lColCount), m_pMainFile);
    if (lWritten != sizeof(m_lColCount))
        throw new CGCursorEx(CGCursorEx::eWriteError);

    lWritten = fwrite(m_pColumns, 1, sizeof(COLUMN) * m_lColCount, m_pMainFile);
    if (lWritten != sizeof(COLUMN) * m_lColCount)
        throw new CGCursorEx(CGCursorEx::eWriteError);

    m_lDataOffset = ftell(m_pMainFile);

    m_pData = new unsigned char[lBufSize];
    memset(m_pData, 0, sizeof(lBufSize));

    m_lBufSize = lBufSize;

    unsigned char *pData = m_pData + 1;
    for (i = 0; i < m_lColCount; i++)
    {
        m_pColBufs[i].pData = pData;
        m_pColBufs[i].pLOB = 0;
        pData += (m_pColumns[i].lSize + 1);
    }
    m_lCurRow = 0;
}


//destructor
CGCursor::~CGCursor()
{
    if (m_pMainFile)
    {
        if (m_bPersist)
        {
            fseek(m_pMainFile, 8, SEEK_SET);
            fwrite(&m_lRowCount, 1, sizeof(int), m_pMainFile);
        }
        fclose(m_pMainFile);
        m_pMainFile = 0;
    }
    if (m_pLobFile)
    {
        fclose(m_pLobFile);
        m_pLobFile = 0;
    }
    if (strlen(m_szFileName) && !m_bPersist)
        DeleteFile(m_szFileName);
    if (strlen(m_szLobFileName) && !m_bPersist)
        DeleteFile(m_szLobFileName);
    if (m_pColumns)
    {
        delete m_pColumns;
        m_pColumns = 0;
    }
    if (m_pColBufs)
    {
        delete m_pColBufs;
        m_pColBufs = 0;
    }
    if (m_pData)
    {
        delete m_pData;
        m_pData = 0;
    }
    _init();
}

//prepare to move operation
void CGCursor::_beforeMove()
{
    int i;
    for (i = 0; i < m_lColCount; i++)
    {
        if (m_pColBufs[i].pLOB)
        {
            delete m_pColBufs[i].pLOB;
            m_pColBufs[i].pLOB = 0;
            m_pColBufs[i].bLOBChanged = false;
        }
    }
}

//----------------------------------------------//
//movements                                     //
//----------------------------------------------//
//go to specified row
void CGCursor::go(int iRow)
{
    if (iRow < 1 || iRow > m_lRowCount)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, iRow);
    _beforeMove();
    fseek(m_pMainFile, (iRow - 1) * m_lBufSize + m_lDataOffset, SEEK_SET);
    long lRead;
    lRead = fread(m_pData, 1, m_lBufSize, m_pMainFile);
    if (lRead != m_lBufSize)
        throw new CGCursorEx(CGCursorEx::eReadError, iRow);
    m_lCurRow = iRow;
}

//returns true if row is deleted row
bool CGCursor::isRowDeleted()
{
    return *m_pData == eDeleted;
}

//returns true if row is updated row
bool CGCursor::isRowUpdated()
{
    return *m_pData == eUpdated;
}

//returns true if row is new row
bool CGCursor::isRowNew()
{
    return *m_pData == eNew;
}

//----------------------------------------------//
//returns integer value
int CGCursor::getInt(int iColumn)
{
    if (m_lCurRow == 0)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);
    switch (m_pColumns[iColumn].eType)
    {
    case    eInt:
            return *(int *)(m_pColBufs[iColumn].pData + 1);
    case    eDouble:
            return (int)*(double *)(m_pColBufs[iColumn].pData + 1);
    case    eChar:
            return atoi((const char *)m_pColBufs[iColumn].pData + 1);
    default:
            throw new CGCursorEx(CGCursorEx::eIncompatibleType);
    }
}


//returns character value
const char * CGCursor::getChar(int iColumn)
{
    if (m_lCurRow == 0)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);

    if (isNull(iColumn))
        return (const char *)"(null)";

    char szBuff[1024];
    char szFrmt[32];
    SYSTEMTIME st;
    int i;
    GUID guid;

    switch (m_pColumns[iColumn].eType)
    {
    case    eChar:
            return (const char *)(m_pColBufs[iColumn].pData + 1);
    case    eCLOB:
            _loadLOB(iColumn);
            return (const char *)m_pColBufs[iColumn].pLOB;
    case    eInt:
            sprintf(szBuff, "%i", *(int *)(m_pColBufs[iColumn].pData + 1));
            m_sLastStr = szBuff;
            return m_sLastStr;
    case    eDouble:
            sprintf(szFrmt, "%%.%if", m_pColumns[iColumn].lScale);
            sprintf(szBuff, szFrmt, *(double *)(m_pColBufs[iColumn].pData + 1));
            m_sLastStr = szBuff;
            return m_sLastStr;
    case    eDate:
            memset(&st, 0, sizeof(st));
            st.wYear = ((CGOdbcStmt::DATE *)(m_pColBufs[iColumn].pData + 1))->iYear;
            st.wMonth = ((CGOdbcStmt::DATE *)(m_pColBufs[iColumn].pData + 1))->iMonth;
            st.wDay = ((CGOdbcStmt::DATE *)(m_pColBufs[iColumn].pData + 1))->iDay;
            GetDateFormat(LOCALE_USER_DEFAULT,
                          DATE_SHORTDATE,
                          &st, NULL, szBuff, 1024);
            m_sLastStr = szBuff;
            return m_sLastStr;
    case    eTimeStamp:
            memset(&st, 0, sizeof(st));
            st.wYear = ((CGOdbcStmt::TIMESTAMP *)(m_pColBufs[iColumn].pData + 1))->iYear;
            st.wMonth = ((CGOdbcStmt::TIMESTAMP  *)(m_pColBufs[iColumn].pData + 1))->iMonth;
            st.wDay = ((CGOdbcStmt::TIMESTAMP  *)(m_pColBufs[iColumn].pData + 1))->iDay;
            st.wHour = ((CGOdbcStmt::TIMESTAMP  *)(m_pColBufs[iColumn].pData + 1))->iHour;
            st.wMinute = ((CGOdbcStmt::TIMESTAMP  *)(m_pColBufs[iColumn].pData + 1))->iMin;
            st.wSecond = ((CGOdbcStmt::TIMESTAMP  *)(m_pColBufs[iColumn].pData + 1))->iSec;
            GetDateFormat(LOCALE_USER_DEFAULT,
                          DATE_SHORTDATE,
                          &st, NULL, szBuff, 1024);
            i = strlen(szBuff);
            szBuff[i++] = ' ';
            GetTimeFormat(LOCALE_USER_DEFAULT,
                          0, &st, NULL, szBuff + i, 1024 - i);
            m_sLastStr = szBuff;
            return m_sLastStr;
    case    eGUID:
            memcpy(&guid, m_pColBufs[iColumn].pData + 1, sizeof(GUID));
            sprintf(szBuff, "%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X",
                             guid.Data1,guid.Data2,guid.Data3,
                             ((WORD)guid.Data4[0]<<8) + guid.Data4[1], guid.Data4[2], guid.Data4[3],
                             guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
            m_sLastStr = szBuff;
            return m_sLastStr;
    default:
            throw new CGCursorEx(CGCursorEx::eIncompatibleType);
    }
}

//returns numeric value
double CGCursor::getNumber(int iColumn)
{
    if (m_lCurRow == 0)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);
    switch (m_pColumns[iColumn].eType)
    {
    case    eInt:
            return (double)*(int *)(m_pColBufs[iColumn].pData + 1);
    case    eDouble:
            return *(double *)(m_pColBufs[iColumn].pData + 1);
    case    eChar:
            return atof((const char *)m_pColBufs[iColumn].pData + 1);
    default:
            throw new CGCursorEx(CGCursorEx::eIncompatibleType);
    }
}

//returns data value
const CGOdbcStmt::DATE * CGCursor::getDate(int iColumn)
{
    if (m_lCurRow == 0)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);
    if (m_pColumns[iColumn].eType == eDate ||
        m_pColumns[iColumn].eType == eTimeStamp)
    {
        return (const CGOdbcStmt::DATE *)(m_pColBufs[iColumn].pData + 1);
    }
    else
        throw new CGCursorEx(CGCursorEx::eIncompatibleType);
}

//returns timestamp value
const CGOdbcStmt::TIMESTAMP * CGCursor::getTimeStamp(int iColumn)
{
    if (m_lCurRow == 0)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);
    if (m_pColumns[iColumn].eType == eTimeStamp)
        return (const CGOdbcStmt::TIMESTAMP *)(m_pColBufs[iColumn].pData + 1);
    else
        throw new CGCursorEx(CGCursorEx::eIncompatibleType);
}

//returns pointer value
const void * CGCursor::getPtr(int iColumn)
{
    if (m_lCurRow == 0)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);
    if (m_pColumns[iColumn].eType == eBLOB || m_pColumns[iColumn].eType == eCLOB)
    {
        _loadLOB(iColumn);
        return (const void  *)m_pColBufs[iColumn].pLOB;
    }
    else
        return (const void  *)(m_pColBufs[iColumn].pData + 1);

}

//get length of the value
int CGCursor::getLength(int iColumn)
{
    if (m_lCurRow == 0)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);
    if (m_pColumns[iColumn].eType == eBLOB || m_pColumns[iColumn].eType == eCLOB)
        return *(int *)(m_pColBufs[iColumn].pData + 1 + 4);
    else
        return m_pColumns[iColumn].lSize;
}

//returns GUID value
const GUID * CGCursor::getGUID(int iColumn)
{
    if (m_lCurRow == 0)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);
    if (m_pColumns[iColumn].eType == eGUID)
        return (const GUID *)(m_pColBufs[iColumn].pData + 1);
    else
        throw new CGCursorEx(CGCursorEx::eIncompatibleType);
}

//returns true if value is null value
bool CGCursor::isNull(int iColumn)
{
    if (m_lCurRow == 0)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);
    return *m_pColBufs[iColumn].pData == 0;
}

//load LOB object
void CGCursor::_loadLOB(int i)
{
    if (m_pColBufs[i].pLOB)
        return;
    int iOffset, iLength, lRead;
    iOffset = *(int *)(m_pColBufs[i].pData + 1);
    iLength = *(int *)(m_pColBufs[i].pData + 1 + 4);
    fseek(m_pLobFile, iOffset, SEEK_SET);
    unsigned char *pBuff = new unsigned char[iLength];
    lRead = fread(pBuff, 1, iLength, m_pLobFile);
    if (lRead != iLength)
    {
        delete pBuff;
        throw new CGCursorEx(CGCursorEx::eReadError, m_lCurRow);
    }
    m_pColBufs[i].pLOB = pBuff;
}

//returns number of rows
int CGCursor::getRowCount()
{
    return m_lRowCount;
}

//returns number of current row
int CGCursor::getRowNo()
{
    return m_lCurRow;
}

//----------------------------------------------//
int CGCursor::getColCount()
{
    return m_lColCount;
}

const CGCursor::COLUMN *CGCursor::getColumn(int iColumn)
{
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);
    return &m_pColumns[iColumn];
}


void CGCursor::_addIndex(CGCursorIndex *pIndex)
{
    m_aIndexes.push_back(pIndex);
}

void CGCursor::_removeIndex(CGCursorIndex *pIndex)
{
    int i;
    for (i = 0; i < m_aIndexes.size(); i++)
    {
        if (m_aIndexes.at(i) == pIndex)
        {
            m_aIndexes.erase(m_aIndexes.begin() + i);
            return;
        }
    }
}

//find column in list
int CGCursor::findColumn(const char *szName)
{
    int i;
    for (i = 0; i < m_lColCount; i++)
        if (!stricmp(szName, m_pColumns[i].szName))
            return i;
    return -1;
}

//----------------------------------------------//
//returns integer value
int CGCursor::getInt(const char *szName)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    return getInt(iCol);
}

//returns character value
const char * CGCursor::getChar(const char *szName)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    return getChar(iCol);
}

//returns numeric value
double CGCursor::getNumber(const char *szName)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    return getNumber(iCol);
}

//returns data value
const CGOdbcStmt::DATE * CGCursor::getDate(const char *szName)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    return getDate(iCol);
}

//returns timestamp value
const CGOdbcStmt::TIMESTAMP * CGCursor::getTimeStamp(const char *szName)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    return getTimeStamp(iCol);
}

//returns pointer value
const void * CGCursor::getPtr(const char *szName)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    return getPtr(iCol);
}

//returns length of the value
int CGCursor::getLength(const char *szName)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    return getLength(iCol);
}

//returns GUID value
const GUID * CGCursor::getGUID(const char *szName)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    return getGUID(iCol);
}

//returns true if value is null value
bool CGCursor::isNull(const char *szName)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    return isNull(iCol);
}

//----------------------------------------------//
//update operations                             //
//----------------------------------------------//
//marks row as delete
void CGCursor::deleteRow()
{
    if (m_lCurRow == 0)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);

    *m_pData = eDeleted;
    update();
}

//update current row
void CGCursor::update()
{
    int i, lWritten, lTemp;

    switch (*m_pData)
    {
    case    eNew:
            m_lCurRow = m_lRowCount + 1;
            m_lRowCount++;
    case    eDeleted:
    case    eUpdated:
            //update changed LOBs
            for (i = 0; i < m_lColCount; i++)
            {
                if (m_pColBufs[i].bLOBChanged)
                {
                    if (*m_pColBufs[i].pData != 0)  //is not null
                    {
                        fseek(m_pLobFile, 0, SEEK_END);
                        *(long *)(m_pColBufs[i].pData + 1) = ftell(m_pLobFile);
                        lWritten = fwrite(m_pColBufs[i].pLOB, 1, *(long *)(m_pColBufs[i].pData + 1 + 4), m_pLobFile);
                        if (lWritten != *(long *)(m_pColBufs[i].pData + 1 + 4))
                            throw new CGCursorEx(CGCursorEx::eWriteError);
                        m_lLobSize = ftell(m_pLobFile);
                    }
                    else
                    {
                        *(long *)(m_pColBufs[i].pData + 1) = 0;
                        *(long *)(m_pColBufs[i].pData + 1 + 4) = 0;
                    }
                }
            }
            *m_pData = eMirror;
            fseek(m_pMainFile, (m_lCurRow - 1) * m_lBufSize + m_lDataOffset, SEEK_SET);
            lWritten = fwrite(m_pData, 1, m_lBufSize, m_pMainFile);
            if (lWritten != m_lBufSize)
                throw new CGCursorEx(CGCursorEx::eWriteError);
            lTemp = m_lCurRow;
            for (i = 0; i < m_aIndexes.size(); i++)
            {
                m_aIndexes.at(i)->_removeRow(lTemp);
                m_aIndexes.at(i)->_addRow(lTemp);
            }
            go(lTemp);
            break;
    default:
            break;
    }
}

//append new row
void CGCursor::append()
{
    m_lCurRow = 0;
    _beforeMove();
    memset(m_pData, 0, m_lBufSize);
    int i;
    for (i = 0; i < m_lColCount; i++)
        *m_pColBufs[i].pData = 0;            //
    *m_pData = eNew;
}

//----------------------------------------------//
//set integer value
void CGCursor::setInt(int iColumn, int iVal)
{
    if (m_lCurRow == 0 && *m_pData != eNew)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);

    switch (m_pColumns[iColumn].eType)
    {
    case    eInt:
            *(int *)(m_pColBufs[iColumn].pData + 1) = iVal;
            break;
    case    eDouble:
            *(double *)(m_pColBufs[iColumn].pData + 1) = iVal;
            break;
    default:
            throw new CGCursorEx(CGCursorEx::eIncompatibleType);
    }
    *m_pColBufs[iColumn].pData = 1;
}

//set character value
void CGCursor::setChar(int iColumn, const char *szVal)
{
    if (m_lCurRow == 0 && *m_pData != eNew)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);
    char *szBuff;
    int iLen;
    switch (m_pColumns[iColumn].eType)
    {
    case    eInt:
            *(int *)(m_pColBufs[iColumn].pData + 1) = atoi(szVal);
            break;
    case    eDouble:
            *(double *)(m_pColBufs[iColumn].pData + 1) = atof(szVal);
            break;
    case    eChar:
            memset((char *)m_pColBufs[iColumn].pData + 1, 0, m_pColumns[iColumn].lSize);
            strncpy((char *)m_pColBufs[iColumn].pData + 1, szVal, m_pColumns[iColumn].lSize - 1);
            break;
    case    eCLOB:
    case    eBLOB:
            iLen = strlen(szVal) + 1;
            szBuff = new char[iLen];
            memcpy(szBuff, szVal, iLen);

            if (m_pColBufs[iColumn].pLOB)
                delete m_pColBufs[iColumn].pLOB;

            *(int *)(m_pColBufs[iColumn].pData + 1 + 4) = iLen;
            m_pColBufs[iColumn].pLOB = (unsigned char *)szBuff;
            m_pColBufs[iColumn].bLOBChanged = true;
            break;
    default:
            throw new CGCursorEx(CGCursorEx::eIncompatibleType);
    }
    *m_pColBufs[iColumn].pData = 1;
}

//set numeric value
void CGCursor::setNumber(int iColumn, double dblVal)
{
    if (m_lCurRow == 0 && *m_pData != eNew)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);

    switch (m_pColumns[iColumn].eType)
    {
    case    eInt:
            *(int *)(m_pColBufs[iColumn].pData + 1) = dblVal;
            break;
    case    eDouble:
            *(double *)(m_pColBufs[iColumn].pData + 1) = dblVal;
            break;
    default:
            throw new CGCursorEx(CGCursorEx::eIncompatibleType);
    }
    *m_pColBufs[iColumn].pData = 1;
}

//set date value
void CGCursor::setDate(int iColumn, const CGOdbcStmt::DATE *pDate)
{
    if (m_lCurRow == 0 && *m_pData != eNew)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);

    switch (m_pColumns[iColumn].eType)
    {
    case    eDate:
            memcpy(m_pColBufs[iColumn].pData + 1, pDate, sizeof(CGOdbcStmt::DATE));
            break;
    case    eTimeStamp:
            memset(m_pColBufs[iColumn].pData + 1, 0, sizeof(CGOdbcStmt::TIMESTAMP));
            memcpy(m_pColBufs[iColumn].pData + 1, pDate, sizeof(CGOdbcStmt::DATE));
            break;
    default:
            throw new CGCursorEx(CGCursorEx::eIncompatibleType);
    }
    *m_pColBufs[iColumn].pData = 1;
}

//set timestamp value
void CGCursor::setTimeStamp(int iColumn, const CGOdbcStmt::TIMESTAMP *pDate)
{
    if (m_lCurRow == 0 && *m_pData != eNew)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);

    switch (m_pColumns[iColumn].eType)
    {
    case    eDate:
            memcpy(m_pColBufs[iColumn].pData + 1, pDate, sizeof(CGOdbcStmt::DATE));
            break;
    case    eTimeStamp:
            memcpy(m_pColBufs[iColumn].pData + 1, pDate, sizeof(CGOdbcStmt::TIMESTAMP));
            break;
    default:
            throw new CGCursorEx(CGCursorEx::eIncompatibleType);
    }
    *m_pColBufs[iColumn].pData = 1;
}

//set binary value
void CGCursor::setBin(int iColumn, const void *_pBuff, int iLen)
{
    if (m_lCurRow == 0 && *m_pData != eNew)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);
    unsigned char *pBuff;
    switch (m_pColumns[iColumn].eType)
    {
    case    eBinary:
            if (iLen > m_pColumns[iColumn].lSize)
                iLen = m_pColumns[iColumn].lSize;
            memset(m_pColBufs[iColumn].pData + 1, 0, m_pColumns[iColumn].lSize);
            memcpy(m_pColBufs[iColumn].pData + 1, _pBuff, iLen);
            break;
    case    eBLOB:
            pBuff = new unsigned char[iLen];
            memcpy(pBuff, _pBuff, iLen);

            if (m_pColBufs[iColumn].pLOB)
                delete m_pColBufs[iColumn].pLOB;

            *(int *)(m_pColBufs[iColumn].pData + 1 + 4) = iLen;
            m_pColBufs[iColumn].pLOB = pBuff;
            m_pColBufs[iColumn].bLOBChanged = true;
            break;
    default:
            throw new CGCursorEx(CGCursorEx::eIncompatibleType);
    }
    *m_pColBufs[iColumn].pData = 1;
}

//set GUID value
void CGCursor::setGUID(int iColumn, const GUID *pGUID)
{
    if (m_lCurRow == 0 && *m_pData != eNew)
        throw new CGCursorEx(CGCursorEx::eInvalidRow, 0);
    if (iColumn < 0 || iColumn >= m_lColCount)
        throw new CGCursorEx(CGCursorEx::eInvalidColumn, iColumn);

    switch (m_pColumns[iColumn].eType)
    {
    case    eGUID:
            memcpy(m_pColBufs[iColumn].pData + 1, pGUID, sizeof(GUID));
            break;
    default:
            throw new CGCursorEx(CGCursorEx::eIncompatibleType);
    }
    *m_pColBufs[iColumn].pData = 1;
}

//set value to null
void CGCursor::setNull(int iColumn)
{
    *m_pColBufs[iColumn].pData = 1;
}

//----------------------------------------------//
//set integer value
void CGCursor::setInt(const char *szName, int iVal)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    setInt(iCol, iVal);
}

//set character value
void CGCursor::setChar(const char *szName, const char *szVal)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    setChar(iCol, szVal);
}

//set numeric value
void CGCursor::setNumber(const char *szName, double dblVal)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    setNumber(iCol, dblVal);
}

//set date value
void CGCursor::setDate(const char *szName, const CGOdbcStmt::DATE *pDate)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    setDate(iCol, pDate);
}

//set timestamp value
void CGCursor::setTimeStamp(const char *szName, const CGOdbcStmt::TIMESTAMP *pDate)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    setTimeStamp(iCol, pDate);
}

//set binary value
void CGCursor::setBin(const char *szName, const void *pBuff, int iLen)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    setBin(iCol, pBuff, iLen);
}

//set GUID value
void CGCursor::setGUID(const char *szName, const GUID *pGUID)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    setGUID(iCol, pGUID);
}

//set value to null
void CGCursor::setNull(const char *szName)
{
    int iCol;
    iCol = findColumn(szName);
    if (iCol == -1)
        throw new CGCursorEx(CGCursorEx::eInvalidColumnName, szName);
    setNull(iCol);
}

