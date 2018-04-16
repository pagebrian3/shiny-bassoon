#include "VBrowser.h"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

VBrowser::VBrowser(int argc, char * argv[]) {
  bfs::path temp_path = getenv("HOME");
  temp_path+="/.video_proj/";
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
    ("app_path",po::value< std::string >()->default_value(temp_path.string().c_str()), "database and temp data path")
    ("default_path", po::value< std::string >()->default_value("/home/ungermax/mt_test/"), "starting path")
    ("progress_time",po::value<int>()->default_value(100), "progressbar update interval")
    ("cache_size",po::value<int>()->default_value(10), "max icon cache size")
    ("image_thresh",po::value<int>()->default_value(4), "image difference threshold");
  std::ifstream config_file("config.cfg");
  po::store(po::parse_command_line(argc, argv, config),vm);
  po::store(po::parse_config_file(config_file, config),vm);
  po::notify(vm);
  TPool = new cxxpool::thread_pool(vm["threads"].as<int>());
  this->set_default_size(vm["win_width"].as<int>(),vm["win_height"].as<int>());
  dbCon = new DbConnector(vm["app_path"].as<std::string>());
  paths.push_back(bfs::path(vm["default_path"].as<std::string>()));
  vu = new video_utils(dbCon,&vm, temp_path);
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
  p_timer_slot = sigc::mem_fun(*this,&VBrowser::progress_timeout);
  this->populate_icons();  
}

VBrowser::~VBrowser() {
  std::system("reset");
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
  if(clean) {
    vid_list.clear();
    fScrollWin->remove();
    delete fFBox;
    resVec.clear();
    delete iconVec;
    video_files.clear();
  }
  int min_vid = dbCon->get_last_vid();
  fFBox = new Gtk::FlowBox();
  fFBox->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
  fFBox->set_sort_func(sigc::mem_fun(*this,&VBrowser::sort_videos));
  fFBox->set_homogeneous(false);
  std::vector<std::future<VidFile * > > vFiles;
  std::vector<VidFile *> vidFiles;
  VidFile * vidTemp;
  for(auto & path: paths) {
    for (bfs::directory_entry & x : bfs::directory_iterator(path)) {
      auto extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(get_extensions().count(extension)) {
	video_files.push_back(x.path());
	bfs::path pathName = x.path();
	if(!dbCon->video_exists(pathName))  {
	  min_vid++;
	  vFiles.push_back(TPool->push([this](bfs::path path, int vid) {return new VidFile(path,vid);},pathName,min_vid));
	}
	else {
	  vidTemp = dbCon->fetch_video(pathName);
	  vidFiles.push_back(vidTemp);
	}
      }
    }
  }
  cxxpool::wait(vFiles.begin(),vFiles.end());
  for(auto &a: vFiles) {
    vidTemp = a.get();
    dbCon->save_video(vidTemp);
    vidFiles.push_back(vidTemp);
  }   
  iconVec =  new std::vector<VideoIcon *> (vidFiles.size());
  int j = 0;
  for(auto &a: vidFiles) {
    VideoIcon * b = new VideoIcon(a);
    (*iconVec)[j]=b;
    vid_list.push_back(a->vid);
    j++;
    fFBox->add(*b);
  }
  fFBox->invalidate_sort();
  fScrollWin->add(*fFBox);
  this->show_all();
  p_timer = Glib::signal_timeout().connect(p_timer_slot,vm["progress_time"].as<int>());
  progressFlag = 1;
  if(j > 0) for(auto &a: (*iconVec)) resVec.push_back(TPool->push([this](VidFile *b) {return vu->create_thumb(b);}, a->get_vid_file()));
}

