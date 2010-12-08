//-[stdhead]-------------------------------------------------------
//Project:
//File   :curidx.cpp
//Started:24.09.02 16:52:11
//Updated:25.09.02 16:06:34
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
#include "stdhead.h"
#include "stmt.h"
#include "cur.h"
#include "curex.h"
#include "curidx.h"

//construct and create index
CGCursorIndex::CGCursorIndex(CGCursor *pCursor, int iColumn, bool (_cdecl *pCallback)(long, long))
{
    m_pCursor = pCursor;
    m_iColumn = iColumn;
    m_eType = m_pCursor->getColumn(iColumn)->eType;

    int i;
    for (i = 1; i <= m_pCursor->getRowCount(); i++)
    {
        if (pCallback)
            if (!pCallback(i, m_pCursor->getRowCount()))
                throw new CGCursorEx(CGCursorEx::eBroken);
        _addRow(i);
    }

    m_pCursor->_addIndex(this);
}

//load index from file
CGCursorIndex::CGCursorIndex(CGCursor *pCursor, const char *szFileName)
{
    FILE *pFile;
    pFile = fopen(szFileName, "rb");
    if (!pFile)
        throw new CGCursorEx(CGCursorEx::eFileOpen, szFileName);
    int iSize;
    int iColumn;
    int iRead;
    iRead = fread(&iSize, 1, sizeof(int), pFile);
    if (iRead != sizeof(int))
    {
        fclose(pFile);
        throw new CGCursorEx(CGCursorEx::eReadError);
    }
    iRead = fread(&iColumn, 1, sizeof(int), pFile);
    if (iRead != sizeof(int))
    {
        fclose(pFile);
        throw new CGCursorEx(CGCursorEx::eReadError);
    }
    m_iColumn = iColumn;
    int i, j;
    for (i = 0; i < iSize; i++)
    {
        iRead = fread(&j, 1, sizeof(int), pFile);
        if (iRead != sizeof(int))
        {
            fclose(pFile);
            throw new CGCursorEx(CGCursorEx::eReadError);
        }
        m_aIndex.push_back(j);
    }
    m_pCursor = pCursor;
    m_pCursor->_addIndex(this);
    fclose(pFile);
}


//destructor
CGCursorIndex::~CGCursorIndex()
{
    m_pCursor->_removeIndex(this);
}

//----------------------------------------------//
//save index to file
void CGCursorIndex::save(const char *szFileName)
{
    int iWritten;
    FILE *pFile;
    pFile = fopen(szFileName, "wb");
    if (!pFile)
        throw new CGCursorEx(CGCursorEx::eFileOpen, szFileName);
    int iSize = m_aIndex.size();

    iWritten = fwrite(&iSize, 1, sizeof(int), pFile);
    if (iWritten != sizeof(int))
    {
        fclose(pFile);
        throw new CGCursorEx(CGCursorEx::eWriteError);
    }
    iWritten = fwrite(&m_iColumn, 1, sizeof(int), pFile);
    if (iWritten != sizeof(int))
    {
        fclose(pFile);
        throw new CGCursorEx(CGCursorEx::eWriteError);
    }
    int i, j;
    for (i = 0; i < m_aIndex.size(); i++)
    {
        j = m_aIndex.at(i);
        iWritten = fwrite(&j, 1, sizeof(int), pFile);
        if (iWritten != sizeof(int))
        {
            fclose(pFile);
            throw new CGCursorEx(CGCursorEx::eWriteError);
        }
    }
    fclose(pFile);
}



//go to row by index
void CGCursorIndex::go(int iIndexRow)
{
    if (iIndexRow < 1 || iIndexRow > m_aIndex.size())
        throw new CGCursorEx(CGCursorEx::eInvalidRow);

    int iRow = m_aIndex.at(iIndexRow - 1);
    m_pCursor->go(iRow);
}


//add row to index
void CGCursorIndex::_addRow(int iRow)
{
    int nLinearFrom, nLinearTo;
    if (m_aIndex.size() < 3)      //we should not use the b-search
    {
        nLinearFrom = 0;
        nLinearTo = m_aIndex.size();
    }
    else
    {
        int nFrom, nTo, nNow;
        nFrom = 0;
        nTo=m_aIndex.size() - 1;
        while(true)
        {
            nNow = nFrom + (nTo - nFrom) / 2;
            if (_compareRows(iRow, m_aIndex.at(nNow)) < 0)
            {
                nTo = nNow;
            }
            else
            {
                nFrom = nNow;
            }
            if ((nTo - nFrom) <= 6) break;
        }
        nLinearFrom = nFrom;
        nLinearTo = nTo + 1;
    }
    int i;
    for (i = nLinearFrom; i < nLinearTo; i++)
        if (_compareRows(iRow, m_aIndex.at(i)) < 0)
            break;

    m_aIndex.insert(m_aIndex.begin() + i, iRow);
    return ;
}

