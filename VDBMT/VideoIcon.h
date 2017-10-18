#include "DbConnector.h"
#include <boost/program_options.hpp>
#include <gtkmm-3.0/gtkmm.h>

namespace po = boost::program_options;

class VideoIcon : public Gtk::Image
{
  public:
  VideoIcon(std::string fileName, DbConnector * DbCon, po::variables_map *vm);

  virtual ~VideoIcon();
  
  VidFile * get_vid_file();

  std::string find_border(std::string fileName, float length, po::variables_map *vm);
  
 protected:
  VidFile* fVidFile;
}; 
