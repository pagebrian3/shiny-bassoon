#ifndef VIDEOUTILS_H
#define VIDEOUTILS_H

#include <set>
#include <cxxpool.h>
#include <boost/filesystem.hpp>
#if defined(_WIN32)
 #define PLATFORM_NAME "windows" // Windows
#elif defined(_WIN64)
 #define PLATFORM_NAME "windows"
#elif defined(__linux__)
#define PLATFORM_NAME "linux"
#endif

namespace bfs=boost::filesystem;

namespace Magick {
   class Image;
}

class DbConnector;
class VidFile;
class qvdb_metadata;
class qvdb_config;

class video_utils
{
 public:

  video_utils();

  bool compare_vids(int i, int j);

  bool compare_vids_fft(int i, int j);

  bool calculate_trace(VidFile * obj);

  bool create_thumb(VidFile * vFile);

  void create_image(bfs::path & fileName, float start_time, std::vector<uint8_t> * imgDat);

  bool compare_images(int vid1, int vid2);

  void compare_icons();

  std::string find_border(bfs::path & fileName, float length, int height, int width);

  Magick::Image * get_image(int vid, bool firstImg);

  void start_thumbs(std::vector<VidFile *> & vFile);
  
  int start_make_traces(std::vector<VidFile *> & vFile);

  void load_trace(int vid);

  int compare_traces();

  int make_vids(std::vector<VidFile *> & vidFiles);

  bfs::path fetch_icon(int vid);

  std::vector<std::future_status> get_status();

  std::set<std::string> get_extensions();

  void set_paths(std::vector<bfs::path> & paths);

  bfs::path save_icon(int vid);

  void close();

  qvdb_config * get_config();

  qvdb_metadata * mdInterface();

  bool vid_factory(std::vector<bfs::path> & paths);

  bool getVidBatch(std::vector<VidFile*> & batch);

  bool thumb_exists(int vid);

 private:
  
  cxxpool::thread_pool * TPool;
  bfs::path tempPath, tracePath;
  DbConnector * dbCon;
  qvdb_config * appConfig;
  qvdb_metadata * metaData;
  std::pair<int,Magick::Image *> img_cache;
  std::map<int,std::vector<uint8_t> > traceData;
  std::map<std::pair<int,int>,int> result_map;
  std::list<std::vector<VidFile *> > completedVFs;
  std::vector<std::future<bool> > resVec;
  std::vector<std::string> cBadChars;
  std::vector<int> fVIDs;
  std::vector<bfs::path> paths;
  std::set<std::string> extensions;
  
};

#endif // VIDEOUTILS_H
