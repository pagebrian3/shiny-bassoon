#include "VideoUtils.h"
#include "QVBMetadata.h"
#include "QVBConfig.h"
#include "VidFile.h"
#include <fstream>
#include <mutex>
#include "MediaInfo/MediaInfo.h"
#include "Magick++.h"
#include "ZenLib/Ztring.h"
#include <boost/process.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <fftw3.h>
#include <iostream>

std::mutex mtx;

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
  metaData = new qvdb_metadata(dbCon);
  if(appConfig->load_config(dbCon->fetch_config())) dbCon->save_config(appConfig->get_data());
  Magick::InitializeMagick("");
  paths.push_back(homeP);
  TPool = new cxxpool::thread_pool(appConfig->get_int("threads"));
  std::string badChars;
  badChars = appConfig->get_string("bad_chars");
  std::string extStrin;
  extStrin = appConfig->get_string("extensions");
  boost::char_separator<char> sep(" \"");
  boost::tokenizer<boost::char_separator<char> > tok(extStrin,sep);
  for(auto &a: tok) extensions.insert(a);
  boost::tokenizer<boost::char_separator<char> > tok1(badChars,sep);
  for(auto &a: tok1) cBadChars.push_back(a);
}

qvdb_metadata * video_utils::mdInterface() {
  return metaData;
}

bool video_utils::compare_vids(int i, int j) {
  bool match=false;
  int counter=0;
  uint t_s,t_x, t_o, t_d;
  float cTraceFPS = appConfig->get_float("trace_fps");
  float cCompTime = appConfig->get_float("comp_time");
  float cSliceSpacing = appConfig->get_float("slice_spacing");
  float cThresh = appConfig->get_float("thresh");
  float cFudge = appConfig->get_float("fudge");
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
  uint numVars=12;
  float cTraceFPS = appConfig->get_float("trace_fps");
  float cSliceSpacing = appConfig->get_float("slice_spacing");
  float cThresh = appConfig->get_float("thresh");
  float cFudge = appConfig->get_float("fudge");
  int trT = cTraceFPS*appConfig->get_float("comp_time");
  uint size = 2;  //size of 1 transform  must be a power of 2
  uint length1 = traceData[i].size()/numVars;
  uint length2 = traceData[j].size()/numVars;
  while(size < length2) size*=2;    
  int dims[]={size};
  double * in=(double *) fftw_malloc(sizeof(double)*numVars*size);
  fftw_complex * out =(fftw_complex*) fftw_malloc(sizeof(fftw_complex)*numVars*(size/2+1));
  std::vector<double>holder(numVars*size);
  std::vector<fftw_complex> cHolder(numVars*(size/2+1));
  std::vector<double> result(length2-trT);
  std::vector<double> coeffs(numVars*(length2-trT));
  mtx.lock();
  fftw_plan fPlan = fftw_plan_many_dft_r2c(1,dims,numVars, in,dims, numVars,1,out, dims,numVars,1,FFTW_ESTIMATE);
  fftw_plan rPlan = fftw_plan_many_dft_c2r(1,dims,numVars, out,dims, numVars,1,in, dims,numVars,1,FFTW_ESTIMATE);
  mtx.unlock();
  uint t_s;  //current slice position
  uint offset = 0;
  uint l = 0;
  uint k ;
  std::vector<double> compCoeffs(numVars);
  std::vector<double> coeff_temp(numVars);
  //calculate normalization for 2nd trace
  for(offset=0; offset < numVars*trT; offset+=numVars) for(l = 0; l < numVars; l++) coeff_temp[l]+=pow(traceData[j][offset+l],2);
  while(offset < numVars*length2)  {
    for(l = 0; l < numVars; l++)  {
      coeffs[offset-numVars*trT+l]=coeff_temp[l];
      coeff_temp[l]+=(pow(traceData[j][offset+l],2)-pow(traceData[j][offset-numVars*trT+l],2));
    }
    offset+=numVars;
  }
  for(k=0; k < numVars*length2; k++)  in[k]=traceData[j][k];
    while(k<numVars*size) {
      in[k]=0.0;
      k++;
    }
    fftw_execute(fPlan);
      for(k=0; k < (size/2+1)*numVars; k++)  {
      cHolder[k][0]=out[k][0];
      cHolder[k][1]=out[k][1];
    }
  //loop over slices
  for(t_s =0; t_s < numVars*(length1-trT); t_s+= numVars*cTraceFPS*cSliceSpacing){
    k=0;
    //load and pad data for slice
    for(k = 0; k < numVars*trT; k++)  in[k]=traceData[i][k+t_s];
    while(k<numVars*size) {
      in[k]=0.0;
      k++;
    }
    std::fill(compCoeffs.begin(),compCoeffs.end(),0.0);
    //calculate normalization for slice
    for(uint deltaT=0; deltaT < numVars*trT; deltaT+=numVars) for(l = 0; l < numVars; l++)  compCoeffs[l]+=pow(traceData[i][deltaT+t_s+l],2);
    fftw_execute(fPlan);
    for(k=0;k<numVars*(size/2+1); k++) {
      double a,b,c,d;
      c = out[k][0];
      d = out[k][1];
      a =cHolder[k][0];
      b = cHolder[k][1];
      out[k][0]=a*c+b*d;
      out[k][1]=b*c-a*d;
    }
    fftw_execute(rPlan);
    double sum;
    for(k = 0; k < length2-trT; k++)  {
      sum=0.0;
      for(l=0; l <numVars; l++) sum+=in[numVars*k+l]/(size* 6*(compCoeffs[l]+coeffs[numVars*k+l]));
      result[k]=pow(sum,cFudge);
    }
    double max_val = *max_element(result.begin(),result.end());
    int max_offset = max_element(result.begin(),result.end()) - result.begin();
    if(max_val > cThresh) {
      std::cout << "Match! " <<i <<" " << j<<" "<<t_s<<" "<< max_val <<" "<<max_offset << std::endl;
      match = true;
	break;
    }
  }
  std::pair<int,int> key(i,j);
  if(match) result_map[key]+=2;
  else if(result_map[key]==0) result_map[key]=4;
  dbCon->update_results(i,j,result_map[key]);
  fftw_free(in);
  fftw_free(out);
  return true;
}

