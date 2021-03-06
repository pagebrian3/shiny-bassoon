#ifndef VIDEOUTILS_H
#define VIDEOUTILS_H

#include "cxxpool.h"
#include "VidFile.h"
#include "DbConnector.h"
#include <boost/program_options.hpp>
#include "Magick++.h"

namespace po = boost::program_options;

class video_utils
{
 public:

  video_utils(po::variables_map & vm);

  video_utils();

  bool compare_vids(int i, int j, std::map<int,std::vector<uint8_t> > & data);

  bool calculate_trace(VidFile * obj);

  bool create_thumb(VidFile * vFile);

  void create_image(bfs::path & fileName, float start_time, std::vector<uint8_t> * imgDat);

  bool compare_images(int vid1, int vid2);

  void compare_icons(std::vector<int> & vid_list);

  std::string find_border(bfs::path & fileName,float length);

  Magick::Image * get_image(int vid);

  void start_thumbs(std::vector<VidFile *> & vFile);
  
  void start_make_traces(std::vector<VidFile *> & vFile);

  void compare_traces(std::vector<int> & vid_list);

  void make_vids(std::vector<VidFile *> & vidFiles);

  std::vector<std::future_status> get_status();

  std::map<std::pair<int,int>,int> result_map;

  std::set<std::string> get_extensions();

  void set_paths(std::vector<std::string> paths);

  std::string save_icon(int vid);

  void save_db();

 private:
  float cTraceFPS, cCompTime, cSliceSpacing, cThresh, cFudge, cStartTime, cThumbT, cBFrames, cCutThresh, cStartT;
  int cHeight, cWidth, cImgThresh, cCache;
  bfs::path tempPath;
  std::vector<bfs::path> paths;
  DbConnector * dbCon;
  std::map<int,Magick::Image *> img_cache;
  std::vector<std::future<bool> > resVec;
  cxxpool::thread_pool * TPool;
  std::set<std::string> extensions;
  std::vector<std::string> cBadChars;
};

#endif // VIDEOUTILS_H
