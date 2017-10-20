#include "VBrowser.h"
#include <boost/process.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <set>

std::set<std::string> extensions{".3gp",".avi",".flv",".m4v",".mkv",".mov",".mp4",".mpeg",".mpg",".mpv",".qt",".rm",".webm",".wmv"};

VBrowser::VBrowser(int argc, char * argv[]) {
  bfs::path temp_path = getenv("HOME");
  temp_path+="/.video_proj/";
  po::options_description config("Configuration");
  config.add_options()
    ("win_width", po::value<int>()->default_value(800), 
          "window width")
    ("win_height", po::value<int>()->default_value(600), 
          "window height")
    ("thumb_width", po::value<int>()->default_value(320), 
          "thumbnail max width")
    ("thumb_height", po::value<int>()->default_value(180), 
          "thumbnail max height")
    ("fudge", po::value<int>()->default_value(10), 
          "difference threshold")
    ("threads", po::value<int>()->default_value(2), 
           "Number of threads to use.")
    ("trace_time", po::value<float>()->default_value(10.0), 
          "trace starting time")
    ("thumb_time", po::value<float>()->default_value(12.0), 
          "thumbnail time")
    ("trace_fps", po::value<float>(&cTraceFPS)->default_value(30.0), 
          "trace fps")
    ("border_frames", po::value<float>()->default_value(20.0), 
           "frames used to calculate border")
    ("cut_thresh", po::value<float>()->default_value(1000.0), 
          "threshold used for border detection")
    ("comp_time", po::value<float>(&cCompTime)->default_value(10.0), 
           "length of slices to compare")
    ("slice_spacing", po::value<float>(&cSliceSpacing)->default_value(60.0), 
          "separation of slices in time")
    ("thresh", po::value<float>(&cThresh)->default_value(200.0), 
          "threshold for video similarity")
    ("app_path", 
     po::value< std::string >()->default_value(temp_path.string().c_str()), 
           "database and temp data path")
    ("default_path", 
     po::value< std::string >()->default_value("/home/ungermax/mt_test/"), 
           "starting path");
  std::ifstream config_file("config.cfg");
  po::store(po::parse_command_line(argc, argv, config),vm);
  po::store(po::parse_config_file(config_file, config),vm);
  po::notify(vm);
  TPool = new cxxpool::thread_pool(vm["threads"].as<int>());
  this->set_default_size(vm["win_width"].as<int>(), vm["win_height"].as<int>());
  dbCon = new DbConnector(vm);
  Gtk::VBox * box_outer = new Gtk::VBox(false, 6);
  this->add(*box_outer);
  path = vm["default_path"].as<std::string>();
  sort_by="size"; //size, name, length
  sort_desc=true;  //true, false
  browse_button = new Gtk::Button("...");
  browse_button->signal_clicked().connect(sigc::mem_fun(*this,&VBrowser::browse_clicked));
  fdupe_button = new Gtk::Button("Find Dupes");
  fdupe_button->signal_clicked().connect(sigc::mem_fun(*this,&VBrowser::fdupe_clicked));
  Gtk::Box * sort_opt = new Gtk::Box();
  Gtk::Label * sort_label = new Gtk::Label("Sort by:");
  sort_combo = new Gtk::ComboBoxText();
  sort_combo->append("size");
  sort_combo->append("length");
  sort_combo->append("name");
  sort_combo->set_active(0);
  sort_combo->signal_changed().connect(sigc::mem_fun(*this,&VBrowser::on_sort_changed));
  Glib::ustring stock_icon = "view-sort-ascending";
  if (sort_desc) stock_icon="view-sort-descending";
  asc_button = new Gtk::Button();
  asc_button->set_image_from_icon_name(stock_icon,Gtk::ICON_SIZE_BUTTON);
  asc_button->signal_clicked().connect(sigc::mem_fun(*this,&VBrowser::asc_clicked));
  sort_opt->add(*sort_label);
  sort_opt->add(*sort_combo);
  sort_opt->add(*asc_button);
  sort_opt->pack_end(*fdupe_button,  false,  false,  0);
  sort_opt->pack_end(*browse_button,  false, false, 0 ); 
  box_outer->pack_start(*sort_opt, false, true, 0);
  fScrollWin = new Gtk::ScrolledWindow();
  box_outer->pack_start(*fScrollWin, true, true, 0);
  this->populate_icons();
  this->show_all();
}

VBrowser::~VBrowser() {
}

void VBrowser::populate_icons(bool clean) {
  if(clean) fScrollWin->remove();
  fFBox = new Gtk::FlowBox();
  fFBox->set_homogeneous(false);
  fFBox->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
  fFBox->set_sort_func(sigc::mem_fun(*this,&VBrowser::sort_videos));
  std::vector<bfs::directory_entry> video_list;
  std::vector<std::future<VideoIcon * > > icons;
  for (bfs::directory_entry & x : bfs::directory_iterator(path)) {
      auto extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(extensions.count(extension)) {
	std::string pathName(x.path().native());
	//std::cout << "PATH: "<<pathName<<std::endl;
	icons.push_back(TPool->push([this](std::string path,DbConnector * con) {return new VideoIcon(path,con,&vm);},pathName,dbCon));
      }
    }
  for(auto &a: icons) fFBox->add(*(a.get()));
  fFBox->invalidate_sort();
  fScrollWin->add(*fFBox);
}

