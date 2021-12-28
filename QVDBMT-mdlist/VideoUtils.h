#ifndef VIDEOUTILS_H
#define VIDEOUTILS_H

#include <set>
#include <list>
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

  bool calculate_trace2(VidFile * obj);

  bool create_thumb(VidFile * vFile);

  bool create_frames(VidFile * vFile);

  void create_image(std::filesystem::path & fileName, float start_time, std::vector<char> * imgDat);

  bool compare_images(int vid1, int vid2);

  void img_comp_thread();

  void compare_icons();

  bool find_border(VidFile * vidFile, uint8_t ** data, std::vector<int> & crop);

  Magick::Image * get_image(int vid, bool firstImg);

  void start_thumbs(std::vector<VidFile *> & vFile);
  
  int start_make_traces(std::vector<VidFile *> & vFile);

  bool StartFaceFrames(std::vector<VidFile *> & vFile);

  bool load_trace(int vid);

  void move_traces();

  int compare_traces();

  bool make_vids(std::vector<VidFile *> & vidFiles);

  std::vector<std::future_status> get_status();

  std::set<std::string> get_extensions();

  std::filesystem::path createPath(std::filesystem::path & path, int vid, std::string extension);

  std::filesystem::path icon_filename(int vid);

  std::filesystem::path get_temp_path() {return tempPath; };

  std::filesystem::path save_path() {return savePath; };

  std::filesystem::path home_path() { return homePath; };

  void close();

  qvdb_config * get_config();

  qvdb_metadata * mdInterface();

  DbConnector * get_db() {return dbCon;};

  bool thumb_exists(int vid);

  bool video_exists(std::filesystem::path & filename);

  bool trace_exists(int vid);

  cxxpool::thread_pool * get_tpool() {return TPool;};

  void set_vid_vec(std::vector<int> * vid_list);

 private:

  std::string decodeDevice;
  cxxpool::thread_pool * TPool;
  std::filesystem::path tempPath, tracePath, savePath, thumbPath, homePath;
  DbConnector * dbCon;
  qvdb_config * appConfig;
  qvdb_metadata * metaData;
  std::pair<int,Magick::Image *> img_cache;
  std::map<int,std::vector<uint8_t> > traceData;
  std::map<std::tuple<int,int,int>,std::pair<int,std::string>> result_map;
  std::vector<std::future<bool> > resVec;
  std::vector<std::string> cBadChars;
  std::vector<int> * fVIDs;
  std::mutex mtx;
  
};

#endif // VIDEOUTILS_H
