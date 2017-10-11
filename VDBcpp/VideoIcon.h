#include "DbConnector.h"
#include <gtkmm-3.0/gtkmm.h>

class VideoIcon : public Gtk::Image
{
  public:
  VideoIcon(std::string fileName, DbConnector * DbCon);

  virtual ~VideoIcon();
  
  VidFile * get_vid_file();

 protected:
  VidFile* fVidFile;
}; 
