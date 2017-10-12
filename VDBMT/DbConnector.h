#include <boost/filesystem.hpp>
#include <sqlite3.h>
#include <VidFile.h>
#include <map>
#include <iostream>

namespace bfs = boost::filesystem;

class DbConnector {

 public:

  DbConnector();

  bool video_exists(std::string filename);

  VidFile* fetch_video(std::string filename);

  void save_video(VidFile *a);

  void fetch_icon(int vid);

  void save_icon(int vid);

  void fetch_results(std::map<std::pair<int,int>, int> & map);

  void update_results(int i, int j, int k);

  bool trace_exists(int vid);

  void save_trace(int vid, std::string & trace);

  void fetch_trace(int vid, std::vector<unsigned short> & trace);

   int get_last_vid();

  char * temp_icon_file();

  void save_db_file();
  
 private:

  sqlite3 * db;
  char* temp_icon;
  int min_vid;

};