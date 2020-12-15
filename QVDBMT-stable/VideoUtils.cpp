#include <DbConnector.h>
#include <VideoUtils.h>
#include <QVBMetadata.h>
#include <QVBConfig.h>
#include <VidFile.h>
#include <QVDec.h>
#include <Magick++.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/math/interpolators/vector_barycentric_rational.hpp>
#include <fftw3.h>
#include <zstd.h>
#include <fstream>
#include <iostream>


video_utils::video_utils() {
  if(strcmp(PLATFORM_NAME,"linux") == 0) {
    savePath = getenv("HOME");
  }
  else if(strcmp(PLATFORM_NAME,"windows") == 0) {
    std::string temp(getenv("USERPROFILE"));
    savePath =temp.substr(temp.find("="));
  }
  else std::cout << "Unsupported platform" << std::endl;
  homePath = savePath;
  savePath+="/.video_proj/";
  tempPath = "/tmp/qvdbtmp/";
  if(!std::filesystem::exists(tempPath))std::filesystem::create_directory(tempPath);
  tracePath=tempPath;
  tracePath+="traces/";
  thumbPath=savePath;
  thumbPath+="thumbs/";
  if(!std::filesystem::exists(tracePath))std::filesystem::create_directory(tracePath);
  if(!std::filesystem::exists(savePath)) std::filesystem::create_directory(savePath);
  if(!std::filesystem::exists(thumbPath)) std::filesystem::create_directory(thumbPath);
  dbCon = new DbConnector(savePath,tempPath);
  dbCon->fetch_results(result_map);
  appConfig = new qvdb_config();
  metaData = new qvdb_metadata(dbCon);
  if(appConfig->load_config(dbCon->fetch_config())) dbCon->save_config(appConfig->get_data());
  Magick::InitializeMagick("");
  TPool = new cxxpool::thread_pool(appConfig->get_int("threads"));
  decodeDevice = appConfig->get_string("hwdecoder");
}

void video_utils::set_vid_vec(std::vector<int> * vid_list) {
  fVIDs = vid_list;
}

qvdb_metadata * video_utils::mdInterface() {
  return metaData;
}

bool video_utils::compare_vids_fft(int i, int j) {
  bool match=false;
  uint numVars=12;
  float cTraceFPS = appConfig->get_float("trace_fps");
  float cSliceSpacing = appConfig->get_float("slice_spacing");
  float cThresh = appConfig->get_float("thresh");
  float cPower = appConfig->get_float("power");
  int trT = cTraceFPS*appConfig->get_float("comp_time");
  int size = 2;  //size of 1 transform  must be a power of 2
  uint length1 = traceData[i].size()/numVars;
  int length2 = traceData[j].size()/numVars;
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
  std::stringstream ss;
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
  float a,b,c,d,sum,max_val;
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
      c = out[k][0];
      d = out[k][1];
      a = cHolder[k][0];
      b = cHolder[k][1];
      out[k][0]=a*c+b*d;
      out[k][1]=b*c-a*d;
    }
    fftwf_execute(rPlan);
    for(k = 0; k < (unsigned)(length2-trT); k++)  {
      sum=0.0;
      for(l=0; l <numVars; l++) sum+=in[numVars*k+l]/(uSize* 6*(compCoeffs[l]+coeffs[numVars*k+l]));
      result[k]=pow(sum,cPower);
    }
    max_val = *max_element(result.begin(),result.end());
    int max_offset = max_element(result.begin(),result.end()) - result.begin();
    if(max_val > cThresh) {
      std::cout << "Match! " <<i <<" " << j<<" "<<t_s<<" "<< max_val <<" "<<max_offset << std::endl;
      ss <<t_s<<" "<< max_val <<" "<<max_offset;
      match = true;
      break;
    }
  }
  std::tuple<int,int,int> key(i,j,1);
  int resultFlag = 1;
  if(match) resultFlag = 2;
  result_map[key]=std::make_pair(resultFlag,ss.str());
  dbCon->update_results(i,j,1,resultFlag,ss.str());
  fftwf_free(in);
  fftwf_free(out);
  return true;
}

