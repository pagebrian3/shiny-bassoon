#ifndef QVDBCONFIG_H
#define QVDBCONFIG_H

#include <vector>
#include <map>
#include <tuple>
#include <string>
#include <boost/tokenizer.hpp>

class qvdb_config
{

public:

  qvdb_config(){};

  ~qvdb_config(){};
  
  void get(std::string config, int & var) {
    var = std::get<0>(config_data[config]);
  };
  
  void get(std::string config, float & var){
    var = std::get<1>(config_data[config]);
  };

  void get(std::string config,  std::string & var){
    var.assign(std::get<2>(config_data[config]));
  };

  void sett(std::string config, int & var){
  config_data[config] = std::make_tuple(var,0.0,"");
};

  void set(std::string config, float & var){
  config_data[config] = std::make_tuple(0,var,"");
};

  void set(std::string config,  std::string & var){
  config_data[config] = std::make_tuple(0,0.0,var);
};
  
 bool  load_config(std::vector<std::tuple<std::string, int, float, std::string>> input) {
    if(input.size() == 0) {
      std::vector<std::string> defaults={"win_height,i,600","win_width,i,800","thumb_height,i,135","thumb_width,i,240","progress_time,i,100","thumb_time,f,25.0","fudge,i,15","image_thresh,i,10000","threads,i,7","trace_time,f,10.0","trace_fps,f,30.0","border_frames,f,6.0","cut_thresh,f,3.0","comp_time,f,10.0","slice_spacing,f,60.0","thresh,f,50.0","cache_size,i,10","extensions,s,.3gp .avi .flv .m4v .mkv .mov .mp4 .mpeg .mpg .mpv .qt .rm .webm .wmv","bad_chars,s,&"};
      boost::char_separator<char> sep(",");
      for( auto &a: defaults) {
	boost::tokenizer<boost::char_separator<char> > tok(a,sep);
	auto beg=tok.begin();
	 std::string label(*beg);
	 beg++;
	 char dType((*beg)[0]);
	 beg++;
	 std::string val(*beg);
	if(dType == 'i') config_data[label]=std::make_tuple(std::stoi(val),0,"");
	else if(dType == 'f') config_data[label]=std::make_tuple(0,std::stof(val),"");
	else if(dType == 's') config_data[label]=std::make_tuple(0,0,val);
	else continue;             
      }
      return true;
    }
    else for(auto &a:input)   config_data[std::get<0>(a)]=std::make_tuple(std::get<1>(a),std::get<2>(a),std::get<3>(a));
    return false;
  };

  std::map<std::string,std::tuple<int, float, std::string > > get_data() {
    return config_data;
  };
  
 private:

  std::map<std::string,std::tuple<int, float, std::string > > config_data;

};

#endif // QVDBCONFIG_H
