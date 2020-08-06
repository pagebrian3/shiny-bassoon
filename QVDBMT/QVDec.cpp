extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
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

qvdec::qvdec(std::filesystem::path & path, std::string hw_type) : file(path) {
  input_ctx = NULL;
  int ret;
  AVStream *video = NULL;
  decoder_ctx = NULL;
  AVCodec *decoder = NULL;
  enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
  int i;
  hwEnable = true;
  hw_device_ctx = NULL;
  data = NULL;
  if(strcmp(hw_type.c_str(),"CPU") !=0 ) {
    type = av_hwdevice_find_type_by_name(hw_type.c_str());
  }
  else hwEnable = false;

  /* open the input file */
  ret = avformat_open_input(&input_ctx, path.c_str(), NULL, NULL);
  if(ret < 0) std::cout <<"input failure on "<<path <<std::endl;
  avformat_find_stream_info(input_ctx, NULL);
  /* find the video stream information */
  ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
  video_stream = ret;
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
  if(ret < 0 || (ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0) std::cout <<"Failure on : "<<path<<std::endl;
  w= decoder_ctx->width;
  h = decoder_ctx->height;
  sws_ctx = NULL;
  std::cout << "Working on: " << path << std::endl;
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
    if (!fail && (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)) {
      av_frame_free(&frame);
      av_frame_free(&sw_frame);
      return ret;
    } else if (ret < 0) {
      std::cout << "Error while decoding"<<std::endl;
      fail = true;
    }
    if (!fail && (hwEnable && frame->format == hw_pix_fmt)) {
      /* retrieve data from GPU to CPU */
      if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
	std::cout << "Error transferring the data to system memory"<<std::endl;
	fail=true;
      }
      tmp_frame = sw_frame;
    } else tmp_frame = frame;
    int dst_linesize[4];
    time = tmp_frame->pts;
    enum AVPixelFormat src_pix_fmt = (AVPixelFormat) tmp_frame->format;
    enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_RGB24;
    int alignment = 32;
    if(sws_ctx == NULL)
    sws_ctx = sws_getContext(w, h, src_pix_fmt,
			     w, h, dst_pix_fmt,
			     SWS_POINT, NULL, NULL, NULL);
    if (!sws_ctx) {
      ret = AVERROR(EINVAL);
      return 0;
    }
    if(data[0] != NULL) av_freep(&(data[0]));
    if (!fail &&((ret = av_image_alloc(data, dst_linesize,
				       w, h, dst_pix_fmt, alignment)) < 0)) {
      std::cout << "Could not allocate destination image\n" <<std::endl;
      return 0;
    }
    /* convert to destination format */
    if(!fail && ((ret = sws_scale(sws_ctx, (const uint8_t * const*)tmp_frame->data,
				  tmp_frame->linesize, 0, h, data, dst_linesize)) < 0)) {
      std::cout <<"Failed to scale."<<std::endl;
      fail=true;
    }
    if(dst_linesize[0] > 3*w) {
      for(int j = 1; j < h; j++) 
	for(int i = 0; i < 3*w; i++) data[0][i+j*3*w]= data[0][i+j*dst_linesize[0]];
    }
    if(fail == true) {
      av_frame_free(&frame);
      av_frame_free(&sw_frame);
      if (ret < 0)
	return ret;
    }
    av_frame_free(&frame);
    av_frame_free(&sw_frame);
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
  int ret = avformat_seek_file(input_ctx,video_stream,tConv*(start_time-0.2),tConv*start_time,tConv*(start_time+0.2),0);
  int loop_num = 0;
  while (ret >= 0) {
    if ((ret = av_read_frame(input_ctx, &packet)) < 0)
      break;
    if (video_stream == packet.stream_index) {
      ret = decode_write();
      break;
    }
    loop_num++;
  }
  packet.data = NULL;
  packet.size = 0;
  ret = decode_write();
  av_packet_unref(&packet);
  return 0;
}

double qvdec::get_trace_data(float & start_time,  std::vector<int> & crop, std::vector<int> & times, std::vector<std::vector<uint8_t> > & traceDat) {
  int cropW = w;
  int cropH = h;
  data = (uint8_t **) malloc(4);
  cropW -= (crop[0]+crop[1]);
  cropH -= (crop[2]+crop[3]);
  float norm = 4.0/(cropW*cropH);
  int ret = avformat_seek_file(input_ctx,video_stream,0.5*tConv*start_time,tConv*start_time,tConv*start_time,0); //seek before
  if(ret < 0) std::cout <<  " avformat_seek_file error return " <<ret<< std::endl;
  int loop_num=0;
  while (ret >= 0) {
    std::cout << file <<" Loop " << loop_num << std::endl;
    loop_num++;
    if ((ret = av_read_frame(input_ctx, &packet)) < 0){
      std::cout << "Break" << std::endl;
      break;
    }
    if (video_stream == packet.stream_index)
      ret = decode_write();
    else continue;
    if(ret < 0) {
      std::cout << "Neg Ret." << std::endl;
      continue;
    }
    else times.push_back(time);
    std::cout <<"Here "<< ret << std::endl;
    int r1(0),r2(0),r3(0),r4(0),g1(0),g2(0),g3(0),g4(0),b1(0),b2(0),b3(0),b4(0);
    int y = 0;
    auto dataIter = data[0];
    for(; y < cropH/2; y++) {
      r1=r2=r3=r4=g1=g2=g3=g4=b1=b2=b3=b4=0;
      dataIter =data[0]+3*((crop[3]+y)*w+crop[0]);
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
      dataIter = &(data[0][0])+3*((crop[3]+y)*w+crop[0]);
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
    traceDat[0].push_back(norm*r1);
    traceDat[1].push_back(norm*g1);
    traceDat[2].push_back(norm*b1);
    traceDat[3].push_back(norm*r2);
    traceDat[4].push_back(norm*g2);
    traceDat[5].push_back(norm*b2);
    traceDat[6].push_back(norm*r3);
    traceDat[7].push_back(norm*g3);
    traceDat[8].push_back(norm*b3);
    traceDat[9].push_back(norm*r4);
    traceDat[10].push_back(norm*g4);
    traceDat[11].push_back(norm*b4);
  }
  //free(data);
  return tConv;
}

qvdec::~qvdec() {
  sws_freeContext(sws_ctx);
  avcodec_free_context(&decoder_ctx);
  avformat_close_input(&input_ctx);
  av_buffer_unref(&hw_device_ctx);
}
