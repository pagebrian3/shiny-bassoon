#ifndef VIDEOUTILS_H
#define VIDEOUTILS_H

#include "DbConnector.h"
#include "cxxpool.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

class video_utils
{
 public:
  video_utils(DbConnector * dbConn, po::variables_map * vm, bfs::path path);

  void find_dupes();

  bool compare_vids(int i, int j, std::map<int,std::vector<unsigned short> > & data);

  bool calculate_trace(VidFile * obj);

  std::set<std::string> get_extensions();

 private:
  int cThreads;
  float cTraceFPS, cCompTime, cSliceSpacing, cThresh, cFudge, cStartTime;
  DbConnector * dbCon;
  bfs::path cPath;
  cxxpool::thread_pool * cPool;
};

#endif // VIDEOUTILS_H
