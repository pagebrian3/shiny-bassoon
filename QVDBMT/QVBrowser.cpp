#include "QVBrowser.h"
#include <ConfigDialog.h>
#include <MetadataDialog.h>
#include <QVBConfig.h>
#include <QVBMetadata.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <QFileDialog>
#include <QMenu>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QAction>
#include <QTimer>
#include <QListView>
#include <QStandardItem>
#include <QResizeEvent>
#include <QModelIndex>

QVBrowser::QVBrowser() : QMainWindow() {
  vu = new video_utils();
  qCfg = vu->get_config();
  qCfg->get("progress_time",fProgTime);
  qMD = vu->mdInterface();
  int winWidth, winHeight;
  qCfg->get("win_width",winWidth);
  qCfg->get("win_height",winHeight);
  resize(winWidth,winHeight);
   qCfg->get("thumb_height",fIH);
  qCfg->get("thumb_width",fIW);
  sort_by="size"; //size, name, length  TODO: make it remember last selection
  sOrder = Qt::DescendingOrder;  //Qt::DescendingOrder Qt::AscendingOrder TODO: TODO: make it remember last selection
  QPushButton *browse_button = new QPushButton("...");
  connect(browse_button, &QPushButton::clicked, this, &QVBrowser::browse_clicked);
  QPushButton * dupe_button = new QPushButton("Find Dupes");
  connect(dupe_button, &QPushButton::clicked, this, &QVBrowser::fdupe_clicked);
  QGroupBox  * sort_opt = new QGroupBox();
  QHBoxLayout * hbox = new QHBoxLayout();  
  QLabel sort_label("Sort by:");
  QComboBox * sort_combo = new QComboBox();
  sort_combo->addItem("size");
  sort_combo->addItem("length");
  sort_combo->addItem("name");
  connect(sort_combo, &QComboBox::currentTextChanged,this,&QVBrowser::on_sort_changed);
  progress_bar = new QProgressBar();
  progress_bar->setMinimum(0);
  progress_bar->setMaximum(100);
  asc_button = new QPushButton();
  asc_button->setIcon(QIcon::fromTheme("view-sort-descending"));
  connect(asc_button, &QPushButton::clicked, this, &QVBrowser::asc_clicked);
  QPushButton * config_button = new QPushButton();
  config_button->setIcon(QIcon::fromTheme("preferences-system"));
  connect(config_button, &QPushButton::clicked, this, &QVBrowser::config_clicked);
  hbox->addWidget(&sort_label);
  hbox->addWidget(sort_combo);
  hbox->addWidget(asc_button);
  hbox->addWidget(progress_bar);
  hbox->addWidget(dupe_button);
  hbox->addWidget(browse_button);
  hbox->addWidget(config_button); 
  sort_opt->setLayout(hbox);
  setMenuWidget(sort_opt);
  fFBox = new QListView(this); 
  fFBox->setResizeMode(QListView::Adjust);
  fFBox->setViewMode(QListView::IconMode);
  fFBox->setMovement(QListView::Static);
  fFBox->setSelectionMode(QListView::ExtendedSelection);
  mDAct = new QAction( ("&Edit Metadata"), this);
  mDAct->setShortcuts(QKeySequence::New);
  connect(mDAct, &QAction::triggered, this, &QVBrowser::edit_md_clicked);
  p_timer = new QTimer(this);
  connect(p_timer, &QTimer::timeout, this, &QVBrowser::progress_timeout);
  populate_icons();
  update();
  show();
}

QVBrowser::~QVBrowser() {
}

void QVBrowser::onSelChanged() {
  QModelIndexList sList = fFBox->selectionModel()->selectedIndexes();
  QStandardItem *  selItem; 
  for(auto & item: (*iconVec) )  item->setText("");
  for(int i = 0; i < sList.size(); i++)  {
    selItem = fModel->itemFromIndex(sList[i]);
    selItem->setText("SELECTED");
  }
  update();
  return;
}

