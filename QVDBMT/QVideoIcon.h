#ifndef QVIDEOICON_H
#define QVIDEOICON_H
#include "VidFile.h"
#include <QStandardItem>

class QVideoIcon : public QStandardItem
{
  public:

  QVideoIcon(VidFile * vidFile);
  virtual ~QVideoIcon();
  VidFile * get_vid_file();

 protected:

  VidFile* fVidFile;
};

#endif  //  QVIDEOICON_H
