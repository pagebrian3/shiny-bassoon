#ifndef VIDEOUTILS_H
#define VIDEOUTILS_H

#include <set>
#include <cxxpool.h>
#include <filesystem>
#include <mutex>
#if defined(_WIN32)
 #define PLATFORM_NAME "windows" // Windows
#elif defined(_WIN64)
 #define PLATFORM_NAME "windows"
#elif defined(__linux__)
#define PLATFORM_NAME "linux"
#endif

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

  bool compare_vids_fft(int i, int j);

  bool calculate_trace(VidFile * obj);

  bool create_thumb(VidFile * vFile);

  void create_image(std::filesystem::path & fileName, float start_time, std::vector<char> * imgDat);

  bool compare_images(int vid1, int vid2);

  void img_comp_thread();

  void compare_icons();

  bool find_border(VidFile * vidFile, std::vector<char> &data, std::vector<int> & crop);

  Magick::Image * get_image(int vid, bool firstImg);

  void start_thumbs(std::vector<VidFile *> & vFile);
  
  int start_make_traces(std::vector<VidFile *> & vFile);

  bool load_trace(int vid);

  void move_traces();

  int compare_traces();

  int make_vids(std::vector<VidFile *> & vidFiles);

  std::vector<std::future_status> get_status();

  std::set<std::string> get_extensions();

  void set_paths(std::vector<std::filesystem::path> & paths);

  std::filesystem::path createPath(std::filesystem::path & path, int vid, std::string extension);

  std::filesystem::path icon_filename(int vid);

  void close();

  qvdb_config * get_config();

  qvdb_metadata * mdInterface();

  bool vid_factory(std::vector<std::filesystem::path> & paths);

  bool getVidBatch(std::vector<VidFile*> & batch);

  bool thumb_exists(int vid);

  bool trace_exists(int vid);

  bool frameNoCrop(std::filesystem::path & fileName, double start_time, std::vector<char> & imgDat);

 private:
  
  enum AVHWDeviceType hwDType;
  cxxpool::thread_pool * TPool;
  std::filesystem::path tempPath, tracePath, savePath, thumbPath;
  DbConnector * dbCon;
  qvdb_config * appConfig;
  qvdb_metadata * metaData;
  std::pair<int,Magick::Image *> img_cache;
  std::map<int,std::vector<char> > traceData;
  std::map<std::tuple<int,int,int>,std::pair<int,std::string>> result_map;
  std::list<std::vector<VidFile *> > completedVFs;
  std::vector<std::future<bool> > resVec;
  std::vector<std::string> cBadChars;
  std::vector<int> fVIDs;
  std::vector<std::filesystem::path> paths;
  std::set<std::string> extensions;
  std::mutex mtx;
  
};

#endif // VIDEOUTILS_H
