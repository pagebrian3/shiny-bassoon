#ifndef FACETOOLS_H
#define FACETOOLS_H
#include <dlib/image_processing/frontal_face_detector.h>
#include <cxxpool.h>
#include <filesystem>

class VidFile;
class video_utils;

class FaceTools {

public:

  FaceTools(video_utils * vu);
  
  void Find_Faces();

  bool face_job(std::filesystem::path p);

  void retrain();

  std::filesystem::path get_face_path() { return fFacePath; };

private:
  
  video_utils * fVU;
  std::vector<std::future<bool> > resVec;
  std::filesystem::path fFacePath,fFramePath,fTrainPath,fModelPath;
  std::vector<dlib::frontal_face_detector> detectors;
  std::set<int> available_fds;
  cxxpool::thread_pool * TPool;
  std::vector<VidFile*>  fVideos;
  
};

#endif // FACETOOLS_H 
