#include "Magick++.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <set>

#define TRACE_TIME 10.0
#define TRACE_FPS 15.0
#define BORDER_FRAMES 20.0
#define CUT_THRESH 1000.0

int main(int argc, char* argv[])
{
  Magick::InitializeMagick(*argv);
  boost::filesystem::path path(argv[1]);
  boost::filesystem::path temp_dir = boost::filesystem::unique_path();
  char home[100]="/home/ungermax/.video_proj/";
  std::strcat(home,temp_dir.c_str());
  boost::filesystem::create_directory(home);
  boost::process::ipstream is;
  boost::process::system((boost::format("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 %s" ) % path).str().c_str(),boost::process::std_out > is);
  std::string outString;
  std::getline(is,outString);
  double length = std::stod(outString);
  double start_time = TRACE_TIME;
  if(length <= TRACE_TIME) start_time=0.0; 
  double fps = BORDER_FRAMES/(length-start_time);
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %s -i \"%s\" -r %d  %s/border%%05d.bmp") % start_time % path % fps % home).str().c_str());
  boost::filesystem::path p(home);
  std::vector<boost::filesystem::directory_entry> bmpList;
  for (boost::filesystem::directory_entry & x : boost::filesystem::recursive_directory_iterator(p))  bmpList.push_back(x);
  std:sort(bmpList.begin(),bmpList.end());
  Magick::Image a;
  Magick::Image b;
  b.read(bmpList[0].path().c_str());
  Magick::Geometry geom = b.size();
  std::vector<double> * rowSums = new std::vector<double>(geom.height());
  std::vector<double> * colSums = new std::vector<double>(geom.width());
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
	(*rowSums)[j]+=corrFactorRow*value;
	(*colSums)[k]+=corrFactorCol*value;
      }
    if((*rowSums)[0] > CUT_THRESH &&(*rowSums)[geom.height()-1] > CUT_THRESH && (*colSums)[0] > CUT_THRESH && (*colSums)[geom.width()-1] > CUT_THRESH) {
      //std::cout << "Broke! " << i << std::endl;
      break;
    }
  }
  int x1(0), x2(geom.width()-1), y1(0), y2(geom.height()-1);
  while((*colSums)[x1] < CUT_THRESH) x1++;
  while((*colSums)[x2] < CUT_THRESH) x2--;
  while((*rowSums)[y1] < CUT_THRESH) y1++;
  while((*rowSums)[y2] < CUT_THRESH) y2--;
  delete rowSums;
  delete colSums;
  x2-=x1-1;  //the -1 corrects for the fact that x2 and y2 are sizes, not positions
  y2-=y1-1;
  std::system((boost::format("rm %s/*.bmp") % home).str().c_str());
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %s -i \"%s\" -r %d -vf \"crop=%i:%i:%i:%i\",scale=2x2 %s/out%%05d.bmp") % start_time % path % TRACE_FPS % x2 % y2 % x1 % y1 %home).str().c_str());
  bmpList.clear();
  for (boost::filesystem::directory_entry & x : boost::filesystem::recursive_directory_iterator(p))  bmpList.push_back(x);
  std::sort(bmpList.begin(),bmpList.end());
  int listSize = bmpList.size();
  std::vector<int> data(12*listSize);
  Magick::Image temp;
  int i = 0;
  double scale = 1.0/256.0;
  for(auto a: bmpList) {
    temp.read(a.path().c_str());
    for(int j = 0; j < 2; j++) for(int k = 0; k < 2; k++) { 
	aC=temp.pixelColor(j,k);
	data[12*i+6*j+3*k]=scale*aC.redQuantum();
	data[12*i+6*j+3*k+1]=scale*aC.greenQuantum();
	data[12*i+6*j+3*k+2]=scale*aC.blueQuantum();
      }
    i++;
  }
  char outfile[100]="trace";
  std::strcat(outfile,argv[2]);
  std::strcat(outfile,".txt");
  std::cout << outfile << " " << data.size() << std::endl;
  std::ofstream * outFile = new std::ofstream(outfile);
  for(auto a: data) *outFile << a <<"\t";
  outFile->close();
  delete outFile;
  boost::filesystem::remove(temp_dir);
  return 0;
}



