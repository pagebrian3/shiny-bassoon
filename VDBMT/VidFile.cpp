#include "VidFile.h"
#include <boost/process.hpp>
#include <boost/format.hpp>

VidFile::VidFile(bfs::path fileName1,int vid1) {
  fileName=fileName1;
  size = bfs::file_size(fileName);
  vid = vid1;
  boost::process::ipstream is;
  std::string outString;
  boost::process::system((boost::format("ffprobe -v error -show_entries stream_tags=rotate -of default=noprint_wrappers=1:nokey=1 %s") % fileName).str(), boost::process::std_out > is);  
  std::getline(is, outString);
  rotate = 0.0;
  if(outString.length() > 0) rotate = std::stod(outString);
  boost::process::ipstream is2;
  std::string outString2;
  boost::process::system((boost::format("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 %s") % fileName).str(), boost::process::std_out > is2);  
  std::getline(is2, outString2);
  length=0.0;
  if(outString2.length() > 0) length=std::stod(outString2);
}
