#ifndef VIDEOICON_H
#define VIDEOICON_H
#include "VidFile.h"
#include <gtkmm-3.0/gtkmm.h>

class VideoIcon : public Gtk::Image
{
  public:

  VideoIcon(VidFile * vidFile);

  virtual ~VideoIcon();

  VidFile * get_vid_file();

 protected:

  VidFile* fVidFile;
  
};

#endif  //  VIDEOICON_H