bool video_utils::calculate_trace(VidFile * obj) {
  float fps = appConfig->get_float("trace_fps");
  float start_time = appConfig->get_float("trace_time");
  if(obj->length <= start_time) start_time=0.0;
  std::string outPath = dbCon->createPath(tracePath,obj->vid,".bin");
  boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -filter:v \"framerate=fps=%.3f,%sscale=2x2:flags=fast_bilinear\" -pix_fmt rgb24 -f image2pipe -vcodec rawvideo %s") % start_time % obj->fileName  % fps % obj->crop %outPath ).str());
  return true; 
}

bool video_utils::create_thumb(VidFile * vidFile) {
  if(dbCon->icon_exists(vidFile->vid)) {
    dbCon->fetch_icon(vidFile->vid);
    return true;
  }
  int cHeight = appConfig->get_int("thumb_height");
  int cWidth = appConfig->get_int("thumb_width");
  std::string crop(find_border(vidFile->fileName, vidFile->length));
  if(crop.length() > 0) {
    vidFile->crop=crop;
    dbCon->save_crop(vidFile);
  }
  //else crop.assign("");  //in case it is null
  float thumb_t = appConfig->get_float("thumb_time");
  if(vidFile->length < thumb_t) thumb_t = vidFile->length/2.0;
  std::string icon_file(dbCon->createPath(tempPath,vidFile->vid,".jpg"));
  std::string command((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -frames:v 1 -filter:v \"%sscale=w=%d:h=%d:force_original_aspect_ratio=decrease\" %s") % thumb_t % vidFile->fileName % crop % cWidth % cHeight % icon_file).str());
  boost::process::system(command);
  return true;
};

std::string video_utils::find_border(bfs::path & fileName,float length) {
  std::string crop("");
  int cBFrames = appConfig->get_int("border_frames");
  float cCutThresh = appConfig->get_float("cut_thresh");
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
  dataFile.close();
  for(int i = 0; i < length; i++) (*imgDat)[i] = buffer[i];
  bfs::remove(temp);
  return;
}
  
