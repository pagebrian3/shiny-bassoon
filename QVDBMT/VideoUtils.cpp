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
#include <zstd.h>
#include <fstream>
#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

video_utils::video_utils() {
  if(strcmp(PLATFORM_NAME,"linux") == 0) {
    savePath = getenv("HOME");
  }
  else if(strcmp(PLATFORM_NAME,"windows") == 0) {
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
    for(k = 0; k < (unsigned)(length2-trT); k++)  {
      sum=0.0;
      for(l=0; l <numVars; l++) sum+=in[numVars*k+l]/(uSize* 6*(compCoeffs[l]+coeffs[numVars*k+l]));
      result[k]=pow(sum,cPower);
    }
    float max_val = *max_element(result.begin(),result.end());
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
  bfs::path outPath = createPath(tracePath,obj->vid,".bin");
  std::vector<int> crop = obj->crop;
  std::vector<int> times;
  std::vector<std::vector<char> > traceDat(12);
  SwsContext *img_convert_ctx;
  AVFormatContext *pFormatContext=NULL;
  bfs::path fileName = obj->fileName;
  int ret = avformat_open_input(&pFormatContext, fileName.c_str(), NULL, NULL);
  if(ret < 0) {
    std::cout << "trouble opening: " << fileName.c_str() << std::endl;
    return false;
  }
  avformat_find_stream_info(pFormatContext,  NULL);
  AVCodec *pCodec;
  AVCodecParameters *pCodecParameters;
  uint index=0;
  AVRational time_base;
  for (; index < pFormatContext->nb_streams; index++) {
    pCodecParameters = pFormatContext->streams[index]->codecpar;
    if (pCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
      time_base = pFormatContext->streams[index]->time_base;
      break;
    }
  }
  AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
  avcodec_parameters_to_context(pCodecContext, pCodecParameters);
  avcodec_open2(pCodecContext, pCodec, NULL);
  int w = pCodecContext->width;
  int h = pCodecContext->height;
  int cropW = w;
  int cropH = h;
  cropW -= (crop[0]+crop[1]);
  cropH -= (crop[2]+crop[3]);
  std::vector<char> imgDat(3*w*h);
  AVPacket *pPacket = av_packet_alloc();
  AVFrame *pFrame = av_frame_alloc();
  AVFrame *pFrameRGB = av_frame_alloc();
  if(!pFrame || !pFrameRGB)
    std::cout << "Couldn't allocate frame" << std::endl;
  double tConv = 1.0/av_q2d(time_base);
  img_convert_ctx = sws_getContext(w, h, pCodecContext->pix_fmt, w, h, AV_PIX_FMT_RGB24, SWS_POINT, NULL, NULL, NULL);
  if (av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize,(const uint8_t *)&(imgDat[0]), AV_PIX_FMT_RGB24, w,h,1) < 0) std::cout <<"avpicture_fill() failed" << std::endl;
  ret = avformat_seek_file(pFormatContext,index,0,tConv*start_time,tConv*start_time,0); //seek before
  if(ret < 0) std::cout << fileName.c_str() << " avformat_seek_file error return " <<ret<< std::endl;
  while(av_read_frame(pFormatContext,pPacket) >= 0) {
    if(pPacket->stream_index != (signed)index) continue;
    int ret = avcodec_send_packet(pCodecContext, pPacket);
    if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      return false;
    }
    float norm = 4.0/(cropW*cropH);
    //deal with odd numbered cropW/cropH
    if(avcodec_receive_frame(pCodecContext, pFrame) == 0) {
      times.push_back(pFrame->pts);
      std::vector<int> temp(12);
      sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, h, pFrameRGB->data, pFrameRGB->linesize);
      int y = 0;
      auto dataIter = imgDat.begin();
      for(; y < cropH/2; y++) {
	dataIter = imgDat.begin()+3*((crop[3]+y)*w+crop[0]);
	int x = 0;
	for(; x < 3*cropW/2; x++) {
	  temp[x%3]+=*dataIter;
	  dataIter++;
	}
	for(; x < 3*cropW; x++) {
	  temp[x%3+3]+=*dataIter;
	  dataIter++;
	}
      }
      for(; y < cropH; y++) {
	auto dataIter = imgDat.begin()+3*((crop[3]+y)*w+crop[0]);
	int x = 0;
	for(; x < 3*cropW/2; x++) {
	  temp[x%3+6]+=*dataIter;
	  dataIter++;
	}
	for(; x < 3*cropW; x++) {
	  temp[x%3+9]+=*dataIter;
	  dataIter++;
	}
      }      
      for(int i = 0; i < 12; i++) traceDat[i].push_back(norm*temp[i]);
    }
  }
  avformat_close_input(&pFormatContext);
  sws_freeContext(img_convert_ctx);
  avcodec_free_context(&pCodecContext);
  av_frame_free(&pFrame);
  av_frame_free(&pFrameRGB);
  av_packet_free(&pPacket); 
  std::vector<boost::math::barycentric_rational<float> *> b;
  for(int i = 0; i < 12; i++) {
    auto c = new boost::math::barycentric_rational<float>(times.begin(),times.end(),traceDat[i].begin());
    b.push_back(c);
  }
  double frame_spacing = tConv/fps;
  double sample_time=start_time*tConv;
  double length = times.back();
  bfs::path traceFile = createPath(tracePath,vid,".bin");
  std::ofstream ofile(traceFile.c_str(),std::ofstream::binary);
  std::vector<char> dataVec;
  for(; sample_time < length; sample_time+=frame_spacing)
    for( int i = 0; i < 12; i++) dataVec.push_back((char) (*b[i])(sample_time));
  int dist_cap = 2*ZSTD_compressBound(dataVec.size());
  std::vector<char> compressVec(dist_cap);
  int dist_size = ZSTD_compress(&compressVec[0],dist_cap,&dataVec[0],dataVec.size(),15);
  ofile.write(&compressVec[0], dist_size);
  ofile.close();
  return true; 
}

