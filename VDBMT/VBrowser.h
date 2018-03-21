#include "VideoIcon.h"
#include "VideoUtils.h"

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
  std::string get_sort();
  void set_sort(std::string sort);
  
 private:
  video_utils * vu;
  po::variables_map vm;
  cxxpool::thread_pool * TPool;
  cxxpool::thread_pool * thumbPool;
  Glib::RefPtr<Gtk::Builder> refBuilder;
  Gtk::ScrolledWindow * fScrollWin;
  Gtk::FlowBox * fFBox;
  std::string sort_by;
  bool sort_desc;
  Gtk::Box * sort_opt;
  Gtk::VBox * box_outer;
  Gtk::Button * browse_button;
  Gtk::Button *asc_button;
  Gtk::Button * fdupe_button;
  Gtk::ComboBoxText * sort_combo;
  Gtk::ProgressBar * progress_bar;
  DbConnector * dbCon;
  bfs::path path;
};
 
