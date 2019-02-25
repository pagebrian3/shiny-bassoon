#ifndef QVBMETADATA_H
#define QVBMETADATA_H

#include <vector>
#include <map>
#include <string>
#include "DbConnector.h"
#include <boost/bimap.hpp>

class qvdb_metadata
{

public:

  qvdb_metadata(DbConnector * dbCon)  : db(dbCon) {
    db->load_metadata_labels(labelMap, typeMap);
  };

  ~qvdb_metadata(){};
  
  void load_file_md(std::vector<int> & vids) {
    db->load_metadata_for_files(vids,fileMap);
    return;
  };
  
  std::vector<int> mdForFile(int vid) {
    return fileMap[vid];
  };

  std::string labelForMDID(int id) {
    return labelMap[id].second;
  };

  auto types() {
    return typeMap.left;
  };

  int mdType(int id) {
    return labelMap[id].first;
  };

  std::string typeLabel(int id) {
    return typeMap.left.at(id);
  };

  boost::bimap<int,std::string> md_types() {
    return typeMap;
  };

  std::map<int,std::pair<int,std::string> > md_lookup() {
    return labelMap;
  };

  std::string metadata_string(int vid) {
    std::stringstream ss;
    for(auto & a:typeMap.left) {
      std::string typeLabel = a.second;
      int typeIndex = a.first;
      ss <<"\n"<< typeLabel << ": ";
      uint i = 0;
      for( ; i +1 < fileMap[vid].size(); i++)  if(labelMap[fileMap[vid][i]].first == typeIndex)  ss << labelMap[fileMap[vid][i]].second << ", ";
      if(fileMap[vid].size() > 0 && labelMap[fileMap[vid][i]].first == typeIndex)  ss << labelMap[fileMap[vid][i]].second ;
      ss << std::endl;
    }
    return ss.str();
  };

  void newType(std::string label) {
    int maxID = 0;
    for(auto & a:typeMap.left) if( a.first > maxID) maxID=a.first;
    maxID++;
    typeMap.insert({maxID,label});
  };

  void newLabel(std::string type,std::string label) {
    int maxID = 0;
    for(auto & a:labelMap) if( a.first > maxID) maxID=a.first;
    maxID++;
    int tID = typeMap.right.at(type);
    labelMap[maxID]=std::make_pair(tID,label);
    return;
  };

  void attachToFile(int vid,std::string label) {
    int labelID = -1;
    for(auto &a: labelMap) {
      if(!strcmp(label.c_str(),a.second.second.c_str())) {
	labelID = a.first;
	break;
      }
    }
    if(labelID != -1) fileMap[vid].push_back(labelID);
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
    auto iter = std::find(fileMap[vid].begin(), fileMap[vid].end(), labelID);
    std::cout << "Erasing from " << vid << " tagid " << *iter << " " << " size before " << fileMap[vid].size() <<std::endl;
    if(iter != fileMap[vid].end()) fileMap[vid].erase(iter);
    std::cout<< "size after " <<  fileMap[vid].size() <<std::endl;
    return;
  };

  void saveMetadata() {
    db->save_metadata(fileMap,labelMap,typeMap);
    return;
  };
  
private:

  std::map<int,std::pair<int,std::string>> labelMap;
  boost::bimap<int, std::string> typeMap;
  std::map<int,std::vector<int >> fileMap;
  DbConnector * db;
};

#endif // QVBMETADATA_H
