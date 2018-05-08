#include "QVideoIcon.h"
#include <boost/format.hpp>

QVideoIcon::QVideoIcon(VidFile * vidFile):QStandardItem()  {
  fVidFile = vidFile;
  float size = fVidFile->size;
  std::string sizeLabel = "B";
  float factor=1.0/1024.0;
  if(factor*size >= 1.0) {
    size*=factor;
    if(factor*size >= 1.0) {
      size*=factor;
      if(factor*size >= 1.0) {
	size*=factor;
	sizeLabel = "GB";
      }
      else sizeLabel = "MB";
    }
    else sizeLabel = "KB";
  }
  int h = fVidFile->length/3600.0;
  int m = ((int)(fVidFile->length)%3600)/60;
  float s = fVidFile->length-m*60.0;
  setIcon(QIcon::fromTheme("image-missing"));
  QString toolTip((boost::format("Filename: %s\nSize: %3.2f%s\nLength: %i:%02i:%02.1f") % fVidFile->fileName %  size % sizeLabel % h % m % s).str().c_str());
  setToolTip(toolTip);
};

QVideoIcon::~QVideoIcon(){
  delete fVidFile;
};

VidFile * QVideoIcon::get_vid_file() {
  return fVidFile;
};