bool video_utils::calculate_trace(VidFile * obj) {
  int vid = obj->vid;
  float fps = appConfig->get_float("trace_fps");
  float start_time = appConfig->get_float("thumb_time");
  if(obj->length <= start_time) start_time=0.0;
  std::filesystem::path outPath = createPath(tracePath,obj->vid,".bin");
  std::vector<int> crop = obj->crop;
  std::vector<float> times;
  std::vector<std::array<float,12> > traceDat;
  qvdec decoder(obj,decodeDevice);
  if(decoder.get_error()) return false;
  double tConv = decoder.get_trace_data(start_time,crop,times,traceDat);
  float time1,time2;
  if(times.size() == 0) {
    std::cout << "Empty time vec for " <<obj->fileName<< std::endl;
    return false;
  }
  time1=times[0];
  for(size_t i = 1; i < times.size(); i++) {
    time2=times[i];
    if(time2 == time1) std::cout <<"Same time" << i << std::endl;
    if(time2 < time1) std::cout <<"Wrong order " << i << std::endl;
    time1=time2;
  }
  double length = times.back();
  boost::math::vector_barycentric_rational<std::vector<float>,std::vector<std::array<float,12>>> * interpolator = new boost::math::vector_barycentric_rational<std::vector<float>,std::vector<std::array<float,12>>>(std::move(times),std::move(traceDat));
  double frame_spacing = tConv/fps;
  double sample_time=start_time*tConv;
  //std::cout <<"Time info: "<<sample_time<<" "<< frame_spacing <<" "<<length<< std::endl;
  std::filesystem::path traceFile = createPath(tracePath,vid,".bin");
  std::ofstream ofile(traceFile.c_str(),std::ofstream::binary);
  std::vector<uint8_t> dataVec;
  for(; sample_time < length; sample_time+=frame_spacing){
    std::array<float,12> dPoint = (*interpolator)(sample_time);
    dataVec.insert(dataVec.end(),dPoint.begin(),dPoint.end());
  }
  int dist_cap = 2*ZSTD_compressBound(dataVec.size());
  std::vector<char> compressVec(dist_cap);
  int dist_size = ZSTD_compress(&compressVec[0],dist_cap,&dataVec[0],dataVec.size(),15);
  ofile.write(&compressVec[0], dist_size);
  ofile.close();
  delete interpolator;
  return true; 
}

bool video_utils::create_thumb(VidFile * vidFile) {
  std::string thumb_size = appConfig->get_string("thumb_size");
  uint8_t * first_frame[4];
  first_frame[0]=NULL;
  std::vector<int> crop(4);
  if(!find_border(vidFile,first_frame, crop)) {
    vidFile->okflag=-1;
    dbCon->save_video(vidFile);
    return false;
  }
  int w = vidFile->width;
  int h = vidFile->height;
  Magick::Image mgk(w, h, "RGB", Magick::CharPixel, first_frame[0]);
  vidFile->crop=crop;
  dbCon->save_video(vidFile);
  int vid = vidFile->vid;
  std::filesystem::path icon_file(icon_filename(vid));
  if(crop[0]+crop[1]+crop[2]+crop[3] > 0) {
    int cropW = w - crop[0] - crop[1];
    int cropH = h - crop[2] - crop[3];
    std::string cropStr=(boost::format("%ix%i+%i+%i") % cropW % cropH % crop[0] % crop[2]).str();
    mgk.crop(cropStr.c_str());
  }
  mgk.resize(thumb_size);
  mgk.write(icon_file.c_str());
  av_freep(&(first_frame[0]));
  return true;
}

bool video_utils::create_frames(VidFile * vidFile) {
  int w = vidFile->width;
  int h = vidFile->height;
  int vid = vidFile->vid;
  uint8_t * imgDat[4];
  imgDat[0]=NULL;
  float start_time = appConfig->get_float("thumb_time");
  float frame_interval = appConfig->get_float("frame_interval");
  std::vector<int> crop(vidFile->crop);
  qvdec decoder(vidFile,decodeDevice);
  if(decoder.get_error()) return false;
  while(start_time < vidFile->length){
    decoder.get_frame(imgDat,start_time);
    std::stringstream ss;
    ss << tempPath.string() << "/face_temp/" << vid << "_"<<start_time<<".png";
    std::filesystem::path icon_file(ss.str());
    Magick::Image mgk(w, h, "RGB", Magick::CharPixel, imgDat[0]);
    if(crop[0]+crop[1]+crop[2]+crop[3] > 0) {
      int cropW = w - crop[0] - crop[1];
      int cropH = h - crop[2] - crop[3];
      std::string cropStr=(boost::format("%ix%i+%i+%i") % cropW % cropH % crop[0] % crop[2]).str();
      mgk.crop(cropStr.c_str());
    }
    mgk.write(icon_file.c_str());
    std::filesystem::rename(icon_file,tempPath/"face_temp/frames"/icon_file.filename());
    start_time+=frame_interval;
  }
  decoder.flush_decoder();
  return true;
}