Magick::Image *video_utils::get_image(int vid) {
  if(img_cache[vid]) return img_cache[vid];
  std::string image_file(dbCon->fetch_icon(vid));
  Magick::Image * img = new Magick::Image(image_file);
  std::system((boost::format("rm %s") % image_file).str().c_str());
  if(img_cache.size() < appConfig->get_int("cache_size")) img_cache[vid] = img;  
  return img;
}

bool video_utils::compare_images(int vid1, int vid2) {
  Magick::Image * img1 = get_image(vid1);
  Magick::Image * img2 = get_image(vid2);
  float width1 = img1->size().width();
  float width2 = img2->size().width();
  int cImgThresh = appConfig->get_int("image_thresh");
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

void video_utils::compare_icons() {
  for(uint i = 0; i +1 < fVIDs.size(); i++) { 
    for(uint j = i+1; j < fVIDs.size(); j++) {
      std::pair<int,int> key(fVIDs[i],fVIDs[j]);
      if (result_map[key]%2  == 1 || result_map[key] ==4) continue;
      else if(compare_images(fVIDs[i],fVIDs[j])) result_map[key]+=1;      
    }
    delete img_cache[fVIDs[i]];
    img_cache.erase(fVIDs[i]);
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

void video_utils::compare_traces() {
  resVec.clear();
  std::map<int,std::vector<uint8_t> > data_holder;
  //loop over files
  for(uint i = 0; i +1 < fVIDs.size(); i++) {
    load_trace(fVIDs[i]);
    //loop over files after i
    for(uint j = i+1; j < fVIDs.size(); j++) {
      if (result_map[std::make_pair(fVIDs[i],fVIDs[j])]/2 >= 1) continue;  
      load_trace(fVIDs[j]);
      resVec.push_back(TPool->push([&](int i, int j){ return compare_vids_fft(i,j);},fVIDs[i],fVIDs[j]));
    }
  }
  return;
}

void video_utils::make_vids(std::vector<VidFile *> & vidFiles) {
  std::vector<std::future<std::vector<VidFile * > > >vFiles;
  std::vector<std::vector<bfs::path> > pathVs(appConfig->get_int("threads"));
  int counter = 0;
  fVIDs.clear();
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
	  pathVs[counter%appConfig->get_int("threads")].push_back(pathName);
	  counter++;
	}
	else {
	  VidFile * v =dbCon->fetch_video(pathName);
	  vidFiles.push_back(v);
	  fVIDs.push_back(v->vid);
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
      fVIDs.push_back(v->vid);
      vidFiles.push_back(v);      
    }
  }
   metaData->load_file_md(fVIDs);
  for(uint i = 0; i < vidFiles.size(); i++) std::cout << vidFiles[i]->vid << " " << vidFiles[i]->fileName << std::endl;
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
  for(uint i = 0; i < files.size(); i++) {
    ZenLib::Ztring zFile;
    auto fileName = files[i];
    zFile += fileName.wstring();
    int size = 0;
    if(bfs::is_symlink(fileName)) {
      size = bfs::file_size(bfs::read_symlink(fileName));
    }
    else size = bfs::file_size(fileName); 
    MI.Open(zFile);
    MI.Option(__T("Inform"),__T("Video;%Duration%"));
    float length = 0.001*ZenLib::Ztring(MI.Inform()).To_float32();
    if(length <= 0) {
      std::cout << "Length failed for " << files[i].string() << std::endl;
      length = 0.0;
    }
    MI.Option(__T("Inform"),__T("Video;%Rotation%"));
    int rotate = ZenLib::Ztring(MI.Inform()).To_float32();
    MI.Close();
    output[i] = new VidFile(fileName,length,size,0,-1,"",rotate);
  }
  return output;
}

void video_utils::load_trace(int vid) {
  std::ifstream dataFile(dbCon->createPath(tracePath,vid,".bin"),std::ios::in|std::ios::binary|std::ios::ate);
  dataFile.seekg (0, dataFile.end);
  int length = dataFile.tellg();
  dataFile.seekg (0, dataFile.beg);
  char buffer[length];
  dataFile.read(buffer,length);
  dataFile.close();
  std::vector<uint8_t> temp(length);
  for(int i = 0; i < length; i++) temp[i] =(uint8_t) buffer[i];
  traceData[vid] = temp;
}

