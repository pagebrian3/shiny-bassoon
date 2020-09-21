#ifndef FACEDIALOG_H
#define FACEDIALOG_H

#include "NameDialog.h"
#include "VidFile.h"
#include "VideoUtils.h"
#include "FaceTools.h"
#include <QDialog>
#include <QListView>
#include <QTimer>
#include <QStandardItemModel>
#include <QMenu>
#include <boost/format.hpp>

pthread_mutex_t fLock;

class FaceDialog: public QDialog {

  using QDialog::QDialog;

public:

  FaceDialog(QWidget * parent, video_utils * vu, std::vector<VidFile *> & vidFiles) : fVU(vu),fVidFiles(vidFiles) {
    fMD=fVU->mdInterface();
    QVBoxLayout * mainLO = new QVBoxLayout;
    fList = new QListView(this);
    fList->setResizeMode(QListView::Adjust);
    fList->setViewMode(QListView::IconMode);
    fList->setMovement(QListView::Static);
    fList->setSelectionMode(QListView::ExtendedSelection);
    fFT = new FaceTools(fVU);
    fTimer = new QTimer(this);
    connect(fTimer, &QTimer::timeout, this, &FaceDialog::progress_timeout);
    start_face_finder();
    fFacePath = fFT->get_face_path();
    fMatchPath=fFacePath;
    fMatchPath+="match";
    fTrainPath = fVU->save_path();
    fTrainPath+="train";
    std::filesystem::create_directory(fTrainPath);
    std::filesystem::create_directory(fMatchPath);
    fModel = new QStandardItemModel(0,1,fList);
    fList->setModel(fModel);
    QItemSelectionModel * selModel = fList->selectionModel();
    QPushButton * retrainButton = new QPushButton("Retrain",this);
    connect(retrainButton, &QPushButton::clicked, this, &FaceDialog::retrain);
    mainLO->addWidget(fList);
    mainLO->addWidget(retrainButton);
    setLayout(mainLO);
    connect(selModel,&QItemSelectionModel::selectionChanged,this,&FaceDialog::onSelChanged);
    fList->show();
  };
  
  void retrain() {
    fFT->retrain();
    //Need to refresh icons here
    return;
  }
  
  void onSelChanged() {
    QModelIndexList sList = fList->selectionModel()->selectedIndexes();
    QStandardItem *  selItem; 
    for(auto & item: fFaceLookup) if(item.second->text().length() > 0) {
	pthread_mutex_lock(&fLock);
	item.second->setText("");
	pthread_mutex_lock(&fLock);
      }
    for(int i = 0; i < sList.size(); i++)  {
      selItem = fModel->itemFromIndex(sList[i]);
      pthread_mutex_lock(&fLock);
      selItem->setText("SELECTED");
      pthread_mutex_unlock(&fLock);
    }
    update();
    return;
  }
  
  void progress_timeout() {
    //update list
    //when finished fTimer->stop();
    fFT->Find_Faces();
    int batch_size = fVU->get_config()->get_int("face_batch");
    //Need
    for(auto& p: std::filesystem::directory_iterator(fMatchPath))
      if(p.is_regular_file()) {
	std::stringstream ss(p.path().stem());
	std::string item1,item2,item3,item4,item5;
	std::getline(ss,item1,'_');
	std::string suggested_name(item1.c_str());
	std::getline(ss,item2,'_');
	float confidence = atof(item2.c_str());
	std::getline(ss,item3,'_');
	int vid = atoi(item3.c_str());
	std::getline(ss,item4,'_');
	int ts = atoi(item4.c_str());
	std::getline(ss,item5);
	int faceNum = atoi(item5.c_str());
	if(fLoadedFaces[std::make_tuple(vid,ts,faceNum)]==2) continue;
	else if(fLoadedFaces[std::make_tuple(vid,ts,faceNum)]==1){
	  QStandardItem * b = fFaceLookup[std::make_tuple(vid,ts,faceNum)];
	  b->setData(p.path().c_str(),Qt::UserRole+2);
	  b->setData(suggested_name.c_str(),Qt::UserRole+3);
	  b->setData(confidence,Qt::UserRole+4);
	  fLoadedFaces[std::make_tuple(vid,ts,faceNum)]=2;
	}
	else {
	  if(fModel->rowCount() >= batch_size) continue;
	  QStandardItem * b = new QStandardItem();	  
	  QPixmap img(QString(p.path().c_str()));
	  b->setSizeHint(img.size());
	  b->setBackground(QBrush(img));
	  b->setIcon(QIcon());
	  b->setData(vid,Qt::UserRole+1);
	  b->setData(p.path().c_str(),Qt::UserRole+2);
	  b->setData(suggested_name.c_str(),Qt::UserRole+3);
	  b->setData(confidence,Qt::UserRole+4);
	  float time = 0.001*ts;
	  QString toolTip((boost::format("VID: %i\nTime: %f\nFaceNum: %i") % vid % time % faceNum).str().c_str());
	  b->setToolTip(toolTip);
	  std::cout << "Adding identified face." << std::endl;
	  fLoadedFaces[std::make_tuple(vid,ts,faceNum)]=2;
	  fFaceLookup[std::make_tuple(vid,ts,faceNum)]=b;
	  fModel->appendRow(b);
	}
      }
    for(auto& p: std::filesystem::directory_iterator(fFacePath))
      if(p.is_regular_file()) {
	if(fModel->rowCount() >= batch_size) continue;
	std::stringstream ss(p.path().stem());
	std::string item1,item2,item3;
	std::getline(ss,item1,'_');
	int vid = atoi(item1.c_str());
	std::getline(ss,item2);
	int ts = atoi(item2.c_str());
	std::getline(ss,item3);
	int faceNum = atoi(item3.c_str());
	if(fLoadedFaces[std::make_tuple(vid,ts,faceNum)]==1) continue;
	QStandardItem * b = new QStandardItem();
	QPixmap img(QString(p.path().c_str()));
	b->setSizeHint(img.size());
	b->setBackground(QBrush(img));
	b->setIcon(QIcon());
	b->setData(vid,Qt::UserRole+1);
	b->setData(p.path().c_str(),Qt::UserRole+2);
	float time = 0.001*ts;
	QString toolTip((boost::format("VID: %i\nTime: %f\nFaceNum: %i") % vid % time % faceNum).str().c_str());
	b->setToolTip(toolTip);
	fLoadedFaces[std::make_tuple(vid,ts,faceNum)]=1;
	fFaceLookup[std::make_tuple(vid,ts,faceNum)]=b;
	fModel->appendRow(b);
      }
    return;
  };

