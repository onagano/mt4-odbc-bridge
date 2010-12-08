#include "stdhead.h"

void test1(CGOdbcConnect *);
void test2(CGOdbcConnect *);

bool _cdecl onIndex(long i1, long i2)
{
    if (i1 % 100 == 0)
        printf("Indexing %10.2f%%\r", (double)i1 * 100 / i2);
    return true;
}

void testIndex()
{
    //create database
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
    int i;

    pCur = cCon.createStatement();
    try
    {
        pCur->execute("drop table indexspeedtest");
    }
    catch(...)
    {
    }
    cCon.freeStatement(pCur);

    pCur = cCon.createStatement();
    try
    {
        pCur->execute("create table indexspeedtest(id int primary key not null, fk int not null, "
                                                  "text varchar(32), unique(fk, id))");
    }
    catch(CGOdbcEx *pE)
    {
        printf("create table error\n%s\n", pE->getMsg());
        return;
    }
    cCon.freeStatement(pCur);

    cCon.setTransMode(false);
    pCur = cCon.createStatement();
    try
    {
        pCur->prepare("insert into indexspeedtest values (?, ?, ?)");
    }
    catch(CGOdbcEx *pE)
    {
        printf("prepare statment error\n%s\n", pE->getMsg());
        return;
    }

    try
    {
            if (!pCur->bindParamsAuto())
            {
                printf("Query contains no parameters");
                return;
            }
    }
    catch(CGOdbcEx *pE)
    {
        printf("bind parameters error\n%s\n", pE->getMsg());
        return;
    }


    for (i = 1; i < 10000; i++)
    {
        if (i % 100 == 0)
            printf("%6i\r", i);
        try
        {
            char szName[32];

            pCur->setInt(0, i);
            pCur->setInt(1, rand());

            sprintf(szName, "Name %i", i);
            pCur->setChar(2, szName);
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

    cCon.commit();
    cCon.setTransMode(true);

    test1(&cCon);
    test2(&cCon);

    cCon.close();
}

void test1(CGOdbcConnect *pCon)
{
    DWORD dwTick = GetTickCount();
    char szQuery[128];
    CGOdbcStmt *pStmt;
    int i;
    for (i = 0; i < 500; i++)
    {
        if (i % 100 == 0)
            printf("Test 1. %4i\r", i);

        int iToSeek = rand();
        sprintf(szQuery, "select * from indexspeedtest where fk = %i", i);

        pStmt = pCon->executeSelect(szQuery);
        pStmt->bindAuto();
        bool bRC;
        for (bRC = pStmt->first(); bRC; bRC = pStmt->next());
        pStmt->getRowCount();
    }
    printf("Test1. Test %i msec per 500 searches\n", GetTickCount() - dwTick);
}

void test2(CGOdbcConnect *pCon)
{
    int i;
    DWORD dwTick = GetTickCount();
    CGOdbcStmt *pStmt;
    pStmt = pCon->executeSelect("select * from indexspeedtest");
    pStmt->bindAuto();
    pStmt->last();
    CGCursor *pCur = new CGCursor(pStmt, "test.cur");
    CGCursorIndex *pIdx = new CGCursorIndex(pCur, 1, onIndex);
    printf("Test2. Preparation %i\n", GetTickCount() - dwTick);
    dwTick = GetTickCount();
    for (i = 0; i < 1000; i++)
    {
        if (i % 100 == 0)
            printf("Test 2. %4i\r", i);
        int iToSeek = rand();
        int iRow;
        iRow = pIdx->seekFor(iToSeek);
        if (iRow > 0)
        {
            while(true)
            {
                pIdx->go(iRow);
                if (pCur->getInt(1) != iToSeek)
                    break;
                iRow++;
            }
        }
    }
    printf("Test2. Test %i msec per 1000 searches\n", GetTickCount() - dwTick);
    delete pIdx;
    delete pCur;
}
