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
  sort_desc=true;  //true, false
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
    delete iconVec;
    video_files.clear();
  }
  
  setCentralWidget(fFBox);
  std::vector<VidFile *> vidFiles;
  vu->make_vids(vidFiles);
  iconVec =  new std::vector<QVideoIcon *> (vidFiles.size());
  fModel = new QStandardItemModel(vidFiles.size(),1,fFBox);
  int j = 0;
  QList<QStandardItem*> qvList;
  for(auto &a: vidFiles) {
    QVideoIcon * b = new QVideoIcon(a);
    (*iconVec)[j]=b;
    video_files.push_back(a->fileName);
    vid_list.push_back(a->vid);
    fModel->appendRow(b);
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
	fModel->removeRow(i);
	fModel->insertRow(i,(*iconVec)[i]);
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

std::string QVBrowser::get_sort() {
  return sort_by;
}

void QVBrowser::set_sort(std::string sort) {
  sort_by = sort;
  return;
}

/*int QVBrowser::sort_videos(Gtk::FlowBoxChild *videoFile1, Gtk::FlowBoxChild *videoFile2) {
  int value=1;
  VidFile *v1=(reinterpret_cast<VideoIcon*>(videoFile1->get_child()))->get_vid_file();
  VidFile *v2=(reinterpret_cast<VideoIcon*>(videoFile2->get_child()))->get_vid_file();
  if(sort_desc) value *= -1;
  if(!std::strcmp(get_sort().c_str(),"size")) return value*(v1->size - v2->size);
  else if(!std::strcmp(get_sort().c_str(),"length")) return value*(v1->length - v2->length);
  else return value*boost::algorithm::to_lower_copy(v1->fileName.native()).compare(boost::algorithm::to_lower_copy(v2->fileName.native()));   
  }*/

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
  for(auto & vIcon: *iconVec) {
    VidFile * vid_obj = vIcon->get_vid_file();
    int video_id = vid_obj->vid;
    vid_list.push_back(video_id);
    videos.push_back(vid_obj);
  }
  vu->compare_icons(vid_list);
  progressFlag=2;
  p_timer->start(fProgTime);
  vu->start_make_traces(videos);
  return;
}

void QVBrowser::on_sort_changed() {
  sort_by = sort_combo->currentText().toStdString();
  //fFBox->invalidate_sort();
  return;
}

void QVBrowser::asc_clicked() {
  sort_desc=! sort_desc;
  QString iname = "view-sort-ascending";
  if (sort_desc) iname = "view-sort-descending";
  asc_button->setIcon(QIcon::fromTheme(iname));
  //fFBox->invalidate_sort();
  return;
}

void QVBrowser::update_progress(int fraction, std::string label) {
  //progress_bar->set_text(label);
  progress_bar->setValue(fraction);
  return;
}



  