//remove row from index
void CGCursorIndex::_removeRow(int iRow)
{
    int i;
    for (i = 0; i < m_aIndex.size(); i++)
        if (m_aIndex.at(i) == iRow)
        {
            m_aIndex.erase(m_aIndex.begin() + i);
            return;
        }
}

//compare two rows of cursor
int CGCursorIndex::_compareRows(int iRow1, int iRow2)
{
    bool bNull1, bNull2;
    switch (m_eType)
    {
    case    CGCursor::eInt:
        {
            int v1, v2;
            m_pCursor->go(iRow1);
            if (!(bNull1 = m_pCursor->isNull(m_iColumn)))
                v1 = m_pCursor->getInt(m_iColumn);
            m_pCursor->go(iRow2);
            if (!(bNull2 = m_pCursor->isNull(m_iColumn)))
                v2 = m_pCursor->getInt(m_iColumn);

            if (bNull1 && bNull2)
                return 0;
            if (!bNull1 && bNull2)
                return 1;
            if (bNull1 && !bNull2)
                return -1;

            if (v2 < v1)
                return 1;
            else if (v2 > v1)
                return -1;
            else
                return 0;
        }
            break;
    case    CGCursor::eDouble:
        {
            double v1, v2;
            m_pCursor->go(iRow1);
            if (!(bNull1 = m_pCursor->isNull(m_iColumn)))
                v1 = m_pCursor->getNumber(m_iColumn);
            m_pCursor->go(iRow2);
            if (!(bNull2 = m_pCursor->isNull(m_iColumn)))
                v2 = m_pCursor->getNumber(m_iColumn);

            if (bNull1 && bNull2)
                return 0;
            if (!bNull1 && bNull2)
                return 1;
            if (bNull1 && !bNull2)
                return -1;

            if (v2 < v1)
                return 1;
            else if (v2 > v1)
                return -1;
            else
                return 0;
        }
    case    CGCursor::eChar:
        {
            std::string v1, v2;
            m_pCursor->go(iRow1);
            if (!(bNull1 = m_pCursor->isNull(m_iColumn)))
                v1 = m_pCursor->getChar(m_iColumn);
            m_pCursor->go(iRow2);
            if (!(bNull2 = m_pCursor->isNull(m_iColumn)))
                v2 = m_pCursor->getChar(m_iColumn);

            if (bNull1 && bNull2)
                return 0;
            if (!bNull1 && bNull2)
                return 1;
            if (bNull1 && !bNull2)
                return -1;

            return strcmp(v1.c_str(), v2.c_str());
        }
    case    CGCursor::eDate:
        {
            CGOdbcStmt::DATE v1, v2;
            m_pCursor->go(iRow1);
            if (!(bNull1 = m_pCursor->isNull(m_iColumn)))
                memcpy(&v1, m_pCursor->getDate(m_iColumn), sizeof(CGOdbcStmt::DATE));
            m_pCursor->go(iRow2);
            if (!(bNull2 = m_pCursor->isNull(m_iColumn)))
                memcpy(&v2, m_pCursor->getDate(m_iColumn), sizeof(CGOdbcStmt::DATE));

            if (bNull1 && bNull2)
                return 0;
            if (!bNull1 && bNull2)
                return 1;
            if (bNull1 && !bNull2)
                return -1;

            return _cmpDate(&v1, &v2);
        }
    case    CGCursor::eTimeStamp:
        {
            CGOdbcStmt::TIMESTAMP v1, v2;
            m_pCursor->go(iRow1);
            if (!(bNull1 = m_pCursor->isNull(m_iColumn)))
                memcpy(&v1, m_pCursor->getTimeStamp(m_iColumn), sizeof(CGOdbcStmt::TIMESTAMP));
            m_pCursor->go(iRow2);
            if (!(bNull2 = m_pCursor->isNull(m_iColumn)))
                memcpy(&v2, m_pCursor->getTimeStamp(m_iColumn), sizeof(CGOdbcStmt::TIMESTAMP));

            if (bNull1 && bNull2)
                return 0;
            if (!bNull1 && bNull2)
                return 1;
            if (bNull1 && !bNull2)
                return -1;

            return _cmpTimeStamp(&v1, &v2);
        }
    case    CGCursor::eGUID:
        {
            GUID v1, v2;
            m_pCursor->go(iRow1);
            if (!(bNull1 = m_pCursor->isNull(m_iColumn)))
                memcpy(&v1, m_pCursor->getGUID(m_iColumn), sizeof(GUID));
            m_pCursor->go(iRow2);
            if (!(bNull2 = m_pCursor->isNull(m_iColumn)))
                memcpy(&v2, m_pCursor->getGUID(m_iColumn), sizeof(GUID));

            if (bNull1 && bNull2)
                return 0;
            if (!bNull1 && bNull2)
                return 1;
            if (bNull1 && !bNull2)
                return -1;

            return _cmpGUID(&v1, &v2);
        }
            break;
    default:
            return 0;
    }
}

