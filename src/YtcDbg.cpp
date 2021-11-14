#include "YtcDbg.hpp"


void* __cdecl operator new(size_t size, const char* filename, int line)
{
    return ::operator new(size, _NORMAL_BLOCK, filename, line);
}

void* __cdecl operator new[](size_t size, const char* filename, int line)
{
    return ::operator new(size, _NORMAL_BLOCK, filename, line);
}

void* __cdecl operator new(size_t size, int type, const char* filename, int line)
{
#ifdef _DEBUG
    return _malloc_dbg(size, type, filename, line);
#else
    return ::operator new(size);
#endif
}


void __cdecl operator delete(void* p)
{
#ifdef _DEBUG
    _free_dbg(p, _NORMAL_BLOCK);
#else
    free(p);
#endif
}

void __cdecl operator delete[](void* p)
{
    ::operator delete(p);
}