#ifndef QT_NO_CONTEXTMENU
void QVBrowser::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(mDAct);
    menu.exec(event->globalPos());
}
#endif // QT_NO_CONTEXTMENU

void QVBrowser::resizeEvent(QResizeEvent* event)
{
  QMainWindow::resizeEvent(event);
  int w = event->size().width();
  int h = event->size().height();
  qCfg->set("win_width",w);
  qCfg->set("win_height",h);
  return;
}

void QVBrowser::edit_md_clicked() {
  QModelIndexList sList = fFBox->selectionModel()->selectedIndexes();
  std::vector<int> selVids;
  for(int i = 0; i < sList.size(); i++)  {
    QStandardItem *  selItem = fModel->itemFromIndex(sList[i]);
    selVids.push_back(selItem->data(Qt::UserRole+4).toInt());
  }
  MetadataDialog * mDiag = new MetadataDialog(this,selVids,qMD);
  if(mDiag->exec())
    for(auto & a: selVids) update_tooltip(a);
  return;
}

void QVBrowser::config_clicked() {
  ConfigDialog * cfgd = new ConfigDialog(this,qCfg);
  cfgd->open();
}

void QVBrowser::closeEvent(QCloseEvent *event) {
  vu->close();
  QMainWindow::closeEvent(event);
}

void QVBrowser::populate_icons(bool clean) {
  if(clean) {
    vid_list.clear();
    delete fModel;
    video_files.clear();
  }
  setCentralWidget(fFBox);
  vu->make_vids(vidFiles);
  iconVec =  new std::vector<QStandardItem *> (vidFiles.size());
  fModel = new QStandardItemModel(vidFiles.size(),1,fFBox);
  int j = 0;
  for(auto &a: vidFiles) {
    QStandardItem * b = new QStandardItem();
    b->setIcon(QIcon::fromTheme("image-missing"));
    b->setSizeHint(QSize(fIH,fIW));
    (*iconVec)[j]=b;
    iconLookup[a->vid]=j;
    video_files.push_back(a->fileName);
    vid_list.push_back(a->vid);
    b->setData(a->size,Qt::UserRole+1);
    b->setData(a->length,Qt::UserRole+2);
    b->setData(a->fileName.string().c_str(),Qt::UserRole+3);
    b->setData(a->vid,Qt::UserRole+4);
    update_tooltip(a->vid);
    fModel->appendRow(b);
    j++;
  }
  fFBox->setModel(fModel);
  QItemSelectionModel * selModel = fFBox->selectionModel();
  connect(selModel,&QItemSelectionModel::selectionChanged,this,&QVBrowser::onSelChanged);
  p_timer->start(fProgTime);
  progressFlag = 1;
  fFBox->show();
  if(j > 0) vu->start_thumbs(vidFiles); 
  return;
}

bool QVBrowser::progress_timeout() {
  if(progressFlag==0) return true;
  update();
  float counter=0.0;
  float total=0.0;
  float percent=0.0;
  std::chrono::milliseconds timer(1);
  auto res = vu->get_status();
  total = res.size();
  if(progressFlag==1) {
    int i=0;
    for(auto &b: res) {
      if(b == std::future_status::ready && vid_list[i] > 0){
	std::string icon_file = vu->save_icon(vid_list[i]);
	QImage img(QString(icon_file.c_str()));
	(*iconVec)[i]->setSizeHint(img.size());
	(*iconVec)[i]->setBackground(QBrush(img));
	(*iconVec)[i]->setIcon(QIcon());
	std::system((boost::format("rm %s") %icon_file).str().c_str());
	vid_list[i]=0;
      }
      else if(b == std::future_status::ready && vid_list[i]==0) counter+=1.0;
      i++;	
    }
    fFBox->resize(fFBox->size()+QSize(0,1));
    fFBox->resize(fFBox->size()-QSize(0,1));
    percent = 100.0*counter/total;
    update_progress(percent,(boost::format("Creating Icons %i/%i: %d%% Complete") % counter % total %  percent).str());
    if(counter == total) {
      update_progress(100,"Icons Complete");
      update_sort();
      return false;
    }
    else return true;
  }
  else if(progressFlag==2) { 
    for(auto &a: res) if(a == std::future_status::ready) counter+=1.0;
    percent = 100.0*counter/total;
    if(percent < 100) {
      update_progress(percent,(boost::format("Creating Traces %i/%i: %d%% Complete") % counter % total %  percent).str());
      return true;
    }
    else {   //once traces are done, compare.
    update_progress(100,"Traces Complete");
    std::cout << "Traces complete." << std::endl;
    vu->compare_traces();
    progressFlag=3;
    return true;
    }
  }
  else if(progressFlag==3 && total > 0) {  
    for(auto &a: res) if(a == std::future_status::ready) counter+=1.0;
    percent = 100.0*counter/total;
    if(percent < 100)  {
      update_progress(percent,(boost::format("Comparing Videos: %d%% Complete") %  percent).str());
      return true;
    }
    else {
      update_progress(100,"Done Dupe Hunting");
      return false;    
    }
  }
  p_timer->start(fProgTime);
  return true;
}


