#ifndef QVDBCONFIG_H
#define QVDBCONFIG_H

#include <vector>
#include <map>
#include <tuple>
#include <string>

class qvdb_config
{
   public:

  qvdb_config();
  
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
  
  void load_config(std::vector<std::tuple<std::string, int, float, std::string>> input) {
    for(auto &a:input) {
      config_data[std::get<0>(a)]=std::make_tuple(std::get<1>(a),std::get<2>(a),std::get<3>(a));
    }
  };
  
 private:
  std::map<std::string,std::tuple<int, float, std::string > > config_data;
}

#endif // QVDBCONFIG_H
