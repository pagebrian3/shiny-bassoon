#include "VideoIcon.h"
#include <boost/format.hpp>
#include <boost/process.hpp>

VideoIcon::VideoIcon(std::string fileName, DbConnector * dbCon,int vid, po::variables_map *vm):Gtk::Image()  {
  if(dbCon->video_exists(fileName)) {
    fVidFile = dbCon->fetch_video(fileName);
    dbCon->fetch_icon(fVidFile->vid);
    std::string icon_file((boost::format("%s%i.jpg") % (*vm)["app_path"].as<std::string>() % fVidFile->vid).str());
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
    std::string cmd((boost::format("./info.sh %s") % fileName).str());
    boost::process::system(cmd,  boost::process::std_out > is);
    std::string outString;
    std::getline(is, outString);
    std::vector<std::string> split_string;
    boost::split(split_string,outString,boost::is_any_of(","));
    double length=0.0;
    if(split_string[0].length() > 0) length=0.001*(double)std::stoi(split_string[0]);
    int rotate = 0;
    if(split_string[1].length() > 0) rotate = std::stod(split_string[1]);
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
