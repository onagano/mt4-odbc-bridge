//-[stdhead]-------------------------------------------------------
//Project:
//File   :curex.cpp
//Started:24.09.02 15:58:06
//Updated:24.09.02 16:06:35
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
#include "stdhead.h"
#include "curex.h"

extern HINSTANCE g_hInst;

CGCursorEx::CGCursorEx(CGCursorEx::eCode lCode)
{
    char szBuf[256] = "";
    LoadString(g_hInst, (long)lCode + 20, szBuf, 256);
    m_sMsg = szBuf;
    m_lCode = lCode;
}

CGCursorEx::CGCursorEx(CGCursorEx::eCode lCode, int lValue)
{
    char szBuf[256] = "";
    char szBuf1[256] = "";
    LoadString(g_hInst, (long)lCode + 20, szBuf, 256);
    sprintf(szBuf1, szBuf, lValue);
    m_sMsg = szBuf1;
    m_lCode = lCode;
}

CGCursorEx::CGCursorEx(CGCursorEx::eCode lCode, const char *szValue)
{
    char szBuf[256] = "";
    char szBuf1[512] = "";
    LoadString(g_hInst, (long)lCode + 20, szBuf, 256);
    sprintf(szBuf1, szBuf, szValue);
    m_sMsg = szBuf1;
    m_lCode = lCode;
}

long CGCursorEx::getCode()
{
    return m_lCode;
}

const char * CGCursorEx::getMsg()
{
    return m_sMsg;
}



