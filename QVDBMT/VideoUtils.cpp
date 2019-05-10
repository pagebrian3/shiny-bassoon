#include <DbConnector.h>
#include <VideoUtils.h>
#include <QVBMetadata.h>
#include <QVBConfig.h>
#include <VidFile.h>
#include <MediaInfo/MediaInfo.h>
#include <Magick++.h>
#include <ZenLib/Ztring.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/math/interpolators/barycentric_rational.hpp>
#include <fftw3.h>
#include <fstream>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

video_utils::video_utils() {
  if(PLATFORM_NAME == "linux") {
    savePath = getenv("HOME");
  }
  else if(PLATFORM_NAME == "windows") {
    std::string temp(getenv("USERPROFILE"));
    savePath =temp.substr(temp.find("="));
  }
  else std::cout << "Unsupported platform" << std::endl;
  bfs::path homeP(savePath);
  savePath+="/.video_proj/";
  tempPath = "/tmp/qvdbtmp/";
  if(!bfs::exists(tempPath))bfs::create_directory(tempPath);
  tracePath=tempPath;
  tracePath+="traces/";
  thumbPath=savePath;
  thumbPath+="thumbs/";
  if(!bfs::exists(tracePath))bfs::create_directory(tracePath);
  if(!bfs::exists(savePath)) bfs::create_directory(savePath);
  if(!bfs::exists(thumbPath)) bfs::create_directory(thumbPath);
  dbCon = new DbConnector(savePath,tempPath);
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
  int size = 2;  //size of 1 transform  must be a power of 2
  uint length1 = traceData[i].size()/numVars;
  uint length2 = traceData[j].size()/numVars;
  while(size < length2) size*=2;    
  int dims[]={size};
  float * in=(float *) fftwf_malloc(sizeof(float)*numVars*size);
  fftwf_complex * out =(fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex)*numVars*(size/2+1));
  std::vector<float>holder(numVars*size);
  std::vector<fftwf_complex> cHolder(numVars*(size/2+1));
  std::vector<float> result(length2-trT);
  std::vector<float> coeffs(numVars*(length2-trT));
  mtx.lock();
  fftwf_plan fPlan = fftwf_plan_many_dft_r2c(1,dims,numVars, in,dims, numVars,1,out, dims,numVars,1,FFTW_ESTIMATE);
  fftwf_plan rPlan = fftwf_plan_many_dft_c2r(1,dims,numVars, out,dims, numVars,1,in, dims,numVars,1,FFTW_ESTIMATE);
  mtx.unlock();
  uint t_s;  //current slice position
  uint offset = 0;
  uint l = 0;
  uint k;
  uint uSize = size;
  std::vector<float> compCoeffs(numVars);
  std::vector<float> coeff_temp(numVars);
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
    while(k<numVars*uSize) {
      in[k]=0.0;
      k++;
    }
    fftwf_execute(fPlan);
      for(k=0; k < (uSize/2+1)*numVars; k++)  {
      cHolder[k][0]=out[k][0];
      cHolder[k][1]=out[k][1];
    }
  //loop over slices
  for(t_s =0; t_s < numVars*(length1-trT); t_s+= numVars*cTraceFPS*cSliceSpacing){
    k=0;
    //load and pad data for slice
    for(k = 0; k < numVars*trT; k++)  in[k]=traceData[i][k+t_s];
    while(k<numVars*uSize) {
      in[k]=0.0;
      k++;
    }
    std::fill(compCoeffs.begin(),compCoeffs.end(),0.0);
    //calculate normalization for slice
    for(uint deltaT=0; deltaT < numVars*trT; deltaT+=numVars) for(l = 0; l < numVars; l++)  compCoeffs[l]+=pow(traceData[i][deltaT+t_s+l],2);
    fftwf_execute(fPlan);
    for(k=0;k<numVars*(uSize/2+1); k++) {
      float a,b,c,d;
      c = out[k][0];
      d = out[k][1];
      a =cHolder[k][0];
      b = cHolder[k][1];
      out[k][0]=a*c+b*d;
      out[k][1]=b*c-a*d;
    }
    fftwf_execute(rPlan);
    float sum;
    for(k = 0; k < length2-trT; k++)  {
      sum=0.0;
      for(l=0; l <numVars; l++) sum+=in[numVars*k+l]/(uSize* 6*(compCoeffs[l]+coeffs[numVars*k+l]));
      result[k]=pow(sum,cFudge);
    }
    float max_val = *max_element(result.begin(),result.end());
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
  fftwf_free(in);
  fftwf_free(out);
  return true;
}

