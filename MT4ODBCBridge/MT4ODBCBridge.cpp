#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers

#include "odbcwrapper.h"
#include <stdio.h>
#include <stdarg.h>

#define MT4_EXPFUNC         __declspec(dllexport)

#define MOB_DEBUG_LOG       false

/**
 * Cache to handle multiple connections.
 * Assuming a connection per thread.
 */
std::map<int, ODBCWrapper*> conns;
int                         nextConId = 1;
HANDLE                      connsLock;

FILE*                       logFile;
HANDLE                      fileLock;

using namespace cppodbc;

/**
 *
 */
static void log(const char *format, ...)
{
    if (MOB_DEBUG_LOG && logFile)
    {
        va_list arg;
        va_start(arg, format);
        WaitForSingleObject(fileLock, INFINITE);
        //TODO: define log file prefix like timestamp, thread ID, etc.
        vfprintf_s(logFile, format, arg);
        ReleaseMutex(logFile);
        va_end(arg);
    }
}

/**
 *
 */
static void init()
{
    connsLock = CreateMutex(NULL, FALSE, _T("connsLock"));
    fileLock = CreateMutex(NULL, FALSE, _T("fileLock"));

    // conns already has an instance.

    if (MOB_DEBUG_LOG)
    {
        WaitForSingleObject(fileLock, INFINITE);
        //TODO: use env var to determine the log file directory
        if (_tfopen_s(&logFile, _T("C:\\Users\\onagano\\mobdebug.log"), _T("a")) != 0)
            logFile = NULL;
        ReleaseMutex(logFile);
    }
    log("init() called.\n");
}

/**
 *
 */
static void deinit()
{
    log("deinit() called.\n");
    WaitForSingleObject(connsLock, INFINITE);
    for (std::map<int, ODBCWrapper*>::iterator ite = conns.begin(); ite != conns.end(); ite++) {
        ODBCWrapper *ow = ite->second;
        if (ow) delete ow;
    }
    conns.clear();
    ReleaseMutex(connsLock);

    if (MOB_DEBUG_LOG && logFile) {
        WaitForSingleObject(fileLock, INFINITE);
        if (fclose(logFile) == 0)
            logFile = NULL;
        ReleaseMutex(logFile);
    }

    CloseHandle(connsLock);
    CloseHandle(fileLock);
}

/**
 * The entry point of this DLL.
 */
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch(ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        init();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        deinit();
        break;
    }
    return(TRUE);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_create()
{
    WaitForSingleObject(connsLock, INFINITE);
    int conId = nextConId++;
    conns[conId] = new ODBCWrapper();
    ReleaseMutex(connsLock);
    return conId;
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_open(const int conId, const TCHAR *dsn, const TCHAR *username, const TCHAR *password)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->open(dsn, username, password);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_close(const int conId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    delete ow;
    WaitForSingleObject(connsLock, INFINITE);
    conns.erase(conId);
    ReleaseMutex(connsLock);
    return MOB_OK;
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_isDead(const int conId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->isDead();
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_commit(const int conId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->commit();
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_rollback(const int conId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->rollback();
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_getAutoCommit(const int conId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->getAutoCommit();
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_setAutoCommit(const int conId, const int autoCommit)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->setAutoCommit(autoCommit);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_execute(const int conId, const TCHAR *sql)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->execute(sql);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_getLastErrorNo(const int conId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->getLastErrorNo();
}

/**
 *
 */
MT4_EXPFUNC const TCHAR* __stdcall MOB_getLastErrorMesg(const int conId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return _T("INVALID CONID");
    return ow->getLastErrorMesg();
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_registerStatement(const int conId, const TCHAR *sql)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->registerStatement(sql);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_unregisterStatement(const int conId, const int stmtId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->unregisterStatement(stmtId);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_bindIntParameter(const int conId, const int stmtId, const int slot, int *intp)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->bindIntParameter(stmtId, slot, intp);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_bindDoubleParameter(const int conId, const int stmtId, const int slot, double *dblp)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->bindDoubleParameter(stmtId, slot, dblp);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_bindStringParameter(const int conId, const int stmtId, const int slot, TCHAR *strp)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->bindStringParameter(stmtId, slot, strp);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_bindDatetimeParameter(const int conId, const int stmtId, const int slot, unsigned int *timep)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->bindDatetimeParameter(stmtId, slot, timep);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_bindIntColumn(const int conId, const int stmtId, const int slot, int *intp)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->bindIntColumn(stmtId, slot, intp);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_bindDoubleColumn(const int conId, const int stmtId, const int slot, double *dblp)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->bindDoubleColumn(stmtId, slot, dblp);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_bindStringColumn(const int conId, const int stmtId, const int slot, TCHAR *strp)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->bindStringColumn(stmtId, slot, strp);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_bindDatetimeColumn(const int conId, const int stmtId, const int slot, unsigned int *timep)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->bindDatetimeColumn(stmtId, slot, timep);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_executeStatement(const int conId, const int stmtId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->executeStatement(stmtId);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_selectToFetch(const int conId, const int stmtId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->selectToFetch(stmtId);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_fetch(const int conId, const int stmtId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->fetch(stmtId);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_fetchedAll(const int conId, const int stmtId)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->fetchedAll(stmtId);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_insertTick(const int conId, const int stmtId,
                                         unsigned int datetime, int fraction,
                                         double *vals, const int size)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->insertTick(stmtId, datetime, fraction, vals, size);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_insertBar(const int conId, const int stmtId,
                                        unsigned int datetime, double *vals, const int size)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->insertBar(stmtId, datetime, vals, size);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_copyRates(const int conId, const int stmtId,
                            RateInfo *rates, const int size, const int start, const int end)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->copyRates(stmtId, rates, size, start, end);
}

/**
 *
 */
MT4_EXPFUNC double __stdcall MOB_selectDouble(const int conId, const TCHAR *sql, const double defaultVal)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->selectDouble(sql, defaultVal);
}

/**
 *
 */
MT4_EXPFUNC int __stdcall MOB_selectInt(const int conId, const TCHAR *sql, const int defaultVal)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->selectInt(sql, defaultVal);
}

/**
 *
 */
MT4_EXPFUNC unsigned int __stdcall MOB_selectDatetime(const int conId, const TCHAR *sql, const unsigned int defaultVal)
{
    ODBCWrapper *ow = conns[conId];
    if (!ow) return MOB_INVALID_CONID;
    return ow->selectDatetime(sql, defaultVal);
}

/**
 * Helper function to retrieve the current time.
 */
MT4_EXPFUNC unsigned int __stdcall MOB_time()
{
    time_t t;
    time(&t);
    return (unsigned int) t;
}

/**
 * Helper function to retrieve the current time in local timezone.
 */
MT4_EXPFUNC unsigned int __stdcall MOB_localtime()
{
    time_t t;
    struct tm lt;
    time(&t);
    localtime_s(&lt, &t);
    time_t ret = _mkgmtime(&lt);
    return (unsigned int) ret;
}

/**
 * Helper function to copy a string to another.
 */
MT4_EXPFUNC unsigned int __stdcall MOB_strcpy(TCHAR *dest, const int size, const TCHAR *src)
{
    return _tcsncpy_s(dest, size, src, _TRUNCATE);
}
