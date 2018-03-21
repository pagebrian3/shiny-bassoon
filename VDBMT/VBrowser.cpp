#include "VBrowser.h"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <set>

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
  thumbPool = new cxxpool::thread_pool(1);
  this->set_default_size(vm["win_width"].as<int>(), vm["win_height"].as<int>());
  dbCon = new DbConnector(vm["app_path"].as<std::string>());
  path = vm["default_path"].as<std::string>();
  vu = new video_utils(dbCon,&vm,path);
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
  progress_bar->hide();
}

VBrowser::~VBrowser() {
  delete TPool;
  delete thumbPool;
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
      if(vu->get_extensions().count(extension)) {
	std::string pathName(x.path().native());
	//std::cout << "PATH: "<<pathName<<std::endl;
	if(!dbCon->video_exists(pathName)) min_vid++;
	icons.push_back(TPool->push([this](std::string path,DbConnector * con, int vid) {return new VideoIcon(path,con,vid,&vm);},pathName,dbCon,min_vid));
      }
  }
  cxxpool::wait(icons.begin(),icons.end());
  std::vector<VideoIcon*> * iconVec =  new std::vector<VideoIcon *> (icons.size());
  int j = 0;
  for(auto &a: icons) {
    VideoIcon * b = a.get();
    vu->update_icon().connect(sigc::mem_fun(*b, &VideoIcon::set_icon));
    if(!b->hasIcon) {
      (*iconVec)[j]=b;
      j++;
    }
    fFBox->add(*b);
  }
  fFBox->invalidate_sort();
  fScrollWin->add(*fFBox);
  this->show_all();
  std::vector<std::future<bool> > * resVec = new std::vector<std::future<bool> >(j);
  if(j > 0) {
    int i = 0;
    for(auto &a: (*iconVec)) {     
      (*resVec)[i]=thumbPool->push([this](VidFile *b) {return vu->create_thumb(b);}, a->get_vid_file());
      i++;
    }
  }
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
    vu->change_path(path);
    this->populate_icons(true);
  }
}

void VBrowser::on_delete() {
  dbCon->save_db_file();
}

void VBrowser::fdupe_clicked(){
  TPool->push([&](){ return vu->find_dupes();});
  return;
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


  