bool video_utils::StartFaceFrames(std::vector<VidFile *> & vFile) {
  for(auto &a: vFile) resVec.push_back(TPool->push([&](VidFile *b) {return create_frames(b);}, a));
  return true;
}

bool video_utils::find_border(VidFile * vidFile, uint8_t ** first_frame, std::vector<int> & crop) {
  float thumb_t = appConfig->get_float("thumb_time");
  int cBFrames = appConfig->get_int("border_frames");
  float cCutThresh = appConfig->get_float("cut_thresh");
  float frame_time = thumb_t;
  qvdec decoder(vidFile,decodeDevice);
  if(decoder.get_error()) return false;
  decoder.get_frame(first_frame,frame_time);
  unsigned int height = vidFile->height;
  unsigned int width = vidFile->width;
  std::filesystem::path fileName = vidFile->fileName;
  float length = vidFile->length;
  float frame_spacing = (length-thumb_t)/(double)cBFrames; 
  uint8_t *imgDat0[4];
  imgDat0[0] = first_frame[0];
  uint8_t * imgDat1[4];
  uint8_t * ptrHolder[4];
  imgDat1[0] = ptrHolder[0] = NULL;
  std::vector<float> rowSums(height);
  std::vector<float> colSums(width);
  float corrFactorCol = 1.0/(float)(cBFrames*height);
  float corrFactorRow = 1.0/(float)(cBFrames*width);
  bool skipBorder = false;
  float value;
  auto r0 = imgDat0[0];
  auto r1(r0),g0(r0),g1(r0),b0(r0),b1(r0);
  for(int i = 0; i < cBFrames-1; i++) {
    frame_time+=frame_spacing;
    decoder.get_frame(imgDat1,frame_time);
    auto dataIter0 = imgDat0[0];
    auto dataIter1 = imgDat1[0];
    for(unsigned int y=0; y < height; y++) 	
      for (unsigned int x=0; x < width; x++) {
	r0=dataIter0;
	dataIter0++;
	g0=dataIter0;
	dataIter0++;
	b0=dataIter0;
	dataIter0++;
	r1=dataIter1;
	dataIter1++;
	g1=dataIter1;
	dataIter1++;
	b1=dataIter1;
	dataIter1++;
	value = sqrt(1.0/3.0*(pow(*r1-*r0,2.0)+pow(*g1-*g0,2.0)+pow(*b1-*b0,2.0)));
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
    //rotate pointers to ensure new one will be freed before allocation.
    ptrHolder[0]=imgDat0[0];
    imgDat0[0]=imgDat1[0];
    imgDat1[0]=ptrHolder[0];
    //preserve first frame
    if(i == 0) imgDat1[0]=NULL;   
  }
  decoder.flush_decoder();
  if(!skipBorder) {
    unsigned int x1(0), x2(0), y1(0), y2(0);
    while(colSums[x1] < cCutThresh && x1 < width) x1++;
    while(colSums[width-1-x2] < cCutThresh && (x2+x1) < width) x2++;
    while(rowSums[y1] < cCutThresh && y1 < height ) y1++;
    while(rowSums[height-1-y2] < cCutThresh && (y2+y1) < height) y2++;
    if( (x1 + x2) < width &&  (y1 + y2) < height) { 
      crop[0]=x1;
      crop[1]=x2;
      crop[2]=y1;
      crop[3]=y2;
    }
  }
  if(imgDat0[0]!=first_frame[0])av_freep(&(imgDat0[0]));
  av_freep(&(imgDat1[0]));
  return true;
}
 
bool video_utils::thumb_exists(int vid) {
  return std::filesystem::exists(icon_filename(vid));
}

bool video_utils::video_exists(std::filesystem::path & filename) {
  return dbCon->video_exists(filename);
}

bool video_utils::trace_exists(int vid) {
  std::filesystem::path traceSave = savePath;
  traceSave+="traces/";
  return std::filesystem::exists(createPath(traceSave,vid,".bin"));
}

Magick::Image *video_utils::get_image(int vid, bool firstImg) {
  if(img_cache.first == vid) return img_cache.second;
  if(firstImg) delete img_cache.second;
  std::filesystem::path image_file(icon_filename(vid));
  Magick::Image * img = new Magick::Image(image_file.string());
  if(firstImg) img_cache = std::make_pair(vid,img);  
  return img;
}

bool video_utils::compare_images(int vid1, int vid2) {
  if(!(thumb_exists(vid1) && thumb_exists(vid2))) return false;
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
    for(int i = 0; i < width; i++) {
      for(int j = 0; j < height; j++)
	for(int k = 0; k < 3; k++) {
      difference+=pow(*pixels,2.0);
      pixels++;
	}
      if(cImgThresh < sqrt(coeff*difference) ) break; 
    }
    delete img2;
    difference = sqrt(coeff*difference);
    if(difference < cImgThresh) {
      std::cout << "IMG MATCH " << vid1 << " " << vid2 << std::endl;
      return true;
    }
  }
  else delete img2;
  return false;
}

