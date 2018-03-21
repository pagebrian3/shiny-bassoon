#include "DbConnector.h"
#include <boost/program_options.hpp>
#include <gtkmm-3.0/gtkmm.h>

namespace po = boost::program_options;

class VideoIcon : public Gtk::Image
{
  public:

  VideoIcon(std::string fileName, DbConnector * DbCon, int vid, po::variables_map *vm);

  virtual ~VideoIcon();

  VidFile * get_vid_file();

  void set_icon(int i, std::string file);

  bool hasIcon;

 protected:

  VidFile* fVidFile;
  
}; 
