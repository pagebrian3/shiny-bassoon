#include <QVBrowser.h>
#include <VidFile.h>
#include <VideoUtils.h>
#include <ConfigDialog.h>
#include <MetadataDialog.h>
#include <Miniplayer.h>
#include <FilterDialog.h>
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
  qMD = vu->mdInterface();
  resize(qCfg->get_int("win_width"),qCfg->get_int("win_height"));
  sort_by="size"; //size, name, length  TODO: make it remember last selection
  sOrder = Qt::DescendingOrder;  //Qt::DescendingOrder Qt::AscendingOrder TODO: TODO: make it remember last selection
  QPushButton *browse_button = new QPushButton("...");
  connect(browse_button, &QPushButton::clicked, this, &QVBrowser::browse_clicked);
  QPushButton * dupe_button = new QPushButton("Find Dupes");
  connect(dupe_button, &QPushButton::clicked, this, &QVBrowser::fdupe_clicked);
  QPushButton *filter_button = new QPushButton("Filter"); //replace with icon
  connect(filter_button, &QPushButton::clicked, this, &QVBrowser::filter_clicked);
  QGroupBox  * sort_opt = new QGroupBox();
  QHBoxLayout * hbox = new QHBoxLayout();  
  QLabel sort_label("Sort by:");
  QComboBox * sort_combo = new QComboBox();
  sort_combo->addItem("size");
  sort_combo->addItem("length");
  sort_combo->addItem("name");
  sort_combo->addItem("vid");
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
  hbox->addWidget(filter_button);
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
  connect(fFBox,&QListView::doubleClicked,this,&QVBrowser::on_double_click);
  setCentralWidget(fFBox);
  mDAct = new QAction( ("&Edit Metadata"), this);
  mDAct->setShortcuts(QKeySequence::New);
  connect(mDAct, &QAction::triggered, this, &QVBrowser::edit_md_clicked);
  p_timer = new QTimer(this);
  connect(p_timer, &QTimer::timeout, this, &QVBrowser::progress_timeout);
  populate_icons();
  show();
}

QVBrowser::~QVBrowser() {
}

void QVBrowser::onSelChanged() {
  QModelIndexList sList = fFBox->selectionModel()->selectedIndexes();
  QStandardItem *  selItem; 
  for(auto & item: iconVec )  item->setText("");
  for(int i = 0; i < sList.size(); i++)  {
    selItem = fModel->itemFromIndex(sList[i]);
    selItem->setText("SELECTED");
  }
  update();
  return;
}

void QVBrowser::on_double_click(const QModelIndex & index) {
  QStandardItem * selItem = fModel->itemFromIndex(index);
  std::string vid_file = selItem->data(Qt::UserRole+3).toString().toStdString();
  Miniplayer player(this,vid_file,qCfg->get_int("preview_height"),qCfg->get_int("preview_width"));
  player.exec();
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
  return;
}

void QVBrowser::filter_clicked() {
  FilterDialog * filtDialog = new FilterDialog(this,fFBox,qMD);
  filtDialog->open();
  return;
}

void QVBrowser::closeEvent(QCloseEvent *event) {
  vu->close();
  QMainWindow::closeEvent(event);
  return;
}

void QVBrowser::populate_icons(bool clean) {
  if(clean) {
    vid_list.clear();
    vidFiles.clear();
    delete fModel;
    for(auto & a: iconVec) delete a;
    iconVec.clear();
  }
  t = new boost::timer::auto_cpu_timer();
  int nItems = vu->make_vids(loadedVFs);  //loadedVFs contains those we already have in db, nItems is total icons.
  totalJobs = nItems - loadedVFs.size();  //totalJobs this is the number left to do.
  fModel = new QStandardItemModel(0,1,fFBox);
  fFBox->setModel(fModel);
  QItemSelectionModel * selModel = fFBox->selectionModel();
  connect(selModel,&QItemSelectionModel::selectionChanged,this,&QVBrowser::onSelChanged);
  p_timer->start(qCfg->get_int("progress_time"));
  progressFlag = 1;
  fFBox->show(); 
  return;
}

