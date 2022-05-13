#ifndef QVDEC_H
#define QVDEC_H
extern "C" {
  #include <libswscale/swscale.h>
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}
#include "VidFile.h"
#include <string>
#include <vector>
#include <array>

class qvdec {
 public:

  qvdec(VidFile * vf, std::string hw_type);

  ~qvdec();

  bool soft_init(const AVCodec * decoder, AVStream * video);
  
  int run_decode1();

  double get_trace_data(float & start_time, std::vector<int> & crop, std::vector<float> & times, std::vector<std::array<float, 12> > & traceDat);

  int get_frame(uint8_t ** data, float & start_time);

  int decode_write();

  int flush_decoder();

  bool get_error(){return initError;};

 private:

  bool initError;

  uint8_t ** data;

  std::filesystem::path file;

  AVFormatContext *input_ctx;

  AVCodecContext *decoder_ctx;

  AVBufferRef *hw_device_ctx;

  AVPacket packet;

  struct SwsContext * sws_ctx;

  int video_stream;

  double tConv;

  bool hwEnable;

  int w;

  int h;

  double time;
  
  double prevTime;
};

#endif // QVDEC_H
