/* Optional lab: expands the default editor's catalogue in memory at read time.
 * Does not write editor files, replace its DLL, or inspect private C++ objects. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <psapi.h>
#include <mutex>
#include <map>
#include <memory>
#include <set>
#include "catalog_injection.h"
#include "sandbox_overlay.h"
#include "../include/explosives_editor.h"
extern "C" const ElEditorApi* explosives_library_editor_api(uint32_t);
namespace {
const HfApi* host;HfMod* owner;
using SizeFn=BOOL(WINAPI*)(HANDLE,PLARGE_INTEGER);
using ReadFn=BOOL(WINAPI*)(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
using CloseFn=BOOL(WINAPI*)(HANDLE);
SizeFn originalSize;ReadFn originalRead;CloseFn originalClose;
decltype(HfApi::get_live) originalGet;
decltype(HfApi::set_live) originalSet;
struct Stream {std::shared_ptr<std::string> bytes;size_t at=0;};
std::mutex lock;
std::map<HANDLE,Stream> streams;
std::vector<editorlab::Record> known;
std::vector<editorlab::Proxy> proxies;
std::set<std::string> readNoted;
std::map<std::string,HfValue> baselines;
std::string BaselineKey(HfRef ref,const HfField* field){return std::string(field->component)+"/"+std::to_string(ref)+"/"+std::to_string(field->offset)+"/"+std::to_string(field->type);}
void KeepBaseline(HfRef ref,const HfField* field,HfLiveValue* value){
 std::lock_guard<std::mutex> guard(lock);auto found=baselines.find(BaselineKey(ref,field));if(found!=baselines.end())value->base=found->second;
}
bool IsEditor(HfMod* mod){const char* id=host->mod_id(mod);return id&&!strcmp(id,"hellforge.editor");}
bool Catalogue(HANDLE file){
 wchar_t path[32768];DWORD n=GetFinalPathNameByHandleW(file,path,32768,FILE_NAME_NORMALIZED);
 if(!n||n>=32768)return false;
 const wchar_t* slash=wcsrchr(path,L'\\');return !_wcsicmp(slash?slash+1:path,L"editor-catalog.json");
}
int SavedFile(HANDLE file){
 wchar_t path[32768];DWORD n=GetFinalPathNameByHandleW(file,path,32768,FILE_NAME_NORMALIZED);if(!n||n>=32768)return 0;
 std::wstring value=path;std::transform(value.begin(),value.end(),value.begin(),towlower);
 const std::wstring mod=L"\\hellforge-editor\\mod-stats.json",stats=L"\\zz.local.stats\\hellforge-stats.json";
 if(value.size()>=mod.size()&&value.compare(value.size()-mod.size(),mod.size(),mod)==0)return 1;
 if(value.size()>=stats.size()&&value.compare(value.size()-stats.size(),stats.size(),stats)==0)return 2;
 return 0;
}
std::vector<editorlab::Record> Records(){
 std::vector<editorlab::Record> result;
 for(unsigned i=0;i<4096;i++){
  HfContent c{};c.size=sizeof c;if(!host->content(owner,i,&c))break;
  if((c.kind==HF_KIND_PROJECTILE||c.kind==HF_KIND_DAMAGE)&&c.name&&c.mod)
   result.push_back({c.kind,(uint32_t)c.id,(uint32_t)c.base,c.name,c.mod});
 }
 return result;
}

BOOL WINAPI Size(HANDLE file,PLARGE_INTEGER size){
 if(!size)return originalSize(file,size);
 {std::lock_guard<std::mutex> guard(lock);auto i=streams.find(file);if(i!=streams.end()){size->QuadPart=i->second.bytes->size();return TRUE;}}
 bool catalogue=Catalogue(file);int saved=catalogue?0:SavedFile(file);
 if(!catalogue&&!saved)return originalSize(file,size);

 LARGE_INTEGER native{},pos{},zero{};
 if(!originalSize(file,&native)||native.QuadPart<=0||native.QuadPart>=(64<<20)||!SetFilePointerEx(file,zero,&pos,FILE_CURRENT))return originalSize(file,size);
 std::string input((size_t)native.QuadPart,'\0');DWORD got=0;
 bool ok=SetFilePointerEx(file,zero,nullptr,FILE_BEGIN)&&originalRead(file,&input[0],(DWORD)input.size(),&got,nullptr)&&got==input.size();
 SetFilePointerEx(file,pos,nullptr,FILE_BEGIN);
 if(!ok)return originalSize(file,size);
 if(saved){
  wchar_t folder[32768];DWORD n=GetEnvironmentVariableW(L"HELLFORGE_SANDBOX_STATS",folder,32768);if(!n||n>=32768)return originalSize(file,size);
  std::string journal,merged;unsigned changes=0;
  if(!hfjson::ReadFile(std::wstring(folder)+L"\\changes.json",&journal)||!editorlab::ReplayJournal(input,journal,saved==1,&merged,&changes)||!changes)return originalSize(file,size);
  auto bytes=std::make_shared<std::string>(std::move(merged));
  {std::lock_guard<std::mutex> guard(lock);streams[file]={bytes,(size_t)pos.QuadPart};}
  size->QuadPart=bytes->size();host->log(owner,"EL INPLACE SAVED OVERLAY: %s replayed %u session change(s), disk unchanged",saved==1?"mod-stats":"Local Stats",changes);return TRUE;
 }
 auto records=Records();editorlab::Expanded info;std::string why,expanded;
 if(!editorlab::Expand(input,records,&expanded,&info,&why)){
  host->log(owner,"EL INPLACE CATALOG REFUSED: %s; default file passed through",why.c_str());return originalSize(file,size);
 }
 if(!info.projectiles&&!info.damage){host->log(owner,"EL INPLACE CATALOG: intercepted but no custom records registered yet; default file passed through");return originalSize(file,size);}
 auto bytes=std::make_shared<std::string>(std::move(expanded));
 {std::lock_guard<std::mutex> guard(lock);streams[file]={bytes,(size_t)pos.QuadPart};known=records;proxies=info.proxies;}
 size->QuadPart=bytes->size();
 host->log(owner,"EL INPLACE CATALOG: injected %u projectile, %u damage and %u runtime impact-explosion entries; disk unchanged",info.projectiles,info.damage,info.explosions);
 return TRUE;
}
BOOL WINAPI Read(HANDLE file,LPVOID buffer,DWORD count,LPDWORD got,LPOVERLAPPED overlap){
 if(!overlap){
  std::lock_guard<std::mutex> guard(lock);auto i=streams.find(file);
  if(i!=streams.end()){
   if(!buffer||!got){SetLastError(ERROR_INVALID_PARAMETER);return FALSE;}
   Stream& s=i->second;size_t n=std::min((size_t)count,s.bytes->size()-std::min(s.at,s.bytes->size()));
   memcpy(buffer,s.bytes->data()+std::min(s.at,s.bytes->size()),n);s.at+=n;*got=(DWORD)n;return TRUE;
  }
 }
 return originalRead(file,buffer,count,got,overlap);
}
BOOL WINAPI Close(HANDLE file){{std::lock_guard<std::mutex> guard(lock);streams.erase(file);}return originalClose(file);}
bool ProxyOf(HfRef ref,editorlab::Proxy* out){std::lock_guard<std::mutex> guard(lock);for(const auto& p:proxies)if(p.ref==ref||ref==(0xE2000000u|p.projectile)||ref==(0xE3000000u|p.projectile)){*out=p;return true;}return false;}
bool Custom(HfRef ref,const HfField* f){
 if(!f||!f->component)return false;
 std::lock_guard<std::mutex> guard(lock);
 for(const auto& c:known)if(c.id==ref&&((c.kind==HF_KIND_PROJECTILE&&!strcmp(f->component,"ProjectileSettings"))||(c.kind==HF_KIND_DAMAGE&&!strcmp(f->component,"DamageInfo"))))return true;
 return false;
}
uint32_t Impact(HfMod* mod,const editorlab::Proxy& proxy){
 HfField field{"ProjectileSettings",25480438,(uint16_t)offsetof(HfProjectileSettings,impact_explosion),4,HF_TYPE_U32,0};HfLiveValue value{};
 return originalGet(mod,proxy.projectile,&field,&value)?(uint32_t)value.now.number:0;
}
uint32_t ImpactDamage(uint32_t actual){
 const ElEditorApi* api=explosives_library_editor_api(1);if(!api)return 0;
 for(unsigned i=0;i<64;i++){ElEditorRecord record{};record.size=sizeof record;if(!api->record(i,&record))break;if(record.id==actual)return record.damage;}
 return 0;
}
int WriteVerified(HfMod* mod,HfRef ref,const HfField* field,const HfValue* value,uint32_t flags){
 HfLiveValue before{},after{};int had=originalGet(mod,ref,field,&before);
 if(!had)return 0; /* Never edit a custom field without a recoverable baseline. */
 HfValue baseline;
 {std::lock_guard<std::mutex> guard(lock);baseline=baselines.emplace(BaselineKey(ref,field),before.base).first->second;}
 before.base=baseline;
 int result=originalSet(mod,ref,field,value,flags);
 int read=result?originalGet(mod,ref,field,&after):0;
 /* A backend reset may have lost its original after a cache/persistence pass.
  * Recover only this field from the baseline captured before its first edit. */
 if(!value&&result&&read&&after.now.number!=baseline.number){
  result=originalSet(mod,ref,field,&baseline,flags&~HF_LIVE_BASE);read=result?originalGet(mod,ref,field,&after):0;
  host->log(owner,"EL INPLACE BASE RESTORE: %s ID %llu offset %u requested=%.9g accepted=%d readback=%d actual=%.9g",field->component,(unsigned long long)ref,field->offset,baseline.number,result!=0,read!=0,read?after.now.number:0);
 }
 if(read)after.base=baseline;
 host->log(owner,"EL INPLACE VERIFY: %s ID %llu offset %u action=%s accepted=%d readback=%d before=%.9g after=%.9g base=%.9g requested=%.9g",field->component,(unsigned long long)ref,field->offset,value?"set":"reset",result!=0,read!=0,had?before.now.number:0,read?after.now.number:0,read?after.base.number:0,value?value->number:(had?before.base.number:0));
 return result;
}
int Get(HfMod* mod,HfRef ref,const HfField* f,HfLiveValue* out){
 if(IsEditor(mod)&&f&&f->component&&!strcmp(f->component,"DamageInfo")&&(ref&0xFF000000u)==0xE2000000u){
  editorlab::Proxy proxy;if(ProxyOf(ref,&proxy)){
   uint32_t actual=Impact(mod,proxy),damage=ImpactDamage(actual);if(!damage)return 0;
   int ok=originalGet(mod,damage,f,out);if(ok)KeepBaseline(damage,f,out);bool note;
   {std::lock_guard<std::mutex> guard(lock);note=readNoted.insert("explosion-damage/"+proxy.name+"/"+std::to_string(damage)+"/"+std::to_string(ok!=0)).second;}
   if(note)host->log(owner,"EL INPLACE DAMAGE READ: %s explosion %u -> damage %u %s",proxy.name.c_str(),actual,damage,ok?"accepted":"REFUSED by HellForge");return ok;
  }
 }
 if(IsEditor(mod)&&f&&f->component&&!strcmp(f->component,"ExplosionSettings")){
  editorlab::Proxy proxy;if(ProxyOf(ref,&proxy)){
   const ElEditorApi* api=explosives_library_editor_api(1);uint32_t actual=Impact(mod,proxy);float now,base;
   if(!out||!api||f->size!=4||f->type!=HF_TYPE_F32||!api->read(actual,f->offset,&now,&base)){
    bool note;{std::lock_guard<std::mutex> guard(lock);note=readNoted.insert("unresolved/"+proxy.name+"/"+std::to_string(actual)).second;}
    if(note)host->log(owner,"EL INPLACE WAITING: %s impact %u is not an active editable EL explosion yet",proxy.name.c_str(),actual);
    return 0;
   }
   *out={};out->now=HF_F32(now);out->base=HF_F32(base);out->mod="hade.explosives-library";
   bool note;{std::lock_guard<std::mutex> guard(lock);note=readNoted.insert("explosion/"+proxy.name).second;}
   if(note)host->log(owner,"EL INPLACE READ: %s -> actual impact explosion %u",proxy.name.c_str(),actual);
   return HF_APPLIES_NOW;
  }
 }
 int result=originalGet(mod,ref,f,out);
 if(IsEditor(mod)&&Custom(ref,f)){
  if(result)KeepBaseline(ref,f,out);
  if(result&&!strcmp(f->component,"ProjectileSettings")&&(f->offset==offsetof(HfProjectileSettings,impact_explosion)||f->offset==offsetof(HfProjectileSettings,expiry_explosion))){
   uint32_t actual=(uint32_t)out->now.number;
   if(ImpactDamage(actual)){
    uint32_t label=(f->offset==offsetof(HfProjectileSettings,expiry_explosion)?0xE3000000u:0xE1000000u)|(uint32_t)ref;
    out->now.number=label;if((uint32_t)out->base.number==actual)out->base.number=label;
   }
  }
  bool note;std::string key=std::string(f->component)+"/"+std::to_string(ref)+"/"+std::to_string(result!=0);
  {std::lock_guard<std::mutex> guard(lock);note=readNoted.insert(key).second;}
  if(note)host->log(owner,"EL INPLACE READ: %s ID %llu offset %u %s",f->component,(unsigned long long)ref,f->offset,result?"accepted":"REFUSED by HellForge");
 }
 return result;
}
int Set(HfMod* mod,HfRef ref,const HfField* f,const HfValue* v,uint32_t flags){
 if(IsEditor(mod)&&f&&f->component&&!strcmp(f->component,"DamageInfo")&&(ref&0xFF000000u)==0xE2000000u){
  editorlab::Proxy proxy;if(ProxyOf(ref,&proxy)){
   uint32_t actual=Impact(mod,proxy),damage=ImpactDamage(actual);if(!damage)return 0;
   int ok=WriteVerified(mod,damage,f,v,flags);
   host->log(owner,"EL INPLACE DAMAGE WRITE: %s explosion %u damage %u offset %u %s%s",proxy.name.c_str(),actual,damage,f->offset,ok?"accepted":"REFUSED by HellForge",v?"":" (reset)");return ok;
  }
 }
 if(IsEditor(mod)&&f&&f->component&&!strcmp(f->component,"ExplosionSettings")){
  editorlab::Proxy proxy;if(ProxyOf(ref,&proxy)){
   const ElEditorApi* api=explosives_library_editor_api(1);uint32_t actual=Impact(mod,proxy);float value=v?(float)v->number:0;
   int ok=api&&f->size==4&&f->type==HF_TYPE_F32&&api->set(actual,f->offset,v?&value:nullptr);
   host->log(owner,"EL INPLACE WRITE: %s actual explosion %u offset %u %s",proxy.name.c_str(),actual,f->offset,ok?"queued (check EL EDIT APPLIED)":"REFUSED");
   return ok?HF_APPLIES_NOW:0;
  }
 }
 bool custom=IsEditor(mod)&&Custom(ref,f);
 HfValue translated{};
 if(custom&&v&&!strcmp(f->component,"ProjectileSettings")&&(f->offset==offsetof(HfProjectileSettings,impact_explosion)||f->offset==offsetof(HfProjectileSettings,expiry_explosion))){
  uint32_t label=(uint32_t)v->number;
  if((label&0xFF000000u)==0xE1000000u||(label&0xFF000000u)==0xE3000000u){
   editorlab::Proxy proxy;if(!ProxyOf(label,&proxy))return 0;
   HfField source=*f;source.offset=(uint16_t)((label&0xFF000000u)==0xE3000000u?offsetof(HfProjectileSettings,expiry_explosion):offsetof(HfProjectileSettings,impact_explosion));
   HfLiveValue target{};if(!originalGet(mod,proxy.projectile,&source,&target)||!ImpactDamage((uint32_t)target.now.number))return 0;
   translated=*v;translated.number=target.now.number;v=&translated;
  }
 }
 int result=custom?WriteVerified(mod,ref,f,v,flags):originalSet(mod,ref,f,v,flags);
 if(IsEditor(mod)&&Custom(ref,f))host->log(owner,"EL INPLACE WRITE: %s ID %llu offset %u %s%s",f->component,(unsigned long long)ref,f->offset,result?"accepted":"REFUSED by HellForge",v?"":" (reset)");
 return result;
}
}
extern "C" int ElInplaceInit(const HfApi* api,HfMod* mod){
 host=api;owner=mod;
 const char* folder=api->mod_folder?api->mod_folder(mod):nullptr;
 if(folder){std::string path=std::string(folder)+"/explosives-library.ini";int n=MultiByteToWideChar(CP_UTF8,0,path.c_str(),-1,nullptr,0);std::vector<wchar_t> wide(n);if(n&&MultiByteToWideChar(CP_UTF8,0,path.c_str(),-1,wide.data(),n)&&!GetPrivateProfileIntW(L"editor",L"enabled",1,wide.data())){api->log(mod,"EL EDITOR DISABLED by configuration; explosion registry remains active");return 1;}}

 if(!api->hook||!api->content||!api->get_live||!api->set_live)return 0;
 HMODULE kernel=GetModuleHandleW(L"kernel32.dll");if(!kernel)return 0;
 /* Hook through HellForge, with trampolines; all other files/mods pass through. */
 if(api->hook(mod,(uintptr_t)GetProcAddress(kernel,"ReadFile"),(void*)Read,(void**)&originalRead)||
    api->hook(mod,(uintptr_t)GetProcAddress(kernel,"CloseHandle"),(void*)Close,(void**)&originalClose)||
    api->hook(mod,(uintptr_t)api->get_live,(void*)Get,(void**)&originalGet)||
    api->hook(mod,(uintptr_t)api->set_live,(void*)Set,(void**)&originalSet)||
    api->hook(mod,(uintptr_t)GetProcAddress(kernel,"GetFileSizeEx"),(void*)Size,(void**)&originalSize)){
  api->log(mod,"EL INPLACE DISABLED: a required hook was refused; inspect mod log");return 0;
 }
 api->log(mod,"EL INPLACE ARMED: default editor DLL unchanged; waiting for editor-catalog.json read. No interception log means editor initialized too early.");
 return 1;
}