/*bool video_utils::calculate_trace(VidFile * obj) {
  float fps = appConfig->get_float("trace_fps");
  float start_time = appConfig->get_float("trace_time");
  if(obj->length <= start_time) start_time=0.0;
  bfs::path outPath = createPath(tracePath,obj->vid,".bin");
  cv::Mat frame;
  std::vector<float> times;
  std::vector<std::vector<float> > cData(12);
  cv::VideoCapture vCap((obj->fileName).c_str());
  vCap.set(cv::CAP_PROP_POS_MSEC,1000.0*start_time);
  vCap >> frame;
  while(!frame.empty()) {
    times.push_back(vCap.get(cv::CAP_PROP_POS_MSEC));
    Magick::Image mgk(frame.cols, frame.rows, "BGR", Magick::CharPixel, (char *)frame.data);
    if(obj->crop.length() > 0) mgk.crop(obj->crop);
    mgk.scale("2x2!");
    Magick::Pixels pxls(mgk);
    const Magick::Quantum * pixels = pxls.getConst(0,0,2,2);
    for(int i = 0; i < 12; i++) cData[i].push_back(pixels[i]);
    vCap >> frame;
  }
  std::vector<boost::math::barycentric_rational<float> *> interps(12);
  for(int i = 0; i < 12; i++) interps[i] = new boost::math::barycentric_rational<float> (times.begin(),times.end(),cData[i].begin());
  float endT = times.back();
  float startT = times.front();
  float incT = 1000.0/fps;
  int tSize = (endT-startT)/incT;
  std::vector<uint8_t> output(12*tSize);
  for(float j=startT; j < endT; j+=incT) for(int i = 0; i < 12;i++) output.push_back((uint8_t)(*(interps[i]))(j));   
  std::ofstream outfile(outPath.c_str(),std::ofstream::binary);
  outfile.write((char*)&(output[0]),output.size());
  outfile.close();
  return true; 
  }*/

bool video_utils::calculate_trace(VidFile * obj) {
  float fps = appConfig->get_float("trace_fps");
  float start_time = appConfig->get_float("trace_time");
  if(obj->length <= start_time) start_time=0.0;
  bfs::path outPath = createPath(tracePath,obj->vid,".bin");
  cv::Mat frame;
  float time0=1000.0*start_time;
  float time1=0.0;
  std::vector<std::vector<float> > cData(12);
  cv::VideoCapture vCap((obj->fileName).c_str());
  vCap.set(cv::CAP_PROP_POS_MSEC,time0);
  float incT = 1000.0/fps;
   std::vector<uint8_t> output;
  float avgTime = time0;
  vCap >> frame;
  Magick::Image mgk0(frame.cols, frame.rows, "BGR", Magick::CharPixel, (char *)frame.data);
  if(obj->crop.length() > 0) mgk0.crop(obj->crop);
  mgk0.scale("2x2!");
  float coeff0,coeff1;
  while(!frame.empty()) {
    time1 = vCap.get(cv::CAP_PROP_POS_MSEC);
    if(time1 < avgTime) {
      time0 = time1;
      vCap >> frame;
      Magick::Image mgk(frame.cols, frame.rows, "BGR", Magick::CharPixel, (char *)frame.data);
      if(obj->crop.length() > 0) mgk0.crop(obj->crop);
      mgk.scale("2x2!");
      mgk0=mgk;
      continue;
    }
    Magick::Image mgk1(frame.cols, frame.rows, "BGR", Magick::CharPixel, (char *)frame.data);
    if(obj->crop.length() > 0) mgk1.crop(obj->crop);
    mgk1.scale("2x2!");
    if(time1 != time0) {
      coeff0 = (avgTime-time0)/(time1-time0);
      coeff1 = 1.0 - coeff0;
    }
    else {
      coeff0 = 1.0;
      coeff1 = 0.0;
    }
    Magick::Pixels pxls0(mgk0);
    Magick::Pixels pxls1(mgk1);
    const Magick::Quantum * pixels0 = pxls0.getConst(0,0,2,2);
    const Magick::Quantum * pixels1 = pxls1.getConst(0,0,2,2);
    for(int i = 0; i < 12; i++) output.push_back((uint8_t)(coeff0*pixels0[i]+coeff1*pixels1[i]));
    vCap >> frame;
    mgk0=mgk1;
    avgTime+=incT;
    time0 = time1;
  }
  std::ofstream outfile(outPath.c_str(),std::ofstream::binary);
  outfile.write((char*)&(output[0]),output.size());
  outfile.close();
  std::cout << "Trace: " << obj->vid << "done" << std::endl;
  return true; 
}