  void start_face_finder() {
    //start job for making faces
    fVU->StartFaceFrames(fVidFiles);
    fFT->Find_Faces();
    fTimer->start(fVU->get_config()->get_int("progress_time"));
    fList->show();
    return;
  };

#ifndef QT_NO_CONTEXTMENU
  void contextMenuEvent(QContextMenuEvent *event)
  {
    QMenu * menu = new QMenu(this);
    QModelIndexList sList = fList->selectionModel()->selectedIndexes();
    std::set<int> names;
    int vid;
    std::set<std::string> suggested_names;
    std::string typeLabel("Performer");
    QVariant value,prevValue;
    int size = sList.size();
    int count=1;
    for(int i = 0; i < sList.size(); i++) {
      value = fModel->itemFromIndex(sList[i])->data(Qt::UserRole+3);
      if(prevValue == value) count++;
      prevValue = value;
    }
    if(count == size && value.typeName() != 0) {
      menu->addSection("NN Suggestion:");
      menu->addAction(value.toString());
    }
    menu->addSection("MD Suggestions:");
    if(fMD->typeExists(typeLabel)) performerIndex = fMD->mdType(typeLabel);
    else performerIndex = fMD->newType(typeLabel);
    for(int i = 0; i < sList.size(); i++)  {
      vid = fModel->itemFromIndex(sList[i])->data(Qt::UserRole+1).toInt();
      fVids.insert(vid);
      names.merge(fMD->mdForFile(vid));
      for(auto & a: names) 
	if(fMD->mdType(a) == performerIndex) 
	  suggested_names.insert(fMD->labelForMDID(a));	      
    }
    for(auto & entry: suggested_names) menu->addAction(entry.c_str()); 
    menu->addSeparator();
    menu->addAction("other");         
    connect(menu, &QMenu::triggered, this,&FaceDialog::select_name);
    menu->popup(event->globalPos());
    return;
  };
#endif // QT_NO_CONTEXTMENU

private:

  void select_name(QAction * act) {
    std::string choice(act->text().toStdString());
    if(strcmp(choice.c_str(),"other") == 0) {
      NameDialog nameDiag(this,fMD);
      if(nameDiag.exec()) choice = nameDiag.get_name();
      else choice.clear();
    }
    if(strcmp(choice.c_str(),"")) {
      for(auto & vID : fVids) fMD->attachToFile(vID,choice);
      fMD->saveMetadata();
      QModelIndexList sList = fList->selectionModel()->selectedIndexes();
      for(int i = 0; i < sList.size(); i++) {
	auto item = fModel->itemFromIndex(sList[i]);
	std::filesystem::path p = item->data(Qt::UserRole+2).toString().toStdString();
	std::filesystem::path new_p = fTrainPath;
	std::stringstream ss(p.stem());
	std::string item1,item2,item3;
	for(int i = 0; i < 2; i++) std::getline(ss,item1,'_');
	int vid = atoi(item1.c_str());
	std::getline(ss,item2);
	int ts = atoi(item2.c_str());
	std::getline(ss,item3);
	int faceNum = atoi(item3.c_str());
	auto index_tup = std::make_tuple(vid,ts,faceNum);
	std::size_t pos = p.filename().string().rfind(choice,0);
	new_p+="/";	
	if(pos == std::string::npos) {  //if this is a guessed name, don't add name a 2nd time
	  new_p+=choice;
	  new_p+="_";
	}	
	new_p+=p.filename();	
	std::filesystem::rename(p,new_p);
	fModel->removeRows(item->row(),1);
	fFaceLookup.erase(index_tup);
	fLoadedFaces.erase(index_tup);
      }
    }
    return;
  }; 
  int performerIndex;
  int index;
  qvdb_metadata * fMD;
  video_utils * fVU;
  FaceTools * fFT;
  QListView * fList;
  QTimer * fTimer;
  QStandardItemModel * fModel;
  std::filesystem::path fFacePath,fTrainPath,fMatchPath;
  std::set<int> fVids;
  std::vector<VidFile*> fVidFiles;
  std::map<std::tuple<unsigned int, unsigned int, unsigned int>,int> fLoadedFaces; //1 mean just loaded, 2 means loaded and unmatched
  std::map<std::tuple<unsigned int, unsigned int, unsigned int>,QStandardItem *> fFaceLookup;
};

#endif //FACEDIALOG_H
