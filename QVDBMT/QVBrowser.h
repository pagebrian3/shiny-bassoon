#ifndef QVBROWSER_H
#define QVBROWSER_H

#include <QMainWindow>
#include <boost/filesystem.hpp>

namespace bfs=boost::filesystem;

class qvdb_metadata;
class qvdb_config;
class video_utils;
class VidFile;
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
  void onSelChanged();
  void on_double_click(const QModelIndex & index); 
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
  void update_tooltip(int vid);

  protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif // QT_NO_CONTEXTMENU
    
 private:
  int progressFlag; //0=none 1=icons 2=traces 3=dupes
  float totalJobs;
  std::string sort_by;
  std::vector<int> vid_list;
  std::map<int,int> iconLookup;
  std::vector<VidFile *> vidFiles;
  std::vector<VidFile *> loadedVFs;
  std::vector<QStandardItem *> * iconVec;
  qvdb_config * qCfg ;
  qvdb_metadata * qMD;
  video_utils * vu;
  QProgressBar * progress_bar;
  QListView * fFBox;
  QStandardItemModel * fModel;
  Qt::SortOrder sOrder;
  QPushButton * asc_button;
  QAction *mDAct;
  QTimer * p_timer;
};

#endif //QVBROWSER_H
 