bool video_utils::create_thumb(VidFile * vidFile) {
  std::string thumb_size = appConfig->get_string("thumb_size");
  int cWidth = appConfig->get_int("thumb_width");
  std::string crop(find_border(vidFile->fileName, vidFile->length, vidFile->height, vidFile->width));
  if(crop.length() > 0) {
    vidFile->crop=crop;
    dbCon->save_crop(vidFile);
  }
  float thumb_t = appConfig->get_float("thumb_time");
  if(vidFile->length < thumb_t) thumb_t = vidFile->length/2.0;
  bfs::path icon_file(createPath(thumbPath,vidFile->vid,".jpg"));
  std::vector<char> * buffer = new std::vector<char>(3*vidFile->height*vidFile->width);
  libav_frame(vidFile->fileName,1000.0*thumb_t,buffer);
  Magick::Image mgk(vidFile->width, vidFile->height, "BGR", Magick::CharPixel, (char*)&((*buffer)[0]));
  if(crop.length() > 0) mgk.crop(crop);
  mgk.resize(thumb_size);
  mgk.write(icon_file.c_str());
  delete buffer;
  return true;
}

std::string video_utils::find_border(bfs::path & fileName, float length, int height, int width) {
  std::string crop("");
  int cBFrames = appConfig->get_int("border_frames");
  float cCutThresh = appConfig->get_float("cut_thresh");
  float frame_time = 0.5*length/(cBFrames+1.0);
  float frame_spacing = length/(cBFrames+1.0);
  std::vector<char> * imgDat0 = new std::vector<char>(width*height*3);
  std::vector<char> * imgDat1 = new std::vector<char>(width*height*3);
  libav_frame(fileName, frame_time, imgDat0);
  std::vector<float> rowSums(height);
  std::vector<float> colSums(width);
  float corrFactorCol = 1.0/(float)(cBFrames*height);
  float corrFactorRow = 1.0/(float)(cBFrames*width);
  bool skipBorder = false;
  std::vector<char> * ptrHolder;
  for(int i = 0; i < cBFrames; i++) {
    frame_time+=frame_spacing;
    libav_frame(fileName,frame_time,imgDat1);
    for (int y=0; y < height; y++)
      for (int x=0; x < width; x++) {
	int rpos=3*(y*width+x);
	int gpos=rpos+1;
	int bpos=gpos+1;
	float value = sqrt(1.0/3.0*(pow((*imgDat1)[rpos]-(*imgDat0)[rpos],2.0)+pow((*imgDat1)[gpos]-(*imgDat0)[gpos],2.0)+pow((*imgDat1)[bpos]-(*imgDat0)[bpos],2.0)));
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
    //This might be taking 1 more pixel in each direction than it should.
    while(colSums[x1] < cCutThresh && x1 < width-1) x1++;
    while(colSums[x2] < cCutThresh && x2 > x1) x2--;
    while(rowSums[y1] < cCutThresh && y1 < height -1) y1++;
    while(rowSums[y2] < cCutThresh && y2 > y1) y2--;
    if( x1 < x2 and  y1 < y2) { 
      x2-=x1;  
      y2-=y1;
      crop = (boost::format("%ix%i+%i+%i,")% x2 % y2 % x1 % y1).str();
    }
    else std::cout << "Find border failed for:" << fileName.string() << std::endl;  
  }
  delete imgDat0;
  delete imgDat1;
  return crop;
}

void video_utils::create_image(bfs::path & fileName, float start_time, std::vector<char> * imgDat) {
  cv::Mat frame;
  cv::VideoCapture vCap(fileName.c_str());
  vCap.set(cv::CAP_PROP_POS_MSEC,1000.0*start_time);
  vCap >> frame;
  int size = 3*frame.total();
  for(int i = 0; i < size; i++) (*imgDat)[i] = frame.data[i];
  return;
}
  
bool video_utils::thumb_exists(int vid) {
  return bfs::exists(createPath(thumbPath,vid,".jpg"));
}

Magick::Image *video_utils::get_image(int vid, bool firstImg) {
  if(img_cache.first == vid) return img_cache.second;
  bfs::path image_file(createPath(thumbPath,vid,".jpg"));
  if(firstImg) delete img_cache.second;
  Magick::Image * img = new Magick::Image(image_file.string());
  if(firstImg) img_cache = std::make_pair(vid,img);  
  return img;
}

bool video_utils::compare_images(int vid1, int vid2) {
  Magick::Image * img1 = get_image(vid1,true);
  Magick::Image * img2 = get_image(vid2,false);
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
    delete img2;
    difference = sqrt(coeff*difference);
    if(difference < cImgThresh) {
      std::cout << "IMG MATCH " << vid1 << " " << vid2 << std::endl;
      return true;
    }
  }
  else {
    delete img2;
  }
  return false;
}

