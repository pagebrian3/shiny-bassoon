#include "Magick++.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <set>

#define START_TIME 2
#define TRACE_TIME "00:00:10"
#define TRACE_FPS 15.0
#define BORDER_FPS 0.5

void create_trace(boost::filesystem::directory_entry &path);

bool process_image(Magick::Image & image);

int main(int argc, char* argv[])
{
  Magick::InitializeMagick(*argv);
  
  std::set<std::string> extensions{".mp4",".wmv",".mov",".rm",".m4v",".flv",".avi",".qt",".mpg",".mpeg",".mpv",".3gp"};
  std::vector<std::string> vid_files;
  std::map<std::pair<int,int>,int> results;
 
  std::string extension;
  boost::filesystem::path p(argv[1]);
  
  for (boost::filesystem::directory_entry & x : boost::filesystem::recursive_directory_iterator(p))
    {
      std::cout << x << std::endl;
      extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(extensions.count(extension)) {
	vid_files.push_back(x.path().string());
	create_trace(x);
      }		     
    }
  
  for(int i = 0; i < vid_files.size()-1; i++)
    for(int j = i+1; j < vid_files.size(); j++) {
      
    }
  return 0;
}

void create_trace(boost::filesystem::directory_entry & path) {
  bool result;
  float time_pos;
  float time_scale=1000.0;
  float next = 1.0;
  boost::filesystem::path temp_dir = boost::filesystem::unique_path();
  std::cout <<boost::filesystem::temp_directory_path()<<"/" <<temp_dir << std::endl;
  char home[100]="/home/ungermax/blah/";
  std::strcat(home,temp_dir.c_str());
  if(boost::filesystem::create_directory(home)) std::cout << "CREATED" << std::endl;
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %s -i \"%s\" -r %d  %s/border%%05d.bmp") % TRACE_TIME % path % BORDER_FPS%home).str().c_str());
  boost::filesystem::path p(home);
  std::vector<boost::filesystem::directory_entry> bmpList;
  for (boost::filesystem::directory_entry & x : boost::filesystem::recursive_directory_iterator(p))  bmpList.push_back(x);
  std:sort(bmpList.begin(),bmpList.end());
  for(int i = 0; i < bmpList.size()-1; i++) {
    Magick::Image a(bmpList[i].path().c_str());
    Magick::Image b(bmpList[i+1].path().c_str());
    a.composite(b,0,0,Magick::DifferenceCompositeOp);
    a.write((boost::format("%s/diff%05d.bmp") % home % i).str());
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


