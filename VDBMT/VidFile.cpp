#include "VidFile.h"
#include "MediaInfo/MediaInfo.h"
#include "ZenLib/Ztring.h"

VidFile::VidFile(bfs::path fileName1,int vid1) : fileName(fileName1), vid (vid1) {
  size = bfs::file_size(fileName); 
  MediaInfoLib::MediaInfo MI;
  ZenLib::Ztring zFile;
  zFile += fileName.wstring();
  MI.Open(zFile);
  MI.Option(__T("Inform"),__T("Video;%Duration%"));
  length = 0.001*ZenLib::Ztring(MI.Inform()).To_float32();
  MI.Option(__T("Inform"),__T("Video;%Rotation%"));
  rotate = ZenLib::Ztring(MI.Inform()).To_float32();
  MI.Close();
}
