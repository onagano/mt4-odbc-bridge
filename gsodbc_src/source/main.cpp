#include "stdhead.h"

HINSTANCE g_hInst;

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hInst;
    }
    return TRUE;
}
