#include "VideoUtils.h"
#include <fstream>
#include <boost/process.hpp>
#include <boost/format.hpp>

video_utils::video_utils(po::variables_map & vm) {
  tempPath = getenv("HOME");
  tempPath+="/.video_proj/"; 
  dbCon = new DbConnector(tempPath);
  dbCon->fetch_results(result_map);
  Magick::InitializeMagick("");
  paths.push_back(bfs::path(vm["default_path"].as<std::string>()));
  TPool = new cxxpool::thread_pool(vm["threads"].as<int>());
  cTraceFPS =     vm["trace_fps"].as<float>();
  cStartT =       vm["trace_time"].as<float>();
  cCompTime =     vm["comp_time"].as<float>();
  cSliceSpacing = vm["slice_spacing"].as<float>();
  cThresh =       vm["thresh"].as<float>();
  cFudge =        vm["fudge"].as<int>();
  cThumbT =       vm["thumb_time"].as<float>();
  cHeight =       vm["thumb_height"].as<int>();
  cWidth =        vm["thumb_width"].as<int>();
  cCache =        vm["cache_size"].as<int>();
  cBFrames =      vm["border_frames"].as<float>();
  cCutThresh =    vm["cut_thresh"].as<float>();
  cImgThresh =    vm["image_thresh"].as<int>();
}

video_utils::video_utils(){}

bool video_utils::compare_vids(int i, int j, std::map<int, std::vector<uint8_t> > & data) {
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
	for(auto & a : accum) if (a > cThresh*cCompTime*cTraceFPS) counter++;
	if(counter != 0) break;
	//pixel/color loop
	for (t_d = 0; t_d < 12; t_d++) {
	  float value = fabs((int)data[i][t_s+t_o+t_d]-(int)(data[j][t_x+t_o+t_d]))-cFudge;
	  if(value < 0) value = 0;
	  accum[t_d]+=pow(value,2.0);
	}
      }
      counter = 0;
      for(auto & a: accum)  if(a < cThresh*cCompTime*cTraceFPS) counter+=1;
      if(counter == 12) match=true;
      if(match) std::cout << "ACCUM " <<i<<" " <<j <<" " <<t_o <<" slice " <<t_s <<" 2nd offset " <<t_x <<" " <<*max_element(accum.begin(),accum.end())  <<std::endl;
    }
  }
  std::pair<int,int> key(i,j);
  if(match) result_map[key]+=2;
  else if(result_map[key]==0) result_map[key]=4;
  dbCon->update_results(i,j,result_map[key]);
  return true;
}

bool video_utils::calculate_trace(VidFile * obj) {
  float start_time = cStartT;
  if(obj->length <= start_time) start_time=0.0;
  boost::process::ipstream is;
  boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -filter:v \"fps=%.3f,%sscale=2x2:flags=fast_bilinear\" -pix_fmt rgb24 -f image2pipe -vcodec rawvideo - ") % start_time % obj->fileName  % cTraceFPS % obj->crop).str(),boost::process::std_out > is);
  std::string outString;
  std::getline(is,outString);
  dbCon->save_trace(obj->vid, outString);
  return true; 
}

bool video_utils::create_thumb(VidFile * vidFile) {
  if(dbCon->icon_exists(vidFile->vid)) {
    dbCon->fetch_icon(vidFile->vid);
    return true;
  }
  std::string crop(find_border(vidFile->fileName, vidFile->length));
  vidFile->crop=crop;
  dbCon->save_crop(vidFile);
  float thumb_t = cThumbT;
  if(vidFile->length < cThumbT) thumb_t = vidFile->length/2.0;
  std::string icon_file(dbCon->create_icon_path(vidFile->vid));
  boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -frames:v 1 -filter:v \"%sscale=w=%d:h=%d:force_original_aspect_ratio=decrease\" %s") % thumb_t % vidFile->fileName % crop % cWidth % cHeight % icon_file).str());
  return true;
};

