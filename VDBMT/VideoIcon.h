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

  bool create_thumb(DbConnector * dbCon, po::variables_map *vm);

  std::string find_border(std::string fileName, float length, po::variables_map *vm);

  bool hasIcon;

 protected:

  VidFile* fVidFile;
  
}; 
