#include "game_tick_fallback.h"
/* Experimental shared registry, supported game build 25480438 only. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../include/explosives_library.h"
#define IMAGE_SIZE 0x4744000u
static uintptr_t gameBase;
static int CodeIs(const HfApi* hf,uintptr_t at,const uint8_t* bytes,size_t n)
{uint8_t got[32];return n<=sizeof got&&!hf->read_code(at,got,n)&&!memcmp(got,bytes,n);}
static uint8_t* AllocateNear(uintptr_t base,size_t size)
{
 SYSTEM_INFO si;GetSystemInfo(&si);uintptr_t step=si.dwAllocationGranularity;
 for(uintptr_t d=step;d<0x60000000u;d+=step){uintptr_t at=(base+d)&~(step-1);void* p=VirtualAlloc((void*)at,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(p)return p;}
 return NULL;
}

#include "registry_patch.h"
static HfMod* owner;
static const HfApi* api;
static int rowsReady,failed;
static DWORD gameThread;
static float retry;
static unsigned used;
static HfExplosionSettings rows[64] __attribute__((aligned(8)));
static ElExplosion specs[64];
static char names[64][128];
static int Same(const ElExplosion* a,const ElExplosion* b){
 return a->base==b->base&&a->damage==b->damage&&a->flags==b->flags&&a->package==b->package&&
 (!(a->flags&EL_RADII)||(a->inner_radius==b->inner_radius&&a->outer_radius==b->outer_radius&&a->shockwave_radius==b->shockwave_radius))&&
 (!(a->flags&EL_SHRAPNEL)||a->shrapnel_count==b->shrapnel_count);
}
#include "editor_bridge.h"
static uint32_t RegisterImpl(const ElExplosion* d){
 if(!nxReady||GetCurrentThreadId()!=gameThread||!d||d->size!=sizeof *d||!d->name||!d->name[0]||
    strlen(d->name)>127||!strchr(d->name,'/')||!d->base||d->base>=423||d->flags&~(EL_RADII|EL_SHRAPNEL)||
    !d->damage||!api->damage_row(d->damage))return 0;
 if((d->flags&EL_RADII)&&(!isfinite(d->inner_radius)||!isfinite(d->outer_radius)||!isfinite(d->shockwave_radius)||
   d->inner_radius<0||d->outer_radius<d->inner_radius||d->shockwave_radius<d->outer_radius))return 0;
 for(unsigned i=0;i<used;i++)if(!strcmp(names[i],d->name))return Same(&specs[i],d)?NX_PRIVATE+i:0;
 if(used==64)return 0;
 if(d->package&&api->load_package(owner,d->package)!=HF_PACKAGE_RESIDENT)return 0;
 uintptr_t donor=0;HfExplosionSettings row;
 if(api->read((uintptr_t)nxBlock+d->base*8,&donor,8)||!donor||api->read(donor,&row,sizeof row))return 0;
 uint32_t nativeId=0;memcpy(&nativeId,&row,4);if(nativeId!=d->base)return 0;
 uintptr_t aux=0;uint64_t count=0;memcpy(&aux,(uint8_t*)&row+40,8);memcpy(&count,(uint8_t*)&row+48,8);
 if(count){uint8_t probe;if(!aux||api->read(aux,&probe,1))return 0;}
 uint32_t id=NX_PRIVATE+used;memcpy(&row,&id,4);row.damage=d->damage;
 if(d->flags&EL_RADII){row.inner_radius=d->inner_radius;row.outer_radius=d->outer_radius;row.shockwave_radius=d->shockwave_radius;}
 if(d->flags&EL_SHRAPNEL)row.shrapnel_count=d->shrapnel_count;
 rows[used]=row;editorDefaults[used]=row;specs[used]=*d;strcpy(names[used],d->name);specs[used].name=names[used];
 MemoryBarrier();InterlockedExchangePointer((PVOID volatile*)((uintptr_t*)nxBlock+id),&rows[used]);used++;
 api->log(owner,"EL REGISTER name=%s explosion=%u donor=%u private_damage=%u radii=%.2f/%.2f/%.2f",d->name,id,d->base,d->damage,row.inner_radius,row.outer_radius,row.shockwave_radius);
 return id;
}
static uint32_t Register(const ElExplosion* d){AcquireSRWLockExclusive(&editorLock);uint32_t id=RegisterImpl(d);ReleaseSRWLockExclusive(&editorLock);return id;}
static int CopyImpl(uint32_t id,HfExplosionSettings* out){
 if(!nxReady||GetCurrentThreadId()!=gameThread||!out||id<NX_PRIVATE||id>=NX_PRIVATE+used)return 0;
 *out=rows[id-NX_PRIVATE];return 1;
}
static int Copy(uint32_t id,HfExplosionSettings* out){AcquireSRWLockShared(&editorLock);int ok=CopyImpl(id,out);ReleaseSRWLockShared(&editorLock);return ok;}
static const char* Status(void){return nxReady?"ready":nxFailure;}
static const ElApi service={sizeof(ElApi),EL_ABI,Register,Copy,Status};
HF_MOD_EXPORT const ElApi* explosives_library_get_api(uint32_t abi){return abi==EL_ABI?&service:NULL;}
static void Ready(HfMod* mod,HfEvent event,void* user){rowsReady=1;}
static void Tick(HfMod* mod,float dt,void* user){
 gameThread=GetCurrentThreadId();EditorTick();if(!rowsReady||nxReady||failed)return;retry-=dt;if(retry>0)return;retry=2;
 if(api->load_package(owner,0xA7B23B55209766AEull)!=HF_PACKAGE_RESIDENT)return;
 if(!NxInstall(mod)){failed=1;api->log(mod,"EL DISABLED: %s at RVA %llx; restart required",nxFailure,(unsigned long long)nxFailureRva);}
}
extern int ElInplaceInit(const HfApi*,HfMod*);
HF_MOD_EXPORT int hellforge_mod_init(const HfApi* hf,HfMod* mod){
 if(hf->version<15||strcmp(hf->build_id(),"25480438"))return 1;
 owner=mod;api=hf;ElInplaceInit(hf,mod);hf->on(mod,HF_EVENT_PROJECTILES_READY,Ready,NULL);
 hf->log(mod,"Explosives Library 0.2.1 ABI 1 + stock editor integration; restart between changes; 64 private explosion slots");
 return !HadeGameTickRegister(hf,mod,Tick,NULL);
}
