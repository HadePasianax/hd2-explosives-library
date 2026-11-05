/* HellForge 0.4 catalogue adapter. Assemble templates in memory, then append
 * custom objects to one listed part; the index keeps its original file list.
 * No editor files are created or modified on disk. */
#pragma once
#include "catalog_injection.h"
namespace editorlab {
struct SplitExpanded {
 std::string partName,partText;
};
static bool SafePart(const std::string& name){
 if(name.empty()||name.front()=='/'||name.front()=='\\'||name.find(':')!=std::string::npos)return false;
 std::string segment;
 for(size_t i=0;i<=name.size();i++){
  char c=i==name.size()?'/':name[i];
  if(c=='/'||c=='\\'){if(segment.empty()||segment=="."||segment=="..")return false;segment.clear();}
  else segment+=c;
 }
 return name.size()>=5&&name.substr(name.size()-5)==".json";
}
template<class Reader>
static bool ExpandSplit(const std::string& text,const std::vector<Record>& records,Reader readPart,
 std::string* output,SplitExpanded* split,Expanded* info,std::string* why){
 Value index;if(!hfjson::Parse(text,&index,why))return false;
 const Value* files=index.Find("files",Value::Array);
 if(!files){*why="split catalogue files missing";return false;}
 if(index.GetNumber("format")!=2||index.GetString("build")!="25480438"){
  *why="split catalogue format/build not reviewed";return false;
 }
 Value combined=index;combined.Erase("files");combined.At("format")=Value::MakeNumber(1,true);
 combined.At("objects").kind=Value::Array;
 std::string targetText,targetName;
 std::set<std::string> visited;
 for(const Value& file:files->items){
  if(file.kind!=Value::String||!SafePart(file.Str())||!visited.insert(file.Str()).second){*why="invalid or duplicate catalogue part";return false;}
  std::string partText;if(!readPart(file.Str(),&partText)){*why="cannot read catalogue part: "+file.Str();return false;}
  Value part;if(!hfjson::Parse(partText,&part,why))return false;
  if(part.GetNumber("format")!=2||part.GetString("build")!=index.GetString("build")||part.GetString("schema")!=index.GetString("schema")){
   *why="catalogue part build/schema mismatch: "+file.Str();return false;
  }
  const Value* objects=part.Find("objects",Value::Array);
  if(!objects){*why="catalogue part objects missing: "+file.Str();return false;}
  for(const Value& object:objects->items){
   const std::string category=object.GetString("category");
   if(category!="projectiles"&&category!="damage"&&category!="explosions")continue;
   combined.At("objects").items.push_back(object);
   if(category=="projectiles"&&targetName.empty()){targetName=file.Str();targetText=partText;}
  }
 }
 if(targetName.empty()){*why="projectile catalogue part missing";return false;}
 const size_t templates=combined.At("objects").items.size();
 std::string expanded;
 if(!Expand(hfjson::Write(combined),records,&expanded,info,why))return false;
 Value augmented;if(!hfjson::Parse(expanded,&augmented,why))return false;
 const auto* objects=augmented.Find("objects",Value::Array);
 if(!objects||objects->items.size()<templates){*why="expanded objects missing";return false;}
 size_t close=ObjectsClose(targetText);
 if(close==std::string::npos){*why="target catalogue objects span missing";return false;}
 Value target;if(!hfjson::Parse(targetText,&target,why))return false;
 const bool hasObjects=!target.Find("objects",Value::Array)->items.empty();
 std::string additions;
 for(size_t i=templates;i<objects->items.size();i++){
  if(hasObjects||!additions.empty())additions+=",";
  additions+=hfjson::Write(objects->items[i]);
 }
 targetText.insert(close,additions);
 const Value* refs=augmented.Find("references");
 if(!refs||!refs->Find("explosion",Value::Array)){*why="expanded explosion references missing";return false;}
 index.At("references").At("explosion")=*refs->Find("explosion",Value::Array);
 *output=hfjson::Write(index);
 if(output->size()>=(64u<<20)||targetText.size()>=(64u<<20)){*why="expanded catalogue part exceeds 64 MiB";return false;}
 split->partName=targetName;split->partText=std::move(targetText);
 return true;
}
}
