#include <boost/filesystem.hpp>
#include <sqlite3.h>
#include <map>
#include <iostream>

namespace bfs = boost::filesystem;

typedef struct {
  std::string fileName;
  double size;
  double length;
  bool okflag;
  int vdatid;
} vid_file;

class DbConnector {

 public:

  DbConnector();

  void save_db_file();

  bool video_exists(std::string filename);

  bool trace_exists(int vid);

  vid_file fetch_video(std::string filename);

  void fetch_icon(int vid);

  void save_icon(int vid);
  
  int get_last_vid();

  char * temp_icon_file();

  void fetch_results(std::map<std::pair<int,int>, int> & map);

  void update_results(int i, int j, int k);

  void save_trace(int vid, std::vector<int> & trace);

  void fetch_trace(int vid, std::vector<int> & trace);
  
 private:

  sqlite3 * db;
  char* temp_icon;
  int min_vid;

};
