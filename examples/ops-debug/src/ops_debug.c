#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "hellforge_mod.h"
#include "explosives_library.h"
#define OPS_ENTITY 0xC897C0D84448AB2Cull
static uint32_t shot,blastDamage,directDamage,explosion;
static int ready,active,waitingLogged,damageConfigured;
static float retry;
static uint64_t lastUpdate;
static float lastPosition[3];
static unsigned damageLogs;
static int ConfigureDamage(HfMod* mod){
 HfDamageInfo* d=(HfDamageInfo*)HF_API(mod)->damage_row(directDamage);
 HfDamageInfo* b=(HfDamageInfo*)HF_API(mod)->damage_row(blastDamage);
 if(!d||d->id!=directDamage||!b||b->id!=blastDamage)return 0;
 d->standard_damage=d->durable_damage=0;d->demolition=d->stagger=d->push_force=0;
 d->status_1_type=d->status_2_type=d->status_3_type=d->status_4_type=0;
 b->standard_damage=b->durable_damage=800;for(unsigned i=0;i<4;i++)b->armor_penetration[i]=5;
 return 1;
}
static void Tick(HfMod* mod,float dt,void* user){
 if(!ready||active)return;retry-=dt;if(retry>0)return;retry=1;
 const HfApi* hf=HF_API(mod);const ElApi* lib=ElFind();
 if(!lib){if(!waitingLogged){hf->log(mod,"OPS DEBUG WAITING: enable Explosives Library; no OPS overwrite applied");waitingLogged=1;}return;}
 if(!damageConfigured){if(!ConfigureDamage(mod))return;damageConfigured=1;}
 if(!explosion){
  ElExplosion e={.size=sizeof e,.base=407,.damage=blastDamage,.flags=EL_RADII,.name="hade/ops_debug/wide_plasma",
   .package=0xA7B23B55209766AEull,.inner_radius=6,.outer_radius=8,.shockwave_radius=10};
  explosion=lib->register_explosion(&e);if(!explosion)return;
 }
 HfProjectileSettings* p=(HfProjectileSettings*)hf->projectile_row(shot);
 uint32_t stride=0;uintptr_t at=hf->component_record("BombardmentComponentData",OPS_ENTITY,&stride);
 if(!p||p->type!=shot||!at||stride!=sizeof(HfBombardment))return;
 p->impact_explosion=explosion;p->expiry_explosion=0;p->damage=directDamage;
 HfBombardment old,updated,verify;
 if(hf->read(at,&old,sizeof old))return;updated=old;
 for(unsigned i=0;i<8;i++)updated.shells[i]=old.shells[i]?shot:0;
 if(!updated.shells[0])return;
 if(hf->write(at,&updated,sizeof updated)||hf->read(at,&verify,sizeof verify)||memcmp(&updated,&verify,sizeof verify)){
  hf->write(at,&old,sizeof old);hf->log(mod,"OPS DEBUG: overwrite failed; rollback requested");return;
 }
 active=1;hf->log(mod,"OPS DEBUG ACTIVE: vanilla OPS shell=%u private explosion=%u blast damage=%u (800/800 AP5, 6/8/10m), direct damage=%u ZERO. Native explosion 407 unchanged.",shot,explosion,blastDamage,directDamage);
}
static void Ready(HfMod* mod,HfEvent event,void* user){ready=1;active=0;retry=0;}
static void Observe(HfMod* mod,const HfEventInfo* info,void* user){
 const HfExplosionEvent* e=(const HfExplosionEvent*)info;
 if(active&&e->type==explosion){lastUpdate=info->update;memcpy(lastPosition,e->position,sizeof lastPosition);HF_API(mod)->log(mod,"OPS DEBUG explosion request %u at %.2f %.2f %.2f",explosion,e->position[0],e->position[1],e->position[2]);}
}
static void DamageObserved(HfMod* mod,const HfEventInfo* info,void* user){
 const HfDamageEvent* e=(const HfDamageEvent*)info;
 if(!active||!lastUpdate||damageLogs>=64||e->kind!=4||info->update<lastUpdate||info->update-lastUpdate>8)return;
 float distance=0;for(unsigned i=0;i<3;i++){float d=e->position[i]-lastPosition[i];distance+=d*d;}
 if(distance>144)return;damageLogs++;
 HF_API(mod)->log(mod,"OPS DEBUG nearby explosion damage: target=%u amount=%d distance_squared=%.2f (correlation)",e->target,e->amount,distance);
}
HF_MOD_EXPORT int hellforge_mod_init(const HfApi* hf,HfMod* mod){
 if(hf->version<15||strcmp(hf->build_id(),"25480438"))return 1;
 HfDamage d={.size=sizeof d,.base=61,.name="hade/ops_debug/blast",.scale=1};blastDamage=hf->add_damage(mod,&d);
 d.base=264;d.name="hade/ops_debug/zero_direct";directDamage=hf->add_damage(mod,&d);
 HfProjectile p={.size=sizeof p,.base=100,.name="hade/ops_debug/shell",.damage=directDamage,.visual_from=193,
  .spawn_effect=0x386423966E9FF9A2ull,.flight_effect=0xA424C1FF90103592ull};shot=hf->add_projectile(mod,&p);
 if(!shot||!blastDamage||!directDamage)return 1;
 hf->on(mod,HF_EVENT_PROJECTILES_READY,Ready,NULL);hf->subscribe(mod,HF_ON_EXPLOSION,Observe,NULL,0);hf->subscribe(mod,HF_ON_DAMAGE,DamageObserved,NULL,0);
 return !hf->on_game_thread(mod,Tick,NULL);
}
