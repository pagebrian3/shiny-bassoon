#include "DbConnector.h"
#include <gtkmm-3.0/gtkmm.h>

class VideoIcon : public Gtk::EventBox
{
  public:
  VideoIcon(char *fileName, DbConnector * DbCon);
  vid_file get_vid_file();

  private:
  vid_file fVidFile;
}; 
