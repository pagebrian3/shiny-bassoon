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
  std::ifstream infile;
  infile.open(argv[1]);
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
  return 0;
}
