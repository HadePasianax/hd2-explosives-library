#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
static ULONGLONG now;
static ULONGLONG TestNow(void){return now;}
#define HADE_TICK_TEST
#define HADE_TICK_NOW TestNow
#include "../src/game_tick_fallback.h"
static unsigned nativeCalls,hooks,ticks,originals,events;
static int accepts,hookFails;
static uintptr_t image;
static void Original(void* p,float dt){originals++;}
static void Tick(HfMod* m,float dt,void* u){assert(dt==.1f);ticks++;}
static int Native(HfMod* m,HfTickCallback cb,void* u){nativeCalls++;return accepts;}
static uintptr_t Module(const char* n,size_t* z){*z=0x4744000;return image;}
static int Hook(HfMod* m,uintptr_t t,void* d,void** out){hooks++;assert(t==image+FRAME_UPDATE_RVA);if(hookFails)return 1;*out=(void*)Original;return 0;}
static void Log(HfMod* m,const char* f,...){}
static int Event(HfMod* m,HfEvent e,HfEventCallback cb,void* u){assert(e==HF_EVENT_GAME_LOADED);events++;return 1;}
static void Call(uint32_t at){uint8_t* p=(uint8_t*)image+at;p[0]=0xe8;int32_t rel=FRAME_UPDATE_RVA-at-5;memcpy(p+1,&rel,4);}
int main(void){
 HfApi a={.on_game_thread=Native,.module_base=Module,.hook=Hook,.log=Log,.on=Event};
 accepts=1;assert(HadeGameTickRegister(&a,NULL,Tick,NULL)&&!hooks);
 accepts=0;assert(HadeGameTickRegister(&a,NULL,Tick,NULL)&&events==1);
 image=(uintptr_t)VirtualAlloc(NULL,0x4744000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(image);
 assert(!HadeGameTickRegister(&a,NULL,Tick,NULL)&&!hooks); // Wrong callsites fail closed.
 Call(FRAME_CALL_RVA);Call(FRAME_CALL2_RVA);
 assert(HadeGameTickRegister(&a,NULL,Tick,NULL)&&hooks==1&&frameOriginal);
 now=1000;assert(FromFrameUpdate(frameReturn));
 now=1250;assert(!FromFrameUpdate(frameReturn2));
 now=1251;assert(FromFrameUpdate(frameReturn2));
 assert(!FromFrameUpdate(123));
 now=1300;assert(FromFrameUpdate(frameReturn));assert(!FromFrameUpdate(frameReturn2));
 FrameUpdateDetour(NULL,.1f);assert(originals==1&&!ticks); // Unknown caller still forwards original.
 gtCallback(gtMod,.1f,gtUser);assert(ticks==1);
 hookFails=1;assert(!HadeGameTickRegister(&a,NULL,Tick,NULL));
 assert(nativeCalls==5);VirtualFree((void*)image,0,MEM_RELEASE);
 puts("PASS callback fallback: native precedence, deferred module load, bad callsite refusal, shared hook install/failure, primary/secondary 250 ms policy, original forwarding.");
}
