#include <boost/filesystem.hpp>
#include <sqlite3.h>

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

  DbConnector(bfs::path db_path);

  void save_db_file();

  bool video_exists(std::string filename);

  bool trace_exists(std::string filename);

  vid_file fetch_video(std::string filename);

  char * fetch_icon(int vid);

  int get_last_vid();
  
 private:

  sqlite3 * db;

}
