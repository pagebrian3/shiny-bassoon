#ifndef QVBROWSER_H
#define QVBROWSER_H

#include "VideoUtils.h"
#include <QMainWindow>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QListView>
#include <QStandardItem>
#include <QResizeEvent>
#include <boost/timer/timer.hpp>

class QVBrowser: public QMainWindow
{
 public:
  QVBrowser();
  virtual ~QVBrowser();
  void populate_icons(bool clean = false);
  void browse_clicked();
  void fdupe_clicked();
  void config_clicked();
  void asc_clicked();
  void on_sort_changed(const QString & text);
  void resizeEvent(QResizeEvent * event);
  void closeEvent(QCloseEvent * event);
  bool progress_timeout();
  void set_sort(std::string sort);
  void update_sort();
  void update_progress(int percent, std::string label);
    
 private:
  QProgressBar * progress_bar;
  QListView * fFBox;
  QStandardItemModel * fModel;
  qvdb_config * qCfg ;
  std::string sort_by;
  Qt::SortOrder sOrder;
  int progressFlag; //0=none 1=icons 2=traces 3=dupes
  int fProgTime;
  QPushButton * asc_button;
  std::vector<int> vid_list;
  std::vector<VidFile *> vidFiles;
  std::vector<QStandardItem *> * iconVec;
  QTimer * p_timer;
  std::vector<bfs::path> video_files;
  video_utils * vu;
  boost::timer::auto_cpu_timer * fTimer;
};

#endif //QVBROWSER_H
 
