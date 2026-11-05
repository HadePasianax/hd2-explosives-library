#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include "../src/library.c"
static HfApi testApi;
static void SetApiForTest(const HfApi* value){api=value;}
static HfExplosionSettings nativeRows[423];
static HfDamageInfo damage={.id=650,.standard_damage=800,.durable_damage=800};
static unsigned writes;
static int Read(uintptr_t at,void* out,size_t n){memcpy(out,(void*)at,n);return 0;}
static int Write(uintptr_t at,const void* in,size_t n){writes++;memcpy((void*)at,in,n);return 0;}
static void Log(HfMod* m,const char* f,...){}
static uintptr_t DamageRow(uint32_t id){return id==650?(uintptr_t)&damage:0;}
int main(void){
 HfApi old={.version=15};assert(hellforge_mod_init(&old,NULL)==1);
 testApi=(HfApi){.size=sizeof testApi,.version=16,.read=Read,.read_code=Read,.write=Write,.log=Log,.damage_row=DamageRow};const HfApi* a=&testApi;HfMod* mod=(void*)&a;
 gameBase=(uintptr_t)VirtualAlloc(NULL,IMAGE_SIZE,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(gameBase);
 uintptr_t pointers[423];for(unsigned i=0;i<423;i++){memcpy(&nativeRows[i],&i,4);pointers[i]=(uintptr_t)&nativeRows[i];}
 nativeRows[407].damage=61;nativeRows[407].inner_radius=3;nativeRows[407].outer_radius=4;nativeRows[407].shockwave_radius=5;
 memcpy((void*)(gameBase+NX_TABLE),pointers,sizeof pointers);HfExplosionSettings before=nativeRows[407];
 assert(NxBuildRows(&testApi,pointers));assert(!memcmp(&before,&nativeRows[407],sizeof before));
 uint8_t code[40]={0x3d};uint32_t count=423;memcpy(code+1,&count,4);uint8_t reader[]={0x4c,0x8b,0xb4,0xc1};memcpy(code+5,reader,4);uint32_t table=NX_TABLE;memcpy(code+9,&table,4);
 unsigned refs=0,bounds=0;assert(NxScan(code,0x1000,sizeof code,gameBase+NX_TABLE+0x10000,&refs,&bounds));assert(refs==1&&bounds==1&&nxPatchCount==2&&!writes);
 nxPatchCount=0;memcpy(code+30,&table,4);assert(!NxScan(code,0x1000,sizeof code,gameBase+NX_TABLE+0x10000,&refs,&bounds));assert(!writes);
 /* Regression: table LEA precedes the kick bound, so the old scan misses it. */
 uint8_t kick[]={0x3d,0xa7,1,0,0,0x73,8,0x85,0xc0,0x74,4,0x4c,0x8b,0x34,0xc2};
 memcpy((void*)(gameBase+0x13c551e),kick,sizeof kick);
 uint8_t lea[]={0x48,0x8d,0x15};memcpy((void*)(gameBase+0x13c54df),lea,3);
 int32_t disp=(int32_t)(NX_TABLE-(0x13c54df+7));memcpy((void*)(gameBase+0x13c54df+3),&disp,4);
 nxPatchCount=0;assert(NxPlanKickBound(&testApi));assert(nxPatchCount==1&&nxPatches[0].at==gameBase+0x13c551f&&nxPatches[0].before==423&&nxPatches[0].after==487&&!writes);
 ((uint8_t*)(gameBase+0x13c551e))[5]=0x74;nxPatchCount=0;assert(!NxPlanKickBound(&testApi)&&!nxPatchCount&&!writes);
 nxReady=1;nxBlock=VirtualAlloc(NULL,16384,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(nxBlock);
 memcpy(nxBlock,pointers,sizeof pointers);gameThread=GetCurrentThreadId();owner=mod;
 /* "api" is the harness value; the library's global API pointer is set below. */
 SetApiForTest(&testApi);
 ElExplosion e={.size=sizeof e,.base=407,.damage=650,.flags=EL_RADII,.name="test/one",.inner_radius=6,.outer_radius=8,.shockwave_radius=10};
 uint32_t id=Register(&e);assert(id==423&&used==1);assert(Register(&e)==id&&used==1);
 HfExplosionSettings result;assert(Copy(id,&result)&&result.damage==650&&result.outer_radius==8);
 e.outer_radius=9;assert(!Register(&e)&&used==1);e.name="test/two";assert(Register(&e)==424);
 assert(!memcmp(&before,&nativeRows[407],sizeof before));assert(damage.standard_damage==800);
 e.name="test/invalid";e.damage=61;assert(!Register(&e));e.damage=650;e.inner_radius=-1;assert(!Register(&e));
 e.inner_radius=6;e.outer_radius=8;e.name="test/one";DWORD saved=gameThread;gameThread=0;assert(!Register(&e));gameThread=saved;
 const ElEditorApi* editor=explosives_library_editor_api(1);assert(editor&&!explosives_library_editor_api(2));
 ElEditorRecord rec={.size=sizeof rec};assert(editor->record(0,&rec)&&rec.id==423&&!strcmp(rec.name,"test/one"));assert(!editor->record(64,&rec));
 float now,base;assert(editor->read(423,20,&now,&base)&&now==8&&base==8);
 float radius=9;assert(editor->set(423,20,&radius));assert(Copy(423,&result)&&result.outer_radius==8);
 EditorTick();assert(Copy(423,&result)&&result.outer_radius==9);assert(editor->read(423,20,&now,&base)&&now==9&&base==8);
 assert(editor->set(423,20,NULL));EditorTick();assert(Copy(423,&result)&&result.outer_radius==8);
 radius=-1;assert(!editor->set(423,20,&radius));radius=NAN;assert(!editor->set(423,20,&radius));
 assert(!editor->set(423,0,NULL)&&!editor->set(422,20,NULL)&&!editor->set(487,20,NULL));
 radius=100;assert(editor->set(423,20,&radius));EditorTick();assert(Copy(423,&result)&&result.outer_radius==8);
 radius=7;assert(editor->set(423,16,&radius));radius=9;assert(editor->set(423,20,&radius));EditorTick();assert(Copy(423,&result)&&result.inner_radius==7&&result.outer_radius==9);
 e.name="test/one";assert(Register(&e)==423);assert(!memcmp(&before,&nativeRows[407],sizeof before));
 for(unsigned i=used;i<64;i++){char key[30];snprintf(key,sizeof key,"test/slot%u",i);e.name=key;assert(Register(&e)==423+i);}
 e.name="test/full";assert(!Register(&e)&&used==64);assert(!Copy(487,&result));
 assert(!memcmp(&before,&nativeRows[407],sizeof before));
 assert(explosives_library_get_api(1)&&!explosives_library_get_api(2));
 puts("PASS editor bridge: enumeration, pending writes, game-thread application, reset, immutable registration defaults, invalid IDs/fields/NaN/radii rejection and donor isolation. Registry: ID allocation, idempotence, conflicting names, capacity, private damage validation, radius validation, thread restriction, donor isolation, guarded scanner and kick bound.");
 return 0;
}

int ElInplaceInit(const HfApi* api,HfMod* mod){return 1;}
