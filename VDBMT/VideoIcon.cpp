#include "VideoIcon.h"
#include <boost/format.hpp>
#include <boost/process.hpp>

VideoIcon::VideoIcon(VidFile * vidFile):Gtk::Image()  {
  fVidFile = vidFile;
  int size = fVidFile->size/1024;
  std::string toolTip((boost::format("Filename: %s\nSize: %ikB\nLength: %is") % fVidFile->fileName %  size % fVidFile->length).str());
  this->set_tooltip_text(toolTip);
};

VideoIcon::~VideoIcon(){
  delete fVidFile;
};

VidFile * VideoIcon::get_vid_file() {
  return fVidFile;
};

void VideoIcon::set_icon(int i, std::string fileName) {
  if(fVidFile->vid == i) this->set(fileName);
}
