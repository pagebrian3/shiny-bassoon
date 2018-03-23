#include "VBrowser.h"
#include "VideoIcon.h"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

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
    ("trace_fps", po::value<float>()->default_value(30.0), 
          "trace fps")
    ("border_frames", po::value<float>()->default_value(20.0), 
           "frames used to calculate border")
    ("cut_thresh", po::value<float>()->default_value(1000.0), 
          "threshold used for border detection")
    ("comp_time", po::value<float>()->default_value(10.0), 
           "length of slices to compare")
    ("slice_spacing", po::value<float>()->default_value(60.0), 
          "separation of slices in time")
    ("thresh", po::value<float>()->default_value(200.0), 
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
  this->set_default_size(vm["win_width"].as<int>(),vm["win_height"].as<int>());
  dbCon = new DbConnector(vm["app_path"].as<std::string>());
  path = vm["default_path"].as<std::string>();
  vu = new video_utils(dbCon,&vm);
  sort_by="size"; //size, name, length
  sort_desc=true;  //true, false
  box_outer = new Gtk::VBox(false, 6);
  this->add(*box_outer);
  browse_button = new Gtk::Button("...");
  browse_button->signal_clicked().connect(sigc::mem_fun(*this,&VBrowser::browse_clicked));
  fdupe_button = new Gtk::Button("Find Dupes");
  fdupe_button->signal_clicked().connect(sigc::mem_fun(*this,&VBrowser::fdupe_clicked));
  sort_opt = new Gtk::Box();
  Gtk::Label sort_label("Sort by:");
  sort_combo = new Gtk::ComboBoxText();
  sort_combo->append("size");
  sort_combo->append("length");
  sort_combo->append("name");
  sort_combo->set_active(0);
  sort_combo->signal_changed().connect(sigc::mem_fun(*this,&VBrowser::on_sort_changed));
  progress_bar = new Gtk::ProgressBar();
  progress_bar->set_show_text();
  Glib::ustring stock_icon = "view-sort-ascending";
  if (sort_desc) stock_icon="view-sort-descending";
  asc_button = new Gtk::Button();
  asc_button->set_image_from_icon_name(stock_icon,Gtk::ICON_SIZE_BUTTON);
  asc_button->signal_clicked().connect(sigc::mem_fun(*this,&VBrowser::asc_clicked));
  sort_opt->add(sort_label);
  sort_opt->add(*sort_combo);
  sort_opt->add(*asc_button);
  sort_opt->pack_end(*fdupe_button,  false,  false,  0);
  sort_opt->pack_end(*browse_button,  false, false, 0 );
  sort_opt->pack_end(*progress_bar,true,true,1);
  box_outer->pack_start(*sort_opt, false, true, 0);
  fScrollWin = new Gtk::ScrolledWindow();
  box_outer->pack_start(*fScrollWin, true, true, 0);
  this->populate_icons();
}

VBrowser::~VBrowser() {
  delete TPool;
  delete dbCon;
  delete fScrollWin;
  delete asc_button;
  delete sort_combo;
  delete browse_button;
  delete box_outer;
  delete sort_opt;
  delete fdupe_button;
  delete fFBox;
  }
  
void VBrowser::populate_icons(bool clean) {
  if(clean) fScrollWin->remove();
  int min_vid = dbCon->get_last_vid();
  fFBox = new Gtk::FlowBox();
  fFBox->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
  fFBox->set_sort_func(sigc::mem_fun(*this,&VBrowser::sort_videos));
  fFBox->set_homogeneous(false);
  std::vector<bfs::directory_entry> video_list;
  std::vector<std::future<VideoIcon * > > icons;
  for (bfs::directory_entry & x : bfs::directory_iterator(path)) {
      auto extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(get_extensions().count(extension)) {
	std::string pathName(x.path().native());
	if(!dbCon->video_exists(pathName)) min_vid++;
	icons.push_back(TPool->push([this](std::string path,DbConnector * con, int vid) {return new VideoIcon(path,con,vid,&vm);},pathName,dbCon,min_vid));
      }
  }
  cxxpool::wait(icons.begin(),icons.end());
  iconVec =  new std::vector<VideoIcon *> (icons.size());
  int j = 0;
  for(auto &a: icons) {
    VideoIcon * b = a.get();
    if(!b->hasIcon) {
      (*iconVec)[j]=b;
      icon_list.push_back(b->get_vid_file()->vid);
      j++;
    }
    fFBox->add(*b);
  }
  fFBox->invalidate_sort();
  fScrollWin->add(*fFBox);
  this->show_all();
  if(j > 0) for(auto &a: (*iconVec))
      resVec.push_back(TPool->push([this](VideoIcon *b) {return vu->create_thumb(b);}, a));
  i_timer_slot = sigc::mem_fun(*this,&VBrowser::icon_timeout);
  i_timer = Glib::signal_timeout().connect(i_timer_slot,100);
}

