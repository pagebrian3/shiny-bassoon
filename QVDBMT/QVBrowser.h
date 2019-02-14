#ifndef QVBROWSER_H
#define QVBROWSER_H

#include "VideoUtils.h"
#include <boost/timer/timer.hpp>
#include <QMainWindow>

class QPushButton;
class QProgressBar;
class QTimer;
class QListView;
class QStandardItem;
class QStandardItemModel;
class QResizeEvent;
class QAction;
class QContextMenuEvent;

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
  void edit_md_clicked();
  void on_sort_changed(const QString & text);
  void resizeEvent(QResizeEvent * event);
  void closeEvent(QCloseEvent * event);
  bool progress_timeout();
  void set_sort(std::string sort);
  void update_sort();
  void update_progress(int percent, std::string label);

  protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif // QT_NO_CONTEXTMENU
    
 private:
  QProgressBar * progress_bar;
  QListView * fFBox;
  QStandardItemModel * fModel;
  qvdb_config * qCfg ;
  qvdb_metadata * qMD;
  std::string sort_by;
  Qt::SortOrder sOrder;
  int progressFlag; //0=none 1=icons 2=traces 3=dupes
  int fProgTime;
  int fIW, fIH;
  QPushButton * asc_button;
  QAction *mDAct;
  std::vector<int> vid_list;
  std::vector<VidFile *> vidFiles;
  std::vector<QStandardItem *> * iconVec;
  QTimer * p_timer;
  std::vector<bfs::path> video_files;
  video_utils * vu;
  boost::timer::auto_cpu_timer * fTimer;

};

#endif //QVBROWSER_H
 
