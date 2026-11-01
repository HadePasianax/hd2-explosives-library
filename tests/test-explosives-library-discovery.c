#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include "../include/explosives_library.h"
int main(int argc,char** argv){
 assert(argc==2);
 assert(!ElFind());
 HMODULE module=LoadLibraryA(argv[1]);
 assert(module);const ElApi* a=ElFind();assert(a&&a->abi==1&&a->size==sizeof *a&&a->register_explosion&&a->copy_explosion);
 assert(a==ElFind());assert(!a->register_explosion(NULL));
 assert(FreeLibrary(module));assert(!ElFind());puts("PASS DLL discovery: exported ABI found independently of filename, stable service pointer, absent-provider handling.");return 0;
}
