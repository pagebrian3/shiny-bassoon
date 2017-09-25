#include "VBrowser.h"

std::set<std::string> extensions{".mp4",".wmv",".mov",".rm",".m4v",".flv",".avi",".qt",".mpg",".mpeg",".mpv",".3gp"};

VBrowser::VBrowser() {
  Gtk::VBox * box_outer = new GTK::VBox(false, 6);
  this->add(box_outer);
  sort_by=default_sort; //size, name, length
  sort_desc=default_desc;  //true, false
  browse_button = new Gtk::Button("...");
  browse_button->signal_clicked().connect(sigc::ptr_fun(&browse_clicked));
  fdupe_button = new Gtk::Button("Find Dupes");
  fdupe_button->signal_clicked().connect(sigc::ptr_fun(&fdupe_clicked));
  GtkBox * sort_opt = new Gtk::Box();
  GtkLabel * sort_label = new Gtk::Label("Sort by:");
  sort_combo = new Gtk::ComboBoxText();
  sort_combo->append("size");
  sort_combo->append("length");
  sort_combo->append("name");
  sort_combo->set_active(0);
  sort_combo->on_changed().connect(sigc::ptr_fun(&on_sort_changed));
  GtkStockItem stock_icon = GTK_STOCK_SORT_ASCENDING);
  if (sort_desc) stock_icon=GTK_STOCK_SORT_DESCENDING
  Gtk::Button * asc_button = new Gtk::Button();
  asc_button->set_image(asc_button,Gtk::Image(stock_icon));
  asc_button->signal_clicked().connect(sigc::ptr_fun(&asc_clicked));
  sort_opt->add(sort_label);
  sort_opt->add(sort_combo);
  sort_opt->add(asc_button);
  sort_opt->pack_end(fdupe_button,  false,  false,  0);
  sort_opt->pack_end(browse_button,  false, false, 0 ); 
  box_outer->pack_start(sort_opt, false, true, 0);
  fScrollWin = new Gtk::ScrolledWindow(NULL,NULL);
  box_outer->pack_start(fScrollWin, true, true, 0);
  populate_icons();
}

void VBrowser::populate_icons(bool clean=false) {
  if(clean) fScrollWin->remove(fFBox);
  fFBox = new Gtk::FlowBox();
  fFBox->set_sort_func(sort_videos,NULL,NULL);
  std::vector<bfs::directory_entry x> video_list;
  for (bfs::directory_entry & x : bfs::recursive_directory_iterator(path))  {
    extension = x.path().extension().generic_string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if(extensions.count(extension)) {
      fBox->add(VideoIcon(x.path().str());     
    }
  }   
}

VideoIcon::VideoIcon(char *fileName, DbConnector * dbCon){
  char * img_dat;
  if(dbCon->video_exists(fileName)) {
    fVidFile = dbCon->fetch_video(fileName);
    img_dat = dbCon->fetch_icon(fVidFile.vdatid);
    }
  else {
    fVidFile.fileName=fileName;
    fVidFile.size = bfs::file_size(bfs::path(fileName));
    auto mihandle = MediaInfo_New();
    fVidFile.length = mihandle->MediaInfo_Open(fileName); 
  }
};
  