bool video_utils::create_thumb(VidFile * vidFile) {
  std::string thumb_size = appConfig->get_string("thumb_size");
  int w = vidFile->width;
  int h = vidFile->height;
  int vid = vidFile->vid;
  std::vector<char> first_frame(3*w*h);
  std::vector<int> crop(4);
  if(!find_border(vidFile,first_frame, crop)) return false;
  bfs::path icon_file(icon_filename(vid));
  Magick::Image mgk(w, h, "RGB", Magick::CharPixel, &(first_frame[0]));
  vidFile->crop=crop;
  dbCon->save_crop(vidFile);
  if(crop[0]+crop[1]+crop[2]+crop[3] > 0) {
    int cropW = w - crop[0] - crop[1];
    int cropH = h - crop[2] - crop[3];
    std::string cropStr=(boost::format("%ix%i+%i+%i") % cropW % cropH % crop[0] % crop[2]).str();
    mgk.crop(cropStr.c_str());
  }
  mgk.resize(thumb_size);
  mgk.write(icon_file.c_str());
  return true;
}

bool video_utils::find_border(VidFile * vidFile, std::vector<char> &first_frame, std::vector<int> & crop) {
  bfs::path fileName = vidFile->fileName;
  unsigned int height = vidFile->height;
  unsigned int width = vidFile->width;
  double length = vidFile->length;
  double thumb_t = appConfig->get_float("thumb_time");
  int cBFrames = appConfig->get_int("border_frames");
  double cCutThresh = appConfig->get_float("cut_thresh");
  double frame_time = thumb_t;
  double frame_spacing = (length-thumb_t)/(double)cBFrames;
  std::vector<char> imgDat0;
  std::vector<char> imgDat1(3*width*height);
  if(!frameNoCrop(fileName, frame_time, first_frame)) {
    return false;
  }
  imgDat0 = first_frame;
  std::vector<float> rowSums(height);
  std::vector<float> colSums(width);
  float corrFactorCol = 1.0/(float)(cBFrames*height);
  float corrFactorRow = 1.0/(float)(cBFrames*width);
  bool skipBorder = false;
  std::vector<char> ptrHolder;
  for(int i = 0; i < cBFrames; i++) {
    frame_time+=frame_spacing;
    frameNoCrop(fileName,frame_time,imgDat1);
    for (unsigned int y=0; y < height; y++)
      for (unsigned int x=0; x < width; x++) {
	int rpos=3*(y*width+x);
	int gpos=rpos+1;
	int bpos=gpos+1;
	float value = sqrt(1.0/3.0*(pow(imgDat1[rpos]-imgDat0[rpos],2.0)+pow(imgDat1[gpos]-imgDat0[gpos],2.0)+pow(imgDat1[bpos]-imgDat0[bpos],2.0)));
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
    ptrHolder=imgDat1;
    imgDat1=imgDat0;
    imgDat0=ptrHolder;
  }
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
  return true;
}
 
bool video_utils::thumb_exists(int vid) {
  return bfs::exists(icon_filename(vid));
}

bool video_utils::trace_exists(int vid) {
  bfs::path traceSave = savePath;
  traceSave+="traces/";
  return bfs::exists(createPath(traceSave,vid,".bin"));
}

Magick::Image *video_utils::get_image(int vid, bool firstImg) {
  if(img_cache.first == vid) return img_cache.second;
  if(firstImg) delete img_cache.second;
  bfs::path image_file(icon_filename(vid));
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
  else delete img2;
  return false;
}

void video_utils::img_comp_thread() {
  TPool->push([&]() {return compare_icons();});
  return;
}

void video_utils::compare_icons() {
  for(uint i = 0; i +1 < fVIDs.size(); i++) { 
    for(uint j = i+1; j < fVIDs.size(); j++) {
      std::tuple<int,int,int> key(fVIDs[i],fVIDs[j],2);
      if (result_map[key].first  != 0) continue;
      int result = 1;
      if(compare_images(fVIDs[i],fVIDs[j])) result = 2;
      else result_map[key]=std::make_pair(result,"");  //It would be nice to capture a closeness metric here
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
    if(!load_trace(fVIDs[i])) continue;
    //loop over files after i
    for(uint j = i+1; j < fVIDs.size(); j++) {
      if (result_map[std::make_tuple(fVIDs[i],fVIDs[j],1)].first > 0 ) continue;  
      if(!load_trace(fVIDs[j])) continue;
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
    std::vector<int> orphans = dbCon->cleanup(path,current_dir_files);
    bfs::path traceSave = savePath;
    traceSave+="traces/";
    for(auto & a: orphans) {
      bfs::remove(icon_filename(a));
      bfs::remove(createPath(traceSave,a,".bin"));
    }
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
  unsigned int nThreads = appConfig->get_int("threads");
  std::vector<int> blank(4);
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
      VidFile * newVid = new VidFile(fileName, length, size, 0, -1, blank, rotate, height, width);
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
  mtx.lock();
  completedVFs.pop_front();
  mtx.unlock();
  return true;
}

bool video_utils::load_trace(int vid) {
  if(!trace_exists(vid)) return false;
  bfs::path traceSave = savePath;
  traceSave+="traces/";
  bfs::path traceFN = createPath(traceSave,vid,".bin");  
  std::ifstream dataFile(traceFN.string(),std::ios::in|std::ios::binary|std::ios::ate);
  dataFile.seekg (0, dataFile.end);
  int length = dataFile.tellg();
  dataFile.seekg (0, dataFile.beg);
  char buffer[length];
  dataFile.read(buffer,length);
  dataFile.close();
  int decompSize = ZSTD_getFrameContentSize(buffer,length);
  std::vector<char> * temp = new std::vector<char>(decompSize);
  ZSTD_decompress(&(*temp)[0], decompSize, buffer, length);
  traceData[vid] = *temp;
  return true;
}

bfs::path video_utils::createPath(bfs::path & path, int vid, std::string extension) {
  return bfs::path((boost::format("%s%i%s") % path.string() % vid % extension).str());
}

bfs::path video_utils::icon_filename(int vid) {
  return createPath(thumbPath,vid,".jpg");
}

bool video_utils::frameNoCrop(bfs::path & fileName, double start_time, std::vector<char> & imgDat) {
  SwsContext *img_convert_ctx;
  AVFormatContext *pFormatContext=NULL;
  int ret = avformat_open_input(&pFormatContext, fileName.c_str(), NULL, NULL);
  if(ret < 0) {
    std::cout << "trouble opening: " << fileName.c_str() << std::endl;
    return false;
  }
  avformat_find_stream_info(pFormatContext,  NULL);
  AVCodec *pCodec;
  AVCodecParameters *pCodecParameters;
  unsigned int index=0;
  AVRational time_base;
  for (; index < pFormatContext->nb_streams; index++) {
    pCodecParameters = pFormatContext->streams[index]->codecpar;
    if (pCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
      time_base = pFormatContext->streams[index]->time_base;
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
  if(!pFrame || !pFrameRGB)
    std::cout << "Couldn't allocate frame" << std::endl;
  double tConv = 1.0/av_q2d(time_base);
  bool exit_flag = false;
  ret = avformat_seek_file(pFormatContext,index,0,tConv*start_time,tConv*start_time,0); //seek before
  if(ret < 0) std::cout << fileName.c_str() << " avformat_seek_file error return " <<ret<< std::endl;
  while(av_read_frame(pFormatContext,pPacket) >= 0) {
    if(pPacket->stream_index != (signed)index) continue;
    int ret = avcodec_send_packet(pCodecContext, pPacket);
    if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      return false;
    }
    if(avcodec_receive_frame(pCodecContext, pFrame) == 0) {
      exit_flag=true;
      break;      
    }
    if(exit_flag) break;   
  }
  img_convert_ctx = sws_getContext(w, h, pCodecContext->pix_fmt, w, h, AV_PIX_FMT_RGB24, SWS_POINT, NULL, NULL, NULL);
  if (av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize,(const uint8_t *)&(imgDat[0]), AV_PIX_FMT_RGB24, w,h,1) < 0) std::cout <<"avpicture_fill() failed" << std::endl;
  sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, h, pFrameRGB->data, pFrameRGB->linesize);
  avformat_close_input(&pFormatContext);
  sws_freeContext(img_convert_ctx);
  avcodec_free_context(&pCodecContext);
  av_frame_free(&pFrame);
  av_frame_free(&pFrameRGB);
  av_packet_free(&pPacket);
  return true;
}
