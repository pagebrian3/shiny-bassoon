#include <string>

class VidFile {
 public:
 VidFile(): fileName(""),okflag(0),length(0),size(0),vid(0){};
 VidFile(std::string file, int length, int size, int flag, int vid): fileName(file), length(length), size(size), okflag(flag), vid(vid){};
  std::string fileName;
  int okflag;
  int length;
  int size;
  int vid;
};

