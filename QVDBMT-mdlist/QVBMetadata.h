#ifndef QVBMETADATA_H
#define QVBMETADATA_H

#include <DbConnector.h>
#include <iostream>

class qvdb_metadata
{

public:

  qvdb_metadata(DbConnector * dbCon)  : db(dbCon) {
    db->load_metadata_labels(labelMap, typeMap);
    for(auto & a: labelMap) labelLookup[a.second.first].insert(a.second.second);
  };

  ~qvdb_metadata(){};
  
  void load_file_md(std::vector<int> & vids) {
    db->load_metadata_for_files(vids,fileMap);
    return;
  };
  
  std::set<int> mdForFile(int vid) {
    return fileMap[vid];
  };

  bool labelExists(int type, std::string label) {
    if(labelLookup[type].count(label) == 1) return true;
    else return false;
  };

  bool typeExists(std::string type) {
    if(typeMap.right.count(type) == 1) return true;
    else return false;
  }
    
  std::string labelForMDID(int id) {
    return labelMap[id].second;
  };

  auto types() {
    return typeMap.left;
  };

  int mdType(int id) {
    return labelMap[id].first;
  };

  int typeCount(int tID) {
    int count = 0;
    for(auto & a:labelMap) if(a.second.first == tID) count++;
    return count;
  };
      
  std::string typeLabel(int id) {
    return typeMap.left.at(id);
  };

  boost::bimap<int,std::string> md_types() {
    return typeMap;
  };

  int mdType(std::string tLabel) {
    return typeMap.right.at(tLabel);
  };

  std::map<int,std::pair<int,std::string> > md_lookup() {
    return labelMap;
  };

  std::map< int, std::map<int,int> > typeCountMap() {
    std::map< int, std::map<int,int> > countMap;
    for(auto & a: fileMap) {  //loop over files
      std::map<int,int> typeCounter;
      for(auto & b: a.second) {  //loop over labels in set
	countMap[labelMap[b].first][b]+=1;
	typeCounter[labelMap[b].first]+=1;
      }
      for(unsigned int i=1; i < typeMap.left.size()+1; i++)
	if(typeCounter[i]==0) countMap[i][-1]+=1;
    }
    return countMap;
  };

  std::string metadata_string(int vid) {
    std::stringstream ss;
    for(auto & a:typeMap.left) {
      std::string typeLabel = a.second;
      int typeIndex = a.first;
      ss <<"\n"<< typeLabel << ": ";
      std::vector<std::string> tags;
      for(auto & tagid: fileMap[vid])  if(labelMap[tagid].first == typeIndex) tags.push_back(labelMap[tagid].second);
      for(unsigned int i=0 ; i +1 < tags.size(); i++)  ss << tags[i] << ", ";
      if(tags.size() > 0)  ss << tags.back() ;
    }
    return ss.str();
  };

  int newType(std::string label) {
    int maxID = 0;
    for(auto & a:typeMap.left) if( a.first > maxID) maxID=a.first;
    maxID++;
    typeMap.insert({maxID,label});
    return maxID;
  };

  int newLabel(std::string type,std::string label) {
    int maxID = 0;
    for(auto & a:labelMap) if( a.first > maxID) maxID=a.first;
    maxID++;
    int tID = 0;
    auto s = typeMap.right;
    if(s.find(type) == s.end()) tID = newType(type);
    else tID = typeMap.right.at(type);
    labelMap[maxID]=std::make_pair(tID,label);
    labelLookup[tID].insert(label);
    return tID;
  };

  void attachToFile(int vid,std::string label) {
    int labelID = -1;
    for(auto &a: labelMap) {  
      if(!strcmp(label.c_str(),a.second.second.c_str())) {
	labelID = a.first;
	break;
      }
    }
    if(labelID != -1) fileMap[vid].insert(labelID);
    return;
  };

  void removeFromFile(int vid, std::string label) {
    int labelID = -1;
    for(auto &a: labelMap) {
      if(!strcmp(label.c_str(),a.second.second.c_str())) {
	labelID = a.first;
	break;
      }
    }
    if(labelID != -1) fileMap[vid].erase(labelID);
    return;
  };

  void saveMetadata() {
    db->save_metadata(fileMap,labelMap,typeMap);
    return;
  };
  
private:
  std::map<int,std::pair<int,std::string>> labelMap; //<labelid,<typeid,label_string>
  boost::bimap<int, std::string> typeMap; //<typeid,type_string>
  std::map<int,std::set<int >> fileMap;  //<vid,set(labelid)>
  std::map<int,std::set<std::string>> labelLookup;  //<labelid,label_string>
  DbConnector * db;
};

#endif // QVBMETADATA_H
