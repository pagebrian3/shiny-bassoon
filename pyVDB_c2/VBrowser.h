#include "VideoIcon.h"

class VBrowser: public Gtk::Window
{
 public:
  VBrowser();
  ~VBrowser();
  void populate_icons(bool clean = false);
  int sort_videos(VideoIcon vFile1, VideoIcon vFile2);
  void browse_clicked();
  void on_delete();
  void calculate_trace(vid_file obj);
  void fdupe_clicked();
  void asc_clicked();
  void on_sort_changed();
  
 private:
  Gtk::ScrolledWindow * fScrollWin;
  Gtk::FlowBox * fFBox;
  std::string sort_by;
  bool sort_desc;
  Gtk::Button * browse_button;
  Gtk::Button * fdupe_button;
  Gtk::Button * asc_button;
  Gtk::ComboBoxText * sort_combo;
  DbConnector * dbCon;
  bfs::path path;
};
 
