#ifndef DBCONNECTOR_H
#define DBCONNECTOR_H

#include <map>
#include <boost/variant.hpp>
#include <boost/bimap.hpp>
#include <filesystem>

class VidFile;
class sqlite3;

class DbConnector {

 public:

  DbConnector(std::filesystem::path & path, std::filesystem::path & tempPath);

  bool video_exists(std::filesystem::path & filename);

  VidFile* fetch_video(std::filesystem::path & filename);

  void save_video(VidFile *a);

  void save_crop(VidFile *a);

  void fetch_results(std::map<std::tuple<int,int,int>, std::pair<int,std::string> > & map);

  void update_results(int i, int j, int k, int l, std::string details);

  void save_db_file();

  std::vector<int> cleanup(std::filesystem::path & dir, std::vector<std::filesystem::path> & files);

  std::vector<std::pair<std::string,boost::variant<int,float,std::string> > > fetch_config();

  void save_config(std::map<std::string,boost::variant<int,float,std::string> >  config);

  void load_metadata_labels(std::map<int,std::pair<int,std::string> > & labelLookup, boost::bimap<int, std::string> & typeLookup) ;

  void load_metadata_for_files(std::vector<int> & vids,std::map<int,std::set<int > > & file_metdata); 

  void save_metadata(std::map<int,std::set<int >> & file_metdata,std::map<int,std::pair<int,std::string>> & labelLookup, boost::bimap<int,std::string> & typeLookup);

  bool video_has_md(int vid);

  int fileVid(std::filesystem::path & fileName);
  
 private:

  bool newDB;

  sqlite3 * db;

  std::filesystem::path db_path;

  std::filesystem::path db_tmp;

};

#endif // DBCONNECTOR_H
