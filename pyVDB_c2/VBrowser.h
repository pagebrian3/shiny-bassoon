#include <gtkmm-3.0/gtkmm.h>
#include "DbConnector.h"

class VBrowser: public Gtk::Window
{
 public:
  VBrowser();
  ~VBrowser();
  void populate_icons(bool clean = false);
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

  private:

  vid_file fVidFile;
};  