void video_utils::img_comp_thread() {
  TPool->push([&]() {return compare_icons();});
  return;
}

void video_utils::compare_icons() {
  for(uint i = 0; i +1 < fVIDs.size(); i++) { 
    for(uint j = i+1; j < fVIDs.size(); j++) {
      std::pair<int,int> key(fVIDs[i],fVIDs[j]);
      if (result_map[key]%2  == 1 || result_map[key] ==4) continue;
      else if(compare_images(fVIDs[i],fVIDs[j])) result_map[key]+=1;      
    }
  }
  return;
}

void video_utils::start_thumbs(std::vector<VidFile *> & vFile) {
  for(auto &a: vFile) resVec.push_back(TPool->push([&](VidFile *b) {return create_thumb(b);}, a));
  return;
}

int video_utils::start_make_traces(std::vector<VidFile *> & vFile) {
  resVec.clear();
  bfs::path traceDir = savePath;
  traceDir+="traces/";
  for(auto & b: vFile) {
    bfs::path tPath(createPath(traceDir,b->vid,".bin"));
    if(!bfs::exists(tPath)) resVec.push_back(TPool->push([&](VidFile * b ){ return calculate_trace(b);},b));
  }
  return resVec.size();
}

void video_utils::move_traces() {
  bfs::path traceSave = savePath;
  traceSave+="traces/";
  if(!bfs::exists(traceSave)) bfs::create_directory(traceSave);
  for(auto & x: bfs::directory_iterator(tracePath)) {
    auto extension = x.path().extension().c_str();
    if(strcmp(extension,".bin") == 0) {
      bfs::path source = x.path();
      bfs::path destination = traceSave;
      destination+=relative(source,tracePath);
      bfs::copy(source,destination);
    }
  }
  bfs::remove_all(tracePath);
  return;
}

std::vector<std::future_status>  video_utils::get_status() {
  std::chrono::milliseconds timer(1);
  return cxxpool::wait_for(resVec.begin(), resVec.end(),timer);  
}

int video_utils::compare_traces() {
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
  return resVec.size();
}

int video_utils::make_vids(std::vector<VidFile *> & vidFiles) {
  std::vector<bfs::path> pathVs;
  fVIDs.clear();
  int counter = 0;
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
	  pathVs.push_back(pathName);
	}
	else {
	  VidFile * v =dbCon->fetch_video(pathName);
	  vidFiles.push_back(v);
	  fVIDs.push_back(v->vid);
	  current_dir_files.push_back(pathName);
	}
	counter++;
      }     
    }    
    //if(!new_db) dbCon->cleanup(path,current_dir_files);  //Bring back
  }
  TPool->push([this](std::vector<bfs::path> pathvs) {return vid_factory(pathvs);},pathVs);
  resVec.clear();  //need to do it here because start_thumbs is called multiple times.
  return counter;
}

void video_utils::set_paths(std::vector<bfs::path> & folders) {
  paths.clear();
  for(auto & a: folders) paths.push_back(a);
  return;
}

void video_utils::close() {
  dbCon->save_config(appConfig->get_data());
  dbCon->save_db_file();
  std::system("reset");
  return;
}

qvdb_config * video_utils::get_config(){
  return appConfig;
}

