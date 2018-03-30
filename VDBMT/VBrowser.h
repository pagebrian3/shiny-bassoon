#ifndef VBROWSER_H
#define VBROWSER_H

#include "VideoUtils.h"
#include <gtkmm-3.0/gtkmm.h>
#include "DbConnector.h"
#include "cxxpool.h"
#include <boost/program_options.hpp>
#include <boost/signals2/signal.hpp>
#include <set>

class video_utils;

namespace po = boost::program_options;

class VBrowser: public Gtk::Window
{
 public:
  VBrowser(int argc, char * argv[]);
  ~VBrowser();
  void populate_icons(bool clean = false);
  int sort_videos(Gtk::FlowBoxChild *vFile1, Gtk::FlowBoxChild *vFile2);
  void browse_clicked();
  void on_delete();
  void fdupe_clicked();
  void asc_clicked();
  void on_sort_changed();
  void update_icon();
  bool progress_timeout();
  std::string get_sort();
  void set_sort(std::string sort);
  po::variables_map * get_vm();
  DbConnector * get_dbcon();
  std::set<std::string> get_extensions();
  void update_progress(double fraction, std::string label);
  
  
 private:
  Gtk::ProgressBar * progress_bar;
  po::variables_map vm;
  cxxpool::thread_pool * TPool;
  Gtk::ScrolledWindow * fScrollWin;
  Gtk::FlowBox * fFBox;
  std::string sort_by;
  bool sort_desc;
  int progressFlag; //0=none 1=icons 2=traces 3=dupes
  Gtk::Box * sort_opt;
  Gtk::VBox * box_outer;
  Gtk::Button * browse_button;
  Gtk::Button *asc_button;
  Gtk::Button * fdupe_button;
  Gtk::ComboBoxText * sort_combo;
  std::vector<std::future_status> res;
  std::vector<std::future<bool> > resVec;
  std::vector<int> icon_list;
  std::vector<VideoIcon*> * iconVec;
  sigc::slot<bool>  p_timer_slot;
  sigc::connection p_timer;
  DbConnector * dbCon;
  bfs::path path;
  video_utils * vu;
};

#endif //VBROWSER_H
 
