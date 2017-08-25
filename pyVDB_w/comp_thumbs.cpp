#include <wand/MagickWand.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <set>

#define TRACE_TIME 10.0
#define TRACE_FPS 25.0
#define BORDER_FRAMES 20.0
#define CUT_THRESH 1000.0
#define EXTENSION ".png"
#define HOME_PATH "/home/ungermax/.video_proj/"
#define ThrowWandException(wand)					\
  {									\
    char								\
      *description;							\
									\
    ExceptionType							\
      severity;								\
									\
    description=MagickGetException(wand,&severity);			\
    (void) fprintf(stderr,"%s %s %ld %s\n",GetMagickModule(),description); \
    description=(char *) MagickRelinquishMemory(description);		\
    exit(-1);								\
  }

int main(int argc, char* argv[])
{
  boost::filesystem::path path(argv[1]);
  boost::filesystem::path temp_dir = boost::filesystem::unique_path();
  char home[100]=HOME_PATH;
  char img_path[100]="border.png";
  char imgp[100] =HOME_PATH;
  std::strcat(imgp,img_path);
  std::strcat(home,temp_dir.c_str());
  boost::filesystem::create_directory(home);
  boost::process::ipstream is;
  boost::process::system((boost::format("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 %s" ) % path).str().c_str(),boost::process::std_out > is);
  std::string outString;
  std::getline(is,outString);
  double length = std::stod(outString);
  double start_time = TRACE_TIME;
  if(length <= TRACE_TIME) start_time=0.0; 
  double frame_spacing = (length-start_time)/BORDER_FRAMES;
  std::cout <<(boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %.03d -vframes 1 %s") % path % start_time % imgp ).str().c_str() << std::endl;
    std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %.03d -vframes 1 %s") % path % start_time % imgp ).str().c_str());
  std::vector<boost::filesystem::directory_entry> bmpList;
  MagickWandGenesis();
  MagickWand *image_wand1=NewMagickWand();
  MagickWand *image_wand2=NewMagickWand();
  MagickBooleanType status=MagickReadImage(image_wand2,imgp);
  if (status == MagickFalse) ThrowWandException(image_wand1);
  MagickPixelPacket pixel;
  std::vector<double> blah;
  unsigned long width,height;
  register long x;
  long y;
  height = MagickGetImageHeight(image_wand2);
  width = MagickGetImageWidth(image_wand2);
  if (status == MagickFalse) ThrowWandException(image_wand2);
  std::cout << "BLAH " <<status<<" "<< width <<" "<<height<<std::endl;
  std::vector<double> rowSums(height);
  std::vector<double> colSums(width);
  double corrFactorCol = 1.0/(double)(BORDER_FRAMES*height);
  double corrFactorRow = 1.0/(double)(BORDER_FRAMES*width);
  for(int i = 1; i < BORDER_FRAMES; i++) {
    image_wand1 = CloneMagickWand(image_wand2);
    start_time+=frame_spacing;
    std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %d -vframes 1 %s") % path % start_time % imgp ).str().c_str());
    MagickReadImage(image_wand2,imgp);
    MagickCompositeImage(image_wand1,image_wand2,DifferenceCompositeOp, 0,0);
    PixelIterator* iterator = NewPixelIterator(image_wand1);
    for (y=0; y < height; y++)
      {
	//std::cout << x <<" "<<y << std::endl;
	PixelWand ** pixels=PixelGetNextIteratorRow(iterator,&width);
	if (pixels == (PixelWand **) NULL) break;
	for (x=0; x < (long) width; x++)
	  {
	    PixelGetMagickColor(pixels[x],&pixel);
	    double value =sqrt(1.0/3.0*(pow(pixel.red,2.0)+pow(pixel.green,2.0)+pow(pixel.blue,2.0)));	    
	    rowSums[y]+=corrFactorRow*value;
	    colSums[x]+=corrFactorCol*value;
	  }
      }
    if(rowSums[0] > CUT_THRESH &&
       rowSums[height-1] > CUT_THRESH &&
       colSums[0] > CUT_THRESH &&
       colSums[width-1] > CUT_THRESH) break;
  }
  int x1(0), x2(width-1), y1(0), y2(height-1);
  while(colSums[x1] < CUT_THRESH) x1++;
  while(colSums[x2] < CUT_THRESH) x2--;
  while(rowSums[y1] < CUT_THRESH) y1++;
  while(rowSums[y2] < CUT_THRESH) y2--;
  x2-=x1-1;  //the -1 corrects for the fact that x2 and y2 are sizes, not positions
  y2-=y1-1;
  std::cout << x2 << " "<<y2 <<" "<<x1 <<" "<<y1 << std::endl;
  std::cout << (boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %d -vf fps=%d -filter \"crop=%i:%i:%i:%i,scale=2x2\" %s/out%%05d%s") % path % TRACE_TIME % TRACE_FPS % x2 % y2 % x1 % y1 %home % EXTENSION).str().c_str();
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %s -vf fps=%d -filter \"crop=%i:%i:%i:%i,scale=2x2\" %s/out%%05d%s") % path % TRACE_TIME % TRACE_FPS % x2 % y2 % x1 % y1 %home % EXTENSION).str().c_str());
  bmpList.clear();
  boost::filesystem::path p(home);
  for (boost::filesystem::directory_entry & x : boost::filesystem::recursive_directory_iterator(p))  bmpList.push_back(x);
  std::sort(bmpList.begin(),bmpList.end());
  int listSize = bmpList.size();
  std::vector<int> data(12*listSize);
  int i = 0;
  double scale = 1.0/256.0;
  for(auto tFile: bmpList) {
    MagickReadImage(image_wand1,tFile.path().c_str());
    PixelIterator *iterator = NewPixelIterator(image_wand1);
    for (y=0; y < (long) MagickGetImageHeight(image_wand1); y++)
      {
	PixelWand ** pixels=PixelGetNextIteratorRow(iterator,&width);
	if (pixels == (PixelWand **) NULL) 
	  break;
	for (x=0; x < (long) width; x++)
	  {
	    PixelGetMagickColor(pixels[x],&pixel);
	    data[12*i+6*y+3*x]=scale*pixel.red;
	    data[12*i+6*y+3*x+1]=scale*pixel.green;
	    data[12*i+6*y+3*x+2]=scale*pixel.blue;
	  }   
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
  //boost::filesystem::remove(temp_dir);
  return 0;
}

