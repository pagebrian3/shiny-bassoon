#ifndef QVDBCONFIG_H
#define QVDBCONFIG_H

class qvdb_config
{
   public:

  qvdb_config();
  
  void get(std::string &config, int & var) {
    var = config_data[config].get<0>;
  };
  
  void get(std::string &config, float & var){
    var = config_data[config].get<1>;
  };
  void get(std::string &config,  std::string & var){
    var. assign(config_data[config].get<2>);
  };
  void sett(std::string &config, int & var){
  config_data[config] = std::make_tuple(var,0.0,"");
};
  void set(std::string &config, float & var){
  config_data[config] = std::make_tuple(0,var,"");
};
  void set(std::string &config,  std::string & var){
  config_data[config] = std::make_tuple(0,0.0,var);
};
  
  void load_config(std::vector<std::tuple<std:string, int, float, std::string>> input) {
    for(auto &a:input) {
      config_data[a.get<0>]=std::make_tuple(a.get<1>,a.get<2>,a.get<3>);
    }
  };
  
 private:
  std::map<std::string,std::tuple<int, float, std::string > > config_data;
}

#endif // QVDBCONFIG_H
