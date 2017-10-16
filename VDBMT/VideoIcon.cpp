#include "VideoIcon.h"
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <iostream>

#define THUMB_TIME  12.0
#define ICON_WIDTH 320
#define ICON_HEIGHT 180
#define FUZZ 5000

VideoIcon::VideoIcon(std::string fileName, DbConnector * dbCon):Gtk::Image()  {
  std::cout << fileName << std::endl;
  if(dbCon->video_exists(fileName)) {
    fVidFile = dbCon->fetch_video(fileName);
    dbCon->fetch_icon(fVidFile->vid);
  }
  else {
    fVidFile = new VidFile();
    std::string temp_icon = dbCon->temp_icon_file();
    fVidFile->fileName=fileName;
    fVidFile->size = bfs::file_size(fileName);
    boost::process::ipstream is;
    std::string cmd((boost::format("./info.sh %s") % fileName).str());
    //std::cout <<"COMMAND: " << cmd << std::endl;
    boost::process::system(cmd,  boost::process::std_out > is);
    std::string outString;
    std::getline(is, outString);
    //std::cout <<"HERE: " <<fileName <<" "<< outString << std::endl;
    std::vector<std::string> split_string;
    boost::split(split_string,outString,boost::is_any_of(","));
    int width=std::stoi(split_string[0]);
    int height=std::stoi(split_string[1]);
    double length=0.001*(double)std::stoi(split_string[2]);
    int rotate = std::stod(split_string[3]);
    if(rotate == 90 || rotate ==-90) {
      int temp = width;
      width=height;
      height=temp;
    }
    fVidFile->length = length;
    dbCon->save_video(fVidFile);
    double thumb_t = THUMB_TIME;
    if(length < THUMB_TIME) thumb_t = length/2.0;
    std::string cmdln3((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.03d -i %s -frames:v 1 -pix_fmt rgb24 -f image2pipe -vcodec rawvideo - | convert -size %ix%i -depth 8 RGB:- -fuzz %i -trim -thumbnail %ix%i %s%i.jpg") % thumb_t % fVidFile->fixed_filename() % width % height % FUZZ % ICON_WIDTH % ICON_HEIGHT % temp_icon % fVidFile->vid).str());
    std::system(cmdln3.c_str());    
    dbCon->save_icon(fVidFile->vid);
  }
  std::string icon_file((boost::format("%s%i.jpg") % dbCon->temp_icon_file() % fVidFile->vid).str());
  this->set(icon_file);
  std::system((boost::format("rm %s") %icon_file).str().c_str());
  std::stringstream ss;
  ss <<"Filename: "<<fileName<< "\nSize: "<<fVidFile->size/1024<<"kB\nLength: " <<fVidFile->length<<"s";
  this->set_tooltip_text(ss.str());
};

VideoIcon::~VideoIcon(){
};

VidFile * VideoIcon::get_vid_file() {
  return fVidFile;
};
