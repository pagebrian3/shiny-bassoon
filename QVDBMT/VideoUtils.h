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
  
  void start_make_traces(std::vector<VidFile *> & vFile);

  void load_trace(int vid);

  void compare_traces();

  void make_vids(std::vector<VidFile *> & vidFiles);

  std::vector<std::future_status> get_status();

  std::set<std::string> get_extensions();

  void set_paths(std::vector<std::string> & paths);

  std::string save_icon(int vid);

  void close();

  qvdb_config * get_config();

  qvdb_metadata * mdInterface(); 

  std::vector<VidFile *> vid_factory(std::vector<bfs::path> & paths);

 private:
  
  bfs::path tempPath, tracePath;
  std::vector<bfs::path> paths;
  DbConnector * dbCon;
  qvdb_config * appConfig;
  qvdb_metadata * metaData;
  std::pair<int,Magick::Image *> img_cache;
  std::map<int,std::vector<uint8_t> > traceData;
  std::map<std::pair<int,int>,int> result_map;
  std::vector<std::future<bool> > resVec;
  cxxpool::thread_pool * TPool;
  std::set<std::string> extensions;
  std::vector<std::string> cBadChars;
  std::vector<int> fVIDs;
};

#endif // VIDEOUTILS_H
