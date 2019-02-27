#ifndef DBCONNECTOR_H
#define DBCONNECTOR_H

#include <sqlite3.h>
#include <map>
#include <VidFile.h>
#include <boost/variant.hpp>
#include <boost/bimap.hpp>

class DbConnector {

 public:

  DbConnector(bfs::path & path);

  std::string createPath(bfs::path & path, int vid, std::string extension);

  bool video_exists(bfs::path & filename);

  bool icon_exists(int vid);

  VidFile* fetch_video(bfs::path & filename);

  void save_video(VidFile *a);

  std::string fetch_icon(int vid);

  void save_icon(int vid);

  void save_crop(VidFile *a);

  void fetch_results(std::map<std::pair<int,int>, int> & map);

  void update_results(int i, int j, int k);

  void save_db_file();

  void cleanup(bfs::path & dir, std::vector<bfs::path> & files);

  std::vector<std::pair<std::string,boost::variant<int,float,std::string> > >fetch_config();

  void save_config(std::map<std::string,boost::variant<int,float,std::string> >  config);

  void load_metadata_labels(std::map<int,std::pair<int,std::string> > & labelLookup, boost::bimap<int, std::string> & typeLookup) ;

  void load_metadata_for_files(std::vector<int> & vids,std::map<int,std::vector<int > > & file_metdata); 

  void save_metadata(std::map<int,std::vector<int >> & file_metdata,std::map<int,std::pair<int,std::string>> & labelLookup, boost::bimap<int,std::string> & typeLookup);

  bool video_has_md(int vid);
  
 private:

  sqlite3 * db;

  bfs::path icon_path;

};

#endif // DBCONNECTOR_H