bool VBrowser::icon_timeout() {
  int saved_icons = 0;
  std::chrono::milliseconds timer(1);
  res.clear();
  res = cxxpool::wait_for(resVec.begin(), resVec.end(),timer,res);
  int i=0;
  int counter = 0;
  for(auto &b: res) {
    if(b == std::future_status::ready && icon_list[i] > 0){
      int vid = icon_list[i];
      std::string icon_file = dbCon->create_icon_path(vid);
      dbCon->save_icon(vid);
      (*iconVec)[i]->set(icon_file);
      (*iconVec)[i]->show();
      std::system((boost::format("rm %s") %icon_file).str().c_str());
      icon_list[i]=0;
    }
    else if(b == std::future_status::ready && icon_list[i]==0) counter++;
    i++;	
  }
  if(counter == res.size()) return false;
  else return true;
}

bool VBrowser::progress_timeout() {
  double counter;
  double percent;
  std::chrono::milliseconds timer(1);
  res.clear();
  res = cxxpool::wait_for(resVec.begin(), resVec.end(),timer,res);
  double total = resVec.size();
  counter = 0.0;
  for(auto &a: res) if(a == std::future_status::ready) counter+=1.0;
  percent = 100.0*counter/total;
  update_progress(counter/total,(boost::format("Comparing Videos: %d%% Complete") %  percent).str());
  if(res.size()-counter < 0.1)  {
    update_progress(1.0,"Done Dupe Hunting");
    return false;
  }
  else return true;
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
}

void VBrowser::on_delete() {
  dbCon->save_db_file();
  return;
}

void VBrowser::fdupe_clicked(){
   std::vector<VidFile *> videos;
  std::vector<int> vids;
  float total = 0.0;
  float counter = 0.0;
  float percent = 0.0;
  std::chrono::milliseconds timer(500);
  for (bfs::directory_entry & x : bfs::directory_iterator(path)) {
    auto extension = x.path().extension().generic_string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if(get_extensions().count(extension)) {
      bfs::path full_path = bfs::absolute(x.path());
      VidFile * vid_obj = dbCon->fetch_video(full_path.c_str());
      int video_id = vid_obj->vid;
      vids.push_back(video_id);
      std::cout << x.path() <<" "<< video_id<<std::endl;
      if(!dbCon->trace_exists(video_id)) videos.push_back(vid_obj);
    }
  }
  resVec.clear();
  for(auto & b: videos) resVec.push_back(TPool->push([&](VidFile * b ){ return vu->calculate_trace(b);},b));
  total = resVec.size();
  while(counter < total) {
    counter = 0;    
    res = cxxpool::wait_for(resVec.begin(), resVec.end(),timer,res);
    for(auto &a: res) if(a == std::future_status::ready) counter+=1.0;
    percent = 100.0*counter/total;
    update_progress(counter/total,(boost::format("Creating Traces %i/%i: %d%% Complete") % counter % total %  percent).str());
    res.clear();
  }
  cxxpool::wait(resVec.begin(),resVec.end());
  update_progress(1.0,"Traces Complete");
  resVec.clear();
  std::map<std::pair<int,int>,int> result_map;
  std::map<int,std::vector<unsigned short> > data_holder;
  dbCon->fetch_results(result_map);
  total=0.0;
  //loop over files 
  for(int i = 0; i +1 < vids.size(); i++) {
    dbCon->fetch_trace(vids[i],data_holder[vids[i]]);
    //loop over files after i
    for(int j = i+1; j < vids.size(); j++) {
      if (result_map[std::make_pair(vids[i],vids[j])]) {
	continue;
      }
      else {
	dbCon->fetch_trace(vids[j],data_holder[vids[j]]);
	resVec.push_back(TPool->push([&](int i, int j, std::map<int, std::vector<unsigned short>> & data){ return vu->compare_vids(i,j,data);},vids[i],vids[j],data_holder));
      }
    }
  }
  p_timer_slot = sigc::mem_fun(*this,&VBrowser::progress_timeout);
  p_timer = Glib::signal_timeout().connect(p_timer_slot,100);
  return;
}

void VBrowser::on_sort_changed() {
  sort_by = sort_combo->get_active_text();
  fFBox->invalidate_sort();
  return;
}

void VBrowser::asc_clicked() {
  sort_desc=! sort_desc;
  Glib::ustring iname;
  if (sort_desc) iname = "view-sort-descending";
  else iname = "view-sort-ascending";
  asc_button->set_image_from_icon_name(iname,Gtk::ICON_SIZE_BUTTON);
  fFBox->invalidate_sort();
  return;
}

po::variables_map * VBrowser::get_vm() {
  return &vm;
}

DbConnector * VBrowser::get_dbcon() {
  return dbCon;
}

std::set<std::string> VBrowser::get_extensions() {
  std::set<std::string> extensions{".3gp",".avi",".flv",".m4v",".mkv",".mov",".mp4",".mpeg",".mpg",".mpv",".qt",".rm",".webm",".wmv"};
  return extensions;
}

void VBrowser::update_progress(double fraction, std::string label) {
  progress_bar->set_text(label);
  progress_bar->set_fraction(fraction);
  return;
}


  