void video_utils::img_comp_thread() {
  TPool->push([&]() {return compare_icons();});
  return;
}

void video_utils::compare_icons() {
  std::cout << "Num vids:" << (*fVIDs).size() << std::endl;
  for(uint i = 0; i +1 < (*fVIDs).size(); i++) {
    for(uint j = i+1; j < (*fVIDs).size(); j++) {
      std::tuple<int,int,int> key((*fVIDs)[i],(*fVIDs)[j],2);
      if (result_map[key].first  != 0) continue;
      int result = 1;
      if(compare_images((*fVIDs)[i],(*fVIDs)[j])) result = 2;
      else result_map[key]=std::make_pair(result,"");  //It would be nice to capture a closeness metric here
    }
  }
  delete img_cache.second;
  return;
}

int video_utils::start_make_traces(std::vector<VidFile *> & vFile) {
  resVec.clear();
  std::filesystem::path traceDir = savePath;
  traceDir+="traces/";
  for(auto & b: vFile) {
    std::filesystem::path tPath(createPath(traceDir,b->vid,".bin"));
    if(!std::filesystem::exists(tPath)) resVec.push_back(TPool->push([&](VidFile * b ){ return calculate_trace(b);},b));
  }
  return resVec.size();
}

void video_utils::move_traces() {
  std::filesystem::path traceSave = savePath;
  traceSave+="traces/";
  if(!std::filesystem::exists(traceSave)) std::filesystem::create_directory(traceSave);
  for(auto & x: std::filesystem::directory_iterator(tracePath)) {
    auto extension = x.path().extension().c_str();
    if(strcmp(extension,".bin") == 0) {
      std::filesystem::path source = x.path();
      std::filesystem::path destination = traceSave;
      destination+=relative(source,tracePath);
      std::filesystem::copy(source,destination);
    }
  }
  std::filesystem::remove_all(tracePath);
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
  for(uint i = 0; i +1 < (*fVIDs).size(); i++) {
    if(!load_trace((*fVIDs)[i])) continue;
    //loop over files after i
    for(uint j = i+1; j < (*fVIDs).size(); j++) {
      if (result_map[std::make_tuple((*fVIDs)[i],(*fVIDs)[j],1)].first > 0 ) continue;
      if(!load_trace((*fVIDs)[j])) continue;
      resVec.push_back(TPool->push([&](int i, int j){ return compare_vids_fft(i,j);},(*fVIDs)[i],(*fVIDs)[j]));
    }
  }
  return resVec.size();
}

bool video_utils::make_vids(std::vector<VidFile *> & vidFiles) {
  for(auto &a: vidFiles) resVec.push_back(TPool->push([&](VidFile *b) {return create_thumb(b);}, a));
  return true;
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

bool video_utils::load_trace(int vid) {
  if(!trace_exists(vid)) return false;
  std::filesystem::path traceSave = savePath;
  traceSave+="traces/";
  std::filesystem::path traceFN = createPath(traceSave,vid,".bin");  
  std::ifstream dataFile(traceFN.string(),std::ios::in|std::ios::binary|std::ios::ate);
  dataFile.seekg (0, dataFile.end);
  int length = dataFile.tellg();
  dataFile.seekg (0, dataFile.beg);
  char buffer[length];
  dataFile.read(buffer,length);
  dataFile.close();
  int decompSize = ZSTD_getFrameContentSize(buffer,length);
  std::vector<char> temp(decompSize);
  ZSTD_decompress(&temp[0], decompSize, buffer, length);
  traceData[vid] = temp;
  return true;
}

std::filesystem::path video_utils::createPath(std::filesystem::path & path, int vid, std::string extension) {
  return std::filesystem::path((boost::format("%s%i%s") % path.string() % vid % extension).str());
}

std::filesystem::path video_utils::icon_filename(int vid) {
  return createPath(thumbPath,vid,".jpg");
}

