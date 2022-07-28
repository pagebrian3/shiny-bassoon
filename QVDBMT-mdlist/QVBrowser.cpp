#include <QVBrowser.h>
#include <VidFile.h>
#include <VideoUtils.h>
#include <ConfigDialog.h>
#include <MetadataDialog.h>
#include <MDIEDialog.h>
#include <FilterDialog.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <QFileDialog>
#include <QMenu>
#include <QProgressBar>
#include <QAction>
#include <QTimer>
#include <QStandardItem>
#include <QResizeEvent>
#include <QModelIndex>
#include <QThread>
#include <iostream>
#include <chrono>

QVBrowser::QVBrowser() : QMainWindow() {
  vu = new video_utils();
  vu->set_vid_vec(&vid_list);
  qCfg = vu->get_config();
  qMD = vu->mdInterface();
  resize(qCfg->get_int("win_width"),qCfg->get_int("win_height"));
  sort_by=qCfg->get_string("sort_by"); //size, name, length, vid  
  if(qCfg->get_int("sort_ascending") == 0) sOrder = Qt::DescendingOrder;
  else sOrder=Qt::AscendingOrder;
  paths.push_back(vu->home_path());
  QPushButton *browse_button = new QPushButton("...");
  connect(browse_button, &QPushButton::clicked, this, &QVBrowser::browse_clicked);
  QPushButton *iemd_button = new QPushButton("MD Import/Export");
  connect(iemd_button, &QPushButton::clicked, this, &QVBrowser::iemd_clicked);
  QPushButton * dupe_button = new QPushButton("Find Dupes");
  connect(dupe_button, &QPushButton::clicked, this, &QVBrowser::fdupe_clicked);
  QPushButton *filter_button = new QPushButton("Filter"); //replace with icon
  connect(filter_button, &QPushButton::clicked, this, &QVBrowser::filter_clicked);
  QGroupBox  * sort_opt = new QGroupBox();
  QHBoxLayout * hbox = new QHBoxLayout();  
  QLabel sort_label("Sort by:");
  QComboBox * sort_combo = new QComboBox();
  QStringList items = {"size","length","name","vid","date"};
  sort_combo->addItems(items);
  sort_combo->setCurrentText(sort_by.c_str());
  connect(sort_combo, &QComboBox::currentTextChanged,this,&QVBrowser::on_sort_changed);
  progress_bar = new QProgressBar();
  progress_bar->setMinimum(0);
  progress_bar->setMaximum(100);
  asc_button = new QPushButton();
  if(sOrder == Qt::DescendingOrder) asc_button->setIcon(QIcon::fromTheme("view-sort-descending-symbolic"));
  else asc_button->setIcon(QIcon::fromTheme("view-sort-ascending-symbolic"));
  connect(asc_button, &QPushButton::clicked, this, &QVBrowser::asc_clicked);
  QPushButton * config_button = new QPushButton();
  config_button->setIcon(QIcon::fromTheme("preferences-system-symbolic"));
  connect(config_button, &QPushButton::clicked, this, &QVBrowser::config_clicked);
  hbox->addWidget(&sort_label);
  hbox->addWidget(sort_combo);
  hbox->addWidget(asc_button);
  hbox->addWidget(filter_button);
  hbox->addWidget(progress_bar);
  hbox->addWidget(dupe_button);
  hbox->addWidget(browse_button);
  hbox->addWidget(iemd_button);
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
  std::string extStrin;
  extStrin = qCfg->get_string("extensions");
  boost::char_separator<char> sep(" \"");
  boost::tokenizer<boost::char_separator<char> > tok(extStrin,sep);
  for(auto &a: tok) extensions.insert(a);
  populate_icons();
  show();
}