//seek for absolute row number
int CGCursorIndex::seekForAbsRow(int iRow)
{
    int i;
    for (i = 0; i < m_aIndex.size(); i++)
        if (m_aIndex.at(i) == iRow)
        {
            return i + 1;
        }
    return 0;
}

//seek for integer value
int CGCursorIndex::seekFor(int iVal)
{
    size_t num = m_aIndex.size();

    int lo = 0;
    int hi = num - 1;
    int mid;
    unsigned int half;
    int result;

    while (lo <= hi)
    {
        if (half = num / 2)
        {
            mid = lo + (num & 1 ? half : (half - 1));
            if ((result = _compareWith(iVal, m_aIndex.at(mid))) == 0)
            {
                int i;
                i = mid - 1;
                while(i >= 0 && _compareWith(iVal, m_aIndex.at(i)) == 0)
                    i--;
                return (i + 2);
            }
            else if (result < 0)
            {
                hi = mid - 1;
                num = num & 1 ? half : half - 1;
            }
            else
            {
                lo = mid + 1;
                num = half;
            }
        }
        else
        {
            if (num)
            {
                if (_compareWith(iVal, m_aIndex.at(lo)) != 0)
                    return -1;
                int i;
                i = lo - 1;
                while(i >= 0 && _compareWith(iVal, m_aIndex.at(i)) == 0)
                    i--;
                return (i + 2);
            }
            else
                break;
        }
    }
    return -1;
}

//seek for double value
int CGCursorIndex::seekFor(double iVal)
{
    size_t num = m_aIndex.size();

    int lo = 0;
    int hi = num - 1;
    int mid;
    unsigned int half;
    int result;

    while (lo <= hi)
    {
        if (half = num / 2)
        {
            mid = lo + (num & 1 ? half : (half - 1));
            if ((result = _compareWith(iVal, m_aIndex.at(mid))) == 0)
            {
                int i;
                i = mid - 1;
                while(i >= 0 && _compareWith(iVal, m_aIndex.at(i)) == 0)
                    i--;
                return (i + 2);
            }
            else if (result < 0)
            {
                hi = mid - 1;
                num = num & 1 ? half : half - 1;
            }
            else
            {
                lo = mid + 1;
                num = half;
            }
        }
        else
        {
            if (num)
            {
                if (_compareWith(iVal, m_aIndex.at(lo)) != 0)
                    return -1;
                int i;
                i = lo - 1;
                while(i >= 0 && _compareWith(iVal, m_aIndex.at(i)) == 0)
                    i--;
                return (i + 2);
            }
            else
                break;
        }
    }
    return -1;
}

//seek for character value
int CGCursorIndex::seekFor(const char *szVal, bool bExact)
{
    size_t num = m_aIndex.size();

    int lo = 0;
    int hi = num - 1;
    int mid;
    unsigned int half;
    int result;

    while (lo <= hi)
    {
        if (half = num / 2)
        {
            mid = lo + (num & 1 ? half : (half - 1));
            if ((result = _compareWith(szVal, m_aIndex.at(mid), bExact)) == 0)
            {
                int i;
                i = mid - 1;
                while(i >= 0 && _compareWith(szVal, m_aIndex.at(i), bExact) == 0)
                    i--;
                return (i + 2);
            }
            else if (result < 0)
            {
                hi = mid - 1;
                num = num & 1 ? half : half - 1;
            }
            else
            {
                lo = mid + 1;
                num = half;
            }
        }
        else
        {
            if (num)
            {
                if (_compareWith(szVal, m_aIndex.at(lo), bExact) != 0)
                    return -1;
                int i;
                i = lo - 1;
                while(i >= 0 && _compareWith(szVal, m_aIndex.at(i), bExact) == 0)
                    i--;
                return (i + 2);
            }
            else
                break;
        }
    }
    return -1;
}

