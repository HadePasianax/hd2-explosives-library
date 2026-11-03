/* The stock editor reloads disk snapshots on reopen; replay this session's
 * sandbox journal into that read so saved removals do not resurrect. */
#pragma once
#include "json.h"
namespace editorlab {
static bool ReplayJournal(const std::string& saved,const std::string& journal,bool mods,std::string* output,unsigned* count){
 hfjson::Value document,changes;std::string why;
 if(!hfjson::Parse(saved,&document,&why)||document.kind!=hfjson::Value::Object||!hfjson::Parse(journal,&changes,&why)||changes.GetNumber("format")!=1)return false;
 const auto* list=changes.Find("changes",hfjson::Value::Array);if(!list)return false;*count=0;
 for(const auto& change:list->items){
  std::string category=change.GetString("category"),target=change.GetString("target"),key=change.GetString("key");
  if(category.empty()||target.empty()||key.empty()||(category.rfind("mod:",0)==0)!=mods)continue;
  if(change.GetBool("remove")){
   auto* cat=document.Find(category);if(!cat)continue;auto* item=cat->Find(target);if(!item)continue;
   item->Erase(key);if(item->members.empty())cat->Erase(target);if(cat->members.empty())document.Erase(category);(*count)++;
  }else if(const auto* value=change.Find("value",hfjson::Value::Number)){
   document.At(category).At(target).At(key)=*value;(*count)++;
  }
 }
 *output=hfjson::Write(document);return true;
}
}
