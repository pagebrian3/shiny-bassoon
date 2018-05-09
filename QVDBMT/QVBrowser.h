#ifndef QVBROWSER_H
#define QVBROWSER_H

#include "VideoUtils.h"
#include "QVideoIcon.h"
#include <QMainWindow>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QListView>
#include <QGroupBox>
#include <QComboBox>
#include <boost/program_options.hpp>
#include <set>
#include <boost/timer/timer.hpp>

namespace po = boost::program_options;

class QVBrowser: public QMainWindow
{
 public:
  QVBrowser(po::variables_map &vm);
  //~QVBrowser();
  virtual ~QVBrowser();
  void populate_icons(bool clean = false);
  //int sort_videos(Gtk::FlowBoxChild *vFile1, Gtk::FlowBoxChild *vFile2);
  void browse_clicked();
  void on_delete();
  void fdupe_clicked();
  void asc_clicked();
  void on_sort_changed();
  bool progress_timeout();
  std::string get_sort();
  void set_sort(std::string sort);
  void update_progress(int percent, std::string label);
    
 private:
  QProgressBar * progress_bar;
  QListView * fFBox;
  QStandardItemModel * fModel;
  std::string sort_by;
  bool sort_desc;
  int progressFlag; //0=none 1=icons 2=traces 3=dupes
  int fProgTime;
  QGroupBox * sort_opt;
  QPushButton * browse_button;
  QPushButton * asc_button;
  QPushButton * fdupe_button;
  QComboBox * sort_combo;
  std::vector<int> vid_list;
  std::vector<QVideoIcon*> * iconVec;
  //sigc::slot<bool> p_timer_slot;
  QTimer * p_timer;
  std::vector<bfs::path> video_files;
  video_utils * vu;
  boost::timer::auto_cpu_timer * fTimer;
};

#endif //QVBROWSER_H
 