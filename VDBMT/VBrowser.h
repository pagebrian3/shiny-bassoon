#include "VideoIcon.h"
#include "cxxpool.h"

class VBrowser: public Gtk::Window
{
 public:
  VBrowser(int argc, char * argv[]);
  ~VBrowser();
  void populate_icons(bool clean = false);
  int sort_videos(Gtk::FlowBoxChild *vFile1, Gtk::FlowBoxChild *vFile2);
  void browse_clicked();
  void on_delete();
  bool calculate_trace(VidFile * obj);
  void fdupe_clicked();
  void asc_clicked();
  void on_sort_changed();
  bool compare_vids(int i, int j, std::map<int,std::vector<unsigned short> > & data);
  std::string get_sort();
  void set_sort(std::string sort);
  
 private:
  po::variables_map vm;
  float cTraceFPS, cCompTime, cSliceSpacing, cThresh;
  cxxpool::thread_pool * TPool;
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
  Gtk::InfoBar * info_bar;
  DbConnector * dbCon;
  bfs::path path;
};
 
