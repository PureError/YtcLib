#pragma once

#define THIS_FILE __FILE__

#ifdef _MSC_VER
#include <crtdbg.h>
void* __cdecl operator new(size_t size, int type, const char* file, int line);
void* __cdecl operator new(size_t size, const char* file, int line);
void* __cdecl operator new[](size_t size, int type, const char* file, int line);
void __cdecl operator delete(void* ptr);
void __cdecl operator delete[](void* ptr);
#ifdef   _DEBUG
#define  SET_CRT_DEBUG_FIELD(a) \
            _CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#define  CLEAR_CRT_DEBUG_FIELD(a) \
            _CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))


#define malloc(s) _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)

#define DBG_NEW new(THIS_FILE, __LINE__)
#else 
#define DBG_NEW new
#endif



class MemLeakChecker
{
public:
	MemLeakChecker(const MemLeakChecker&) = delete;
	MemLeakChecker& operator=(const MemLeakChecker&) = delete;

	MemLeakChecker()
	{
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
		SET_CRT_DEBUG_FIELD(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	}

	~MemLeakChecker()
	{
		_CrtDumpMemoryLeaks();
		CLEAR_CRT_DEBUG_FIELD(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	}

};

#endif

