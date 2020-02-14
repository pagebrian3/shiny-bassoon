#ifndef FACEDIALOG_H
#define FACEDIALOG_H

#include "VidFile.h"
#include "VideoUtils.h"
#include "FaceTools.h"
#include <QDialog>
#include <QListView>
#include <QTimer>

class FaceDialog: public QDialog {

  using QDialog::QDialog;

public:

  FaceDialog(QWidget * parent, video_utils * vu, std::vector<VidFile *> & vidFiles) : fVU(vu),fVidFiles(vidFiles) {  
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
    fList->show();
  };
  
  void progress_timeout() {
    //update list
    //when finished fTimer->stop();
    fFT->Find_Faces();
  };

  void start_face_finder() {
    //start job for making faces
    fVU->StartFaceFrames(fVidFiles);
    fFT->Find_Faces();
    fTimer->start(fVU->get_config()->get_int("progress_time"));
    fList->show();
  };

private:
  
  qvdb_metadata * fMD;
  video_utils * fVU;
  FaceTools * fFT;
  QListView * fList;
  QTimer * fTimer;
  std::vector<VidFile*> fVidFiles;
  
};

#endif //FACEDIALOG_H
