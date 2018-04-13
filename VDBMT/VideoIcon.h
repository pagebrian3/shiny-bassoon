#ifndef VIDEOICON_H
#define VIDEOICON_H
#include "DbConnector.h"
#include <gtkmm-3.0/gtkmm.h>

class VideoIcon : public Gtk::Image
{
  public:

  VideoIcon(VidFile * vidFile);

  virtual ~VideoIcon();

  VidFile * get_vid_file();

  void set_icon(int i, std::string file);

 protected:

  VidFile* fVidFile;
  
};

#endif  //  VIDEOICON_H