bool video_utils::vid_factory(std::vector<bfs::path> & files) {
  MediaInfoLib::MediaInfo MI;
  int nThreads = appConfig->get_int("threads");
  for(uint h = 0; h*nThreads < files.size(); h++) {
    std::vector<VidFile *> batch;   
    for(uint i = 0; i < nThreads && i+h*nThreads < files.size(); i++) {
      ZenLib::Ztring zFile;
      auto fileName = files[i+h*nThreads];
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
      MI.Option(__T("Inform"),__T("Video;%Height%"));
      int height = ZenLib::Ztring(MI.Inform()).To_int32s();
      MI.Option(__T("Inform"),__T("Video;%Width%"));
      int width = ZenLib::Ztring(MI.Inform()).To_int32s();
      MI.Close();
      VidFile * newVid = new VidFile(fileName, length, size, 0, -1, "", rotate, height, width);
      batch.push_back(newVid);
    }
    mtx.lock();
    completedVFs.push_back(batch);
    mtx.unlock();
  }
  return true;
}

bool video_utils::getVidBatch(std::vector<VidFile*> & batch) {
  if(completedVFs.size() == 0) return false;
  batch = completedVFs.front();
  std::vector<int> bVids;
  for(auto & a: batch) {
    dbCon->save_video(a);	
    fVIDs.push_back(a->vid);
    bVids.push_back(a->vid);
  }
  metaData->load_file_md(bVids);
  mtx.lock();
  completedVFs.pop_front();
  mtx.unlock();
  return true;
}

void video_utils::load_trace(int vid) {
  bfs::path traceSave = savePath;
  traceSave+="traces/";
  std::ifstream dataFile(createPath(traceSave,vid,".bin").string(),std::ios::in|std::ios::binary|std::ios::ate);
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

bfs::path video_utils::createPath(bfs::path & path, int vid, std::string extension) {
  return bfs::path((boost::format("%s%i%s") % path.string() % vid % extension).str());
}

bfs::path video_utils::icon_filename(int vid) {
  return createPath(thumbPath,vid,".jpg");
}

void video_utils::libav_frame(bfs::path & fileName, float start_time, std::vector<char> * imgDat) {
  AVFormatContext *pFormatContext = NULL;
  int ret = avformat_open_input(&pFormatContext, fileName.c_str(), NULL, NULL);
  if(ret < 0) std::cout << "trouble opening: " << fileName.c_str() << std::endl;
  avformat_find_stream_info(pFormatContext,  NULL);
  AVCodec *pCodec;
  AVCodecParameters *pCodecParameters;
  int index=0;
  for (; index < pFormatContext->nb_streams; index++) {
    pCodecParameters = pFormatContext->streams[index]->codecpar;
    if (pCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
      break;
    }
  }
  AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
  avcodec_parameters_to_context(pCodecContext, pCodecParameters);
  avcodec_open2(pCodecContext, pCodec, NULL);
  int w = pCodecContext->width;
  int h = pCodecContext->height;
  AVPacket *pPacket = av_packet_alloc();
  AVFrame *pFrame = av_frame_alloc();
  AVFrame *pFrameRGB = av_frame_alloc();
  if(!pFrame || !pFrameRGB) {
    std::cout << "Couldn't allocate frame" << std::endl;
  }
  ret = 0;
  int done = 0;
  int ts = 0;
  if(start_time > 0.0) {
    ret = avformat_seek_file(pFormatContext,index,1000*start_time,1000*start_time,INT64_MAX,0);
    if(ret < 0) while(av_read_frame(pFormatContext,pPacket) >= 0 && ts <= 1000*start_time) ts = pPacket->pts;
  }
  while(av_read_frame(pFormatContext,pPacket) >= 0) {
    if (pPacket->stream_index == index) {
      int ret = avcodec_send_packet(pCodecContext, pPacket);
      if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
	std::cout << "avcodec_send_packet: " << ret << std::endl;
	break;
      }
      while (ret  >= 0) {
	ret = avcodec_receive_frame(pCodecContext, pFrame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || pFrame->key_frame == 0) {
	  //std::cout << "avcodec_receive_frame: " << ret << std::endl;
	  break;
	}
	else {
	  done = 1;
	  break;
	}
      }
    }
    av_packet_unref(pPacket);
    if(done) {
      static struct SwsContext *img_convert_ctx = sws_getContext(w, h, pCodecContext->pix_fmt, w, h, AV_PIX_FMT_RGB24, SWS_POINT, NULL, NULL, NULL);;
      if (av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize,(const uint8_t *)&((*imgDat)[0]), AV_PIX_FMT_RGB24, w,h,1) < 0) std::cout <<"avpicture_fill() failed" << std::endl;
      std::cout <<fileName.c_str()<< " Time: " << pFrame->pts << std::endl;
      sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, h, pFrameRGB->data, pFrameRGB->linesize);
      return;
    }
  }
  return;
}
 
