#ifndef VIDEOUTILS_H
#define VIDEOUTILS_H

#include <map>
#include <vector>
#include "VidFile.h"
#include "DbConnector.h"
#include <boost/program_options.hpp>
#include "Magick++.h"

namespace po = boost::program_options;

class video_utils
{
 public:

  video_utils(DbConnector * dbCon, po::variables_map * vm, bfs::path tempPath);

  video_utils();

  bool compare_vids(int i, int j, std::map<int,std::vector<unsigned short> > & data);

  bool calculate_trace(VidFile * obj);

  bool create_thumb(VidFile * vFile);

  void create_image(std::string fileName, float start_time, std::vector<short> * imgDat);

  bool compare_images(int vid1, int vid2);

  std::string find_border(std::string fileName,float length);

  Magick::Image * get_image(int vid);

 private:
  float cTraceFPS, cCompTime, cSliceSpacing, cThresh, cFudge, cStartTime;
  po::variables_map * vm;
  bfs::path tempPath;
  DbConnector * dbCon;
  std::map<int,Magick::Image *> img_cache;
};

#endif // VIDEOUTILS_H
