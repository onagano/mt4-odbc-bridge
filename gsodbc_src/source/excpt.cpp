//-[stdhead]-------------------------------------------------------
//Project:
//File   :excpt.cpp
//Started:19.09.02 12:35:14
//Updated:24.09.02 15:53:52
//Author :GehtSoft
//Subj   :
//Version:
//Require:
//-----------------------------------------------------------------
#include "stdhead.h"
#include "excpt.h"

extern HINSTANCE g_hInst;

CGOdbcEx::CGOdbcEx(bool bUsage, long lCode, const char * szMsg)
{
    m_bUsage = bUsage;
    m_lCode = lCode;
    if (bUsage)
    {
        char szBuff[256] = "";
        LoadString(g_hInst, lCode + 10, szBuff, 256);
        m_sMsg = szBuff;
    }
    else
        m_sMsg = szMsg;
}

bool CGOdbcEx::isUsage()
{
    return m_bUsage;
}

long CGOdbcEx::getCode()
{
    return m_lCode;
}

const char * CGOdbcEx::getMsg()
{
    return m_sMsg;
}