std::string video_utils::find_border(bfs::path & fileName,float length) {
  std::string crop("");
  float frame_time = 0.5*length/(cBFrames+1.0);
  float frame_spacing = length/(cBFrames+1.0);
  boost::process::ipstream is;
  boost::process::system((boost::format("ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of csv=p=0 %s") % fileName).str(),boost::process::std_out > is);
  std::string output;
  std::vector<std::string> split_string;
  std::getline(is,output);
  boost::split(split_string,output,boost::is_any_of(","));
  int width=std::stoi(split_string[0]);
  int height=std::stoi(split_string[1]);
  std::vector<uint8_t> * imgDat0 = new std::vector<uint8_t>(width*height*3);
  std::vector<uint8_t> * imgDat1 = new std::vector<uint8_t>(width*height*3);
  create_image(fileName, frame_time, imgDat0);
  std::vector<double> rowSums(height);
  std::vector<double> colSums(width);
  double corrFactorCol = 1.0/(double)(cBFrames*height);
  double corrFactorRow = 1.0/(double)(cBFrames*width);
  bool skipBorder = false;
  std::vector<uint8_t> * ptrHolder;
  for(int i = 0; i < cBFrames; i++) {
    frame_time+=frame_spacing;
    create_image(fileName,frame_time,imgDat1);
    for (int y=0; y < height; y++)
      for (int x=0; x < width; x++) {
	int rpos=3*(y*width+x);
	int gpos=rpos+1;
	int bpos=gpos+1;
	double value =sqrt(1.0/3.0*(pow((*imgDat1)[rpos]-(*imgDat0)[rpos],2.0)+pow((*imgDat1)[gpos]-(*imgDat0)[gpos],2.0)+pow((*imgDat1)[bpos]-(*imgDat0)[bpos],2.0)));
	rowSums[y]+=corrFactorRow*value;
	colSums[x]+=corrFactorCol*value;      
      }
    if(rowSums[0] > cCutThresh &&
       rowSums[height-1] > cCutThresh &&
       colSums[0] > cCutThresh &&
       colSums[width-1] > cCutThresh) {
      skipBorder=true;
      break;
    }
    ptrHolder = imgDat1;
    imgDat1=imgDat0;
    imgDat0=ptrHolder;
  }
  if(!skipBorder) {
    int x1(0), x2(width-1), y1(0), y2(height-1);
    while(colSums[x1] < cCutThresh) x1++;
    while(colSums[x2] < cCutThresh) x2--;
    while(rowSums[y1] < cCutThresh) y1++;
    while(rowSums[y2] < cCutThresh) y2--;
    x2-=x1-1;  
    y2-=y1-1;
    crop = (boost::format("crop=%i:%i:%i:%i,")% x2 % y2 % x1 % y1).str();
  }
  delete imgDat0;
  delete imgDat1;
  return crop;
}

void video_utils::create_image(bfs::path & fileName, float start_time, std::vector<uint8_t> * imgDat) {
  bfs::path temp = tempPath;
  temp+=bfs::unique_path();
  temp+=".bin";  
  std::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -vframes 1 -f image2pipe -pix_fmt rgb24 -vcodec rawvideo - > %s") % start_time % fileName % temp).str().c_str());
  std::ifstream dataFile(temp.c_str(),std::ios::in|std::ios::binary|std::ios::ate);
  dataFile.seekg (0, dataFile.end);
  int length = dataFile.tellg();
  dataFile.seekg (0, dataFile.beg);
  char buffer[length];
  dataFile.read(buffer,length);
  for(int i = 0; i < length; i++)    (*imgDat)[i] = buffer[i];
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
    if(img_cache.size() < cCache) img_cache[vid] = img;  
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
    Magick::Image diff(*img1);
    diff.composite(*img2,0,0,Magick::DifferenceCompositeOp);
    Magick::Pixels v1(diff);
    float difference = 0.0;
    float coeff = 1.0/(3.0*width*height);
    const Magick::Quantum * pixels = v1.getConst(0,0,width,height);
    for(int i = 0; i < 3*width*height; i++){
      difference+=pow(*pixels,2.0);
      pixels++;
    }
    difference = sqrt(coeff*difference);
    if(difference < cImgThresh) {
      std::cout << "IMG MATCH " << vid1 << " " << vid2 << std::endl;
      return true;
    }
    if(!img_cache[vid2]) delete img2;
    else return false;
  }
}

