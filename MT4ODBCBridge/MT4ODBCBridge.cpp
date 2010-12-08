#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers

#include "MT4ODBCBridge.h"
#include "ODBCWrapper.h"

HINSTANCE g_hInst;
ODBCWrapper *cCon;

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

MT4_EXPFUNC void __stdcall MOB_open(const char *dns, const char *username, const char *password)
{
	cCon = new ODBCWrapper();
	cCon->open(dns, username, password);
}

MT4_EXPFUNC void __stdcall MOB_close()
{
	cCon->close();
	delete cCon;
}

MT4_EXPFUNC void __stdcall MOB_execute(const char *sql)
{
	cCon->execute(sql);
}
