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

  qvdb_metadata(DbConnector * dbCon, std::vector<int> & vids) {
  dbCon->load_metadata_labels(labelMap, typeMap);
  dbCon->load_metadata_for_files(vids,fileMap);
  };

  ~qvdb_metadata(){};
  
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
    int type = a.first;
    std::string typeLabel = a.second;
    ss << typeLabel << ": ";
    for(auto &b: fileMap[vid]) {
      ss << labelMap[b].second << ", ";
    }
    ss << std::endl;
  }
  return ss.str();
  };
  
 private:

  std::map<int,std::pair<int,std::string> >  labelMap;
  boost::bimap<int, std::string> typeMap;
  std::map<int,std::vector<int > > fileMap;
};

#endif // QVBMETADATA_H