void video_utils::compare_icons(std::vector<int> & vid_list) {
  for(int i = 0; i +1 < vid_list.size(); i++) { 
    for(int j = i+1; j < vid_list.size(); j++) {
      std::pair<int,int> key(vid_list[i],vid_list[j]);
      if (result_map[key]%2  == 1 || result_map[key] ==4) continue;
      else if(compare_images(vid_list[i],vid_list[j])) result_map[key]+=1;      
    }
    delete img_cache[vid_list[i]];
    img_cache.erase(vid_list[i]);
  }
  return;
}

void video_utils::start_thumbs(std::vector<VidFile *> & vFile) {
  resVec.clear();
  for(auto &a: vFile) resVec.push_back(TPool->push([&](VidFile *b) {return create_thumb(b);}, a));
  return;
}

void video_utils::start_make_traces(std::vector<VidFile *> & vFile) {
  resVec.clear();
  for(auto & b: vFile) if(!dbCon->trace_exists(b->vid))resVec.push_back(TPool->push([&](VidFile * b ){ return calculate_trace(b);},b));
  return;
}

std::vector<std::future_status>  video_utils::get_status() {
  std::chrono::milliseconds timer(1);
  return cxxpool::wait_for(resVec.begin(), resVec.end(),timer);  
}

void video_utils::compare_traces(std::vector<int> & vid_list) {
  resVec.clear();
  std::map<int,std::vector<uint8_t> > data_holder;
  //loop over files
  for(int i = 0; i +1 < vid_list.size(); i++) {
    dbCon->fetch_trace(vid_list[i],data_holder[vid_list[i]]);
    //loop over files after i
    for(int j = i+1; j < vid_list.size(); j++) {
      if (result_map[std::make_pair(vid_list[i],vid_list[j])]/2 >= 1) continue;
      else {
	dbCon->fetch_trace(vid_list[j],data_holder[vid_list[j]]);
	resVec.push_back(TPool->push([&](int i, int j, std::map<int, std::vector<uint8_t>> & data){ return compare_vids(i,j,data);},vid_list[i],vid_list[j],data_holder));
      }
    }
  }
  return;
}

void video_utils::make_vids(std::vector<VidFile *> & vidFiles) {
  VidFile * vidTemp;
  int min_vid = dbCon->get_last_vid();
  std::vector<std::future<VidFile * > > vFiles;
  for(auto & path: paths) {
    for (bfs::directory_entry & x : bfs::directory_iterator(path)) {
      auto extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(get_extensions().count(extension)) {
	bfs::path pathName = x.path();
	if(!dbCon->video_exists(pathName))  {
	  min_vid++;
	  vFiles.push_back(TPool->push([this](bfs::path path, int vid) {return new VidFile(path,vid);},pathName,min_vid));
	}
	else {
	  vidTemp = dbCon->fetch_video(pathName);
	  vidFiles.push_back(vidTemp);
	}
      }
    }
  }
  cxxpool::wait(vFiles.begin(),vFiles.end());
  for(auto &a: vFiles) {
    vidTemp = a.get();
    dbCon->save_video(vidTemp);
    vidFiles.push_back(vidTemp);
  }
  return;
}

std::set<std::string> video_utils::get_extensions() {
  std::set<std::string> extensions{".3gp",".avi",".flv",".m4v",".mkv",".mov",".mp4",".mpeg",".mpg",".mpv",".qt",".rm",".webm",".wmv"};
  return extensions;
}

void video_utils::set_paths(std::vector<std::string> folders) {
  paths.clear();
  for(auto & a: folders) paths.push_back(bfs::path(a));
  return;
}

std::string video_utils::save_icon(int vid) {
  std::string icon_file = dbCon->create_icon_path(vid);
  dbCon->save_icon(vid);
  return icon_file;
}

void video_utils::save_db() {
  dbCon->save_db_file();
  return;
}
