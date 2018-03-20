#include "VideoUtils.h"

#include <boost/timer/timer.hpp>
#include <boost/process.hpp>
#include <boost/format.hpp>

std::set<std::string> extensions{".3gp",".avi",".flv",".m4v",".mkv",".mov",".mp4",".mpeg",".mpg",".mpv",".qt",".rm",".webm",".wmv"};

video_utils::video_utils(DbConnector * dbConn, po::variables_map * vm, bfs::path path) {
  dbCon = dbConn;
  cPath = path;
  cStartTime = (*vm)["trace_time"].as<float>();
  cThreads = (*vm)["threads"].as<int>();
  cTraceFPS = (*vm)["trace_fps"].as<float>();
  cCompTime = (*vm)["comp_time"].as<float>();
  cSliceSpacing = (*vm)["slice_spacing"].as<float>();
  cThresh = (*vm)["thresh"].as<float>();
  cFudge = (*vm)["fudge"].as<int>();
  cPool = new cxxpool::thread_pool(cThreads);
}

void video_utils::find_dupes() {
  boost::timer::auto_cpu_timer bt;
  std::vector<VidFile *> videos;
  std::vector<int> vids;
  float total = 0.0;
  float counter = 0.0;
  float percent = 0.0;
  std::vector<std::future_status> res;
  std::chrono::milliseconds timer(500);
  for (bfs::directory_entry & x : bfs::directory_iterator(cPath)) {
    auto extension = x.path().extension().generic_string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if(extensions.count(extension)) {
      bfs::path full_path = bfs::absolute(x.path());
      VidFile * vid_obj = dbCon->fetch_video(full_path.c_str());
      int video_id = vid_obj->vid;
      vids.push_back(video_id);
      std::cout << x.path() <<" "<< video_id<<std::endl;
      if(!dbCon->trace_exists(video_id)) videos.push_back(vid_obj);
    }
  }
  std::vector<std::future< bool > > jobs;
  for(auto & b: videos) jobs.push_back(cPool->push([&](VidFile * b ){ return calculate_trace(b);},b));
  total = jobs.size();
  //progress_bar->show();
  //progress_bar->set_show_text();
  while(counter < total) {
    counter = 0;    
    res = cxxpool::wait_for(jobs.begin(), jobs.end(),timer,res);
    for(auto &a: res) if(a == std::future_status::ready) counter+=1.0;
    percent = 100.0*counter/total;
    //progress_bar->set_text((boost::format("Creating Traces %i/%i: %d%% Complete") % counter % total %  percent).str());
    //progress_bar->set_fraction(counter/total);
    res.clear();
  }
  cxxpool::wait(jobs.begin(),jobs.end());
  //progress_bar->set_text("Traces Complete");
  //progress_bar->set_fraction(1);
  std::cout << "Done Making Traces." << std::endl;
  jobs.clear();
  std::map<std::pair<int,int>,int> result_map;
  std::map<int,std::vector<unsigned short> > data_holder;
  dbCon->fetch_results(result_map);
  total=0.0;
  //loop over files 
  for(int i = 0; i +1 < vids.size(); i++) {
    dbCon->fetch_trace(vids[i],data_holder[vids[i]]);
    //loop over files after i
    for(int j = i+1; j < vids.size(); j++) {
      //std::cout << vids[i] << " "<<vids[j] << std::endl;
      if (result_map[std::make_pair(vids[i],vids[j])]) {
	std::cout <<"ALREADY Computed." <<std::endl;
	continue;
      }
      else {
	dbCon->fetch_trace(vids[j],data_holder[vids[j]]);
	jobs.push_back(cPool->push([&](int i, int j, std::map<int, std::vector<unsigned short>> & data){ return compare_vids(i,j,data);},vids[i],vids[j],data_holder));
      }
    }
  }
  total = jobs.size();
  //progress_bar->set_text("Comparing Videos: 0% Complete");
  while(counter < total) {
    counter = 0.0;
    res = cxxpool::wait_for(jobs.begin(), jobs.end(),timer,res);
    for(auto &a: res) if(a == std::future_status::ready) counter+=1.0;
    //progress_bar->set_fraction(counter/total);
    percent = 100.0*counter/total;
    //std::cout << "Blah: "<< counter << " "<<total <<" "<<jobs.size()<<" "<<res.size()<<std::endl;
    //progress_bar->set_text((boost::format("Comparing Videos: %d%% Complete") %  percent).str());
    res.clear();
  }
  cxxpool::wait(jobs.begin(),jobs.end());
  //progress_bar->set_text("Done Dupe Hunting.");
  //progress_bar->hide();
  std::cout <<"Done Dupe Hunting!" << std::endl;
  return;
}

bool video_utils::compare_vids(int i, int j, std::map<int, std::vector<unsigned short> > & data) {
  bool match=false;
  int counter=0;
  uint t_s,t_x, t_o, t_d;
  //loop over slices
  for(t_s =0; t_s < data[i].size()-12*cTraceFPS*cCompTime; t_s+= 12*cTraceFPS*cSliceSpacing){
    if(match) break;
    //starting offset for 2nd trace-this is the loop for the indiviual tests
    for(t_x=0; t_x < data[j].size()-12*cTraceFPS*cCompTime; t_x+=12){
      if(match) break;
      std::vector<int> accum(12);
      //offset loop
      for(t_o = 0; t_o < 12*cCompTime*cTraceFPS; t_o+=12){
	counter = 0;
	for(auto & a : accum) if (a > cThresh*cCompTime*cTraceFPS) counter+=1;
	if(counter != 0) break;
	//pixel/color loop
	for (t_d = 0; t_d < 12; t_d++) {
	  int value = pow((int)(data[i][t_s+t_o+t_d])-(int)(data[j][t_x+t_o+t_d]),2)-pow(cFudge,2);
	  if(value < 0) value = 0;
	  accum[t_d]+=value;
	}
      }
      counter = 0;
      for(auto & a: accum)  if(a < cThresh*cCompTime*cTraceFPS) counter+=1;
      if(counter == 12) match=true;
      if(match) std::cout << "ACCUM " <<i<<" " <<j <<" " <<t_o <<" slice " <<t_s <<" 2nd offset " <<t_x <<" " <<*max_element(accum.begin(),accum.end())  <<std::endl;
    }
  }
  int result = 1;
  if (match) result = 2;
  dbCon->update_results(i,j,result);
  return true;
}

bool video_utils::calculate_trace(VidFile * obj) {
  //std::cout << obj->vid << " Started" << std::endl;
  float start_time = cStartTime;
  if(obj->length <= start_time) start_time=0.0;
  boost::process::ipstream is;
  boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -filter:v \"fps=%.3f,%sscale=2x2\" -pix_fmt rgb24 -f image2pipe -vcodec rawvideo - ") % start_time % obj->fixed_filename()  % cTraceFPS % obj->crop).str(),boost::process::std_out > is);
  std::string outString;
  std::getline(is,outString);
  dbCon->save_trace(obj->vid, outString);
  return true; 
}

std::set<std::string> video_utils::get_extensions() {
  return extensions;
}


