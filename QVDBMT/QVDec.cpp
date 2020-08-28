extern "C" {
  #include <libavutil/hwcontext.h>
  #include <libavutil/avassert.h>
  #include <libavutil/imgutils.h>
  #include <libavutil/display.h>
}
#include "QVDec.h"
#include <iostream>

static enum AVPixelFormat hw_pix_fmt;

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
  const enum AVPixelFormat *p;

  for (p = pix_fmts; *p != -1; p++) {
    if (*p == hw_pix_fmt)
      return *p;
  }
  std::cout << "Failed to get HW surface format."<<std::endl;
  return AV_PIX_FMT_NONE;
};

qvdec::qvdec(VidFile * vf, std::string hw_type) : file(vf->fileName) {
  initError = false;
  input_ctx = NULL;
  sws_ctx = NULL;
  int ret;
  AVStream *video = NULL;
  decoder_ctx = NULL;
  AVCodec *decoder = NULL;
  enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
  int i;
  ret = 1;
  hwEnable = true;
  hw_device_ctx = NULL;
  data = NULL;
  int size = std::filesystem::file_size(file);
  if(strcmp(hw_type.c_str(),"CPU") !=0 ) {
    type = av_hwdevice_find_type_by_name(hw_type.c_str());
  }
  else {
    hwEnable = false;
    //std::cout << "CPU encoding." << std::endl;
  }
  do {
    /* open the input file */
    if(avformat_open_input(&input_ctx, file.c_str(), NULL, NULL) < 0) {
      initError=true;
      std::cout <<"input failure on "<<file <<std::endl;
      break;
    }
    if(avformat_find_stream_info(input_ctx, NULL) > 0) {
      initError=true;
      std::cout <<"Failure to find stream for "<<file <<std::endl;
      break;
    }
    /* find the video stream information */
    video_stream = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (hwEnable == true) {
      for (i = 0;; i++) {
	const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);       
	if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
	    config->device_type == type) {
	  hw_pix_fmt = config->pix_fmt;
	  break;
	}
      }
    }
    decoder_ctx = avcodec_alloc_context3(decoder);
    video = input_ctx->streams[video_stream];
    avcodec_parameters_to_context(decoder_ctx, video->codecpar);
    AVRational time_base = video->time_base;
    tConv = 1.0/av_q2d(time_base);
    if(hwEnable){
      decoder_ctx->get_format  = get_hw_format;
      if ((ret = av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0)) >= 0) decoder_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    }
    if(ret < 0 || (ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0){
      std::cout <<"Codec open failure on : "<< file << std::endl;
      initError = true;
      break;
    }
    float length = static_cast<float>(video->duration) / static_cast<float>(time_base.den);
    w = decoder_ctx->width;
    h = decoder_ctx->height;
    uint8_t* displaymatrix = av_stream_get_side_data(video, AV_PKT_DATA_DISPLAYMATRIX, NULL);
    int rotate = 0; 
    if(displaymatrix) rotate = -av_display_rotation_get((int32_t*) displaymatrix);
    vf->length=length;
    vf->size=size;
    vf->rotate=rotate;
    vf->height=h;
    vf->width=w;
    //std::cout << "Working on: " << file << std::endl;
  } while(0);
}

int qvdec::decode_write()
{
  AVFrame *frame = NULL, *sw_frame = NULL;
  AVFrame *tmp_frame = NULL;
  int ret = 0;
  bool fail = false;
  ret = avcodec_send_packet(decoder_ctx, &packet);   
  if (ret < 0) {
    std::cout << "Error during decoding"<<std::endl;
    return ret;
  }
  while (1) {
    if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
      std::cout << "Can not alloc frame"<<std::endl;
      ret = AVERROR(ENOMEM);
      fail = true;
    }
    ret = avcodec_receive_frame(decoder_ctx, frame);
    if (!fail) {
      if(ret == AVERROR(EAGAIN)  ) {
	av_frame_free(&frame);
	av_frame_free(&sw_frame);
	return ret;
      }
      else if(ret == AVERROR_EOF){
	av_frame_free(&frame);
	av_frame_free(&sw_frame);
	return 0;
      }
      else if (ret < 0) {
	std::cout << "Error while decoding code ret: "<< ret <<std::endl;
	fail = true;
      }
    }
    if (!fail && (hwEnable && frame->format == hw_pix_fmt)) {
      /* retrieve data from GPU to CPU */
      if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
	std::cout << "Error transferring the data to system memory"<<std::endl;
	fail=true;
	return ret;
      }
      tmp_frame = sw_frame;
    } else tmp_frame = frame;
    int dst_linesize[4];
    time = frame->pts;
    enum AVPixelFormat src_pix_fmt = (AVPixelFormat) tmp_frame->format;
    enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_RGB24;
    int alignment = 32;
    if(sws_ctx == NULL) sws_ctx = sws_getContext(w, h, src_pix_fmt,
			       w, h, dst_pix_fmt,
			       SWS_POINT, NULL, NULL, NULL);
    if (!sws_ctx) {
      std::cout << "SWS Context issue." << std::endl;
      ret = AVERROR(EINVAL);
      return 0;
    }
    if(data[0] != NULL) av_freep(&(data[0]));
    if (!fail ) {
      if(av_image_alloc(data, dst_linesize,w, h, dst_pix_fmt, alignment) < 0) {
	std::cout << "Could not allocate destination image\n" <<std::endl;
	return 0;
      }
    }
    /* convert to destination format */
    if(!fail && ((ret = sws_scale(sws_ctx, (const uint8_t * const*)tmp_frame->data,
				  tmp_frame->linesize, 0, h, data, dst_linesize)) < 0)) {
      std::cout <<"Failed to scale."<<std::endl;
      fail=true;
    }

    if(!fail && dst_linesize[0] > 3*w) {
      for(int j = 1; j < h; j++) 
	for(int i = 0; i < 3*w; i++) {
	  data[0][i+j*3*w]= data[0][i+j*dst_linesize[0]];
	}
    }
    if(fail == true) {
      std::cout << "Failed" << std::endl;
      av_frame_free(&frame);
      av_frame_free(&sw_frame);
      if (ret < 0)
	return ret;
    }
    else {
      av_frame_free(&frame);
      av_frame_free(&sw_frame);
      return 9999;
    }
  }
  return 0;
}

