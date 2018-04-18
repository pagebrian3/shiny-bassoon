#include "VideoIcon.h"
#include <boost/format.hpp>

VideoIcon::VideoIcon(VidFile * vidFile):Gtk::Image()  {
  fVidFile = vidFile;
  int size = fVidFile->size/1024;
  set_from_icon_name("missing-image",Gtk::ICON_SIZE_BUTTON); 
  std::string toolTip((boost::format("Filename: %s\nSize: %ikB\nLength: %is") % fVidFile->fileName %  size % fVidFile->length).str());
  set_tooltip_text(toolTip);
};

VideoIcon::~VideoIcon(){
  delete fVidFile;
};

VidFile * VideoIcon::get_vid_file() {
  return fVidFile;
};

