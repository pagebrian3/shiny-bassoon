#ifndef VIDEOUTILS_H
#define VIDEOUTILS_H

#include <map>
#include <vector>
#include "VideoIcon.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

class video_utils
{
 public:

  video_utils(DbConnector * dbCon, po::variables_map * vm, bfs::path tempPath);

  video_utils();

  bool compare_vids(int i, int j, std::map<int,std::vector<unsigned short> > & data);

  bool calculate_trace(VidFile * obj);

  bool create_thumb(VideoIcon * icon);

  void load_image(std::string fileName, float start_time, std::vector<short> * imgDat);

  std::string find_border(std::string fileName,float length);

 private:
  float cTraceFPS, cCompTime, cSliceSpacing, cThresh, cFudge, cStartTime;
  po::variables_map * vm;
  bfs::path tempPath;
  DbConnector * dbCon;
};

#endif // VIDEOUTILS_H
