#include <windows.h>
#include "loader.h"
#include "tcg.h"

DECLSPEC_IMPORT LPVOID WINAPI KERNEL32$VirtualAlloc ( LPVOID, SIZE_T, DWORD, DWORD );

char _DLL_ [0] __attribute__ ( ( section ( "dll" ) ) );

#define GETRESOURCE(x) ( char * ) &x

void go ( )
{
	/* populate funcs */
	IMPORTFUNCS funcs;
	funcs.LoadLibraryA   = LoadLibraryA;
	funcs.GetProcAddress = GetProcAddress;

	/* get the DLL appended to the loader */
	char * dll_src = GETRESOURCE ( _DLL_ );

	/* parse its headers */
	DLLDATA dll_data;
	ParseDLL ( dll_src, &dll_data );

	/* allocate some RWX memory */
	char * dll_dst = KERNEL32$VirtualAlloc ( NULL, SizeOfDLL ( &dll_data ), MEM_COMMIT | MEM_RESERVE | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE );

	/* copy it into memory */
	LoadDLL ( &dll_data, dll_src, dll_dst );

	/* process its imports */
	ProcessImports ( &funcs, &dll_data, dll_dst );

	/* get its entry point */
	DLLMAIN_FUNC entry_point = EntryPoint ( &dll_data, dll_dst );

	/* call it */
	entry_point ( ( HINSTANCE ) dll_dst, DLL_PROCESS_ATTACH, NULL );
    
	// Use the relocation-safe GetExport to get and keep the callback
    typedef void (WINAPI * _GetVersions)();
    _GetVersions pGetVersions = (_GetVersions)GetExport(dll_dst, "GetVersions");

    if (pGetVersions != NULL) {
        pGetVersions(); 
    }

}

FARPROC resolve ( DWORD mod_hash, DWORD func_hash )
{
    HANDLE module = findModuleByHash ( mod_hash );
    return findFunctionByHash ( module, func_hash );
}