#ifndef QVDBCONFIG_H
#define QVDBCONFIG_H

#include <vector>
#include <map>
#include <string>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>

class qvdb_config
{

public:

  qvdb_config(){};

  ~qvdb_config(){};
  
  int get_int(std::string config) {
    return boost::get<int>(config_data[config]);
  };
  
  float get_float(std::string config) {
    return boost::get<float>(config_data[config]);
  };

  std::string get_string(std::string config) {
    return boost::get<std::string>(config_data[config]);
  };

  void set(std::string config, int  var){
    config_data[config] = var;
  };

  void set(std::string config, float var){
    config_data[config] = var;
  };

  void set(std::string config, std::string var){
    config_data[config] = var;
  };
  
  bool load_config(std::vector<std::pair<std::string,boost::variant<int, float, std::string>>>input) {
    if(input.size() == 0) {
      std::vector<std::string> defaults={"win_height,i,600","win_width,i,800","thumb_size,s,256x144","progress_time,i,200","thumb_time,f,25.0","image_thresh,i,4","threads,i,1","trace_fps,f,30.0","border_frames,i,6","cut_thresh,f,1.0","comp_time,f,60.0","slice_spacing,f,60.0","power,f,1.0","thresh,f,40.0","extensions,s,.mp4 .wmv .avi .flv .m4v .mkv .mov .mpeg .mpg .mpv .qt .rm .webm .3gp","preview_height,i,432","preview_width,i,768","sort_by,s,size","sort_ascending,i,0","hwdecoder,s,vaapi","frame_interval,f,5.0","confidence_thresh,f,0.95","face_batch,i,100","face_size,i,200"};
      boost::char_separator<char> sep(",");
      for( auto &a: defaults) {
	boost::tokenizer<boost::char_separator<char>> tok(a,sep);
	auto beg=tok.begin();
	std::string label(*beg);
	beg++;
	char dType((*beg)[0]);
	beg++;
	std::string val(*beg);
	if(dType == 'i') config_data[label]=std::stoi(val);
	else if(dType == 'f') config_data[label]=std::stof(val);
	else if(dType == 's') config_data[label]=val;
	else continue;             
      }
      return true;
    }
    else for(auto &a:input)   config_data[a.first]=a.second;
    return false;
  };

  std::map<std::string,boost::variant<int, float, std::string > > get_data() {
    return config_data;
  };
  
private:

  std::map<std::string,boost::variant<int, float, std::string > > config_data;

};

#endif // QVDBCONFIG_H
