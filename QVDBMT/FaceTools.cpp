#include "FaceTools.h"
#include "VideoUtils.h"
#include "QVBConfig.h"
#include <Magick++.h>
#include <boost/format.hpp>
#define DLIB_PNG_SUPPORT
#include <dlib/image_io.h>

pthread_mutex_t fLock1;

FaceTools::FaceTools(video_utils * vu) : fVU(vu) {
  TPool = fVU->get_tpool();
  std::filesystem::path tempPath = fVU->get_temp_path();
  tempPath+="face_temp";
  if(!std::filesystem::exists(tempPath))std::filesystem::create_directory(tempPath);
  fFramePath=tempPath;
  fFramePath+="/frames";
  if(!std::filesystem::exists(fFramePath))std::filesystem::create_directory(fFramePath);
  fFacePath=tempPath;
  fFacePath+="/faces/";
  if(!std::filesystem::exists(fFacePath))std::filesystem::create_directory(fFacePath);
  fTrainPath = fVU->save_path();
  fTrainPath += "/train";
  fModelPath = fVU->save_path();
  fModelPath += "model.keras";
  Magick::InitializeMagick("");
}

void FaceTools::Find_Faces() {
  for(auto& p: std::filesystem::directory_iterator(fFramePath))
    if(p.is_regular_file())
      resVec.push_back(TPool->push([&](std::filesystem::path p){ return face_job(p);},p.path())); 
  std::string command((boost::format("python FaceClassifier.py %i %s %s %f") % fVU->get_config()->get_int("face_size") % fFacePath % fModelPath % fVU->get_config()->get_float("confidence_thresh")).str());
  system(command.c_str());
  return;
}

void FaceTools::retrain() {
  std::string command((boost::format("python FaceTrainer.py %i %s") % fVU->get_config()->get_int("face_size") % fTrainPath).str());
  system(command.c_str());
  std::filesystem::path classPath = fVU->save_path();
  classPath += "classes.txt";
  std::filesystem::path current = std::filesystem::current_path();
  current += "/model.keras";
  std::filesystem::rename(current,fModelPath);
  current = std::filesystem::current_path();
  current += "/classes.txt";
  std::filesystem::rename(current,classPath);
  return;
}

bool FaceTools::face_job(std::filesystem::path p) {
  int detNum;
  dlib::frontal_face_detector detector;
  pthread_mutex_lock(&fLock1);
  if(available_fds.size() == 0) {
    detNum=detectors.size();
    detectors.push_back(dlib::get_frontal_face_detector());
    detector=detectors.back();
  }
  else {
    detNum = *(available_fds.begin());
    detector = detectors[detNum];
    available_fds.erase(detNum);
  }
  pthread_mutex_unlock(&fLock1);
  int resize_thresh = 480;
  dlib::array2d<unsigned char> img;
  if(std::filesystem::exists(p)) dlib::load_image(img, p);
  else return false;
  float heightCorr=img.nr();
  float widthCorr=img.nc();
  if(heightCorr < resize_thresh || widthCorr < resize_thresh) dlib::pyramid_up(img);
  heightCorr/=(float)img.nr();
  widthCorr/=(float)img.nc();
  std::vector<dlib::rectangle> dets;
  try {
    dets = detector(img);
  }
  catch(...){
    return false;
  }
  /*std::stringstream ss(p.stem());
  std::string item1,item2;
  std::getline(ss,item1,'_');
  int vid = atoi(item1.c_str());
  std::getline(ss,item2);
  int ts = atoi(item2.c_str());
  if(dets.size() > 0)std::cout << "VID ts rect1x rect2x " << vid << " " << ts << " top " <<dets[0].top() << " left " <<dets[0].left() << " bottom " << dets[0].bottom() << " right " << dets[0].right() <<" "<<heightCorr << " " << widthCorr << " " <<img.nr()<< " " << img.nc()<<  std::endl; */
  //build faces images before deleting
  int faceCt = 0;
  for(auto & rect: dets) {
    Magick::Image mgk;
    mgk.read(p.c_str());
    mgk.repage();
    int cropW = widthCorr*(rect.right()-rect.left());
    int cropH = heightCorr*(rect.bottom()-rect.top());
    mgk.crop(Magick::Geometry(cropW,cropH,widthCorr*rect.left(),heightCorr*rect.top()));
    std::filesystem::path faceIcon(fFacePath);
    faceIcon+=p.stem();
    faceIcon+=(boost::format("_%i.png") % faceCt).str();
    mgk.write(faceIcon.c_str());
    faceCt++;
  }
  pthread_mutex_lock(&fLock1);
  available_fds.insert(detNum);
  pthread_mutex_unlock(&fLock1);
  std::filesystem::remove(p);
  return true;
}

