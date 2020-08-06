#ifndef QVDEC_H
#define QVDEC_H
extern "C" {
  #include <libswscale/swscale.h>
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}
#include <filesystem>
#include <string>
#include <vector>

class qvdec {
 public:

  qvdec(std::filesystem::path & path, std::string hw_type);

  ~qvdec();
  
  int run_decode1();

  double get_trace_data(float & start_time, std::vector<int> & crop, std::vector<int> & times, std::vector<std::vector<uint8_t>> &traceDat);

  int get_frame(uint8_t ** data, float & start_time);

  int decode_write();

 private:

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

  int time;
};

#endif // QVDEC_H
