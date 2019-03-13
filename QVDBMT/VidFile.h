#ifndef VIDFILE_H
#define VIDFILE_H

#include <boost/filesystem.hpp>
#include <string>

namespace bfs = boost::filesystem;

class VidFile {

  public:

  VidFile(bfs::path file, float length, int size, int flag, int vid, std::string crop, int rot, int height, int width): fileName(file), length(length), size(size), okflag(flag), vid(vid), crop(crop), rotate(rot), height(height), width(width) {};

  bfs::path fileName;
  float length;
  int size;
  int okflag;
  int vid;
  std::string crop;  
  int rotate;
  int height;
  int width;
  
};

#endif // VIDFILE_H