void QVBrowser::set_sort(std::string sort) {
  sort_by = sort;
  return;
}

void QVBrowser::browse_clicked() {
  QFileDialog dialog(this, tr("Please choose a folder or folders"));
  dialog.setFileMode(QFileDialog::Directory);
  QStringList fileNames;
  if(dialog.exec()) {
    fileNames = dialog.selectedFiles();
    std::vector<std::string> files;
    for(auto &a: fileNames) files.push_back(a.toStdString());
    vu->set_paths(files);
    populate_icons(true);
  }
  return;
}

void QVBrowser::fdupe_clicked(){
  std::vector<VidFile *> videos;
  vid_list.clear();
  for(auto & vFile: vidFiles) 
    vid_list.push_back(vFile->vid);
  vu->compare_icons();
  progressFlag=2;
  p_timer->start(fProgTime);
  vu->start_make_traces(vidFiles);
  return;
}

void QVBrowser::on_sort_changed(const QString & text) {
  sort_by = text.toStdString();  
  update_sort();
  return;
}

void QVBrowser::asc_clicked() {
  QString iname = "view-sort-ascending";
  if(sOrder == Qt::DescendingOrder){
    sOrder = Qt::AscendingOrder;
  }
  else{
    iname = "view-sort-descending";
    sOrder = Qt::DescendingOrder;
  }
  asc_button->setIcon(QIcon::fromTheme(iname));
  update_sort();
  return;
}

void QVBrowser::update_sort() {
  int roleIndex = 1;
  if(sort_by == "length") roleIndex=2;
  else if(sort_by == "name") roleIndex=3;
  fModel->setSortRole(Qt::UserRole+roleIndex);
  fModel->sort(0,sOrder);
  update();
  return;
}

void QVBrowser::update_progress(int fraction, std::string label) {
  //progress_bar->set_text(label);
  progress_bar->setValue(fraction);
  return;
}

void QVBrowser::update_tooltip(int vid) {
  QStandardItem * currItem = (*iconVec)[iconLookup[vid]];
  VidFile * a = vidFiles[iconLookup[vid]];
  float size = a->size;
  float length = a->length;
  std::string sizeLabel = "B";
    float factor=1.0/1024.0;
    if(factor*size >= 1.0) {
      size*=factor;
      if(factor*size >= 1.0) {
	size*=factor;
	if(factor*size >= 1.0) {
	  size*=factor;
	  sizeLabel = "GB";
	}
	else sizeLabel = "MB";
      }
      else sizeLabel = "KB";
    }
    int h = length/3600.0;
    int m = ((int)(length-h*3600))/60;
    float s = length-m*60.0-h*3600.0;
     std::string mdString = qMD->metadata_string(a->vid);
    QString toolTip((boost::format("Filename: %s\nSize: %3.2f%s\nLength: %i:%02i:%02.1f\n%s") % a->fileName %  size % sizeLabel % h % m % s % mdString).str().c_str());
    currItem->setToolTip(toolTip);
}



  
