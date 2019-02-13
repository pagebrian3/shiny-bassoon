#ifndef VIDEOUTILS_H
#define VIDEOUTILS_H
#include "QVBConfig.h"
#include <set>
#include "cxxpool.h"
#include "VidFile.h"
#include "DbConnector.h"
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

  void compare_icons(std::vector<int> & vid_list);

  std::string find_border(bfs::path & fileName,float length);

  Magick::Image * get_image(int vid);

  void start_thumbs(std::vector<VidFile *> & vFile);
  
  void start_make_traces(std::vector<VidFile *> & vFile);

  void load_trace(int vid);

  void compare_traces(std::vector<int> & vid_list);

  void make_vids(std::vector<VidFile *> & vidFiles);

  std::vector<std::future_status> get_status();

  std::map<std::pair<int,int>,int> result_map;

  std::set<std::string> get_extensions();

  void set_paths(std::vector<std::string> & paths);

  std::string save_icon(int vid);

  void close();

  qvdb_config * get_config();

  void load_metadata(std::vector<int> & vids);

  std::vector<VidFile *> vid_factory(std::vector<bfs::path> & paths);

  std::string metadata_string(int vid);

  std::vector<int> metadataForVid(int vid);

  std::map<int,std::pair<int,std::string> > md_lookup();

  boost::bimap<int,std::string> md_types();

 private:
  float cTraceFPS, cCompTime, cSliceSpacing, cThresh, cFudge, cStartTime, cThumbT, cCutThresh, cStartT;
  int cHeight, cWidth, cImgThresh, cCache, numThreads, cBFrames;
  bfs::path tempPath, tracePath;
  std::vector<bfs::path> paths;
  DbConnector * dbCon;
  qvdb_config * appConfig;
  std::map<int,Magick::Image *> img_cache;
  std::map<int,std::vector<uint8_t> > traceData;
  std::vector<std::future<bool> > resVec;
  cxxpool::thread_pool * TPool;
  std::set<std::string> extensions;
  std::vector<std::string> cBadChars;
  std::map<int,std::pair<int,std::string> > labelData;
  boost::bimap<int,std::string> labelTypes;
  std::map<int,std::vector<int> > fileMetadata;
};

#endif // VIDEOUTILS_H
