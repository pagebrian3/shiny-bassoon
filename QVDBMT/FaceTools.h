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

private:
  
  video_utils * fVU;
  std::filesystem::path fTempPath,fFacePath,fFramePath;
  dlib::frontal_face_detector detector;
  cxxpool::thread_pool * TPool;
  std::vector<VidFile*>  fVideos;
  
};

#endif // FACETOOLS_H 
