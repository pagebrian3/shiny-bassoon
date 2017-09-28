#include "VideoIcon.h"
#include "boost/format.hpp"
#include "MediaInfo/MediaInfo.h"

#define THUMB_TIME  12000
#define ICON_WIDTH 320
#define ICON_HEIGHT 180
#define FUZZ 5000

VideoIcon::VideoIcon(std::string fileName, DbConnector * dbCon){
  char * img_dat;
  if(dbCon->video_exists(fileName)) {
    std::cout << fileName << " Exists" <<std::endl;
    fVidFile = dbCon->fetch_video(fileName);
    dbCon->fetch_icon(fVidFile.vdatid);
  }
  else {
    std::string temp_icon = dbCon->temp_icon_file();
    fVidFile.fileName=fileName;
    fVidFile.size = bfs::file_size(fileName);
    MediaInfoLib::MediaInfo MI; 
    std::wstring fileNameW(fileName.length(), L' '); // Make room for characters
  
     // Copy string to wstring.
     std::copy(fileName.begin(), fileName.end(), fileNameW.begin());
    MI.Open(fileNameW);
    int length = std::atof(reinterpret_cast<const char*>(MI.Get(MediaInfoLib::Stream_General, 0 ,__T("mdhd_Duration"), MediaInfoLib::Info_Text, MediaInfoLib::Info_Name).c_str())); //Might be duration instead of length
    fVidFile.length = length;
    double thumb_t = THUMB_TIME;
    if(length < THUMB_TIME) thumb_t = length/2.0;
    thumb_t/=1000.0;
    std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss 00:00:%i.00 -i \"%s\" -vframes 1 -f image2pipe -vcodec png - | convert png:- -fuzz %i -trim -thumbnail %ix%i %s") % thumb_t % fileName % FUZZ % ICON_WIDTH % ICON_HEIGHT % temp_icon ).str().c_str());
    dbCon->save_video(fVidFile);
    dbCon->save_icon(fVidFile.vdatid);
  }
  Gtk::Image image(dbCon->temp_icon_file());
  this->add(image);
  std::stringstream ss;
  ss <<"Filename: "<<fileName<< "\nSize: "<<fVidFile.size/1024<<"kB\nLength: " <<fVidFile.length/1000.0<<"s";
  this->set_tooltip_text(ss.str());
};

vid_file VideoIcon::get_vid_file() {
  return fVidFile;
};