QVBrowser::~QVBrowser() {
  delete vu;
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
  boost::algorithm::replace_all(vid_file,"\\\\\\\'","'");
  boost::algorithm::replace_all(vid_file,"\(","(");
  std::string command((boost::format("/bin/mpv \"%s\" &") % vid_file).str());
  std::system(command.c_str());
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
    if(fFBox->isRowHidden(sList[i].row())) continue;  //Just to be sure we aren't selecting hidden items when filtering.
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
  if(cfgd->exec()) {
    int w = qCfg->get_int("win_width");
    int h = qCfg->get_int("win_height");
    if(w != width() || h != height()) resize(w,h);
  }
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
    newVids.clear();
    completedJobs.clear();
    vid_list.clear();
    vidFiles.clear();
    delete fModel;
    iconVec.clear();
  }
  QIcon initIcon = QIcon::fromTheme("image-missing");
  std::vector<std::string> dims;
  std::string tDims = qCfg->get_string("thumb_size");
  boost::split(dims, tDims, boost::is_any_of("x"));
  QSize size_hint = QSize(std::atoi(dims[0].c_str()),std::atoi(dims[1].c_str()));
  fModel = new QStandardItemModel(0,1,fFBox);
  fFBox->setModel(fModel);
  QItemSelectionModel * selModel = fFBox->selectionModel();
  t = new boost::timer::auto_cpu_timer();
  DbConnector * db = vu->get_db();
  int i = 0;
  std::vector<int> tempVids;
  //Might want to parallelize this proccess or at least put it on a separate thread as it currently tends to cause the UI to freeze
  for(auto & path: paths) {
    std::vector<std::filesystem::path> current_dir_files;
    for(auto & x: std::filesystem::directory_iterator(path)) {
      auto extension = x.path().extension().generic_string();
      std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
      if(extensions.count(extension)) {
	std::filesystem::path pathName = x.path();
	current_dir_files.push_back(pathName);
	VidFile * a;
	QStandardItem * b;
	auto lwt = std::filesystem::last_write_time(pathName);
	std::time_t cftime = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(lwt));
	if(!db->video_exists(pathName))  {
	  new_icon_lookup[i]=newVids.size();
	  a = new VidFile(pathName);
	  newVids.push_back(a);
	  b = new QStandardItem(initIcon,"");
	  b->setData(a->fileName.string().c_str(),Qt::UserRole+3);
	  b->setSizeHint(size_hint);
	}
	else {
	  a = db->fetch_video(pathName);
	  current_dir_files.push_back(pathName);
	  b = new QStandardItem();
	  if(a->okflag == -1)  b->setIcon(initIcon);
	  else {
	    std::filesystem::path icon_file(vu->icon_filename(a->vid));
	    QPixmap img(QString(icon_file.c_str()));
	    b->setSizeHint(img.size());
	    b->setBackground(QBrush(img));
	    b->setIcon(QIcon());
	  }
	  b->setData(a->vid,Qt::UserRole+4);
	  tempVids.push_back(a->vid);
	  b->setData(a->size,Qt::UserRole+1);
	  b->setData(a->length,Qt::UserRole+2);
	  b->setData(a->fileName.string().c_str(),Qt::UserRole+3);
	  b->setData((int)cftime,Qt::UserRole+5);
	  iconLookup[a->vid]=i;	  
	}
        vidFiles.push_back(a);
	iconVec.push_back(b);
	fModel->appendRow(b);
	i++;
      }
    }
    std::vector<int> orphans = db->cleanup(path,current_dir_files);
    std::filesystem::path traceSave = vu->save_path();
    traceSave+="traces/";
    for(auto & a: orphans) {
      //probably need more cleanup tasks here.
      std::filesystem::remove(vu->icon_filename(a));
      std::filesystem::remove(vu->createPath(traceSave,a,".bin"));
    }
  }
  qMD->load_file_md(tempVids);
  for( auto & v: tempVids) update_tooltip(v);
  totalJobs=newVids.size();
  completedJobs.resize(totalJobs);
  vu->make_vids(newVids); 
  connect(selModel,&QItemSelectionModel::selectionChanged,this,&QVBrowser::onSelChanged);
  progressFlag = 1;
  p_timer->start(qCfg->get_int("progress_time"));
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
  std::vector<int> tempVids;
  if(progressFlag==1) {
    int i=0;
    for(auto &c: res) {  //this leads to alot of redundant looping, perhaps we can remove completed elements from res and and vid_list, or remember the position of the first job which was not completed from the last loop.
      if(completedJobs[i]==1) counter+=1;
      else if(c == std::future_status::ready) { //Job is done, icon needs to be added
	VidFile * a = newVids[i];
	int vid = a->vid;
	iconLookup[vid]=new_icon_lookup[i];
	QStandardItem * b = iconVec[new_icon_lookup[i]];
	if(a->okflag != -1) {
	  std::filesystem::path iconFilename(vu->icon_filename(vid));
	  QPixmap img(QString(iconFilename.c_str()));
	  b->setSizeHint(img.size());
	  b->setBackground(QBrush(img));
	  b->setIcon(QIcon());
	}
	b->setData(a->vid,Qt::UserRole+4);
	b->setData(a->size,Qt::UserRole+1);
	b->setData(a->length,Qt::UserRole+2);
	b->setData(a->fileName.string().c_str(),Qt::UserRole+3);
        counter+=1.0;  //Job is done and icon already added
	completedJobs[i] = 1;
	tempVids.push_back(a->vid);
      }
      i++;	
    }
    qMD->load_file_md(tempVids);
    for( auto & v: tempVids) update_tooltip(v);
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
  QFileDialog dialog(this, tr("Please Choose a Folder or Folders"));
  dialog.setFileMode(QFileDialog::Directory);
  QStringList fileNames;
  if(dialog.exec()) {
    fileNames = dialog.selectedFiles();
    paths.clear();
    for(auto &a: fileNames) paths.push_back(std::filesystem::path(a.toStdString()));
    populate_icons(true);
  }
  return;
}

void QVBrowser::iemd_clicked() {
  MDIEDialog mdieDiag(this,qMD,vu->get_db());
  mdieDiag.exec();
  return;
}

void QVBrowser::fdupe_clicked(){
  std::vector<VidFile *> videos;
  vid_list.clear();
  for(auto & vFile: vidFiles) 
    vid_list.push_back(vFile->vid);
  std::cout << "Starting img comp" << std::endl;
  vu->img_comp_thread();
  t = new boost::timer::auto_cpu_timer();
  progressFlag=2;
  p_timer->start(qCfg->get_int("progress_time"));
  std::cout << "num traces" << vidFiles.size() << std::endl;
  totalJobs = vu->start_make_traces(vidFiles);
  return;
}

void QVBrowser::on_sort_changed(const QString & text) {
  sort_by = text.toStdString();
  qCfg->set("sort_by",sort_by);
  update_sort();
  return;
}

void QVBrowser::asc_clicked() {
  QString iname = "view-sort-ascending-symbolic";
  if(sOrder == Qt::DescendingOrder){
    sOrder = Qt::AscendingOrder;
    qCfg->set("sort_ascending",1);
  }
  else{
    iname = "view-sort-descending-symbolic";
    sOrder = Qt::DescendingOrder;
    qCfg->set("sort_ascending",0);
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
  else if(sort_by == "date") roleIndex=5;
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
  if(vid == 0) return;
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
  
