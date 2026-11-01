#ifndef EXPLOSIVES_LIBRARY_H
#define EXPLOSIVES_LIBRARY_H
#include <windows.h>
#include <psapi.h>
#include <stdint.h>
#include "hellforge_mod.h"
#define EL_ABI 1u
#define EL_RADII 1u
#define EL_SHRAPNEL 2u
/* Call service functions on HellForge's game thread. Definitions are immutable.
   Register private damage with add_damage during mod init, before READY. */
typedef struct ElExplosion {
 uint32_t size, base, damage, flags;
 const char* name; /* globally namespaced stable key, <=127 bytes */
 uint64_t package; /* native effect package; 0 means already managed by caller */
 float inner_radius,outer_radius,shockwave_radius;
 uint32_t shrapnel_count;
} ElExplosion;
typedef struct ElApi {
 uint32_t size,abi;
 uint32_t (*register_explosion)(const ElExplosion* definition);
 int (*copy_explosion)(uint32_t id,HfExplosionSettings* out);
 const char* (*status)(void);
} ElApi;
typedef const ElApi* (*ElGetApi)(uint32_t abi);
/* Discover by exported ABI, not a DLL filename: HellForge may rename DLLs.
   Return NULL until the provider is loaded; reject duplicate providers. */
#ifndef EL_CUSTOM_DISCOVERY
static inline const ElApi* ElFind(void){
 HMODULE modules[1024];DWORD bytes=0;
 if(!K32EnumProcessModules(GetCurrentProcess(),modules,sizeof modules,&bytes)||bytes>sizeof modules)return NULL;
 const ElApi* result=NULL;
 for(unsigned i=0;i<bytes/sizeof(HMODULE);i++){
  ElGetApi get=(ElGetApi)(uintptr_t)GetProcAddress(modules[i],"explosives_library_get_api");
  if(!get)continue;const ElApi* api=get(EL_ABI);
  if(!api||api->abi!=EL_ABI||api->size<sizeof(ElApi)||result)return NULL;
  result=api;
 }
 return result;
}
#endif
#endif
