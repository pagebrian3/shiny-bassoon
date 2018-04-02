#include "VideoUtils.h"
#include <fstream>
#include <boost/process.hpp>
#include <boost/format.hpp>

video_utils::video_utils(DbConnector * dbCon1, po::variables_map * vm1, bfs::path temp) {
  vm = vm1;
  dbCon = dbCon1;
  tempPath = temp;
  Magick::InitializeMagick("");
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
  float start_time = cStartTime;
  if(obj->length <= start_time) start_time=0.0;
  boost::process::ipstream is;
  boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -filter:v \"fps=%.3f,%sscale=2x2\" -pix_fmt rgb24 -f image2pipe -vcodec rawvideo - ") % start_time % obj->fixed_filename()  % cTraceFPS % obj->crop).str(),boost::process::std_out > is);
  std::string outString;
  std::getline(is,outString);
  dbCon->save_trace(obj->vid, outString);
  return true; 
}

bool video_utils::create_thumb(VidFile * vidFile) {
  std::string crop(find_border(vidFile->fixed_filename(), vidFile->length));
  vidFile->crop=crop;
  dbCon->save_crop(vidFile);
  double thumb_t = (*vm)["thumb_time"].as<float>();
  int height_t = (*vm)["thumb_height"].as<int>();
  int width_t = (*vm)["thumb_width"].as<int>();
  if(vidFile->length < thumb_t) thumb_t = vidFile->length/2.0;
  int x1,x2,y1,y2;
  int width, height;
  std::string icon_file((boost::format("%s%i.jpg") % (*vm)["app_path"].as<std::string>() % vidFile->vid).str());
  boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -frames:v 1 -filter:v \"%sscale=w=%d:h=%d:force_original_aspect_ratio=decrease\" %s") % thumb_t % vidFile->fixed_filename() % crop % width_t % height_t % icon_file).str());
  return true;
};

std::string video_utils::find_border(std::string fileName,float length) {
  std::string crop("");
  float start_time = (*vm)["trace_time"].as<float>();
  if(length <= start_time) start_time=0.0;
  float frame_time = start_time;
  float border_frames=(*vm)["border_frames"].as<float>();
  float cut_thresh=(*vm)["cut_thresh"].as<float>();
  float frame_spacing = (length-start_time)/border_frames;
  boost::process::ipstream is;
  boost::process::system((boost::format("ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of csv=p=0 %s") % fileName).str(),boost::process::std_out > is);
  std::string output;
  std::vector<std::string> split_string;
  std::getline(is,output);
  boost::split(split_string,output,boost::is_any_of(","));
  int width=std::stoi(split_string[0]);
  int height=std::stoi(split_string[1]);
  std::vector<short> * imgDat0 = new std::vector<short>(width*height*3);
  std::vector<short> * imgDat1 = new std::vector<short>(width*height*3);
  create_image(fileName, frame_time, imgDat0);
  std::vector<double> rowSums(height);
  std::vector<double> colSums(width);
  double corrFactorCol = 1.0/(double)(border_frames*height);
  double corrFactorRow = 1.0/(double)(border_frames*width);
  bool skipBorder = false;
  std::vector<short> * ptrHolder;
  for(int i = 1; i < border_frames; i++) {
    frame_time+=frame_spacing;
    create_image(fileName,frame_time,imgDat1);
    for (int y=0; y < height; y++) {
      for (int x=0; x < width; x++) {
	int rpos=3*(y*width+x);
	int gpos=rpos+1;
	int bpos=gpos+1;
	double value =sqrt(1.0/3.0*(pow((*imgDat1)[rpos]-(*imgDat0)[rpos],2.0)+pow((*imgDat1)[gpos]-(*imgDat0)[gpos],2.0)+pow((*imgDat1)[bpos]-(*imgDat0)[bpos],2.0)));
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
    ptrHolder = imgDat1;
    imgDat1=imgDat0;
    imgDat0=ptrHolder;
  }
  if(!skipBorder) {
    int x1(0), x2(width-1), y1(0), y2(height-1);
    while(colSums[x1] < cut_thresh) x1++;
    while(colSums[x2] < cut_thresh) x2--;
    while(rowSums[y1] < cut_thresh) y1++;
    while(rowSums[y2] < cut_thresh) y2--;
    x2-=x1-1;  
    y2-=y1-1;
    crop = (boost::format("crop=%i:%i:%i:%i,")% x2 % y2 % x1 % y1).str();
  }
  return crop;
}

void video_utils::create_image(std::string fileName, float start_time, std::vector<short> * imgDat) {
  bfs::path temp = tempPath;
  temp+=bfs::unique_path();
  temp+=".bin";  
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -vframes 1 -f image2pipe -pix_fmt rgb24 -vcodec rawvideo - > %s") % start_time % fileName % temp).str().c_str());
  std::ifstream dataFile(temp.c_str(),std::ios::in|std::ios::binary|std::ios::ate);
  dataFile.seekg (0, dataFile.end);
  int length = dataFile.tellg();
  dataFile.seekg (0, dataFile.beg);
  char * buffer = new char[length];
  dataFile.read(buffer,length);
  for(int i = 0; i < length; i++)  {
    unsigned short value = buffer[i];
    if(value > 255) value = 256-(65536-value);
    (*imgDat)[i] = value;
  }
  bfs::remove(temp);
  return;
}
  
Magick::Image *video_utils::get_image(int vid) {
  Magick::Image * img;
  if(img_cache[vid]) {
    img = img_cache[vid];
  }
  else {
    std::string image_file(dbCon->fetch_icon(vid));
    img = new Magick::Image(image_file);
    std::system((boost::format("rm %s") % image_file).str().c_str());
    if(img_cache.size() < (*vm)["cache_size"].as<int>()) img_cache[vid] = img;  
  }
  return img;
}

bool video_utils::compare_images(int vid1, int vid2) {
  Magick::Image * img1 = get_image(vid1);
  Magick::Image * img2 = get_image(vid2);
  float width1 = img1->size().width();
  float width2 = img2->size().width();
  if(fabs(width1 - width2) > 20) return false;
  else {
    float width = width1;
    if(width1 > width2) width = width2;
    float height = img2->size().height();
    Magick::Pixels v1(*img1);
    Magick::Pixels v2(*img2);
    double difference;
    const Magick::Quantum * pixels1 = v1.getConst(0,0,width,height);
    const Magick::Quantum * pixels2 = v2.getConst(0,0,width,height);
    for(int i = 0; i < height; i++) for( int j = 0; j < width; j++) for(int k = 0; k < 3; k++){
	  if(i == 0 && j ==0 ) std::cout <<*pixels1 << std::endl;
	  difference+=pow(*pixels1-*pixels2,2.0);
	  pixels1++;
	  pixels2++;
	}
    difference/=(width*height);
    difference = sqrt(difference);
    std::cout <<"Channels: " <<img1->channels() <<" "<< vid1 <<" "<< vid2 << " " << difference << std::endl;
  return true;
  }
}




