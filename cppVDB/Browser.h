#include <gtkmm/window.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/button.h>
#include <gtkmm/combobox.h>
#include <string>


class MyWindow: public Gtk::Window
{
 public:
  MyWindow();

  ~MyWindow();

  void populate_icons(bool clean=false);

  int sort_videos(auto videoFile1, auto videoFile2);

  void add_filters();

  void browse_clicked();

  void fdupe_clicked();

  void on_sort_changed();
  
  void asc_clicked();

  void on_delete();

 private:
  //DBCon * fDbCon;
  Gtk::FlowBox * fBox;
  Gtk::ScrolledWindow * fScrollWin;
  Gtk::Button * fAscButton;
  Gtk::ComboBoxText * fSortCombo;
  bool fSortDesc;
  std::string fSortBy;
  std::string fDirectory;
  
};
