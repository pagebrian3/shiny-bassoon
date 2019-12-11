#include <QVBMetadata.h>
#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
  std::filesystem::path savePath = getenv("HOME");
  savePath+="/.video_proj/";
  std::filesystem::path tempPath = "/tmp/qvdbtmp/";
  DbConnector *dbCon = new DbConnector(savePath,tempPath);
  qvdb_metadata * md = new qvdb_metadata(dbCon);
  std::string STRING;
  std::string label;
  std::filesystem::path filename;
  std::filesystem::path baseDir("/home/pageb/Downloads/blah/");
  
  std::string operation(argv[1]);
  
  if(strcmp(operation.c_str(),"import") == 0) {
    std::ifstream infile;
    infile.open(argv[2]);
    size_t spacePos;
    int currentType;
    std::string currentTypeLabel;
    while(!infile.eof()) {
      getline(infile,STRING);
      if(STRING.length() < 2) continue;      
      spacePos = STRING.find_first_of(' ');
      if(spacePos == std::string::npos) {
	if(md->md_types().right.count(STRING) == 0) md->newType(STRING);
	currentType=md->md_types().right.at(STRING);
	currentTypeLabel=STRING;
      }
      else {
	filename=baseDir;
	label = STRING.substr(0,spacePos);
	filename +=STRING.substr(spacePos+1);
	std::cout << filename.string() << std::endl;
	if(!dbCon->video_exists(filename)) continue;
	if(!md->labelExists(currentType,label)) md->newLabel(currentTypeLabel,label);
	md->attachToFile(dbCon->fileVid(filename),label);
      }
    }
    infile.close();
    md->saveMetadata();
    dbCon->save_db_file();
  }
  else if(strcmp(operation.c_str(),"export") == 0) {
    std::ofstream outfile;
    outfile.open(argv[2]);
    auto typeMap = md->types();
    auto labelMap = md->md_lookup();
    std::map<int,std::filesystem::path> vidMap;
    dbCon->load_fileVIDs(vidMap);
    std::vector<int> VIDs;
    for(auto & a : vidMap) VIDs.push_back(a.first);
    md->load_file_md(VIDs);
    for(auto & a: typeMap) {
      int typeID = a.first;
      std::string typeLabel(a.second);
      outfile << typeLabel << "\n";
      for(auto & tag: labelMap) {
	if(tag.second.first != typeID) continue;
	int tagID = tag.first;
	for(auto & vPath: vidMap) if(md->mdForFile(vPath.first).count(tagID) != 0) outfile << tag.second.second << " "<<vPath.second.native()<<"\n";
      }      
    }
    outfile.close();
  }
  return 0;
}
