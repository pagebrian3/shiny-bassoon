#include "opencv2/videoio.hpp"
#include "opencv2/video.hpp"
#include "opencv2/opencv.hpp"
#include "Magick++.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include <string>
#include <vector>

#define THUMB_DIR "/home/ungermax/vthumb_temp/"
#define START_TIME 2

Magick::Image Mat2Magick(cv::Mat& src);

void create_thumb(boost::filesystem::directory_entry &path);

bool process_image(Magick::Image & image);

int main(int argc, char* argv[])
{
  Magick::InitializeMagick(*argv);
  
  std::set<std::string> extensions{".mp4",".wmv",".mov",".rm",".m4v",".flv",".avi",".qt",".mpg",".mpeg",".mpv",".3gp"};
  std::vector<std::string> vid_files;
  std::map<std::pair<int,int>,int> results;
 
  std::string extension;
  boost::filesystem::path p(argv[1]);
  boost::filesystem::create_directory(THUMB_DIR);
  
  for (boost::filesystem::directory_entry & x : boost::filesystem::recursive_directory_iterator(p))
    {
      std::cout << x << std::endl;
      if(is_directory(x)) {
	std::string new_dir = THUMB_DIR+x.path().stem().generic_string();
	boost::filesystem::create_directory(new_dir);
      }
      extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(extensions.count(extension)) {
	vid_files.push_back(x.path().string());
	create_thumb(x);
      }		     
    }
  
  for(int i = 0; i < vid_files.size()-1; i++)
    for(int j = i+1; j < vid_files.size(); j++) {
      
    }
      
}

void create_thumb(boost::filesystem::directory_entry & path) {
  std::cout << path << std::endl;
  cv::Mat frame;
  cv::Mat a;
  cv::VideoCapture vCapt(path.path().c_str());
  bool result;
  float time_pos;
  float time_scale=1000.0;
  float next = 1.0;
  while(true)
    {     
      vCapt >> frame;
      if (frame.empty()) break;
      time_pos = vCapt.get(cv::CAP_PROP_POS_MSEC);
      if(time_pos >= time_scale*START_TIME) {
	Magick::Image image= Mat2Magick(frame);
	if(!process_image(image)) next+=1.0;
	else
	  {
	    std::string out_path=THUMB_DIR+path.path().stem().string()+".jpg";
	    std::cout<<"OUT PATH " <<out_path << std::endl;
	    image.write(out_path.c_str());
	    return;
	  }
      }
    }
  return;
}

bool process_image(Magick::Image &image)
{
  image.trim();
  if(image.size().width() < 10 || image.size().height() < 10) {
    std::cout << "Blank Frame" <<std::endl;
    return false;
  }
  image.resize("320x200");
  return true;
}

Magick::Image Mat2Magick(cv::Mat& src)
{
  Magick::Image mgk(src.cols, src.rows, "BGR", Magick::CharPixel, (char *)src.data);
   return mgk;
}
