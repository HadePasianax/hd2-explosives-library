#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include "../src/inplace_editor.cpp"
static HfApi testApi{};
static int ReadMemory(uintptr_t at,void* out,size_t n){memcpy(out,(void*)at,n);return 0;}
static std::map<std::pair<uint64_t,unsigned>,double> fixtureValues;
static bool driftBaseline=false,loseReset=false;
static unsigned originals,writes;static uint64_t writtenId;static float writtenValue;
static const char* ModId(HfMod* mod){return mod==(HfMod*)1?"hellforge.editor":"other.mod";}
static void Log(HfMod*,const char* fmt,...){va_list args;va_start(args,fmt);vprintf(fmt,args);puts("");fflush(stdout);va_end(args);}
static int Content(HfMod*,uint32_t index,HfContent* c){
 if(index>1)return 0;
 c->kind=index?HF_KIND_DAMAGE:HF_KIND_PROJECTILE;c->id=index?700:391;c->base=index?56:142;
 c->name=index?"fixture/private_damage":"fixture/private_shot";c->mod="fixture.mod";return 1;
}
static int NativeGet(HfMod*,HfRef id,const HfField* f,HfLiveValue* out){
 originals++;*out={};auto found=fixtureValues.find({id,f->offset});out->now=HF_F32(id==391&&f->offset==offsetof(HfProjectileSettings,impact_explosion)?450:found==fixtureValues.end()?123:found->second);out->base=HF_F32(123);if(driftBaseline)out->base=out->now;return 1;
}
static int NativeSet(HfMod*,HfRef id,const HfField* field,const HfValue* value,uint32_t){if(value)fixtureValues[{id,field->offset}]=value->number;else if(!loseReset)fixtureValues.erase({id,field->offset});writes++;writtenId=id;writtenValue=value?(float)value->number:0;return 1;}
static int ElRecordFake(uint32_t index,ElEditorRecord* record){if(index)return 0;record->id=450;record->damage=700;return 1;}
static int ElReadFake(uint32_t id,uint32_t offset,float* now,float* base){if(id!=450||offset!=20)return 0;*now=8;*base=4;return 1;}
static int ElSetFake(uint32_t id,uint32_t offset,const float* value){writtenId=id;writtenValue=value?*value:4;return id==450&&offset==20;}
extern "C" const ElEditorApi* explosives_library_editor_api(uint32_t abi){static const ElEditorApi api={sizeof api,1,ElRecordFake,ElReadFake,ElSetFake};return abi==1?&api:nullptr;}
int main(){
 testApi.read=ReadMemory;testApi.mod_id=ModId;testApi.log=Log;testApi.content=Content;host=&testApi;owner=(HfMod*)2;
 originalSize=GetFileSizeEx;originalRead=ReadFile;originalClose=CloseHandle;originalGet=NativeGet;originalSet=NativeSet;
 const wchar_t* path=L"tests/fixtures/editor-catalog.json";
 std::string before;assert(hfjson::ReadFile(path,&before));
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(file!=INVALID_HANDLE_VALUE);
 LARGE_INTEGER size;assert(Size(file,&size)&&!streams.empty()&&size.QuadPart>0);
 std::string injected((size_t)size.QuadPart,'\0');DWORD got=0;
 assert(Read(file,&injected[0],123,&got,nullptr)&&got==123);
 assert(Read(file,&injected[123],(DWORD)injected.size()-123,&got,nullptr)&&got==injected.size()-123);
 assert(Close(file)&&streams.empty());
 hfjson::Value document;std::string why;assert(hfjson::Parse(injected,&document,&why));
 unsigned added=0;for(const auto& object:document.Find("objects")->items)if(object.GetString("id").rfind("editor-lab/",0)==0){
  added++;assert(object.GetString("category")=="projectiles"||object.GetString("category")=="damage"||object.GetString("category")=="explosions");
  for(const auto& section:object.Find("page")->items)for(const auto& sub:section.Find("subs")->items)for(const auto& row:sub.Find("rows")->items){
   assert(row.GetString("cat")=="mod:fixture.mod");assert(!row.Find("hd2r"));assert(row.GetNumber("offset")!=0);
  }
 }
 assert(added==3&&proxies.size()==1);
 assert(editorlab::DisplayName("hade/plasma_stratagems/library/mortar_max_shell")=="hade/plasma_stratagems/mortar_max_shell");
 unsigned damageFields=0;
 const auto* refs=document.Find("references")->Find("explosion");bool named=false;
 for(const auto& item:refs->items)if(item.GetNumber("value")==double(0xE1000000u|391)){named=true;assert(item.GetString("label")=="fixture/private_shot");}assert(named);
 HfField payload{"ProjectileSettings",25480438,(uint16_t)offsetof(HfProjectileSettings,impact_explosion),4,HF_TYPE_U32,0};HfLiveValue shown{};
 assert(Get((HfMod*)1,391,&payload,&shown)&&shown.now.number==double(0xE1000000u|391));
 HfValue namedPayload=HF_U32(0xE1000000u|391);assert(Set((HfMod*)1,391,&payload,&namedPayload,0)&&writtenId==391&&writtenValue==450);writes=0;

 for(const auto& object:document.Find("objects")->items)if(object.GetString("id").rfind("editor-lab/",0)==0){
  assert(object.GetString("name").find("Impact explosion for")==std::string::npos);
  if(object.GetString("category")=="explosions")for(const auto& section:object.Find("page")->items)for(const auto& part:section.Find("subs")->items)for(const auto& row:part.Find("rows")->items)if(row.GetString("component")=="DamageInfo"){damageFields++;assert(editorlab::Ref(row)==(0xE2000000u|391));}
 }
 assert(damageFields>=8);

 HfField radius{"ExplosionSettings",25480438,20,4,HF_TYPE_F32,0};HfLiveValue value{};
 assert(Get((HfMod*)1,proxies[0].ref,&radius,&value)&&value.now.number==8&&value.base.number==4);
 HfValue v=HF_F32(9);assert(Set((HfMod*)1,proxies[0].ref,&radius,&v,0)&&writtenId==450&&writtenValue==9&&!writes);
 assert(Set((HfMod*)1,proxies[0].ref,&radius,nullptr,0)&&writtenValue==4);
 HfField damage{"DamageInfo",25480438,4,4,HF_TYPE_I32,0};v=HF_I32(777);
 assert(Set((HfMod*)1,700,&damage,&v,0)&&writes==1&&writtenId==700&&writtenValue==777);
 assert(Set((HfMod*)2,700,&damage,&v,0)&&writes==2);
 uint32_t damageView=0xE2000000u|391;
 assert(Get((HfMod*)1,damageView,&damage,&value)&&value.now.number==777);
 assert(Set((HfMod*)1,damageView,&damage,&v,0)&&writes==3&&writtenId==700&&writtenValue==777);
 driftBaseline=true;loseReset=true;
 assert(Get((HfMod*)1,damageView,&damage,&value)&&value.now.number==777&&value.base.number==123);
 assert(Get((HfMod*)1,700,&damage,&value)&&value.base.number==123);
 assert(Set((HfMod*)1,damageView,&damage,nullptr,0)&&writes==5&&writtenId==700&&writtenValue==123);
 assert(Get((HfMod*)1,damageView,&damage,&value)&&value.now.number==123);

 std::string after;assert(hfjson::ReadFile(path,&after)&&before==after);
 std::string other;assert(hfjson::ReadFile(L"hellforge-mod.json",&other));
 assert(!other.empty());
 const std::string edge="{\"format\":1,\"build\":\"25480438\",\"note\":\"objects [ ] \\\"\",\"objects\":[]}";
 assert(editorlab::ObjectsClose(edge)==edge.rfind(']'));
 std::string expanded;editorlab::Expanded info;assert(!editorlab::Expand("{\"format\":2}",Records(),&expanded,&info,&why));
 std::string merged;unsigned replayed=0;
 const std::string stale=R"({"format":1,"weapons":{"test":{"damage":800}},"mod:test":{"shot":{"damage":400}}})";
 const std::string journal=R"({"format":1,"changes":[{"category":"weapons","target":"test","key":"damage","remove":true},{"category":"mod:test","target":"shot","key":"damage","remove":true}]})";
 assert(editorlab::ReplayJournal(stale,journal,false,&merged,&replayed)&&replayed==1);hfjson::Value restored;assert(hfjson::Parse(merged,&restored,&why)&&!restored.Find("weapons")&&restored.Find("mod:test"));
 assert(editorlab::ReplayJournal(stale,journal,true,&merged,&replayed)&&replayed==1);assert(hfjson::Parse(merged,&restored,&why)&&!restored.Find("mod:test")&&restored.Find("weapons"));
 assert(editorlab::ReplayJournal(merged,journal,true,&other,&replayed)&&replayed==0);
 puts("PASS sandbox reopen regression: reset removals survive stale saved-file reload; ordinary and custom stats remain isolated.");

 // SDK 16 / catalogue format 2: native loader reads only listed category parts.
 const std::wstring splitFolder=L"build/split-editor-test";CreateDirectoryW(splitFolder.c_str(),nullptr);
 hfjson::Value index;assert(hfjson::Parse(before,&index,&why));
 const auto sourceObjects=index.Find("objects")->items;
 index.Erase("objects");index.At("format")=hfjson::Value::MakeNumber(2,true);
 index.At("files").kind=hfjson::Value::Array;
 std::map<std::string,std::string> parts;
 for(const std::string category:{"projectiles","damage","explosions"}){
  std::string name=category+".json";index.At("files").items.push_back(hfjson::Value::MakeString(name));
  hfjson::Value part;part.kind=hfjson::Value::Object;part.At("format")=hfjson::Value::MakeNumber(2,true);
  part.At("build")=index.At("build");part.At("schema")=index.At("schema");part.At("objects").kind=hfjson::Value::Array;
  for(const auto& object:sourceObjects)if(object.GetString("category")==category)part.At("objects").items.push_back(object);
  parts[name]=hfjson::Write(part);assert(hfjson::WriteAtomically(splitFolder+L"/"+hfjson::Wide(name),parts[name]));
 }
 const std::string rawIndex=hfjson::Write(index);assert(hfjson::WriteAtomically(splitFolder+L"/editor-catalog.json",rawIndex));
 auto virtualRead=[&](const std::wstring& name){
  HANDLE f=CreateFileW((splitFolder+L"/"+name).c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(f!=INVALID_HANDLE_VALUE);
  LARGE_INTEGER n;assert(Size(f,&n));std::string result((size_t)n.QuadPart,'\0');DWORD got=0;
  const DWORD first=(DWORD)result.size()/2;assert(Read(f,&result[0],first,&got,nullptr)&&got==first);
  assert(Read(f,&result[first],(DWORD)result.size()-first,&got,nullptr)&&got==result.size()-first);assert(Close(f));return result;
 };
 std::string newIndex=virtualRead(L"editor-catalog.json");hfjson::Value parsedIndex;assert(hfjson::Parse(newIndex,&parsedIndex,&why));
 assert(parsedIndex.GetNumber("format")==2&&!parsedIndex.Find("objects")&&parsedIndex.Find("files")->items.size()==3);
 unsigned custom=0;
 for(const auto& f:parsedIndex.Find("files")->items){
  const std::string part=virtualRead(hfjson::Wide(f.Str()));hfjson::Value parsed;assert(hfjson::Parse(part,&parsed,&why)&&parsed.GetNumber("format")==2);
  for(const auto& o:parsed.Find("objects")->items)if(o.GetString("id").rfind("editor-lab/",0)==0)custom++;
  std::string disk;assert(hfjson::ReadFile(splitFolder+L"/"+hfjson::Wide(f.Str()),&disk)&&disk==parts[f.Str()]);
 }
 assert(custom==3&&proxies.size()==1);bool label=false;
 for(const auto& ref:parsedIndex.Find("references")->Find("explosion")->items)if(ref.GetString("label")=="fixture/private_shot")label=true;
 assert(label);std::string diskIndex;assert(hfjson::ReadFile(splitFolder+L"/editor-catalog.json",&diskIndex)&&diskIndex==rawIndex);
 editorlab::SplitExpanded splitOutput;editorlab::Expanded splitInfo;std::string ignored;
 auto read=[&](const std::string& name,std::string* value){auto i=parts.find(name);if(i==parts.end())return false;*value=i->second;return true;};
 hfjson::Value invalid=index;invalid.At("files").items[0]=hfjson::Value::MakeString("../outside.json");
 assert(!editorlab::ExpandSplit(hfjson::Write(invalid),Records(),read,&ignored,&splitOutput,&splitInfo,&why));
 auto bad=parts;hfjson::Value mismatch;assert(hfjson::Parse(parts["damage.json"],&mismatch,&why));mismatch.At("schema")=hfjson::Value::MakeString("wrong");parts["damage.json"]=hfjson::Write(mismatch);
 assert(!editorlab::ExpandSplit(rawIndex,Records(),read,&ignored,&splitOutput,&splitInfo,&why));parts=bad;
 puts("PASS SDK 16 split catalogue: index references and custom projectile/damage/blast pages injected through listed parts, partial reads, native files unchanged, traversal/schema mismatch rejected.");
 puts("PASS default-editor lab: real catalogue augmented in memory, partial reads, handle cleanup, custom ID edit routing, late explosion resolution/reset, other-mod pass-through and byte-identical catalogue on disk.");
}
