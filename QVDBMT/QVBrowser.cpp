#include "QVBrowser.h"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>

QVBrowser::QVBrowser(po::variables_map & vm) {
  fTimer = new boost::timer::auto_cpu_timer();
  fProgTime = vm["progress_time"].as<int>();
  setMinimumWidth(vm["win_width"].as<int>());
  setMinimumHeight(vm["win_height"].as<int>()); 
  vu = new video_utils(vm);
  sort_by="size"; //size, name, length
  sOrder = Qt::DescendingOrder;  //Qt::DescendingOrder Qt::AscendingOrder
  browse_button = new QPushButton("...");
  connect(browse_button, &QPushButton::clicked, this, &QVBrowser::browse_clicked);
  fdupe_button = new QPushButton("Find Dupes");
  connect(fdupe_button, &QPushButton::clicked, this, &QVBrowser::fdupe_clicked);
  sort_opt = new QGroupBox();
  QHBoxLayout * hbox = new QHBoxLayout();  
  QLabel sort_label("Sort by:");
  sort_combo = new QComboBox();
  sort_combo->addItem("size");
  sort_combo->addItem("length");
  sort_combo->addItem("name");
  connect(sort_combo, &QComboBox::currentTextChanged,this,&QVBrowser::on_sort_changed);
  progress_bar = new QProgressBar();
  progress_bar->setMinimum(0);
  progress_bar->setMaximum(100);
  asc_button = new QPushButton();
  QString iname = "view-sort-descending";
  asc_button->setIcon(QIcon::fromTheme(iname));
  connect(asc_button, &QPushButton::clicked, this, &QVBrowser::asc_clicked); 
  hbox->addWidget(&sort_label);
  hbox->addWidget(sort_combo);
  hbox->addWidget(asc_button);
  hbox->addWidget(progress_bar);
  hbox->addWidget(fdupe_button);
  hbox->addWidget(browse_button); 
  sort_opt->setLayout(hbox);
  setMenuWidget(sort_opt);
  fFBox = new QListView(this);
  fFBox->setResizeMode(QListView::Adjust);
  fFBox->setViewMode(QListView::IconMode);
  fFBox->setMovement(QListView::Static);
  p_timer = new QTimer(this);
  connect(p_timer, &QTimer::timeout, this, &QVBrowser::progress_timeout);
  populate_icons();
  update();
  show();
}

QVBrowser::~QVBrowser() {
  std::system("reset");
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
    float size = a->size;
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
    int h = a->length/3600.0;
    int m = ((int)(a->length)%3600)/60;
    float s = a->length-m*60.0;
    b->setIcon(QIcon::fromTheme("image-missing"));
    QString toolTip((boost::format("Filename: %s\nSize: %3.2f%s\nLength: %i:%02i:%02.1f") % a->fileName %  size % sizeLabel % h % m % s).str().c_str());
    b->setToolTip(toolTip);
    (*iconVec)[j]=b;
    video_files.push_back(a->fileName);
    vid_list.push_back(a->vid);
    QList<QStandardItem * > row;
    row.append(b); 
    QStandardItem * i1 = new QStandardItem((boost::format("%010i") % a->size).str().c_str());
    row.append(i1);
    i1 = new QStandardItem((boost::format("%08i") % a->length).str().c_str());
    row.append(i1);
    i1 = new QStandardItem(boost::algorithm::to_lower_copy((boost::format("%s") % a->fileName).str()).c_str());
    row.append(i1);
    fModel->appendRow(row);
    j++;
  }
  fFBox->setModel(fModel);
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
	(*iconVec)[i]->setIcon(QIcon());
	(*iconVec)[i]->setSizeHint(img.size());
	(*iconVec)[i]->setBackground(QBrush(img));
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
    vu->compare_traces(vid_list);
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

void QVBrowser::on_delete() {
  vu->save_db();
  return;
}

void QVBrowser::fdupe_clicked(){
  std::vector<VidFile *> videos;
  vid_list.clear();
  for(auto & vFile: vidFiles) 
    vid_list.push_back(vFile->vid);
  vu->compare_icons(vid_list);
  progressFlag=2;
  p_timer->start(fProgTime);
  vu->start_make_traces(vidFiles);
  return;
}

void QVBrowser::on_sort_changed() {
  sort_by = sort_combo->currentText().toStdString();  
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
  int column = 1;
  if(sort_by == "length") column=2;
  else if(sort_by == "name") column=3;
  fModel->sort(column,sOrder);
  update();
  return;
}

void QVBrowser::update_progress(int fraction, std::string label) {
  //progress_bar->set_text(label);
  progress_bar->setValue(fraction);
  return;
}



  
