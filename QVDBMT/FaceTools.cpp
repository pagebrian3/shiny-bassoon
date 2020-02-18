#include <list>
#include "FaceTools.h"
#include "VidFile.h"
#include "VideoUtils.h"
#define DLIB_PNG_SUPPORT
#include <dlib/image_io.h>


FaceTools::FaceTools(video_utils * vu) : fVU(vu) {
  detector = dlib::get_frontal_face_detector();
  TPool = fVU->get_tpool();
  fTempPath = fVU->get_temp_path();
  fTempPath+="/face_temp";
  if(!std::filesystem::exists(fTempPath))std::filesystem::create_directory(fTempPath);
  fFramePath=fTempPath;
  fFramePath+="/frames";
  if(!std::filesystem::exists(fFramePath))std::filesystem::create_directory(fFramePath);
  fFacePath=fTempPath;
  fFacePath+="/faces";
  if(!std::filesystem::exists(fFacePath))std::filesystem::create_directory(fFacePath);
}

void FaceTools::Find_Faces() {
  //Need logic so files which have been checked aren't rechecked.
  //Periodically check the thumb folder for created snaps and run face detection on them.
  for(auto& p: std::filesystem::directory_iterator(fFramePath))
    if(p.is_regular_file()) 
    {
      std::stringstream ss(p.path().stem());
      std::string item1,item2;
      std::getline(ss,item1,'_');
      int vid = atoi(item1.c_str());
      std::getline(ss,item2);
      int ts = atoi(item2.c_str());
      dlib::array2d<unsigned char> img;
      dlib::load_image(img, p.path());
      dlib::pyramid_up(img);
      std::vector<dlib::rectangle> dets = detector(img);
      if(dets.size() > 0)std::cout << "VID ts rect1x rect2x" << vid << " " << ts << " " <<dets[0].top() << " " <<dets[0].left() << std::endl;
      //build faces images before deleting
      std::filesystem::remove(p.path());
    }
  return;
}
