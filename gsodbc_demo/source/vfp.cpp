#include "stdhead.h"

void testVFP()
{
    printf("VFP ODBC\n");
    CGOdbcConnect cCon;
    try
    {
        char szDir[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, szDir);
        _bstr_t sDrvStr;
        sDrvStr = "Driver=Microsoft Visual FoxPro Driver;Exclusive=yes;";
        sDrvStr += "SourceDb=";
        sDrvStr += szDir;
        sDrvStr += ";SourceType=DBF;Deleted=Yes";
        cCon.connect(sDrvStr);
    }
    catch(CGOdbcEx *pE)
    {
        printf("connection error\n%s\n", pE->getMsg());
        return;
    }

    printf("Driver: %s\n", cCon.getDriver());

    CGOdbcStmt *pCur;

    pCur = cCon.createStatement();
    try
    {
        pCur->execute("drop table newlibtest");
    }
    catch(...)
    {
    }
    cCon.freeStatement(pCur);

    pCur = cCon.createStatement();
    try
    {
        pCur->execute("create table newlibtest(id i, name c(32), "
                                              "descript m, "
                                              "a1 n(10,2), "
                                              "a2 n(10, 5), "
                                              "stamp t)");
    }
    catch(CGOdbcEx *pE)
    {
        printf("create table error\n%s\n", pE->getMsg());
        return;
    }
    cCon.freeStatement(pCur);

    pCur = cCon.createStatement();
    try
    {
        pCur->prepare("insert into newlibtest values (?, ?, ?, ?, ?, ?)");
    }
    catch(CGOdbcEx *pE)
    {
        printf("prepare statment error\n%s\n", pE->getMsg());
        return;
    }

    try
    {
        pCur->setParamNumber(6);
        pCur->bindParam(0, SQL_INTEGER, SQL_C_LONG, 4, 0, 0);
        pCur->bindParam(1, SQL_CHAR, SQL_C_CHAR, 33, 32, 0);
        pCur->bindParam(2, SQL_LONGVARCHAR, SQL_C_CHAR, 256000, 256000, 0);
        pCur->bindParam(3, SQL_DOUBLE, SQL_C_DOUBLE, 0, 8, 5);
        pCur->bindParam(4, SQL_DOUBLE, SQL_C_DOUBLE, 0, 8, 5);
        pCur->bindParam(5, SQL_TIMESTAMP, SQL_C_TIMESTAMP, 23, 23, 2);
    }
    catch(CGOdbcEx *pE)
    {
        printf("bind parameters error\n%s\n", pE->getMsg());
        return;
    }

    int i, j;

    for (i = 1; i < 10; i++)
    {
        try
        {
            char szName[16];
            char szDesc[128001];
            CGOdbcStmt::TIMESTAMP st;
            SYSTEMTIME tm;
            GetLocalTime(&tm);
            st.iYear = tm.wYear;
            //st.iMonth = tm.wMonth;
            st.iDay = tm.wDay;
            st.iHour = tm.wHour;
            st.iMin = tm.wMinute;
            st.iSec = tm.wSecond;

            st.iMonth = i;

            pCur->setInt(0, i);

            sprintf(szName, "Name %i", i);

            pCur->setChar(1, szName);

            for (j = 0; j < 128000; j++)
            {
                szDesc[j] = i + '0' + j % 10;
            }
            szDesc[128000] = 0;
            pCur->setChar(2, szDesc);


            pCur->setNumber(3, i / 3.);
            pCur->setNumber(4, i / 3.);
            pCur->setTimeStamp(5, &st);
        }
        catch(CGOdbcEx *pE)
        {
            printf("set value error\n%s\n", pE->getMsg());
            return;
        }

        try
        {
            pCur->execute();
        }
        catch(CGOdbcEx *pE)
        {
            cCon.freeStatement(pCur);
            printf("bind parameters error\n%s\n", pE->getMsg());
            return;
        }
    }
    cCon.freeStatement(pCur);

    pCur = cCon.createStatement();
    try
    {
        pCur->execute("select * from newlibtest");
    }
    catch(CGOdbcEx *pE)
    {
        cCon.freeStatement(pCur);
        printf("execute query error\n%s\n", pE->getMsg());
        return;
    }

    try
    {
        if (!pCur->bindAuto())
        {
            printf("cursor doesn't contain resultset\n");
            return;
        }
    }
    catch(CGOdbcEx *pE)
    {
        cCon.freeStatement(pCur);
        printf("binding error\n%s\n", pE->getMsg());
        return;
    }

    try
    {
        bool bRC;
        for (bRC = pCur->first(); bRC; bRC = pCur->next())
        {
            printf("%i '%s'", i = pCur->getInt("id"), pCur->getChar("name"));
            //verify long fields
            const char *pBuff;
            int iLen;
            iLen = pCur->getLength("descript");
            printf(" %i", iLen);
            pBuff = pCur->getChar("descript");
            for (j = 0; j < iLen; j++)
            {
                if (pBuff[j] == i + '0' + j % 10)
                    continue;
                else
                {
                    printf("!%i", j);
                    break;
                }
            }

            printf(" %s ", pCur->getChar("a1"));
            printf(" %s ", pCur->getChar("a2"));
            printf(" %s ", pCur->getChar("stamp"));


            printf("\n");
        }
    }
    catch(CGOdbcEx *pE)
    {
        cCon.freeStatement(pCur);
        printf("load cursor error\n%s\n", pE->getMsg());
        return;
    }
    cCon.freeStatement(pCur);

}

