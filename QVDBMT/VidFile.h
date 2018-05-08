#ifndef VIDFILE_H
#define VIDFILE_H

#include <boost/filesystem.hpp>
#include <string>

namespace bfs = boost::filesystem;

class VidFile {
 public:

  VidFile(bfs::path fileName, int vid);

 VidFile(bfs::path file, float length, int size, int flag, int vid, std::string crop, int rot): fileName(file), length(length), size(size), okflag(flag), vid(vid), crop(crop), rotate(rot) {};

  bfs::path fileName;
  float length;
  int size;
  int okflag;
  int vid;
  std::string crop;  
  int rotate;
};

#endif // VIDFILE_H
