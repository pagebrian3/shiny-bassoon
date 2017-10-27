#include "VideoIcon.h"
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <wand/MagickWand.h>

VideoIcon::VideoIcon(std::string fileName, DbConnector * dbCon, po::variables_map *vm):Gtk::Image()  {
  if(dbCon->video_exists(fileName)) {
    fVidFile = dbCon->fetch_video(fileName);
    dbCon->fetch_icon(fVidFile->vid);
    std::string icon_file((boost::format("%s%i.jpg") % (*vm)["app_path"].as<std::string>() % fVidFile->vid).str());
    this->set(icon_file);
    std::system((boost::format("rm %s") %icon_file).str().c_str());
    hasIcon = true;
  }
  else {
    hasIcon=false;
    this->set_from_icon_name("missing-image",Gtk::ICON_SIZE_BUTTON);
    fVidFile = new VidFile();
    fVidFile->fileName=fileName;
    fVidFile->size = bfs::file_size(fileName);
    boost::process::ipstream is;
    std::string cmd((boost::format("./info.sh %s") % fileName).str());
    boost::process::system(cmd,  boost::process::std_out > is);
    std::string outString;
    std::getline(is, outString);
    std::vector<std::string> split_string;
    boost::split(split_string,outString,boost::is_any_of(","));
    int width=std::stoi(split_string[0]);
    int height=std::stoi(split_string[1]);
    double length=0.001*(double)std::stoi(split_string[2]);
    int rotate = 0;
    if(split_string[3].length() > 0) int rotate = std::stod(split_string[3]);
    /*if(rotate == 90 || rotate ==-90) {  //not sure if this is needed
      int temp = width;
      width=height;
      height=temp;
      }*/
    fVidFile->length = length;
    dbCon->save_video(fVidFile);    
  }
  int size = fVidFile->size/1024;
  std::string toolTip((boost::format("Filename: %s\nSize: %ikB\nLength: %is") % fileName %  size % fVidFile->length).str());
  this->set_tooltip_text(toolTip);
};

VideoIcon::~VideoIcon(){
  delete fVidFile;
};

VidFile * VideoIcon::get_vid_file() {
  return fVidFile;
};

bool  VideoIcon::create_thumb(DbConnector * dbCon, po::variables_map *vm) {
  if(hasIcon) return true;
  std::string crop(find_border(fVidFile->fileName, fVidFile->length, vm));
  double thumb_t = (*vm)["thumb_time"].as<float>();
  if(fVidFile->length < thumb_t) thumb_t = fVidFile->length/2.0;
  if(crop.length() != 0) crop.append(",");
  fVidFile->crop=crop;
  dbCon->save_crop(fVidFile);
  std::string cmd((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.03d -i %s -frames:v 1 -filter:v \"%sscale=w=%i:h=%i:force_original_aspect_ratio=decrease\" %s%i.jpg") % thumb_t % fVidFile->fixed_filename()% crop  % (*vm)["thumb_width"].as<int>()% (*vm)["thumb_height"].as<int>() % (*vm)["app_path"].as<std::string>() % fVidFile->vid).str());
  std::system(cmd.c_str());
  std::string icon_file((boost::format("%s%i.jpg") % (*vm)["app_path"].as<std::string>() % fVidFile->vid).str());
  this->set(icon_file);
  dbCon->save_icon(fVidFile->vid);
  std::system((boost::format("rm %s") %icon_file).str().c_str());
  return true;
};

std::string VideoIcon::find_border(std::string fileName,float length, po::variables_map * vm) {
  
  std::string crop("");
  bfs::path path(fileName);
  bfs::path imgp = getenv("HOME");
  imgp+="/.video_proj/";
  imgp+=bfs::unique_path();
  imgp+=".png";
  float start_time = (*vm)["trace_time"].as<float>();
  if(length <= start_time) start_time=0.0;
  float frame_time = start_time;
  float border_frames=(*vm)["border_frames"].as<float>();
  float cut_thresh=(*vm)["cut_thresh"].as<float>();
  float frame_spacing = (length-start_time)/border_frames;
  std::string cmdTmpl("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -frames:v 1 %s");
  std::string command((boost::format(cmdTmpl)% start_time % fileName % imgp ).str());
  
  std::system(command.c_str());
  MagickWand *image_wand1;
  MagickWand *image_wand2=NewMagickWand();
  PixelIterator* iterator;
  PixelWand ** pixels;
  bool ok = MagickReadImage(image_wand2,imgp.string().c_str());
  ExceptionType severity;
  MagickPixelPacket pixel;
  unsigned long width,height;
  register long x;
  long y;
  height = MagickGetImageHeight(image_wand2);
  width = MagickGetImageWidth(image_wand2);
  std::vector<double> rowSums(height);
  std::vector<double> colSums(width);
  double corrFactorCol = 1.0/(double)(border_frames*height);
  double corrFactorRow = 1.0/(double)(border_frames*width);
  bool skipBorder = false;
  for(int i = 1; i < border_frames; i++) {
    image_wand1 = CloneMagickWand(image_wand2);
    frame_time+=frame_spacing;
    std::system((boost::format(cmdTmpl) % frame_time % fileName  % imgp ).str().c_str());
    MagickReadImage(image_wand2,imgp.string().c_str());
    MagickCompositeImage(image_wand1,image_wand2,DifferenceCompositeOp, 0,0);
    iterator = NewPixelIterator(image_wand1);
    for (y=0; y < height; y++) {
      pixels=PixelGetNextIteratorRow(iterator,&width);
      if (pixels == (PixelWand **) NULL) break;
      for (x=0; x < (long) width; x++) {
	PixelGetMagickColor(pixels[x],&pixel);
	double value =sqrt(1.0/3.0*(pow(pixel.red,2.0)+pow(pixel.green,2.0)+pow(pixel.blue,2.0)));
	rowSums[y]+=corrFactorRow*value;
	colSums[x]+=corrFactorCol*value;
      }
    }
    if(rowSums[0] > cut_thresh &&
       rowSums[height-1] > cut_thresh &&
       colSums[0] > cut_thresh &&
       colSums[width-1] > cut_thresh) {
      skipBorder=true;
      break;
    }
    iterator=DestroyPixelIterator(iterator);
    image_wand1=DestroyMagickWand(image_wand1);
  }
  if(!skipBorder) {
    int x1(0), x2(width-1), y1(0), y2(height-1);
    while(colSums[x1] < cut_thresh) x1++;
    while(colSums[x2] < cut_thresh) x2--;
    while(rowSums[y1] < cut_thresh) y1++;
    while(rowSums[y2] < cut_thresh) y2--;
    x2-=x1-1;  
    y2-=y1-1;
    crop = (boost::format("crop=%i:%i:%i:%i")% x2 % y2 % x1 % y1).str();
  }
  bfs::remove(imgp);
  image_wand2=DestroyMagickWand(image_wand2);
  return crop;
}
