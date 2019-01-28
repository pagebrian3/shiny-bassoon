#include "VidFile.h"

//#include "MediaInfo/MediaInfo.h"
//#include "ZenLib/Ztring.h"
//#include <iostream>

VidFile::VidFile(bfs::path fileName1) : fileName(fileName1) {
  size = bfs::file_size(fileName); 
  //MediaInfoLib::MediaInfo MI;
  QMediaPlayer * player = new QMediaPlayer;
  player->setMedia(QUrl::fromLocalFile(fileName.string().c_str()));
  length = 0.001 * player->duration();
  rotate = player->metaData("orientation").toInt();
  delete player;
  /*ZenLib::Ztring zFile;
  zFile += fileName.wstring();
  MI.Open(zFile);
  MI.Option(__T("Inform"),__T("Video;%Duration%"));
  length = 0.001*ZenLib::Ztring(MI.Inform()).To_float32();
  if(length < 60.0) std::cout << fileName << " " << length << std::endl;
  MI.Option(__T("Inform"),__T("Video;%Rotation%"));
  rotate = ZenLib::Ztring(MI.Inform()).To_float32();
  MI.Close();*/
}
