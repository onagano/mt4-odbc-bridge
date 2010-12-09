#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers

//#include <windows.h>
#include "ODBCWrapper.h"

HINSTANCE g_hInst;
int nextConId = 1;
std::map<int, ODBCWrapper*> conns;

#define MT4_EXPFUNC __declspec(dllexport)

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hInst = hInst;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return(TRUE);
}

MT4_EXPFUNC int __stdcall MOB_create()
{
	//TODO: check if non-deleted object
	int id = nextConId++;
	conns[id] = new ODBCWrapper(id);
	return id;
}

MT4_EXPFUNC void __stdcall MOB_open(int conId, const char *dns, const char *username, const char *password)
{
	conns[conId]->open(dns, username, password);
}

MT4_EXPFUNC void __stdcall MOB_close(int conId)
{
	//TODO: check if the wrapper deletion is needed
	conns[conId]->close();
}

MT4_EXPFUNC void __stdcall MOB_execute(int conId, const char *sql)
{
	conns[conId]->execute(sql);
}

MT4_EXPFUNC int __stdcall MOB_getLastErrorNo(int conId)
{
	return conns[conId]->getLastErrorNo();
}

MT4_EXPFUNC char* __stdcall MOB_getLastErrorMesg(int conId)
{
	return conns[conId]->getLastErrorMesg();
}