int qvdec::run_decode1() {
  int ret = 0;
  while (ret >= 0) {
    if ((ret = av_read_frame(input_ctx, &packet)) < 0)
      break;
    if (video_stream == packet.stream_index)
      ret = decode_write();
    av_packet_unref(&packet);
  }
  packet.data = NULL;
  packet.size = 0;
  ret = decode_write();
  av_packet_unref(&packet);
  return 0;
}

int qvdec::get_frame(uint8_t ** buffer, float & start_time) {
  data = buffer;
  //std::cout << "Seeking to " << start_time << " on " << file << std::endl;
  int ret = avformat_seek_file(input_ctx,video_stream,tConv*(start_time-0.2),tConv*start_time,tConv*(start_time+0.2),0);
  if(ret < 0) std::cout << "Seek error: " << ret <<" "<< std::endl;
  while (ret >= 0) {
    if ((ret = av_read_frame(input_ctx, &packet)) < 0) {
      std::cout << "Error in read_frame: " << ret << std::endl;
      break;
    }
    if (video_stream == packet.stream_index) {
      ret = decode_write();
      if(ret == 9999) break;
      else if(ret==-11) ret=0;
    }
    av_packet_unref(&packet);
  }
  return 0;
}

int qvdec::flush_decoder() {
  packet.data = NULL;
  packet.size = 0;
  decode_write();
  av_packet_unref(&packet);
  return 0;
}

double qvdec::get_trace_data(float & start_time,  std::vector<int> & crop, std::vector<float> & times, std::vector<std::array<float,12>> & traceDat) {
  int cropW = w;
  int cropH = h;
  prevTime = 0;
  time=0;
  uint8_t * buffer[4];
  buffer[0] = NULL;
  data = buffer;
  cropW -= (crop[0]+crop[1]);
  cropH -= (crop[2]+crop[3]);
  float norm = 4.0/(cropW*cropH);
  int ret = avformat_seek_file(input_ctx,video_stream,tConv*(start_time-0.2),tConv*start_time,tConv*(start_time+0.2),0);
  //int ret = avformat_seek_file(input_ctx,video_stream,0.5*tConv*start_time,tConv*start_time,tConv*start_time,0); //seek before
  if(ret < 0) std::cout <<  " avformat_seek_file error return " <<ret<< std::endl;
  while (true) {
    if ((ret = av_read_frame(input_ctx, &packet)) < 0) {
      break;
    }
    if (video_stream == packet.stream_index) {
      ret = decode_write();
      av_packet_unref(&packet);
    }
    else continue;
    if(ret != 9999) continue;
    if(prevTime >= time) continue;
    prevTime=time;
    times.push_back(time);
    ret = 0;
    float r1(0),r2(0),r3(0),r4(0),g1(0),g2(0),g3(0),g4(0),b1(0),b2(0),b3(0),b4(0);
    int y = 0;
    auto dataIter = &(data[0][0]);
    for(; y < cropH/2; y++) {
      dataIter = &(data[0][0])+3*((crop[2]+y)*w+crop[0]);
      int x = 0;
      for(; x < 3*cropW/2; x+=3) {
	r1+=*dataIter;
	dataIter++;
	g1+=*dataIter;
	dataIter++;
	b1+=*dataIter;
	dataIter++;
      }
      for(; x < 3*cropW; x+=3) {
	r2+=*dataIter;
	dataIter++;
	g2+=*dataIter;
	dataIter++;
	b2+=*dataIter;
	dataIter++;
      }
    }
    for(; y < cropH; y++) {
      dataIter = &(data[0][0])+3*((crop[2]+y)*w+crop[0]);
      int x = 0;
      for(; x < 3*cropW/2; x+=3) {
	r3+=*dataIter;
	dataIter++;
	g3+=*dataIter;
	dataIter++;
	b3+=*dataIter;
	dataIter++;
      }
      for(; x < 3*cropW; x+=3) {
	r4+=*dataIter;
	dataIter++;
	g4+=*dataIter;
	dataIter++;
	b4+=*dataIter;
	dataIter++;
      }
    }
    std::array<float,12> trace_point;
    trace_point[0]=norm*r1;
    trace_point[1]=norm*g1;
    trace_point[2]=norm*b1;
    trace_point[3]=norm*r2;
    trace_point[4]=norm*g2;
    trace_point[5]=norm*b2;
    trace_point[6]=norm*r3;
    trace_point[7]=norm*g3;
    trace_point[8]=norm*b3;
    trace_point[9]=norm*r4;
    trace_point[10]=norm*g4;
    trace_point[11]=norm*b4;
    traceDat.push_back(trace_point);
  }
  av_freep(&data[0]);
  return tConv;
}

qvdec::~qvdec() {
  sws_freeContext(sws_ctx);
  avcodec_free_context(&decoder_ctx);
  avformat_close_input(&input_ctx);
  av_buffer_unref(&hw_device_ctx);
}
