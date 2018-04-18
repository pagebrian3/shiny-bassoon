#include "VBrowser.h"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

VBrowser::VBrowser(po::variables_map & vm) {
  fProgTime = vm["progress_time"].as<int>();
  set_default_size(vm["win_width"].as<int>(),vm["win_height"].as<int>()); 
  vu = new video_utils(vm);
  sort_by="size"; //size, name, length
  sort_desc=true;  //true, false
  box_outer = new Gtk::VBox(false, 6);
  add(*box_outer);
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
  populate_icons();  
}

VBrowser::~VBrowser() {
  std::system("reset");
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
    delete iconVec;
    video_files.clear();
  }
  fFBox = new Gtk::FlowBox();
  fFBox->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
  fFBox->set_sort_func(sigc::mem_fun(*this,&VBrowser::sort_videos));
  fFBox->set_homogeneous(false);
  std::vector<VidFile *> vidFiles;
  vu->make_vids(vidFiles);
  iconVec =  new std::vector<VideoIcon *> (vidFiles.size());
  int j = 0;
  for(auto &a: vidFiles) {
    VideoIcon * b = new VideoIcon(a);
    (*iconVec)[j]=b;
    video_files.push_back(a->fileName);
    vid_list.push_back(a->vid);
    j++;
    fFBox->add(*b);
  }
  fFBox->invalidate_sort();
  fScrollWin->add(*fFBox);
  show_all();
  p_timer = Glib::signal_timeout().connect(p_timer_slot,fProgTime);
  progressFlag = 1;
  if(j > 0) vu->start_thumbs(vidFiles); 
  return;
}

bool VBrowser::progress_timeout() {
  if(progressFlag==0) return true;
  float counter=0.0;
  float total=0.0;
  float percent=0.0;
  std::chrono::milliseconds timer(1);
  auto res = vu->get_status();
  total = res.size();
  if(progressFlag==1) {
    int i=0;
    for(auto &b: res) {
      if(b == std::future_status::ready && vid_list[i] > 0){
	int vid = vid_list[i];
	std::string icon_file = vu->save_icon(vid);
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
    if(counter == total) {
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
    vu->compare_traces(vid_list);
    progressFlag=3;
    return true;
    }
  }
  else if(progressFlag==3 && total > 0) {  
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
  return;
}

int VBrowser::sort_videos(Gtk::FlowBoxChild *videoFile1, Gtk::FlowBoxChild *videoFile2) {
  int value=1;
  VidFile *v1=(reinterpret_cast<VideoIcon*>(videoFile1->get_child()))->get_vid_file();
  VidFile *v2=(reinterpret_cast<VideoIcon*>(videoFile2->get_child()))->get_vid_file();
  if(sort_desc) value *= -1;
  if(!std::strcmp(get_sort().c_str(),"size")) return value*(v1->size - v2->size);
  else if(!std::strcmp(get_sort().c_str(),"length")) return value*(v1->length - v2->length);
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
    vu->set_paths(dialog.get_filenames());
    populate_icons(true);
  }
  return;
}

void VBrowser::on_delete() {
  vu->save_db();
  return;
}

void VBrowser::fdupe_clicked(){
  std::vector<VidFile *> videos;
  vid_list.clear();
  for(auto & vIcon: *iconVec) {
    VidFile * vid_obj = vIcon->get_vid_file();
    int video_id = vid_obj->vid;
    vid_list.push_back(video_id);
    videos.push_back(vid_obj);
  }
  vu->compare_icons(vid_list);
  progressFlag=2;
  p_timer = Glib::signal_timeout().connect(p_timer_slot,fProgTime);
  vu->start_make_traces(videos);
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

void VBrowser::update_progress(double fraction, std::string label) {
  progress_bar->set_text(label);
  progress_bar->set_fraction(fraction);
  return;
}



  
