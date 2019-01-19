#include <QApplication>
#include <QVBrowser.h>

int main(int argc, char *argv[])
{
  po::options_description config("Configuration");
  config.add_options()
    ("win_width", po::value<int>()->default_value(800), "window width")
    ("win_height", po::value<int>()->default_value(600), "window height")
    ("thumb_width", po::value<int>()->default_value(320), "thumbnail max width")
    ("thumb_height", po::value<int>()->default_value(180), "thumbnail max height")
    ("fudge", po::value<int>()->default_value(10),"difference threshold")
    ("threads", po::value<int>()->default_value(2),"Number of threads to use.")
    ("trace_time", po::value<float>()->default_value(10.0), "trace starting time")
    ("thumb_time", po::value<float>()->default_value(12.0),"thumbnail time")
    ("trace_fps", po::value<float>()->default_value(30.0), "trace fps")
    ("border_frames", po::value<float>()->default_value(20.0),"frames used to calculate border")
    ("cut_thresh", po::value<float>()->default_value(1000.0),"threshold used for border detection")
    ("comp_time", po::value<float>()->default_value(10.0),"length of slices to compare")
    ("slice_spacing", po::value<float>()->default_value(60.0),"separation of slices in time")
    ("thresh", po::value<float>()->default_value(200.0),"threshold for video similarity")
    ("default_path", po::value< std::string >()->default_value("/home/ungermax/mt_test/"), "starting path")
    ("extensions", po::value< std::string >()->default_value(".mp4"), "extensions to process")
    ("bad_chars", po::value< std::string >()->default_value("&"), "characters to remove from filenames")
    ("progress_time",po::value<int>()->default_value(100), "progressbar update interval")
    ("cache_size",po::value<int>()->default_value(10), "max icon cache size")
    ("image_thresh",po::value<int>()->default_value(4), "image difference threshold");
  std::ifstream config_file("config.cfg");
  po::variables_map vm;
  char * a[1];
  po::store(po::parse_command_line(1, a, config),vm);
  po::store(po::parse_config_file(config_file, config),vm);
  po::notify(vm);
  QApplication app(argc, argv);
  QVBrowser mainWin(vm);
  mainWin.show();
  return app.exec();
}


 

