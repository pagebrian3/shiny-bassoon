#include "VideoIcon.h"
#include <boost/format.hpp>
#include <boost/process.hpp>

VideoIcon::VideoIcon(bfs::path fileName, DbConnector * dbCon,int vid):Gtk::Image()  {
  if(dbCon->video_exists(fileName)) {
    fVidFile = dbCon->fetch_video(fileName);
    std::string icon_file(dbCon->fetch_icon(fVidFile->vid));
    this->set(icon_file);
    std::system((boost::format("rm %s") %icon_file).str().c_str());
    hasIcon = true;
  }
  else {
    hasIcon=false;
    this->set_from_icon_name("missing-image",Gtk::ICON_SIZE_BUTTON);
    fVidFile = new VidFile();
    fVidFile->fileName=fileName;
    fVidFile->size = bfs::file_size(fileName);
    fVidFile->vid = vid;
    boost::process::ipstream is;
    std::string outString;
    boost::process::system((boost::format("ffprobe -v error -show_entries stream_tags=rotate -of default=noprint_wrappers=1:nokey=1 %s") % fileName).str(), boost::process::std_out > is);  
    std::getline(is, outString);
    int rotate = 0;
    if(outString.length() > 0) rotate = std::stod(outString);
    boost::process::ipstream is2;
    std::string outString2;
    boost::process::system((boost::format("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 %s") % fileName).str(), boost::process::std_out > is2);  
    std::getline(is2, outString2);
    double length=0.0;
    if(outString2.length() > 0) length=std::stod(outString2);
    fVidFile->length = length;
    fVidFile->rotate = rotate;
    dbCon->save_video(fVidFile);
  }
  int size = fVidFile->size/1024;
  std::string toolTip((boost::format("Filename: %s\nSize: %ikB\nLength: %is") % fileName %  size % fVidFile->length).str());
  this->set_tooltip_text(toolTip);
};

VideoIcon::~VideoIcon(){
  delete fVidFile;
};

VidFile * VideoIcon::get_vid_file() {
  return fVidFile;
};

void VideoIcon::set_icon(int i, std::string fileName) {
  if(fVidFile->vid == i) this->set(fileName);
}
