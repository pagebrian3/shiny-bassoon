#include "VideoIcon.h"

#define THUMB_TIME  12000
#define ICON_WIDTH 320
#define ICON_HEIGHT 180
#define FUZZ 5000

VideoIcon::VideoIcon(char *fileName, DbConnector * dbCon){
  char * img_dat;
  if(dbCon->video_exists(fileName)) {
    fVidFile = dbCon->fetch_video(fileName);
    img_dat = dbCon->fetch_icon(fVidFile.vdatid);
    }
  else {
    fVidFile.fileName=fileName;
    fVidFile.size = bfs::file_size(fileName);
    MediaInfoLib::MediaInfo MI;
    MI.Open(__T(fileName));
    double length = std::atof(MI.Get(Stream_General, 0 ,__T("Length"), Info_Text, Info_Name).c_str()); //Might be duration instead of length
    fVidFile.length = length;
    double thumb_t = THUMB_TIME;
    if(length < THUMB_TIME) thumb_t = length/2.0;
    std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss 00:00:%i.00 -i \"%s\" -vframes 1 -f image2pipe -vcodec png - | convert png:- -fuzz %i -trim -thumbnail %ix%i %s") % thumb_t/1000 % fileName % FUZZ % ICON_WIDTH % ICON_HEIGHT % temp_icon ).str().c_str());
    dbCon->save_icon(vid);
  }
  GTK::Image image(dbCon->temp_icon_file());
  this->add(image);
  this->set_tooltip_text("Filename: "+fileName+ "\nSize: "+fVidFile.size/1024+"kB\nLength: " +fVidFile.length/1000.0+"s");
};

VideoIcon::get_vid_file() {
  return fVidFile;
};
