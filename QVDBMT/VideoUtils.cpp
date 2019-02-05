#include "Magick++.h"
#include "VideoUtils.h"
#include <fstream>
#include "MediaInfo/MediaInfo.h"
#include "ZenLib/Ztring.h"
#include <boost/process.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <fftw3.h>


video_utils::video_utils() {
  if(PLATFORM_NAME == "linux") {
    tempPath = getenv("HOME");
  }
  else if(PLATFORM_NAME == "windows") {
    std::string temp(getenv("USERPROFILE"));
    tempPath =temp.substr(temp.find("="));
  }
  else std::cout << "Unsupported platform" << std::endl;
  bfs::path homeP(tempPath);
  tempPath+="/.video_proj/"; 
  dbCon = new DbConnector(tempPath);
  tracePath=tempPath;
  tracePath+="traces/";
  bfs::create_directory(tracePath);
  dbCon->fetch_results(result_map);
  appConfig = new qvdb_config();
  if(appConfig->load_config(dbCon->fetch_config())) dbCon->save_config(appConfig->get_data());
  Magick::InitializeMagick("");
  paths.push_back(homeP);
  appConfig->get("threads",numThreads);
  TPool = new cxxpool::thread_pool(numThreads);
  appConfig->get("trace_fps",cTraceFPS);
  appConfig->get("trace_time",cStartT);
  appConfig->get("comp_time",cCompTime);
  appConfig->get("slice_spacing",cSliceSpacing);
  appConfig->get("thresh",cThresh);
  appConfig->get("fudge",cFudge);
  appConfig->get("thumb_time",cThumbT);
  appConfig->get("thumb_height",cHeight);
  appConfig->get("thumb_width",cWidth);
  appConfig->get("cache_size",cCache);
  appConfig->get("border_frames",cBFrames);
  appConfig->get("cut_thresh",cCutThresh);
  appConfig->get("image_thresh",cImgThresh);
  std::string badChars;
  appConfig->get("bad_chars",badChars);
  std::string extStrin;
  appConfig->get("extensions",extStrin);
  boost::char_separator<char> sep(" \"");
  boost::tokenizer<boost::char_separator<char> > tok(extStrin,sep);
  for(auto &a: tok) extensions.insert(a);
  boost::tokenizer<boost::char_separator<char> > tok1(badChars,sep);
  for(auto &a: tok1) cBadChars.push_back(a);
}