bool VBrowser::progress_timeout() {
  if(progressFlag==0) return true;
  float counter=0.0;
  float total=0.0;
  float percent=0.0;
  std::chrono::milliseconds timer(1);
  res.clear();
  res = cxxpool::wait_for(resVec.begin(), resVec.end(),timer);
  total = resVec.size();
  if(progressFlag==1) {
    int i=0;
    for(auto &b: res) {
      if(b == std::future_status::ready && vid_list[i] > 0){
	int vid = vid_list[i];
	std::string icon_file = dbCon->create_icon_path(vid);
	dbCon->save_icon(vid);
	(*iconVec)[i]->set(icon_file);
	(*iconVec)[i]->show();
	std::system((boost::format("rm %s") %icon_file).str().c_str());
	vid_list[i]=0;
      }
      else if(b == std::future_status::ready && vid_list[i]==0) counter+=1.0;
      i++;	
    }
    percent = 100.0*counter/total;
    update_progress(counter/total,(boost::format("Creating Icons %i/%i: %d%% Complete") % counter % total %  percent).str());
    if(counter == res.size()) {
      update_progress(1.0,"Icons Complete");
      return false;
    }
    else return true;
  }
  else if(progressFlag==2) { 
    for(auto &a: res) if(a == std::future_status::ready) counter+=1.0;
    percent = 100.0*counter/total;
    if(percent < 100) {
      update_progress(counter/total,(boost::format("Creating Traces %i/%i: %d%% Complete") % counter % total %  percent).str());
      return true;
    }
    else {   //once traces are done, compare.
    update_progress(1.0,"Traces Complete");
    std::cout << "Traces complete." << std::endl;
    compare_traces();
    return true;
    }
  }
  else if(progressFlag==3 && res.size() > 0) {  
    for(auto &a: res) if(a == std::future_status::ready) counter+=1.0;
    percent = 100.0*counter/total;  
    if(percent < 100)  {
      update_progress(counter/total,(boost::format("Comparing Videos: %d%% Complete") %  percent).str());
      return true;
    }
    else {
      update_progress(1.0,"Done Dupe Hunting");
      return false;    
    }
  }
  return true;
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
  else return value*boost::algorithm::to_lower_copy(v1->fileName.native()).compare(boost::algorithm::to_lower_copy(v2->fileName.native()));   
}

void VBrowser::browse_clicked() {
  Gtk::FileChooserDialog dialog("Please choose a folder or folders", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  dialog.set_select_multiple();
  dialog.set_default_size(800, 400);
  dialog.set_transient_for(*this);
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("Select", Gtk::RESPONSE_OK);
  auto response = dialog.run();
  if (response == Gtk::RESPONSE_OK){
    paths.clear();
    std::vector<std::string> folders  = dialog.get_filenames();
    for(auto & a: folders) {
      paths.push_back(bfs::path(a));
    }
    this->populate_icons(true);
  }
}

void VBrowser::on_delete() {
  dbCon->save_db_file();
  return;
}

void VBrowser::fdupe_clicked(){
  std::vector<VidFile *> videos;
  vid_list.clear();
  for(auto & vIcon: *iconVec) {
    VidFile * vid_obj = vIcon->get_vid_file();
    int video_id = vid_obj->vid;
    vid_list.push_back(video_id);
    if(!dbCon->trace_exists(video_id)) videos.push_back(vid_obj);
  }
  vu->compare_icons(vid_list);
  resVec.clear();
  progressFlag=2;
  p_timer = Glib::signal_timeout().connect(p_timer_slot,vm["progress_time"].as<int>());
  for(auto & b: videos) resVec.push_back(TPool->push([&](VidFile * b ){ return vu->calculate_trace(b);},b));
  return;
}

void VBrowser::compare_traces() {
  resVec.clear();
  std::map<int,std::vector<uint8_t> > data_holder;
  //loop over files
  for(int i = 0; i +1 < vid_list.size(); i++) {
    dbCon->fetch_trace(vid_list[i],data_holder[vid_list[i]]);
    //loop over files after i
    for(int j = i+1; j < vid_list.size(); j++) {
      if (vu->result_map[std::make_pair(vid_list[i],vid_list[j])]/2 >= 1) continue;
      else {
	dbCon->fetch_trace(vid_list[j],data_holder[vid_list[j]]);
	resVec.push_back(TPool->push([&](int i, int j, std::map<int, std::vector<uint8_t>> & data){ return vu->compare_vids(i,j,data);},vid_list[i],vid_list[j],data_holder));
      }
    }
  }
  std::cout << "Blah1" << std::endl;
  progressFlag=3;
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

std::set<std::string> VBrowser::get_extensions() {
  std::set<std::string> extensions{".3gp",".avi",".flv",".m4v",".mkv",".mov",".mp4",".mpeg",".mpg",".mpv",".qt",".rm",".webm",".wmv"};
  return extensions;
}

void VBrowser::update_progress(double fraction, std::string label) {
  progress_bar->set_text(label);
  progress_bar->set_fraction(fraction);
  return;
}



  
