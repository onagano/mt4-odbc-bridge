#include "stdhead.h"

#define AUTOBIND

void testMSSQL()
{
    printf("MSSQL\n\n");
    CGOdbcConnect cCon;
    try
    {
        cCon.connect("MSSQL", "sa", "dbo");
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
        pCur->execute("create table newlibtest(id int, name varchar(32), "
                                              "descript text, binid binary(32), "
                                              "contents image, a1 numeric(10,2), "
                                              "a2 numeric(10, 5), "
                                              "stamp smalldatetime)");
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
        pCur->prepare("insert into newlibtest values (?, ?, ?, ?, ?, ?, ?, ?)");
    }
    catch(CGOdbcEx *pE)
    {
        printf("prepare statment error\n%s\n", pE->getMsg());
        return;
    }

    try
    {
        #ifdef AUTOBIND
            if (!pCur->bindParamsAuto())
            {
                printf("Query contains no parameters");
                return;
            }
        #else
            pCur->setParamNumber(8);
            pCur->bindParam(0, SQL_INTEGER, SQL_C_LONG, 4, 0, 0);
            pCur->bindParam(1, SQL_CHAR, SQL_C_CHAR, 33, 32, 0);
            pCur->bindParam(2, SQL_LONGVARCHAR, SQL_C_CHAR, 0, 0, 0);
            pCur->bindParam(3, SQL_BINARY, SQL_C_BINARY, 32, 32, 0);
            pCur->bindParam(4, SQL_LONGVARBINARY, SQL_C_BINARY, 0, 0, 0);
            pCur->bindParam(5, SQL_DOUBLE, SQL_C_DOUBLE, 0, 8, 5);
            pCur->bindParam(6, SQL_DOUBLE, SQL_C_DOUBLE, 0, 8, 5);
            pCur->bindParam(7, SQL_TIMESTAMP, SQL_C_TIMESTAMP, 23, 23, 2);
        #endif
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
            char bVal[256000];
            char sID[64];
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


            for (j = 0; j < 64; j++)
                sID[j] = i + j;

            pCur->setBin(3, sID, 64);

            for (j = 0; j < 256000; j++)
                bVal[j] = j % 10 + 30 + i;
            pCur->setBin(4, bVal, 256000);

            pCur->setNumber(5, i / 3.);
            pCur->setNumber(6, i / 3.);
            pCur->setTimeStamp(7, &st);
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
            pBuff = (const char *)pCur->getPtr("binid");
            iLen = pCur->getLength("binid");
            printf(" %i", iLen);

            for (j = 0; j < iLen; j++)
                if (pBuff[j] != i + j)
                {
                    printf("!%i", j);
                    break;
                }

            pBuff = (const char *)pCur->getPtr("contents");
            iLen = pCur->getLength("contents");
            printf(" %i", iLen);

            for (j = 0; j < iLen; j++)
                if (pBuff[j] != j % 10 + 30 + i)
                {
                    printf("!%i", j);
                    break;
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

