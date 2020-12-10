#ifndef VIDFILE_H
#define VIDFILE_H

#include <filesystem>
#include <vector>

class VidFile {

  public:

  VidFile(std::filesystem::path file, float length, int size, int flag, int vid, std::vector<int> crop, int rot, int height, int width): fileName(file), length(length), size(size), okflag(flag), vid(vid), crop(crop), rotate(rot), height(height), width(width) {};

  VidFile(std::filesystem::path file): fileName(file), length(0.0), size(0), okflag(0), vid(-1), crop(std::vector<int>(4)), rotate(0), height(0), width(0) {};

  std::filesystem::path fileName;
  float length;
  int size;
  int okflag;
  int vid;
  std::vector<int> crop;  
  int rotate;
  int height;
  int width;
  
};

#endif // VIDFILE_H
