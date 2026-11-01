#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#define EL_CUSTOM_DISCOVERY
struct ElApi;
static const struct ElApi* ElFind(void);
#include "../examples/ops-debug/src/ops_debug.c"
static HfDamageInfo damageRows[2];
static HfProjectileSettings projectile;
static HfBombardment bombardment;
static int available;
static unsigned writes,registers;

static uint32_t Register(const ElExplosion* e){registers++;assert(e->base==407&&e->damage==650&&e->inner_radius==6&&e->outer_radius==8&&e->shockwave_radius==10);return 423;}
static const ElApi service={.size=sizeof(ElApi),.abi=1,.register_explosion=Register};
static const ElApi* ElFind(void){return available?&service:NULL;}
static int Read(uintptr_t at,void* out,size_t n){memcpy(out,(void*)at,n);return 0;}
static int Write(uintptr_t at,const void* in,size_t n){writes++;memcpy((void*)at,in,n);return 0;}
static void Log(HfMod* mod,const char* text,...){}
static uintptr_t DamageRow(uint32_t id){assert(id==650||id==651);return (uintptr_t)&damageRows[id-650];}
static uintptr_t Projectile(uint32_t id){assert(id==351);return (uintptr_t)&projectile;}
static uintptr_t Component(const char* name,uint64_t id,uint32_t* stride){assert(id==OPS_ENTITY&&!strcmp(name,"BombardmentComponentData"));*stride=sizeof bombardment;return (uintptr_t)&bombardment;}
int main(void){
 HfApi api={.size=sizeof api,.version=15,.read=Read,.write=Write,.log=Log,.damage_row=DamageRow,.projectile_row=Projectile,.component_record=Component};
 const HfApi* ptr=&api;HfMod* mod=(void*)&ptr;
 blastDamage=650;directDamage=651;shot=351;ready=1;projectile.type=351;projectile.impact_explosion=343;
 damageRows[0].id=650;damageRows[1].id=651;damageRows[1].standard_damage=100;
 bombardment.shells[0]=100;bombardment.shells[1]=100;bombardment.salvos=1;bombardment.shells_per_salvo=1;
 HfBombardment before=bombardment;
 Tick(mod,0,NULL);assert(!active&&!writes&&!registers&&!memcmp(&before,&bombardment,sizeof before));
 available=1;Tick(mod,2,NULL);assert(active&&writes==1&&registers==1&&explosion==423);
 assert(projectile.damage==651&&projectile.impact_explosion==423&&!projectile.expiry_explosion);
 assert(!damageRows[1].standard_damage&&!damageRows[1].durable_damage&&damageRows[0].standard_damage==800);
 assert(bombardment.shells[0]==351&&bombardment.shells[1]==351&&!bombardment.shells[2]);
 before.shells[0]=351;before.shells[1]=351;assert(!memcmp(&before,&bombardment,sizeof before));
 puts("PASS OPS: missing-library no-op, private blast configuration, zero direct damage, shell replacement, native timing preserved.");return 0;
}
