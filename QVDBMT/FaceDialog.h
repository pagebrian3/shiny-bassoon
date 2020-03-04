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

class FaceDialog: public QDialog {

  using QDialog::QDialog;

public:

  FaceDialog(QWidget * parent, video_utils * vu, std::vector<VidFile *> & vidFiles) : fVU(vu),fVidFiles(vidFiles) {
    fMD=fVU->mdInterface();
    QVBoxLayout * mainLO = new QVBoxLayout;
    fList = new QListView(this);
    fList->setViewMode(QListView::IconMode);
    fList->setMovement(QListView::Static);
    fList->setSelectionMode(QListView::ExtendedSelection);
    mainLO->addWidget(fList);
    setLayout(mainLO);
    fFT = new FaceTools(fVU);
    fTimer = new QTimer(this);
    connect(fTimer, &QTimer::timeout, this, &FaceDialog::progress_timeout);
    start_face_finder();
    fFacePath = fFT->get_face_path();
    fModel = new QStandardItemModel(0,1,fList);
    fList->setModel(fModel);
    QItemSelectionModel * selModel = fList->selectionModel();
    connect(selModel,&QItemSelectionModel::selectionChanged,this,&FaceDialog::onSelChanged);
    fList->show();
  };

  void onSelChanged() {
    QModelIndexList sList = fList->selectionModel()->selectedIndexes();
    QStandardItem *  selItem; 
    for(auto & item: fFaceVec )  item->setText("");
    for(int i = 0; i < sList.size(); i++)  {
      selItem = fModel->itemFromIndex(sList[i]);
      selItem->setText("SELECTED");
    }
    update();
    return;
  };
  
  void progress_timeout() {
    //update list
    //when finished fTimer->stop();
    fFT->Find_Faces();
    for(auto& p: std::filesystem::directory_iterator(fFacePath))
      if(p.is_regular_file()) {
	std::stringstream ss(p.path().stem());
	std::string item1,item2,item3;
	std::getline(ss,item1,'_');
	int vid = atoi(item1.c_str());
	std::getline(ss,item2);
	int ts = atoi(item2.c_str());
	std::getline(ss,item3);
	int faceNum = atoi(item3.c_str());
	if(fLoadedFaces[std::make_tuple(vid,ts,faceNum)]==true) continue;
	QStandardItem * b = new QStandardItem();
	fFaceVec.push_back(b);
	QPixmap img(QString(p.path().c_str()));
	b->setSizeHint(img.size());
	b->setBackground(QBrush(img));
	b->setIcon(QIcon());
	b->setData(vid,Qt::UserRole+1);
	fLoadedFaces[std::make_tuple(vid,ts,faceNum)]=true;
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
    for(int i = 0; i < sList.size(); i++)  {
      vid = fModel->itemFromIndex(sList[i])->data(Qt::UserRole+1).toInt();
      fVids.insert(vid);
      names.merge(fMD->mdForFile(vid));
      for(auto & a: names) 
	if(fMD->mdType(a) == 1) 
	  suggested_names.insert(fMD->labelForMDID(a));	      
    }
    suggested_names.insert("other");
    for(auto & entry: suggested_names) menu->addAction(entry.c_str());      
    connect(menu, &QMenu::triggered, this,&FaceDialog::select_name);
    menu->popup(event->globalPos());
    return;
  };
#endif // QT_NO_CONTEXTMENU

private:

  void select_name(QAction * act)
  {
    std::string choice(act->text().toStdString());
    if(strcmp(choice.c_str(),"other") == 0) {
      NameDialog * nameDiag = new NameDialog(this,fMD);
      nameDiag->open();
      choice = nameDiag->get_name();
    }
    if(strcmp(choice.c_str(),"")) for(auto & vID : fVids) fMD->attachToFile(vID,choice);
    fMD->saveMetadata();
    QModelIndexList sList = fList->selectionModel()->selectedIndexes();
    for(int i = 0; i < sList.size(); i++) fModel->itemFromIndex(sList[i])->setData(choice.c_str(),Qt::UserRole+2);
    //These elements should then be used to train the NN and removed from the GUI.
    return;
  };
  
  qvdb_metadata * fMD;
  video_utils * fVU;
  FaceTools * fFT;
  QListView * fList;
  QTimer * fTimer;
  QStandardItemModel * fModel;
  std::filesystem::path fFacePath;
  std::set<int> fVids;
  std::vector<VidFile*> fVidFiles;
  std::vector<QStandardItem *> fFaceVec;
  std::map<std::tuple<unsigned int, unsigned int, unsigned int>,bool> fLoadedFaces; 
};

#endif //FACEDIALOG_H
