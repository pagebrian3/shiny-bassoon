#ifndef DBCONNECTOR_H
#define DBCONNECTOR_H

#include <sqlite3.h>
#include <VidFile.h>
#include <map>
#include <iostream>

class DbConnector {

 public:

  DbConnector(bfs::path & path);

  bool video_exists(bfs::path & filename);

  bool icon_exists(int vid);

  VidFile* fetch_video(bfs::path & filename);

  void save_video(VidFile *a);

  std::string fetch_icon(int vid);

  void save_icon(int vid);

  std::string create_icon_path(int vid);

  void save_crop(VidFile *a);

  void fetch_results(std::map<std::pair<int,int>, int> & map);

  void update_results(int i, int j, int k);

  bool trace_exists(int vid);

  void save_trace(int vid, std::string & trace);

  void fetch_trace(int vid, std::vector<uint8_t> & trace);

  void save_db_file();

  void cleanup(bfs::path & dir, std::vector<bfs::path> & files);

  std::vector<std::tuple<std::string,int,float,std::string> >fetch_config();

  void save_config(std::map<std::string,std::tuple<int,float,std::string> >  config); 
  
 private:

  sqlite3 * db;

  bfs::path icon_path;

};

#endif // DBCONNECTOR_H
