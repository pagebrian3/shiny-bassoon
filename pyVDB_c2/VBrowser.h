#include <gtkmm-3.0/gtkmm.h>
#include "DbConnector.h"

class VBrowser: public Gtk::Window
{
 public:
  VBrowser();
  ~VBrowser();
  void populate_icons(bool clean = false);
  int sort_videos(VideoIcon vFile1, VideoIcon vFile2);
  void browse_clicked();
  void on_delete();
  
 private:
  Gtk::ScrolledWindow * fScrollWin;
  Gtk::FlowBox * fFBox;
  std::string sort_by;
  bool sort_desc;
  Gtk::Button * browse_button;
  Gtk::Button * fdupe_button;
  Gtk::ComboBoxText * sort_combo;
  DbConnector * dbCon;
  bfs::path path;
};


class VideoIcon : public Gtk::EventBox
{
  public:
  VideoIcon(char *fileName, DbConnector * DbCon);
  vid_file get_vid_file();

  private:
  vid_file fVidFile;
};  
