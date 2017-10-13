#include "VideoIcon.h"
#include <boost/format.hpp>
#include <boost/process.hpp>

#define THUMB_TIME  12.0
#define ICON_WIDTH 320
#define ICON_HEIGHT 180
#define FUZZ 5000

VideoIcon::VideoIcon(std::string fileName, DbConnector * dbCon):Gtk::Image()  {
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
    std::string cmdln1((boost::format("ffprobe -v error -show_entries stream=width,height,duration -of default=noprint_wrappers=1:nokey=1 \"%s\"") % fileName).str());
    boost::process::system(cmdln1,boost::process::std_out > is);
    std::string outString;
    std::getline(is,outString);
    std::cout << outString<<std::endl;
    int width=std::stoi(outString);
    outString.clear();
    std::getline(is,outString);
    std::cout << outString <<std::endl;
    int height=std::stoi(outString);
    outString.clear();
    std::getline(is,outString);
    std::cout << outString <<std::endl;
    double length=std::stod(outString);
    outString.clear();
    std::string cmdln2((boost::format("ffprobe -v error -of default=noprint_wrappers=1:nokey=1 -select_streams v:0 -show_entries stream_tag=rotate \"%s\"") % fileName).str());
     boost::process::ipstream is1;
    boost::process::system(cmdln2,boost::process::std_out > is1);
    std::getline(is1,outString);
    int rotate=std::stoi(outString);
    std::cout <<"LENGTH "<< length <<" HEIGHT "<< height <<" WIDTH " << width << " ROTATE " <<rotate <<std::endl;
    fVidFile->length = length;
    dbCon->save_video(fVidFile);
    double thumb_t = THUMB_TIME;
    if(length < THUMB_TIME) thumb_t = length/2.0;
    std::string cmdln3((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.03d -i %s -frames:v 1 -pix_fmt rgb24 -f image2pipe -vcodec rawvideo - | convert -size %ix%i -depth 8 RGB:- -fuzz %i -trim -thumbnail %ix%i %s%i.png") % thumb_t % fVidFile->fixed_filename() % width % height % FUZZ % ICON_WIDTH % ICON_HEIGHT % temp_icon % fVidFile->vid).str());
    std::system(cmdln3.c_str());    
    dbCon->save_icon(fVidFile->vid);
  }
  std::string icon_file((boost::format("%s%i.png") % dbCon->temp_icon_file() % fVidFile->vid).str());
  this->set(icon_file);
  std::stringstream ss;
  ss <<"Filename: "<<fileName<< "\nSize: "<<fVidFile->size/1024<<"kB\nLength: " <<fVidFile->length<<"s";
  this->set_tooltip_text(ss.str());
};

VideoIcon::~VideoIcon(){
};

VidFile * VideoIcon::get_vid_file() {
  return fVidFile;
};