//seek for date value
int CGCursorIndex::seekFor(const CGOdbcStmt::DATE *pVal)
{
    size_t num = m_aIndex.size();

    int lo = 0;
    int hi = num - 1;
    int mid;
    unsigned int half;
    int result;

    while (lo <= hi)
    {
        if (half = num / 2)
        {
            mid = lo + (num & 1 ? half : (half - 1));
            if ((result = _compareWith(pVal, m_aIndex.at(mid))) == 0)
            {
                int i;
                i = mid - 1;
                while(i >= 0 && _compareWith(pVal, m_aIndex.at(i)) == 0)
                    i--;
                return (i + 2);
            }
            else if (result < 0)
            {
                hi = mid - 1;
                num = num & 1 ? half : half - 1;
            }
            else
            {
                lo = mid + 1;
                num = half;
            }
        }
        else
        {
            if (num)
            {
                if (_compareWith(pVal, m_aIndex.at(lo)) != 0)
                    return -1;
                int i;
                i = lo - 1;
                while(i >= 0 && _compareWith(pVal, m_aIndex.at(i)) == 0)
                    i--;
                return (i + 2);
            }
            else
                break;
        }
    }
    return -1;
}

//seek for timestamp value
int CGCursorIndex::seekFor(const CGOdbcStmt::TIMESTAMP *pVal)
{
    size_t num = m_aIndex.size();

    int lo = 0;
    int hi = num - 1;
    int mid;
    unsigned int half;
    int result;

    while (lo <= hi)
    {
        if (half = num / 2)
        {
            mid = lo + (num & 1 ? half : (half - 1));
            if ((result = _compareWith(pVal, m_aIndex.at(mid))) == 0)
            {
                int i;
                i = mid - 1;
                while(i >= 0 && _compareWith(pVal, m_aIndex.at(i)) == 0)
                    i--;
                return (i + 2);
            }
            else if (result < 0)
            {
                hi = mid - 1;
                num = num & 1 ? half : half - 1;
            }
            else
            {
                lo = mid + 1;
                num = half;
            }
        }
        else
        {
            if (num)
            {
                if (_compareWith(pVal, m_aIndex.at(lo)) != 0)
                    return -1;
                int i;
                i = lo - 1;
                while(i >= 0 && _compareWith(pVal, m_aIndex.at(i)) == 0)
                    i--;
                return (i + 2);
            }
            else
                break;
        }
    }
    return -1;
}

//seek for timestamp value
int CGCursorIndex::seekFor(const GUID *pVal)
{
    size_t num = m_aIndex.size();

    int lo = 0;
    int hi = num - 1;
    int mid;
    unsigned int half;
    int result;

    while (lo <= hi)
    {
        if (half = num / 2)
        {
            mid = lo + (num & 1 ? half : (half - 1));
            if ((result = _compareWith(pVal, m_aIndex.at(mid))) == 0)
            {
                int i;
                i = mid - 1;
                while(i >= 0 && _compareWith(pVal, m_aIndex.at(i)) == 0)
                    i--;
                return (i + 2);
            }
            else if (result < 0)
            {
                hi = mid - 1;
                num = num & 1 ? half : half - 1;
            }
            else
            {
                lo = mid + 1;
                num = half;
            }
        }
        else
        {
            if (num)
            {
                if (_compareWith(pVal, m_aIndex.at(lo)) != 0)
                    return -1;
                int i;
                i = lo - 1;
                while(i >= 0 && _compareWith(pVal, m_aIndex.at(i)) == 0)
                    i--;
                return (i + 2);
            }
            else
                break;
        }
    }
    return -1;
}

//compare value with row
int CGCursorIndex::_compareWith(int iValue, int iRow)
{
    m_pCursor->go(iRow);

    if (m_pCursor->isNull(m_iColumn))
        return 1;

    int iValue1;
    iValue1 = m_pCursor->getInt(m_iColumn);

    if (iValue < iValue1)
        return -1;
    else if (iValue > iValue1)
        return 1;
    else
        return 0;
}

//compare value with row
int CGCursorIndex::_compareWith(double iValue, int iRow)
{
    m_pCursor->go(iRow);
    if (m_pCursor->isNull(m_iColumn))
        return 1;
    double iValue1;
    iValue1 = m_pCursor->getNumber(m_iColumn);
    if (iValue < iValue1)
        return -1;
    else if (iValue > iValue1)
        return 1;
    else
        return 0;
}

