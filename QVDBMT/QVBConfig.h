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
  
  void get(std::string config, int & var) {
    var = boost::get<int>(config_data[config]);
  };
  
  void get(std::string config, float & var){
    var = boost::get<float>(config_data[config]);
  };

  void get(std::string config,  std::string & var){
    var.assign(boost::get<std::string>(config_data[config]));
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
      std::vector<std::string> defaults={"win_height,i,600","win_width,i,800","thumb_height,i,135","thumb_width,i,240","progress_time,i,100","thumb_time,f,25.0","fudge,f,4.0","image_thresh,i,10000","threads,i,7","trace_time,f,10.0","trace_fps,f,30.0","border_frames,i,6","cut_thresh,f,3.0","comp_time,f,10.0","slice_spacing,f,60.0","thresh,f,0.98","cache_size,i,10000","extensions,s,.3gp .avi .flv .m4v .mkv .mov .mp4 .mpeg .mpg .mpv .qt .rm .webm .wmv","bad_chars,s,&","preview_height,i,480"};
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
