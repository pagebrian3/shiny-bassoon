#include <VBrowser.h>
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

typedef struct {
  std::string fileName;
  double size;
  double length;
  bool okflag;
  int vdatid;
} vid_file;

namespace bfs = boost::filesystem;

int iconHeight = 180;
int iconWidth = 320;
int width = 640;
int height = 480;
int thumb_time = 12000;
int trace_time = 10.0;
int comp_time = 20;
double trace_fps= 15.0;
double border_frames = 20.0;
double thresh = 1000.0*comp_time*trace_fps;
double black_thresh = 1.0;
int fudge = 8;
int slice_spacing = 60;
int PROCESSES = 1;
int FUZZ=5000;
std::string default_sort = "size";
bool default_desc = true;
std::vector<std::string> vExts = { ".mp4", ".mov",".mpg",".mpeg",".wmv",".m4v", ".avi", ".flv" };
std::string home_dir="~";//os.path.expanduser('~')
std::string app_path = std::strcat(home_dir,"/.video_proj");
if (!boost::filesystem::is_directory(app_path)) bfs::create_directory(app_path);
bfs::path db_file = app_path;
db_file+="/vdb.sql";
bfs::path db_file_temp= app_path;
db_file_temp+="/temp.sql";
bfs::path temp_icon = app_path;
temp_icon+="/temp.jpg";
bfs::path directory("/home/ungermax/test_vids/");
DbConnector * dbCon = new DbConnector(app_path);
vid = dbCon->get_last_vid();

std::vector<int> calculate_trace(vid_file & file) {
  bfs::path path(file.fileName);
  bfs::path temp_dir = bfs::unique_path();
  bfs::path home = app_path;
  home+=temp_dir;
  bfs::create_directory(home);
  boost::process::ipstream is;
  boost::process::system((boost::format("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 %s" ) % path).str().c_str(),boost::process::std_out > is);
  std::string outString;
  std::getline(is,outString);
  double length = std::stod(outString);
  double start_time = TRACE_TIME;
  if(length <= TRACE_TIME) start_time=0.0; 
  double frame_spacing = (length-start_time)/BORDER_FRAMES;
  //std::cout <<(boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %.03d -vframes 1 %s") % path % start_time % imgp ).str().c_str() << std::endl;
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %.03d -vframes 1 %s") % path % start_time % imgp ).str().c_str());
  std::vector<bfs::directory_entry> bmpList;
  MagickWandGenesis();
  MagickWand *image_wand1=NewMagickWand();
  MagickWand *image_wand2=NewMagickWand();
  PixelIterator* iterator;
  PixelWand ** pixels;
  MagickReadImage(image_wand2,imgp);
  MagickPixelPacket pixel;
  std::vector<double> blah;
  unsigned long width,height;
  register long x;
  long y;
  height = MagickGetImageHeight(image_wand2);
  width = MagickGetImageWidth(image_wand2);
  //std::cout << "BLAH " << width <<" "<<height<<std::endl;
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
    iterator = NewPixelIterator(image_wand1);
    for (y=0; y < height; y++)
      {
	//std::cout << x <<" "<<y << std::endl;
	pixels=PixelGetNextIteratorRow(iterator,&width);
	if (pixels == (PixelWand **) NULL) break;
	for (x=0; x < (long) width; x++)
	  {
	    PixelGetMagickColor(pixels[x],&pixel);
	    double value =sqrt(1.0/3.0*(pow(pixel.red,2.0)+pow(pixel.green,2.0)+pow(pixel.blue,2.0)));
	    rowSums[y]+=corrFactorRow*value;
	    colSums[x]+=corrFactorCol*value;
	  }
	//PixelSyncIterator(iterator);
      }
    //iterator=DestroyPixelIterator(iterator);
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
  //std::cout << x2 << " "<<y2 <<" "<<x1 <<" "<<y1 << std::endl;
  bfs::remove(imgp);
  //std::cout << (boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %d -vf fps=%d -filter \"crop=%i:%i:%i:%i,scale=2x2\" %s/out%%05d%s") % path % TRACE_TIME % TRACE_FPS % x2 % y2 % x1 % y1 %home % EXTENSION).str().c_str();
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -i \"%s\" -ss %d -vf fps=%d -filter \"crop=%i:%i:%i:%i,scale=2x2\" %s/out%%05d%s") % path % TRACE_TIME % TRACE_FPS % x2 % y2 % x1 % y1 %home % EXTENSION).str().c_str());
  bmpList.clear();
  bfs::path p(home);
  for (bfs::directory_entry & x : bfs::recursive_directory_iterator(p))  bmpList.push_back(x);
  std::sort(bmpList.begin(),bmpList.end());
  int listSize = bmpList.size();
  std::vector<int> data(12*listSize);
  int i = 0;
  double scale = 1.0/256.0;
  ClearMagickWand(image_wand1);
  for(auto tFile: bmpList) {
    auto status = MagickReadImage(image_wand1,tFile.path().c_str());
    iterator = NewPixelIterator(image_wand1);
    for (y=0; y < 2; y++)
      {
	pixels=PixelGetNextIteratorRow(iterator,&width);
	if (pixels == (PixelWand **) NULL) 
	  break;
	for (x=0; x < 2; x++)
	  {
	    PixelGetMagickColor(pixels[x],&pixel);    
	    data[12*i+6*y+3*x]=scale*pixel.red;
	    data[12*i+6*y+3*x+1]=scale*pixel.green;
	    data[12*i+6*y+3*x+2]=scale*pixel.blue;
	  }
	//PixelSyncIterator(iterator);
      }
    //iterator=DestroyPixelIterator(iterator);
    i++;
  }
  data.append(vid);
  return data;
}


int main (int argc, char *argv[])
{
  auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

  VBrowser vbrowser;

  //Shows the window and returns when it is closed.
  return app->run(vbrowser);
}
