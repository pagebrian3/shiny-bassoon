#include "VideoUtils.h"
#include "VBrowser.h"
#include <boost/process.hpp>
#include <boost/format.hpp>
#include <opencv2/opencv.hpp>

video_utils::video_utils(DbConnector * dbCon1, po::variables_map * vm1) {
  vm = vm1;
  dbCon = dbCon1;
  cStartTime = (*vm)["trace_time"].as<float>();
  cTraceFPS = (*vm)["trace_fps"].as<float>();
  cCompTime = (*vm)["comp_time"].as<float>();
  cSliceSpacing = (*vm)["slice_spacing"].as<float>();
  cThresh = (*vm)["thresh"].as<float>();
  cFudge = (*vm)["fudge"].as<int>();
}

video_utils::video_utils(){}

bool video_utils::compare_vids(int i, int j, std::map<int, std::vector<unsigned short> > & data) {
  bool match=false;
  int counter=0;
  uint t_s,t_x, t_o, t_d;
  //loop over slices
  for(t_s =0; t_s < data[i].size()-12*cTraceFPS*cCompTime; t_s+= 12*cTraceFPS*cSliceSpacing){
    if(match) break;
    //starting offset for 2nd trace-this is the loop for the indiviual tests
    for(t_x=0; t_x < data[j].size()-12*cTraceFPS*cCompTime; t_x+=12){
      if(match) break;
      std::vector<int> accum(12);
      //offset loop
      for(t_o = 0; t_o < 12*cCompTime*cTraceFPS; t_o+=12){
	counter = 0;
	for(auto & a : accum) if (a > cThresh*cCompTime*cTraceFPS) counter+=1;
	if(counter != 0) break;
	//pixel/color loop
	for (t_d = 0; t_d < 12; t_d++) {
	  int value = pow((int)(data[i][t_s+t_o+t_d])-(int)(data[j][t_x+t_o+t_d]),2)-pow(cFudge,2);
	  if(value < 0) value = 0;
	  accum[t_d]+=value;
	}
      }
      counter = 0;
      for(auto & a: accum)  if(a < cThresh*cCompTime*cTraceFPS) counter+=1;
      if(counter == 12) match=true;
      if(match) std::cout << "ACCUM " <<i<<" " <<j <<" " <<t_o <<" slice " <<t_s <<" 2nd offset " <<t_x <<" " <<*max_element(accum.begin(),accum.end())  <<std::endl;
    }
  }
  int result = 1;
  if (match) result = 2;
  dbCon->update_results(i,j,result);
  return true;
}

bool video_utils::calculate_trace(VidFile * obj) {
  //std::cout << obj->vid << " Started" << std::endl;
  float start_time = cStartTime;
  if(obj->length <= start_time) start_time=0.0;
  boost::process::ipstream is;
  boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -filter:v \"fps=%.3f,%sscale=2x2\" -pix_fmt rgb24 -f image2pipe -vcodec rawvideo - ") % start_time % obj->fixed_filename()  % cTraceFPS % obj->crop).str(),boost::process::std_out > is);
  std::string outString;
  std::getline(is,outString);
  dbCon->save_trace(obj->vid, outString);
  return true; 
}

bool video_utils::create_thumb(VideoIcon * icon) {
  VidFile * vidFile = icon->get_vid_file();
  std::string crop(find_border(vidFile->fileName, vidFile->length));
  vidFile->crop=crop;
  dbCon->save_crop(vidFile);
  double thumb_t = (*vm)["thumb_time"].as<float>();
  if(vidFile->length < thumb_t) thumb_t = vidFile->length/2.0;
  cv::VideoCapture vCapt(vidFile->fileName);
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
    cv::Mat tempFrame = frame(cv::Range(y1,y2),cv::Range(x1,x2));
    frame = tempFrame.clone();
  }
  if(vidFile->rotate == 90) cv::rotate(frame,frame,cv::ROTATE_90_CLOCKWISE);
  else if(vidFile->rotate == -90) cv::rotate(frame,frame,cv::ROTATE_90_COUNTERCLOCKWISE);
  else if(vidFile->rotate == 180) cv::rotate(frame,frame,cv::ROTATE_180);
  cv::MatSize frameSize= frame.size;
  height = frameSize[0];
  width = frameSize[1];
  cv::Mat shrunkFrame;
  float factor = 1.0;
  float tWidth = (*vm)["thumb_width"].as<int>();
  float tHeight = (*vm)["thumb_height"].as<int>();
  if(width >0 && height > 0) {
    if(width/tWidth >= height/tHeight) factor = tWidth/width;
    else factor = tHeight/height;
  } 
  cv::resize(frame,shrunkFrame,cv::Size(0,0),factor,factor,cv::INTER_AREA);
  std::string icon_file((boost::format("%s%i.jpg") % (*vm)["app_path"].as<std::string>() % vidFile->vid).str());
  cv::imwrite(icon_file,shrunkFrame); 
  return true;
};

std::string video_utils::find_border(std::string fileName,float length) {
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





