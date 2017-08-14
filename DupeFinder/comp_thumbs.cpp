#include "Magick++.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <set>

#define START_TIME 2
#define TRACE_TIME 10.0
#define TRACE_FPS 15.0
#define BORDER_FRAMES 20.0
#define CUT_THRESH 1000.0

void create_trace(boost::filesystem::directory_entry &path);

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
  boost::process::ipstream is;
  boost::process::system((boost::format("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 %s" ) % path).str().c_str(),boost::process::std_out > is);
  std::string outString;
  std::getline(is,outString);
  double length = std::stod(outString);
  double fps = BORDER_FRAMES/(length-TRACE_TIME);
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %s -i \"%s\" -r %d  %s/border%%05d.bmp") % TRACE_TIME % path % fps%home).str().c_str());
  boost::filesystem::path p(home);
  std::vector<boost::filesystem::directory_entry> bmpList;
  for (boost::filesystem::directory_entry & x : boost::filesystem::recursive_directory_iterator(p))  bmpList.push_back(x);
  std:sort(bmpList.begin(),bmpList.end());
  Magick::Image a;
  Magick::Image b;
  b.read(bmpList[0].path().c_str());
  Magick::Geometry geom = b.size();
  std::vector<double> rowSums(geom.height());
  std::vector<double> colSums(geom.width());
  double corrFactorCol = 1.0/(double)(bmpList.size()*geom.height());
  double corrFactorRow = 1.0/(double)(bmpList.size()*geom.width());
  Magick::Color aC, bC;
  for(int i = 1; i < bmpList.size()-1; i++) {
    a = b;
    b.read(bmpList[i].path().c_str());
    a.composite(b,0,0,Magick::DifferenceCompositeOp);
    for(int j = 0; j < geom.height(); j++)for(int k = 0; k < geom.width(); k++){
	aC=a.pixelColor(k,j);
	double value =sqrt(1.0/3.0*(pow(aC.redQuantum(),2.0)+pow(aC.greenQuantum(),2.0)+pow(aC.blueQuantum(),2.0)));
	rowSums[j]+=corrFactorRow*value;
	colSums[k]+=corrFactorCol*value;
      }
  }
  int x1(0), x2(geom.width()), y1(0), y2(geom.height());
  while(colSums[x1] < CUT_THRESH) x1++;
  while(colSums[x2] < CUT_THRESH) x2--;
  while(rowSums[y1] < CUT_THRESH) y1++;
  while(rowSums[y2] < CUT_THRESH) y2--;
  x2-=x1;
  y2-=y1;
  std::system((boost::format("rm %s/*.bmp") % home).str().c_str());
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %s -i \"%s\" -r %d -vf \"crop=%i:%i:%i:%i\",scale=2x2 %s/out%%05d.bmp") % TRACE_TIME % path % TRACE_FPS % x2 % y2 % x1 % y1 %home).str().c_str());
  bmpList.clear();
  for (boost::filesystem::directory_entry & x : boost::filesystem::recursive_directory_iterator(p))  bmpList.push_back(x);
  std::sort(bmpList.begin(),bmpList.end());
  int listSize = bmpList.size();
  std::vector<int> data;
  Magick::Image temp;
  for(auto a: bmpList) {
    temp.read(a.path().c_str());
    for(int j = 0; j < 2; j++) for(int k = 0; k < 2; k++) { 
	aC=a.pixelColor(j,k);
	data[6*j+3*k]=aC.redQuantum();
	data[6*j+3*k+1]=aC.greenQuantum();
	data[6*j+3*k+2]=aC.blueQuantum();
      }
  }
  return;
}




