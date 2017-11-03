#include "VideoIcon.h"
#include <opencv2/opencv.hpp>
#include <boost/format.hpp>
#include <boost/process.hpp>

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
    fVidFile->length = length;
    fVidFile->rotate = rotate;
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

bool VideoIcon::create_thumb(DbConnector * dbCon, po::variables_map *vm) {
  if(hasIcon){
    std::cout << "Has Icon" <<std::endl;
    return true;
  }
  std::string crop(find_border(fVidFile->fileName, fVidFile->length, vm));
  fVidFile->crop=crop;
  dbCon->save_crop(fVidFile);
  double thumb_t = (*vm)["thumb_time"].as<float>();
  if(fVidFile->length < thumb_t) thumb_t = fVidFile->length/2.0;
  cv::VideoCapture vCapt(fVidFile->fileName);
  cv::Mat frame;
  int x1,x2,y1,y2;
  vCapt.set(cv::CAP_PROP_POS_MSEC,thumb_t*1000.0);
  vCapt >> frame;
  int width, height;
 
  if(crop.length() != 0) {
    std::vector<std::string> split_string;
    boost::split(split_string,crop,boost::is_any_of(":"));
    int x2=std::stoi(split_string[0]);
    int y2=std::stoi(split_string[1]);
    int x1=std::stoi(split_string[2]);
    int y1=std::stoi(split_string[3]);
    cv::Mat tempFrame = frame(cv::Range(x1,x2),cv::Range(y1,y2));
    frame = tempFrame.clone();
  }
  if(fVidFile->rotate == -90) cv::rotate(frame,frame,cv::ROTATE_90_CLOCKWISE);
  else if(fVidFile->rotate == 90) cv::rotate(frame,frame,cv::ROTATE_90_COUNTERCLOCKWISE);
  else if(fVidFile->rotate == 180) cv::rotate(frame,frame,cv::ROTATE_180);
  cv::MatSize frameSize= frame.size;
  height = frameSize[0];
  width = frameSize[1];
  cv::Mat shrunkFrame;
  float factor = 1.0;
  float tWidth = (*vm)["thumb_width"].as<int>();
  float tHeight = (*vm)["thumb_height"].as<int>();
  std::cout << "HERE2" <<width<<" "<<height<< std::endl;
  if(width >0 && height > 0) {
    if(width/tWidth >= height/tHeight) factor = tWidth/width;
    else factor = tHeight/height;
  } 
  cv::resize(frame,shrunkFrame,cv::Size(0,0),factor,factor,cv::INTER_AREA);
  std::string icon_file((boost::format("%s%i.jpg") % (*vm)["app_path"].as<std::string>() % fVidFile->vid).str());
  std::cout <<"Here " <<icon_file << std::endl;
  cv::imwrite(icon_file,shrunkFrame); 
  this->set(icon_file);
  dbCon->save_icon(fVidFile->vid);
  //std::system((boost::format("rm %s") %icon_file).str().c_str());
  return true;
};

std::string VideoIcon::find_border(std::string fileName,float length, po::variables_map * vm) {
  std::string crop("");
  bfs::path path(fileName);
  float start_time = (*vm)["trace_time"].as<float>();
  if(length <= start_time) start_time=0.0;
  float frame_time = start_time;
  float border_frames=(*vm)["border_frames"].as<float>();
  float cut_thresh=(*vm)["cut_thresh"].as<float>();
  float frame_spacing = (length-start_time)/border_frames;
  cv::Mat frame;
  cv::Mat frame1;
  cv::VideoCapture vCapt(fileName);
  vCapt.set(cv::CAP_PROP_POS_MSEC,start_time*1000.0);
  vCapt >> frame;
  unsigned long width,height;
  register long x;
  long y;
  cv::MatSize frameSize= frame.size;
  height = frameSize[0];
  width = frameSize[1];
  //std::cout << "BLAH " << fileName << " "<<height << " "<<width<<std::endl;
  std::vector<double> rowSums(height);
  std::vector<double> colSums(width);
  double corrFactorCol = 1.0/(double)(border_frames*height);
  double corrFactorRow = 1.0/(double)(border_frames*width);
  bool skipBorder = false;
  cv::Vec3b intensity,intensity1;
  for(int i = 1; i < border_frames; i++) {
    frame1 = frame.clone();
    frame_time+=frame_spacing;
    vCapt.set(cv::CAP_PROP_POS_MSEC,frame_time*1000.0);
    vCapt >> frame;
    for (y=0; y < height; y++) {
      for (x=0; x < width; x++) {
	intensity=frame.at<cv::Vec3b>(y,x);
	intensity1=frame1.at<cv::Vec3b>(y,x);
	double value =sqrt(1.0/3.0*(pow((short)intensity.val[0]-(short)intensity1.val[0],2.0)+pow((short)intensity.val[1]-(short)intensity1.val[1],2.0)+pow((short)intensity.val[2]-(short)intensity1.val[2],2.0)));
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
  }
  if(!skipBorder) {
    int x1(0), x2(width-1), y1(0), y2(height-1);
    while(colSums[x1] < cut_thresh) x1++;
    while(colSums[x2] < cut_thresh) x2--;
    while(rowSums[y1] < cut_thresh) y1++;
    while(rowSums[y2] < cut_thresh) y2--;
    x2-=x1-1;  
    y2-=y1-1;
    crop = (boost::format("%i:%i:%i:%i")% x2 % y2 % x1 % y1).str();
  }
  return crop;
}
