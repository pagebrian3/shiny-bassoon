#include "VideoIcon.h"
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <iomanip>

#define THUMB_TIME  12.0
#define ICON_WIDTH 320
#define ICON_HEIGHT 180
#define FUZZ 5000

VideoIcon::VideoIcon(std::string fileName, DbConnector * dbCon):Gtk::Image()  {
  char * img_dat;
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
    std::string cmdln((boost::format("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"%s\"") % fileName).str());
    boost::process::system(cmdln,boost::process::std_out > is);
    std::string outString;
    std::getline(is,outString);
    double length=std::stod(outString);
    std::cout <<"BLAH " <<fileName <<" "<< length << std::endl;
    fVidFile->length = length;
    double thumb_t = THUMB_TIME;
    if(length < THUMB_TIME) thumb_t = length/2.0;
    std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.03d -i '%s' -frames:v 1 -f image2pipe -vcodec png - | convert png:- -fuzz %i -trim -thumbnail %ix%i %s") % thumb_t % fileName % FUZZ % ICON_WIDTH % ICON_HEIGHT % temp_icon ).str().c_str());
    dbCon->save_video(fVidFile);
    dbCon->save_icon(fVidFile->vid);
  }
  this->set(dbCon->temp_icon_file());
  std::stringstream ss;
  ss <<"Filename: "<<fileName<< "\nSize: "<<fVidFile->size/1024<<"kB\nLength: " <<fVidFile->length<<"s";
  this->set_tooltip_text(ss.str());
};

VideoIcon::~VideoIcon(){
};

VidFile * VideoIcon::get_vid_file() {
  return fVidFile;
};
