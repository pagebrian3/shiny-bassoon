#ifndef QVBMETADATA_H
#define QVBMETADATA_H

#include <vector>
#include <map>
#include <string>
#include <boost/bimap.hpp>

class qvdb_metadata
{

public:

  qvdb_metadata(){};

  ~qvdb_metadata(){};
  
  std::vector<int> mdForFile(int vid) {
    return fileMap[vid];
  };

  std::string labelForMDID(int id) {
    return labelMap[id].second;
  };

  std::map<int,std::string> types() {
    return typeMap.left();
  };

  int mdType(int id) {
    return labelMap[id].first;
  };
  
  void load_metadata(std::map<int,std::vector<int> > & fileMD, std::map<int,std::pair<int,std::string> > & labelLookup, boost::bimap<int,std::string> &typeLookup ) {
    labelMap = labelLookup;
    typeMap = typeLookup;
    fileMap = fileMD;
    return;
  };
  
 private:

  std::map<int,std::pair<int,std::string> >  labelMap;
  boost::bimap<int, std::string> typeMap;
  std::map<int,std::vector<int > > fileMap;
};

#endif // QVBMETADATA_H