//compare value with row
int CGCursorIndex::_compareWith(const char *szValue, int iRow, bool bExact)
{
    m_pCursor->go(iRow);
    if (m_pCursor->isNull(m_iColumn))
        return 1;
    if (bExact)
        return strcmp(szValue, m_pCursor->getChar(m_iColumn));
    else
        return memcmp(szValue, m_pCursor->getChar(m_iColumn), strlen(szValue));
}

//compare value with row
int CGCursorIndex::_compareWith(const CGOdbcStmt::DATE *pDate, int iRow)
{
    m_pCursor->go(iRow);
    if (m_pCursor->isNull(m_iColumn))
        return 1;
    return _cmpDate(pDate, m_pCursor->getDate(m_iColumn));
}

//compare value with row
int CGCursorIndex::_compareWith(const CGOdbcStmt::TIMESTAMP *pDate, int iRow)
{
    m_pCursor->go(iRow);
    if (m_pCursor->isNull(m_iColumn))
        return 1;
    return _cmpTimeStamp(pDate, m_pCursor->getTimeStamp(m_iColumn));
}

//compare value with row
int CGCursorIndex::_compareWith(const GUID *pGuid, int iRow)
{
    m_pCursor->go(iRow);
    if (m_pCursor->isNull(m_iColumn))
        return 1;
    return _cmpGUID(pGuid, m_pCursor->getGUID(m_iColumn));
}

//compare two dates
int CGCursorIndex::_cmpDate(const CGOdbcStmt::DATE *d1, const CGOdbcStmt::DATE *d2)
{
    _int64 iVal1 = 0, iVal2 = 0;
    iVal1 += d1->iYear;
    iVal1 *= 100;
    iVal1 += d1->iMonth;
    iVal1 *= 100;
    iVal1 += d1->iDay;

    iVal2 += d2->iYear;
    iVal2 *= 100;
    iVal2 += d2->iMonth;
    iVal2 *= 100;
    iVal2 += d2->iDay;

    if (iVal1 > iVal2)
        return 1;
    else if (iVal1 < iVal2)
        return -1;
    else
        return 0;
}


//compare two timestamps
int CGCursorIndex::_cmpTimeStamp(const CGOdbcStmt::TIMESTAMP *d1, const CGOdbcStmt::TIMESTAMP *d2)
{
    _int64 iVal1 = 0, iVal2 = 0;

    iVal1 += d1->iYear;
    iVal1 *= 100;
    iVal1 += d1->iMonth;
    iVal1 *= 100;
    iVal1 += d1->iDay;

    iVal2 += d2->iYear;
    iVal2 *= 100;
    iVal2 += d2->iMonth;
    iVal2 *= 100;
    iVal2 += d2->iDay;

    if (iVal1 > iVal2)
        return 1;
    else if (iVal1 < iVal2)
        return -1;
    else
    {
        iVal1 = 0;
        iVal2 = 0;
        iVal1 += d1->iHour;
        iVal1 *= 100;
        iVal1 += d1->iMin;
        iVal1 *= 100;
        iVal1 += d1->iSec;
        iVal2 += d2->iHour;
        iVal2 *= 100;
        iVal2 += d2->iMin;
        iVal2 *= 100;
        iVal2 += d2->iSec;
        if (iVal1 > iVal2)
            return 1;
        else if (iVal1 < iVal2)
            return -1;
        else
        {
            if (d1->iFract > d2->iFract)
                return 1;
            else  if (d1->iFract < d2->iFract)
                return -1;
            else
                return 0;
        }
    }
}

//compare two GUIDs
int CGCursorIndex::_cmpGUID(const GUID * pGUID1, const GUID * pGUID2)
{
    char szBuff1[256], szBuff2[256];
    sprintf(szBuff1, "%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X",
                     pGUID1->Data1,pGUID1->Data2,pGUID1->Data3,
                     ((WORD)pGUID1->Data4[0]<<8) + pGUID1->Data4[1], pGUID1->Data4[2], pGUID1->Data4[3],
                     pGUID1->Data4[4], pGUID1->Data4[5], pGUID1->Data4[6], pGUID1->Data4[7]);
    sprintf(szBuff2, "%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X",
                     pGUID2->Data1,pGUID2->Data2,pGUID2->Data3,
                     ((WORD)pGUID2->Data4[0]<<8) + pGUID2->Data4[1], pGUID2->Data4[2], pGUID2->Data4[3],
                     pGUID2->Data4[4], pGUID2->Data4[5], pGUID2->Data4[6], pGUID2->Data4[7]);
    return strcmp(szBuff1, szBuff2);
}