bool QVBrowser::progress_timeout() {
  if(progressFlag==0) return true;
  update();
  float counter=0.0;
  float percent=0.0;
  std::chrono::milliseconds timer(1);
  auto res = vu->get_status();
  if(progressFlag==1) {
    std::vector<VidFile*> batch;
    bool batchTest = vu->getVidBatch(batch);
    if(batchTest || loadedVFs.size() > 0) {
      if(loadedVFs.size() > 0) {
	batch.insert(batch.end(),loadedVFs.begin(), loadedVFs.end());
	loadedVFs.clear();
      }
      QIcon initIcon = QIcon::fromTheme("image-missing");
      QSize size_hint = QSize(qCfg->get_int("thumb_height"),qCfg->get_int("thumb_width"));
      std::vector<VidFile*> needIcons;
      int list_counter=0;
      for(auto &a: batch) {  //loop over vector of completed VidFiles
	int position=iconVec.size();
	int vid = a->vid;
	bool iExists = vu->thumb_exists(vid);
	vidFiles.push_back(a);
	list_counter++;
	QStandardItem * b;
	if(iExists) {  //if icon exists set as background and mark as done.
	  b = new QStandardItem();
	  iconVec.push_back(b);
	  bfs::path icon_file(vu->icon_filename(vid));
	  QPixmap img(QString(icon_file.c_str()));
	  iconVec[position]->setSizeHint(img.size());
	  iconVec[position]->setBackground(QBrush(img));
	  iconVec[position]->setIcon(QIcon());
	  vid_list.push_back(0);
	}
	else {  //if icon doesn't exist add it's vid to vid_list and needIcons
	  b = new QStandardItem(initIcon,"");
	  iconVec.push_back(b);
	  b->setSizeHint(size_hint);
	  vid_list.push_back(vid);
	  needIcons.push_back(a);
	}
	iconLookup[vid]=position;
	b->setData(a->size,Qt::UserRole+1);
	b->setData(a->length,Qt::UserRole+2);
	b->setData(a->fileName.string().c_str(),Qt::UserRole+3);
	b->setData(vid,Qt::UserRole+4);
	update_tooltip(vid);
	fModel->appendRow(b);
      }
      vu->start_thumbs(needIcons);
    }
    int i=0;
    for(auto &b: res) {  //this leads to alot of redundant looping, perhaps we can remove completed elements from res and and vid_list, or remember the position of the first job which was not completed from the last loop.
      if(b == std::future_status::ready) { //Job is done, icon needs to be added
	if(vid_list[i] != 0) {  
	  bfs::path iconFilename(vu->icon_filename(vid_list[i]));
	  QPixmap img(QString(iconFilename.c_str()));
	  iconVec[i]->setSizeHint(img.size());
	  iconVec[i]->setBackground(QBrush(img));
	  iconVec[i]->setIcon(QIcon());
	  vid_list[i]=0;
	}
        counter+=1.0;  //Job is done and icon already added
      }
      i++;	
    }
    fFBox->resize(fFBox->size()+QSize(0,1));  //This is a hack I hope to remove.
    fFBox->resize(fFBox->size()-QSize(0,1));
    percent = 100.0*counter/totalJobs;
    if(counter < totalJobs) {
      update_progress(percent,(boost::format("Creating Icons: %i/%i: %%p%% Complete") % counter % totalJobs).str());
    }
    else if(counter == totalJobs) {
      delete t;
      update_progress(100,"Icons Complete");
      update_sort();
      p_timer->stop();
      return false;
    }
    else return true;
  }
  else if(progressFlag==2) { 
    for(auto &a: res) if(a == std::future_status::ready) counter+=1.0;
    percent = 100.0*counter/totalJobs;
    if(percent < 100) {
      update_progress(percent,(boost::format("Creating Traces %i/%i: %%p%% Complete") % counter % totalJobs).str());
      return true;
    }
    else {   //once traces are done, compare.
    update_progress(100,"Traces Complete");
    vu->move_traces();
    delete t;
    t = new boost::timer::auto_cpu_timer();
    totalJobs = vu->compare_traces();
    progressFlag=3;
    return true;
    }
  }
  else if(progressFlag==3 && totalJobs > 0) {  
    for(auto &a: res) if(a == std::future_status::ready) counter+=1.0;
    percent = 100.0*counter/totalJobs;
    if(percent < 100)  {
      update_progress(percent,"Comparing Videos: %p% Complete");
      return true;
    }
    else {
      update_progress(100,"Done Dupe Hunting");
      p_timer->stop();
      delete t;
      return false;    
    }
  }
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
    std::vector<bfs::path> files;
    for(auto &a: fileNames) files.push_back(bfs::path(a.toStdString()));
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
  vu->img_comp_thread();
  t = new boost::timer::auto_cpu_timer();
  progressFlag=2;
  p_timer->start(qCfg->get_int("progress_time"));
  totalJobs = vu->start_make_traces(vidFiles);
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
  else if(sort_by == "vid") roleIndex=4;
  fModel->setSortRole(Qt::UserRole+roleIndex);
  fModel->sort(0,sOrder);
  update();
  return;
}

void QVBrowser::update_progress(int fraction, std::string label) {
  progress_bar->setFormat(label.c_str());
  progress_bar->setValue(fraction);
  return;
}

void QVBrowser::update_tooltip(int vid) {
  QStandardItem * currItem = iconVec[iconLookup[vid]];
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
  QString toolTip((boost::format("Filename: %s\nVID: %i\nSize: %3.2f%s\nLength: %i:%02i:%02.1f%s") % a->fileName % a->vid %  size % sizeLabel % h % m % s % mdString).str().c_str());
  currItem->setToolTip(toolTip);
}



  