std::string VBrowser::get_sort() {
  return sort_by;
}

void VBrowser::set_sort(std::string sort) {
  sort_by = sort;
}

int VBrowser::sort_videos(Gtk::FlowBoxChild *videoFile1, Gtk::FlowBoxChild *videoFile2) {
  int value=1;
  VidFile *v1=(reinterpret_cast<VideoIcon*>(videoFile1->get_child()))->get_vid_file();
  VidFile *v2=(reinterpret_cast<VideoIcon*>(videoFile2->get_child()))->get_vid_file();
  if(sort_desc) value *= -1;
  if(!std::strcmp(this->get_sort().c_str(),"size")) return value*(v1->size - v2->size);
  else if(!std::strcmp(this->get_sort().c_str(),"length")) return value*(v1->length - v2->length);
  else return value*boost::algorithm::to_lower_copy(v1->fileName).compare(boost::algorithm::to_lower_copy(v2->fileName));   
}

void VBrowser::browse_clicked() {
  Gtk::FileChooserDialog dialog("Please choose a folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  dialog.set_default_size(800, 400);
  dialog.set_transient_for(*this);
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("Select", Gtk::RESPONSE_OK);
  auto response = dialog.run();
  if (response == Gtk::RESPONSE_OK){
    path  = dialog.get_filename()+"/";
    this->populate_icons(true);
  }
  this->show_all();
}

void VBrowser::on_delete() {
  dbCon->save_db_file();
}

void VBrowser::fdupe_clicked(){
  std::vector<VidFile *> videos;
  std::vector<int> vids;
  for (bfs::directory_entry & x : bfs::directory_iterator(path)) {
    auto extension = x.path().extension().generic_string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if(extensions.count(extension)) {
      bfs::path full_path = bfs::absolute(x.path());
      VidFile * vid_obj = dbCon->fetch_video(full_path.c_str());
      int video_id = vid_obj->vid;
      vids.push_back(video_id);
      if(!dbCon->trace_exists(video_id)) videos.push_back(vid_obj);
    }
  }
  std::vector<std::future< bool > > jobs;
  for(auto & b: videos) {
    jobs.push_back(TPool->push([&](VidFile * b ){ return calculate_trace(b);},b));
  }
  cxxpool::wait(jobs.begin(),jobs.end());
  std::map<std::pair<int,int>,int> result_map;
  std::map<int,std::vector<unsigned short> > data_holder;
  std::vector<std::future< bool > > work;
  dbCon->fetch_results(result_map);
  //loop over files  TODO-make this parallel when we have larger sample
  for(int i = 0; i +1 < vids.size(); i++) {
    dbCon->fetch_trace(vids[i],data_holder[vids[i]]);
    //loop over files after i
    for(int j = i+1; j < vids.size(); j++) {
      //std::cout << vids[i] << " "<<vids[j] << std::endl;
      if (result_map[std::make_pair(vids[i],vids[j])]) {
	//std::cout <<"ALREADY Computed." <<std::endl;
	continue;
      }
      else {
	dbCon->fetch_trace(vids[j],data_holder[vids[j]]);
	work.push_back(TPool->push([&](int i, int j, std::map<int, std::vector<unsigned short>> & data){ return compare_vids(i,j,data);},vids[i],vids[j],data_holder));
      }
    }
  }
  cxxpool::wait(work.begin(),work.end());
  std::cout <<"Done Dupe Hunting!" << std::endl;
}

bool VBrowser::calculate_trace(VidFile * obj) {
  float start_time = vm["trace_time"].as<float>();
  if(obj->length <= start_time) start_time=0.0;
  boost::process::ipstream is;
  if(obj->crop.length() == 0) {
    boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -filter:v \"fps=%.3f,scale=2x2\" -pix_fmt rgb24 -f image2pipe -vcodec rawvideo - ") % start_time % obj->fixed_filename()  % cTraceFPS).str(),boost::process::std_out > is);
  }
  else {
    boost::process::system((boost::format("ffmpeg -y -nostats -loglevel 0 -ss %.3f -i %s -filter:v \"fps=%.3f,%sscale=2x2\" -pix_fmt rgb24 -f image2pipe -vcodec rawvideo - ") % start_time % obj->fixed_filename()  % cTraceFPS % obj->crop).str(),boost::process::std_out > is);
  }
  std::string outString;
  std::getline(is,outString);
  dbCon->save_trace(obj->vid, outString);
  return true; 
}

void VBrowser::on_sort_changed() {
  sort_by = sort_combo->get_active_text();
  fFBox->invalidate_sort();
}

void VBrowser::asc_clicked() {
  sort_desc=! sort_desc;
  Glib::ustring iname;
  if (sort_desc) iname = "view-sort-descending";
  else iname = "view-sort-ascending";
  asc_button->set_image_from_icon_name(iname,Gtk::ICON_SIZE_BUTTON);
  fFBox->invalidate_sort();
}

bool VBrowser::compare_vids(int i, int j, std::map<int, std::vector<unsigned short> > & data) {
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
	  int value = pow((int)(data[i][t_s+t_o+t_d])-(int)(data[j][t_x+t_o+t_d]),2)-pow(vm["fudge"].as<int>(),2);
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
  