bool video_utils::compare_vids(int i, int j) {
  bool match=false;
  int counter=0;
  uint t_s,t_x, t_o, t_d;
  //loop over slices
  for(t_s =0; t_s < traceData[i].size()-12*cTraceFPS*cCompTime; t_s+= 12*cTraceFPS*cSliceSpacing){
    if(match) break;
    //starting offset for 2nd trace-this is the loop for the indiviual tests
    for(t_x=0; t_x < traceData[j].size()-12*cTraceFPS*cCompTime; t_x+=12){
      if(match) break;
      std::vector<int> accum(12);
      //offset loop
      for(t_o = 0; t_o < 12*cCompTime*cTraceFPS; t_o+=12){
	counter = 0;
	for(auto & a : accum) if (a > cThresh*cCompTime*cTraceFPS) counter++;
	if(counter != 0) break;
	//pixel/color loop
	for (t_d = 0; t_d < 12; t_d++) {
	  float value = fabs((int)traceData[i][t_s+t_o+t_d]-(int)(traceData[j][t_x+t_o+t_d]))-cFudge;
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

bool video_utils::compare_vids_fft(int i, int j) {
  bool match=false;
  int size = 2;  //size of 1 transform  must be a power of 2
  uint length1 = traceData[i].size()/12;
  uint length2 = traceData[j].size()/12;
  while(size < length2) size*=2;     //Value to calculate  2*A*B/(A^2+B^2)  find peaks
  int dims[]={size};
  double * in=(double *) fftw_malloc(sizeof(double)*12*size);
  double * out =(double *) fftw_malloc(sizeof(double)*12*size);
  double * holder = (double *) malloc(sizeof(double)*12*size);
  std::vector<double> result(length2-cTraceFPS*cCompTime);
  std::vector<double> coeffs(12*(size-cTraceFPS*cCompTime));
  fftw_r2r_kind fKind[] = {FFTW_R2HC};
   fftw_r2r_kind rKind[] = {FFTW_HC2R};
  fftw_plan fPlan = fftw_plan_many_r2r(1,dims,12, in,dims, 12,1,out, dims,12,1,fKind,FFTW_ESTIMATE);
  fftw_plan rPlan = fftw_plan_many_r2r(1,dims,12, out,dims, 12,1,in, dims,12,1,rKind,FFTW_ESTIMATE);
  std::cout << "starting " << i << " " << j <<" "<<size<< std::endl;
  if(i == 4 and j ==5) std::cout << "BLAH " << (int)traceData[i][0] << " "<<(int)traceData[j][0] << " "<<(int)traceData[i][11] << " "<<(int)traceData[j][11] << std::endl;
  uint t_s;  //current slice position
uint offset = 0;
 uint l = 0;
std::vector<double> compCoeffs(12);
 std::vector<double> coeff_temp(12);
for(uint deltaT=0; deltaT < cTraceFPS*cCompTime; deltaT++) for(l = 0; l < 12; l++) coeff_temp[l]+=pow(traceData[j][12*deltaT+l],2);
    while(offset < 12*(length2-cTraceFPS*cCompTime))  {
      for(l = 0; l < 12; l++)  {
	coeffs[offset+l]=coeff_temp[l];
	coeff_temp[l]+=(pow(traceData[j][offset+12*cTraceFPS*cCompTime+l],2)-pow(traceData[j][offset+l],2));
      }
      offset+=12;
    }
   //loop over slices
  for(t_s =0; t_s < 12*(length1-cTraceFPS*cCompTime); t_s+= 12*cTraceFPS*cSliceSpacing){
    uint k;
    for(k = 0; k < 12*cTraceFPS*cCompTime; k++)  in[k]=traceData[i][k+t_s];
    while(k<12*size) {
      in[k]=0.0;
      k++;
    }
     std::fill(compCoeffs.begin(),compCoeffs.end(),0);
    for(uint deltaT=0; deltaT < cTraceFPS*cCompTime; deltaT++) for(l = 0; l < 12; l++)  compCoeffs[l]+=pow(traceData[i][12*deltaT+t_s+l],2);
    //std::cout <<t_s<< "  execute " <<i << " "<<j<< std::endl;
    fftw_execute(fPlan);
    for(k=0; k < size*12; k++) holder[k]=out[k];
    for(k=0; k < 12*length2; k++)  in[k]=traceData[j][k];
    while(k<12*size) {
      in[k]=0.0;
      k++;
    }
    fftw_execute(fPlan);
    for(k=0;k<=6*size; k++) in[k]=out[k]*holder[k];
    while(k < size*12) {
      in[k]=-1.0*out[k]*holder[k];
      k++;
    }
    fftw_execute(rPlan);
    double sum;
    for(k = 0; k < length2-cTraceFPS*cCompTime; k++)  {
      sum=0.0;
      for(l=0; l <12; l++) sum+=out[12*k+l]/(size* 6*(compCoeffs[l]+coeffs[12*k+l]));
      result[k]=sum;
    }
    double max_val = *max_element(result.begin(),result.end());
    std::cout << "Max result " <<i <<" " << j<<" "<<t_s<<" "<< max_val << std::endl;
  }
  std::pair<int,int> key(i,j);
  if(match) result_map[key]+=2;
  else if(result_map[key]==0) result_map[key]=4;
  dbCon->update_results(i,j,result_map[key]);
  fftw_free(in);
  fftw_free(out);
  free(holder);
  return true;
}

bool video_utils::calculate_trace(VidFile * obj) {
  float start_time = cStartT;
  if(obj->length <= start_time) start_time=0.0;
  std::string outPath = dbCon->createPath(tracePath,obj->vid,".bin");
  boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -filter:v \"fps=%.3f,%sscale=2x2:flags=fast_bilinear\" -pix_fmt rgb24 -f image2pipe -vcodec rawvideo %s") % start_time % obj->fileName  % cTraceFPS % obj->crop %outPath ).str());
  /*std::ifstream ifstr(tracePath.c_str());
  std::string outString;
  ifstr.seekg(0, std::ios::end);   
outString.reserve(ifstr.tellg());
ifstr.seekg(0, std::ios::beg);
 outString.assign(std::istreambuf_iterator<char>(ifstr),std::istreambuf_iterator<char>());
 dbCon->save_trace(obj->vid, outString);*/
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
  std::string icon_file(dbCon->createPath(tempPath,vidFile->vid,".jpg"));
  std::string command((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -frames:v 1 -filter:v \"%sscale=w=%d:h=%d:force_original_aspect_ratio=decrease\" %s") % thumb_t % vidFile->fileName % crop % cWidth % cHeight % icon_file).str());
  boost::process::system(command);
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
    while(colSums[x1] < cCutThresh && x1 < width-1) x1++;
    while(colSums[x2] < cCutThresh && x2 > 0) x2--;
    while(rowSums[y1] < cCutThresh && y1 < height -1) y1++;
    while(rowSums[y2] < cCutThresh && y2 > 0) y2--;
    if( x1 < x2 and  y1 < y2) { 
      x2-=x1-1;  
      y2-=y1-1;
      crop = (boost::format("crop=%i:%i:%i:%i,")% x2 % y2 % x1 % y1).str();
    }
    else std::cout << "Find border failed for:" << fileName.string() << std::endl;  
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
  for(int i = 0; i < length; i++) (*imgDat)[i] = buffer[i];
  bfs::remove(temp);
  return;
}
  
Magick::Image *video_utils::get_image(int vid) {
  if(img_cache[vid]) return img_cache[vid];
  std::string image_file(dbCon->fetch_icon(vid));
  Magick::Image * img = new Magick::Image(image_file);
  std::system((boost::format("rm %s") % image_file).str().c_str());
  if(img_cache.size() < cCache) img_cache[vid] = img;  
  return img;
}

bool video_utils::compare_images(int vid1, int vid2) {
  Magick::Image * img1 = get_image(vid1);
  Magick::Image * img2 = get_image(vid2);
  float width1 = img1->size().width();
  float width2 = img2->size().width();
  if(fabs(width1 - width2) <= 20) {  //20 is untuned parameter, allowable difference in width
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
  return false;
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
  for(auto & b: vFile) {
    bfs::path tPath(dbCon->createPath(tracePath,b->vid,".bin"));
   if(!bfs::exists(tPath))resVec.push_back(TPool->push([&](VidFile * b ){ return calculate_trace(b);},b));
  }
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
    load_trace(vid_list[i]);
    //loop over files after i
    for(int j = i+1; j < vid_list.size(); j++) {
      //if (result_map[std::make_pair(vid_list[i],vid_list[j])]/2 >= 1) continue;  
      load_trace(vid_list[j]);
      resVec.push_back(TPool->push([&](int i, int j){ return compare_vids_fft(i,j);},vid_list[i],vid_list[j]));
    }
  }
  return;
}

void video_utils::make_vids(std::vector<VidFile *> & vidFiles) {
  std::vector<std::future<std::vector<VidFile * > > >vFiles;
  std::vector<std::vector<bfs::path> > pathVs(numThreads);
  int counter = 0;
  std::vector<int> curVIDs;
  for(auto & path: paths){
    std::vector<bfs::path> current_dir_files;
    for(auto & x: bfs::directory_iterator(path)) {
      auto extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(extensions.count(extension)) {
	bfs::path pathName = x.path();
	for(auto & bChar: cBadChars) {
	  std::string nameHolder = pathName.native();
	  if(nameHolder.find(bChar)!=std::string::npos){
	    boost::erase_all(nameHolder,bChar);
	    bfs::path newName(nameHolder);
	    bfs::rename(pathName,newName);
	    pathName = newName;
	  }
	}	
	if(!dbCon->video_exists(pathName))  {
	  pathVs[counter%numThreads].push_back(pathName);
	}
	else {
	  VidFile * v =dbCon->fetch_video(pathName);
	  vidFiles.push_back(v);
	  curVIDs.push_back(v->vid);
	  current_dir_files.push_back(pathName);
	}
      }        
    }
    for(auto & y: pathVs) vFiles.push_back(TPool->push([this](std::vector<bfs::path> pathvs) {return vid_factory(pathvs);},y));
    //if(!new_db) dbCon->cleanup(path,current_dir_files);  //Bring back
  }
  cxxpool::wait(vFiles.begin(),vFiles.end());
  std::vector<VidFile *> vidsTemp;
  for(auto &a: vFiles) {
    vidsTemp = a.get();
    for(auto & v: vidsTemp) {
      dbCon->save_video(v);
      curVIDs.push_back(v->vid);
      vidFiles.push_back(v);      
    }
  }
  for(int i = 0; i < vidFiles.size(); i++) std::cout << vidFiles[i]->vid << " " << vidFiles[i]->fileName << std::endl;
  load_metadata(curVIDs);
  return;
}

void video_utils::set_paths(std::vector<std::string> & folders) {
  paths.clear();
  for(auto & a: folders) paths.push_back(bfs::path(a));
  return;
}

std::string video_utils::save_icon(int vid) {
  std::string icon_file = dbCon->createPath(tempPath,vid,".jpg");  
  dbCon->save_icon(vid);
  return icon_file;
}

void video_utils::close() {
  dbCon->save_config(appConfig->get_data());
  dbCon->save_db_file();
  return;
}

qvdb_config * video_utils::get_config(){
  return appConfig;
}

std::vector<VidFile *> video_utils::vid_factory(std::vector<bfs::path> & files) {
  MediaInfoLib::MediaInfo MI;
  std::vector<VidFile *> output(files.size());
  for(int i = 0; i < files.size(); i++) {
    ZenLib::Ztring zFile;
    auto fileName = files[i];
    zFile += fileName.wstring();
    int size = bfs::file_size(fileName); 
    MI.Open(zFile);
    MI.Option(__T("Inform"),__T("Video;%Duration%"));
    float length = 0.001*ZenLib::Ztring(MI.Inform()).To_float32();
    if(length < 1 ) std::cout << "Length failed for " << files[i].string() << std::endl; 
    MI.Option(__T("Inform"),__T("Video;%Rotation%"));
    int rotate = ZenLib::Ztring(MI.Inform()).To_float32();
    MI.Close();
    output[i] = new VidFile(fileName,length,size,0,-1,"",rotate);
  }
  return output;
}

void video_utils::load_metadata(std::vector<int> & vids) {
}

void video_utils::load_trace(int vid) {
  std::ifstream dataFile(dbCon->createPath(tracePath,vid,".bin"),std::ios::in|std::ios::binary|std::ios::ate);
  dataFile.seekg (0, dataFile.end);
  int length = dataFile.tellg();
  dataFile.seekg (0, dataFile.beg);
  char buffer[length];
  dataFile.read(buffer,length);
  std::vector<uint8_t> temp(length);
  for(int i = 0; i < length; i++) temp[i] =(uint8_t) buffer[i];
  traceData[vid] = temp;
}

