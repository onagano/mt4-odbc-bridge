#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers

//#include <windows.h>
#include "ODBCWrapper.h"

HINSTANCE g_hInst;
std::map<const char*, ODBCWrapper*> conns;

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

MT4_EXPFUNC void __stdcall MOB_create(const char *name)
{
	//TODO: check if non-deleted object
	conns[name] = new ODBCWrapper(name);
}

MT4_EXPFUNC void __stdcall MOB_open(const char *name, const char *dns, const char *username, const char *password)
{
	conns[name]->open(dns, username, password);
}

MT4_EXPFUNC void __stdcall MOB_close(const char *name)
{
	ODBCWrapper *conn = conns[name];
	conn->close();
	delete conn;
}

MT4_EXPFUNC void __stdcall MOB_execute(const char *name, const char *sql)
{
	conns[name]->execute(sql);
}
