#include <string>

class VidFile {
 public:
 VidFile(): fileName(""),okflag(0),length(0.0),size(0),vid(0){};
 VidFile(std::string file, float length, int size, int flag, int vid): fileName(file), length(length), size(size), okflag(flag), vid(vid){};
  std::string fixed_filename() {
    std::string temp(fileName);
    for(int i = 0; i < temp.length();i++) 
      if(temp[i] == '\'' || temp[i] == '$' || temp[i] == ' ') 
      temp.insert(i++,"\\");
    return temp;	 
  };
  std::string fileName;
  int okflag;
  double length;
  int size;
  int vid;
};

