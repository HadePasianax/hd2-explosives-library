/* Included inside library.c; no mutable table addresses leave the provider. */
#include "../include/explosives_editor.h"
static SRWLOCK editorLock=SRWLOCK_INIT;
static HfExplosionSettings editorDefaults[64];
static unsigned editorPending[64];
static float editorValues[64][3];
static int EditorRecord(uint32_t index,ElEditorRecord* out){
 if(!out||out->size!=sizeof *out)return 0;
 AcquireSRWLockShared(&editorLock);
 int ok=index<used;
 if(ok){out->id=NX_PRIVATE+index;out->donor=specs[index].base;out->damage=rows[index].damage;strcpy(out->name,names[index]);}
 ReleaseSRWLockShared(&editorLock);return ok;
}
static int EditorField(uint32_t offset){return offset==16?0:offset==20?1:offset==24?2:-1;}
static int EditorRead(uint32_t id,uint32_t offset,float* now,float* base){
 int f=EditorField(offset);if(!now||!base||f<0)return 0;
 AcquireSRWLockShared(&editorLock);int ok=id>=NX_PRIVATE&&id<NX_PRIVATE+used;
 if(ok){unsigned i=id-NX_PRIVATE;memcpy(now,(uint8_t*)&rows[i]+offset,4);memcpy(base,(uint8_t*)&editorDefaults[i]+offset,4);}
 ReleaseSRWLockShared(&editorLock);return ok;
}
static int EditorSet(uint32_t id,uint32_t offset,const float* value){
 int f=EditorField(offset);if(f<0||(value&&(!isfinite(*value)||*value<0||*value>1000)))return 0;
 AcquireSRWLockExclusive(&editorLock);int ok=id>=NX_PRIVATE&&id<NX_PRIVATE+used;
 if(ok){unsigned i=id-NX_PRIVATE;float v;if(value)v=*value;else memcpy(&v,(uint8_t*)&editorDefaults[i]+offset,4);editorValues[i][f]=v;editorPending[i]|=1u<<f;}
 ReleaseSRWLockExclusive(&editorLock);return ok;
}
static void EditorTick(void){
 AcquireSRWLockExclusive(&editorLock);
 for(unsigned i=0;i<used;i++)if(editorPending[i]){
  float v[3];memcpy(v,(uint8_t*)&rows[i]+16,12);
  for(unsigned f=0;f<3;f++)if(editorPending[i]&(1u<<f))v[f]=editorValues[i][f];
  if(v[0]<=v[1]&&v[1]<=v[2]){memcpy((uint8_t*)&rows[i]+16,v,12);api->log(owner,"EL EDIT APPLIED name=%s radii=%.2f/%.2f/%.2f",names[i],v[0],v[1],v[2]);}
  else api->log(owner,"EL EDIT REFUSED name=%s: radii must satisfy inner <= outer <= shockwave",names[i]);
  editorPending[i]=0;
 }
 ReleaseSRWLockExclusive(&editorLock);
}
static const ElEditorApi editorService={sizeof(ElEditorApi),1,EditorRecord,EditorRead,EditorSet};
HF_MOD_EXPORT const ElEditorApi* explosives_library_editor_api(uint32_t abi){return abi==1?&editorService:NULL;}
