#ifndef DBCONNECTOR_H
#define DBCONNECTOR_H

#include <sqlite3.h>
#include <VidFile.h>
#include <map>
#include <iostream>

class DbConnector {

 public:

  DbConnector(bfs::path path);

  bool video_exists(bfs::path filename);

  bool icon_exists(int vid);

  VidFile* fetch_video(bfs::path filename);

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

  int get_last_vid();

  void save_db_file();
  
 private:

  sqlite3 * db;

  char* temp_icon;

};

#endif // DBCONNECTOR_H
