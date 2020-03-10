#include <list>
#include "FaceTools.h"
#include "VidFile.h"
#include "VideoUtils.h"
#include <Magick++.h>
#include <boost/format.hpp>
#define DLIB_PNG_SUPPORT
#include <dlib/image_io.h>


FaceTools::FaceTools(video_utils * vu) : fVU(vu) {
  detector = dlib::get_frontal_face_detector();
  TPool = fVU->get_tpool();
  fTempPath = fVU->get_temp_path();
  fTempPath+="face_temp";
  if(!std::filesystem::exists(fTempPath))std::filesystem::create_directory(fTempPath);
  fFramePath=fTempPath;
  fFramePath+="/frames";
  if(!std::filesystem::exists(fFramePath))std::filesystem::create_directory(fFramePath);
  fFacePath=fTempPath;
  fFacePath+="/faces/";
  if(!std::filesystem::exists(fFacePath))std::filesystem::create_directory(fFacePath);
  Magick::InitializeMagick("");
}

void FaceTools::Find_Faces() {
  //Need logic so files which have been checked aren't rechecked.
  //Periodically check the thumb folder for created snaps and run face detection on them.
  for(auto& p: std::filesystem::directory_iterator(fFramePath))
    if(p.is_regular_file()) 
    {
      int resize_thresh = 480;
      dlib::array2d<unsigned char> img;
      dlib::load_image(img, p.path());
      float heightCorr=img.nr();
      float widthCorr=img.nc();
      if(heightCorr < resize_thresh || widthCorr < resize_thresh) dlib::pyramid_up(img);
      heightCorr/=(float)img.nr();
      widthCorr/=(float)img.nc();
      std::vector<dlib::rectangle> dets = detector(img);
      std::stringstream ss(p.path().stem());
      std::string item1,item2;
      std::getline(ss,item1,'_');
      int vid = atoi(item1.c_str());
      std::getline(ss,item2);
      int ts = atoi(item2.c_str());
      if(dets.size() > 0)std::cout << "VID ts rect1x rect2x " << vid << " " << ts << " top " <<dets[0].top() << " left " <<dets[0].left() << " bottom " << dets[0].bottom() << " right " << dets[0].right() <<" "<<heightCorr << " " << widthCorr << " " <<img.nr()<< " " << img.nc()<<  std::endl; 
      //build faces images before deleting
      int faceCt = 0;
      for(auto & rect: dets) {
	Magick::Image mgk;
	mgk.read(p.path().c_str());
	mgk.repage();
        int cropW = widthCorr*(rect.right()-rect.left());
	int cropH = heightCorr*(rect.bottom()-rect.top());
	mgk.crop(Magick::Geometry(cropW,cropH,widthCorr*rect.left(),heightCorr*rect.top()));
	std::filesystem::path faceIcon(fFacePath);
	faceIcon+=p.path().stem();
	faceIcon+=(boost::format("_%i.png") % faceCt).str();
	mgk.write(faceIcon.c_str());
	faceCt++;
      }
      //This is causing the cutoff faces for some reason.
      std::filesystem::remove(p.path());
    }
  return;
}

