#ifndef VIDFILE_H
#define VIDFILE_H

#include <boost/filesystem.hpp>
#include <string>

namespace bfs = boost::filesystem;

class VidFile {
 public:
 VidFile(): fileName(""),okflag(0),length(0.0),size(0),vid(0),crop(""),rotate(0){};
 VidFile(bfs::path file, float length, int size, int flag, int vid, std::string crop="", int rot=0): fileName(file), length(length), size(size), okflag(flag), vid(vid), crop(crop), rotate(rot) {};
  bfs::path fixed_filename() {
    std::string temp(fileName.c_str());
    std::string bad_chars("'&();` ");
    for(int i = 0; i < temp.length();i++) 
      if(bad_chars.find(temp[i]) != std::string::npos ) temp.insert(i++,"\\");
    return bfs::path(temp);	 
  };
  std::string crop;
  bfs::path fileName;
  int okflag;
  float length;
  int size;
  int vid;
  int rotate;
};

#endif // VIDFILE_H
