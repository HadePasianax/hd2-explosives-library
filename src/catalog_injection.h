/* Catalogue augmentation for the unchanged editor. No on-disk editor writes. */
#pragma once
#include <algorithm>
#include <stdint.h>
#include <set>
#include <string>
#include <vector>
#include "json.h"
#include "hellforge_mod.h"
namespace editorlab {
using hfjson::Value;
struct Record {uint32_t kind,id,base;std::string name,mod;};
struct Proxy {uint32_t ref,projectile;std::string name;};
struct Expanded {unsigned projectiles=0,damage=0,explosions=0;std::vector<Proxy> proxies;};
static std::string DisplayName(std::string name){
 size_t at;while((at=name.find("/library/"))!=std::string::npos)name.erase(at,8);
 return name;
}
/* Retain the original document verbatim. Pretty-printing the whole 50 MB
 * catalogue can exceed the stock editor's 64 MiB read limit. */
static size_t ObjectsClose(const std::string& text){
 unsigned depth=0;bool quoted=false,escape=false;size_t begin=0;
 for(size_t i=0;i<text.size();i++){
  char c=text[i];
  if(quoted){
   if(escape){escape=false;continue;}if(c=='\\'){escape=true;continue;}
   if(c=='"'){
    quoted=false;
    if(depth==1&&text.substr(begin,i-begin)=="objects"){
     size_t j=i+1;while(j<text.size()&&isspace((unsigned char)text[j]))j++;
     if(j>=text.size()||text[j++]!=':')continue;
     while(j<text.size()&&isspace((unsigned char)text[j]))j++;
     if(j>=text.size()||text[j]!='[')continue;
     unsigned arrays=1;bool q=false,e=false;
     for(j++;j<text.size();j++){
      char k=text[j];if(q){if(e)e=false;else if(k=='\\')e=true;else if(k=='"')q=false;continue;}
      if(k=='"')q=true;else if(k=='[')arrays++;else if(k==']'&&!--arrays)return j;
     }
     return std::string::npos;
    }
   }
   continue;
  }
  if(c=='"'){quoted=true;begin=i+1;}else if(c=='{'||c=='[')depth++;else if(c=='}'||c==']')depth--;
 }
 return std::string::npos;
}
static Value* Array(Value& v,const char* name){Value* p=v.Find(name);return p&&p->kind==Value::Array?p:nullptr;}
static uint64_t Ref(const Value& row){std::string s=row.GetString("ref");return strtoull(s.c_str(),nullptr,s.rfind("0x",0)==0?16:10);}
static const Value* Template(const Value& objects,const char* category,const char* component,uint32_t base){
 const Value* fallback=nullptr;
 for(const Value& o:objects.items){
  if(o.GetString("category")!=category)continue;
  const Value* page=o.Find("page",Value::Array);if(!page)continue;
  for(const Value& section:page->items){const Value* subs=section.Find("subs",Value::Array);if(!subs)continue;
   for(const Value& part:subs->items){const Value* rows=part.Find("rows",Value::Array);if(!rows)continue;
    for(const Value& row:rows->items)if(row.GetString("component")==component){if(!fallback)fallback=&o;if(Ref(row)==base)return &o;}
   }
  }
 }
 return fallback;
}
static Value CopyRecord(const Value& source,const Record& rec,const char* component,const char* category,uint32_t ref,bool proxy){
 Value o=source;std::string stable=rec.mod+":"+rec.name+(proxy?"/impact-explosion":"");
 o.At("id")=Value::MakeString("editor-lab/"+stable);
 o.At("name")=Value::MakeString(DisplayName(rec.name));
 o.At("category")=Value::MakeString(category);o.At("subtitle")=Value::MakeString("Custom: "+rec.mod);
 o.At("search")=Value::MakeString(rec.name+" "+rec.mod+" custom "+std::to_string(rec.id));
 if(auto page=Array(o,"page"))for(Value& section:page->items){
  section.At("key")=Value::MakeString("editor-lab/"+stable+"|"+section.GetString("key"));
  if(auto subs=Array(section,"subs"))for(Value& part:subs->items){
   part.At("key")=Value::MakeString("editor-lab/"+stable+"|"+part.GetString("key"));
   if(auto rows=Array(part,"rows")){
    std::vector<Value> kept;std::set<unsigned> offsets;
    for(Value row:rows->items){
     unsigned offset=(unsigned)row.GetNumber("offset");
     if(row.GetString("component")!=component||offset==0||row.Find("stat",Value::Array)||!offsets.insert(offset).second)continue;
     if(proxy&&(row.GetString("type")!="f32"||(offset!=16&&offset!=20&&offset!=24)))continue;
     row.At("cat")=Value::MakeString("mod:"+rec.mod);
     row.At("target")=Value::MakeString(rec.name+(proxy?"/impact-explosion":""));
     row.At("key")=Value::MakeString(std::string(component)+"."+std::to_string(offset));
     row.At("ref")=Value::MakeString(std::to_string(ref));
     row.At("link")=Value::MakeString(std::string(component)+"::"+std::to_string(ref)+":"+std::to_string(offset));
     row.At("shared")=Value::MakeBool(true);
     row.At("note")=Value::MakeString(proxy?"Resolves this custom projectile's current EL impact explosion when read; radii edits are queued to EL's game thread.":"Actual custom record; edits use the default editor's normal live-edit API.");
     for(const char* key:{"also","alsoIf","alsoAmmo","hd2r","hd2rKind","hd2rAs","pick","fires","slots","slot","choice","options","values","exclude","mountSlot"})row.Erase(key);
     if(!strcmp(component,"ProjectileSettings")&&(offset==offsetof(HfProjectileSettings,impact_explosion)||offset==offsetof(HfProjectileSettings,expiry_explosion)))row.At("pick")=Value::MakeString("explosion");
     kept.push_back(std::move(row));
    }
    rows->items=std::move(kept);
   }
  }
 }
 return o;
}
static bool Expand(const std::string& text,const std::vector<Record>& records,std::string* output,Expanded* info,std::string* why){
 Value root;if(!hfjson::Parse(text,&root,why))return false;
 if(root.GetNumber("format")!=1||root.GetString("build")!="25480438"){*why="catalogue format/build not reviewed";return false;}
 Value* objects=Array(root,"objects");if(!objects){*why="objects missing";return false;}
 std::vector<Value> additions;std::set<std::string> names;
 Value& payloads=root.At("references").At("explosion");
 /* Splice the reference array separately, retaining the original document. */
 std::vector<Value> payloadNames;
 for(const Record& r:records){
  if(!r.id||r.name.empty()||r.mod.empty()||!names.insert(r.mod+":"+r.name).second)continue;
  bool projectile=r.kind==HF_KIND_PROJECTILE;if(!projectile&&r.kind!=HF_KIND_DAMAGE)continue;
  const char* cat=projectile?"projectiles":"damage";const char* component=projectile?"ProjectileSettings":"DamageInfo";
  if(const Value* source=Template(*objects,cat,component,r.base)){
   additions.push_back(CopyRecord(*source,r,component,cat,r.id,false));
   if(projectile)info->projectiles++;else info->damage++;
  }
  if(projectile&&r.id<0x01000000){
   for(unsigned suffix=0;suffix<2;suffix++){
    Value label;label.kind=Value::Object;label.At("value")=Value::MakeNumber((suffix?0xE3000000u:0xE1000000u)|r.id,true);
    label.At("label")=Value::MakeString(DisplayName(r.name)+(suffix?"/expiry":""));label.At("group")=Value::MakeString("Custom payloads");payloadNames.push_back(std::move(label));
   }
   if(const Value* source=Template(*objects,"explosions","ExplosionSettings",407)){
    uint32_t proxy=0xE1000000u|r.id;
    Value explosion=CopyRecord(*source,r,"ExplosionSettings","explosions",proxy,true);
    if(const Value* damage=Template(*objects,"damage","DamageInfo",56)){
     Value view=CopyRecord(*damage,r,"DamageInfo","explosions",0xE2000000u|r.id,false);
     if(auto page=Array(view,"page"))for(Value& section:page->items){
      section.At("title")=Value::MakeString("Explosion damage");
      if(auto subs=Array(section,"subs"))for(Value& part:subs->items)if(auto rows=Array(part,"rows"))for(Value& row:rows->items){
       row.At("target")=Value::MakeString(r.name+"/impact-explosion");
       row.At("note")=Value::MakeString("Resolves the current impact explosion's damage record. Shared explosions/damage affect every projectile using that record.");
      }
      Array(explosion,"page")->items.push_back(std::move(section));
     }
    }
    additions.push_back(std::move(explosion));
    info->proxies.push_back({proxy,r.id,r.name});info->explosions++;
   }
  }
 }
 size_t close=ObjectsClose(text);if(close==std::string::npos){*why="objects array span not found";return false;}
 std::string extra;
 for(const Value& value:additions){if(!extra.empty()||!objects->items.empty())extra+=",";extra+=hfjson::Write(value);}
 *output=text;output->insert(close,extra);
 if(!payloadNames.empty()){
  /* References is already parsed and validated by the stock catalogue schema.
   * Serialize only its small explosion list; never pretty-print the full file. */
  if(payloads.kind!=Value::Array){*why="explosion references missing";return false;}
  for(Value& label:payloadNames)payloads.items.push_back(std::move(label));
  const std::string marker="\"explosion\"";size_t refs=output->find("\"references\"");size_t key=output->find(marker,refs);
  if(refs==std::string::npos||key==std::string::npos){*why="explosion reference span missing";return false;}
  size_t first=output->find('[',key+marker.size()),last=first;unsigned depth=0;bool q=false,e=false;
  for(;last<output->size();last++){char c=(*output)[last];if(q){if(e)e=false;else if(c=='\\')e=true;else if(c=='"')q=false;continue;}if(c=='"')q=true;else if(c=='[')depth++;else if(c==']'&&!--depth)break;}
  if(first==std::string::npos||last>=output->size()){*why="invalid reference span";return false;}
  output->replace(first,last-first+1,hfjson::Write(payloads));
 }
 if(output->size()>=(64u<<20)){*why="expanded catalogue exceeds the editor's 64 MiB limit";return false;}
 return true;
}
}
