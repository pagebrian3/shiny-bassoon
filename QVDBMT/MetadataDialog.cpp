#include "MetadataDialog.h"
#include "QVBMetadata.h"
#include <QMainWindow>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QInputDialog>
#include <QGridLayout>
#include <QLabel>

MetadataDialog::MetadataDialog(QMainWindow * parent,std::vector<int> & vids, qvdb_metadata * md)  : fMD(md),fVids(vids) {
  firstRun = true;
  type_combo = new QComboBox();
  type_combo->setInsertPolicy(QComboBox::InsertAtCurrent);
  QPushButton * addType = new QPushButton("+");
  addType->setMaximumWidth(20);
  QGroupBox * typeBox = new QGroupBox;
  QHBoxLayout * typelo = new QHBoxLayout;
  typelo->addWidget(type_combo);
  typelo->addWidget(addType);
  typeBox->setLayout(typelo);
  QGroupBox * flob = new QGroupBox;
  QGridLayout *  hlo = new QGridLayout;
  flob->setLayout(hlo);
  lList = new QListWidget(this);
  flList = new QListWidget(this);
  QPushButton * rightArrow = new QPushButton(">");
  QPushButton * addLabel = new QPushButton("+");
  QPushButton * leftArrow = new QPushButton("<");
  addLabel->setMaximumWidth(20);
  rightArrow->setMaximumWidth(20);
  leftArrow->setMaximumWidth(20);
  QLabel * h1 = new QLabel("Available Tags");
  QLabel * h2 = new QLabel("Applied Tags");
  hlo->setColumnMinimumWidth(1,25);
  hlo->setColumnMinimumWidth(3,25);
  hlo->setColumnStretch(0,1);
  hlo->setColumnStretch(4,1);
  hlo->setRowStretch(1,1);
  hlo->setRowStretch(7,1);
  hlo->addWidget(h1,0,0);
  hlo->addWidget(h2,0,4);
  hlo->addWidget(lList,1,0,7,1);
  hlo->addWidget(addLabel,4,2,1,1);
  hlo->addWidget(rightArrow,2,2,1,1);
  hlo->addWidget(leftArrow,6,2,1,1);
  hlo->addWidget(flList,1,4,7,1);
  QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  updateTypes();
  updateLabels();
  connect(buttonBox, &QDialogButtonBox::accepted, this, &MetadataDialog::on_accept);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(type_combo, &QComboBox::currentTextChanged,this,&MetadataDialog::updateLabels);
  connect(addType,&QPushButton::clicked,this,&MetadataDialog::onTypeAddClicked);
  connect(addLabel, &QPushButton::clicked,this,&MetadataDialog::onLabelAddClicked);
  connect(rightArrow, &QPushButton::clicked,this,&MetadataDialog::onRightArrowClicked);
  connect(leftArrow, &QPushButton::clicked,this,&MetadataDialog::onLeftArrowClicked);
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(typeBox);
  mainLayout->addWidget(flob);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);
  setWindowTitle(tr("Edit Metadata"));
  firstRun=false;
}

void MetadataDialog::on_accept() {
  fMD->saveMetadata();
  return;
}

void MetadataDialog::updateTypes() {
  if(!firstRun) type_combo->clear();
  for(auto &a: fMD->md_types().left) {
    type_combo->addItem(a.second.c_str());
  }
  return;
}

void MetadataDialog::updateLabels() {
  if(!firstRun) {
    lList->clear();
    flList->clear();
  }
  if(type_combo->currentText().isEmpty()) return;
  std::vector<int>mdIDs1 = fMD->mdForFile(fVids[0]);
  std::vector<int> mdIsx(mdIDs1.size());
  std::sort(mdIDs1.begin(),mdIDs1.end());
  for(int i = 1; i < fVids.size(); i++) {
    std::vector<int> mdIDs2 = fMD->mdForFile(fVids[i]);
    std::sort(mdIDs2.begin(),mdIDs2.end());  
    auto it = std::set_intersection(mdIDs1.begin(), mdIDs1.end(),mdIDs2.begin(),mdIDs2.end(),mdIsx.begin());
    mdIsx.resize(it-mdIsx.begin());
    mdIDs1 = mdIsx;
  }
  //right now just one file
  for(auto &b: fMD->md_lookup()) { 
    int tID = fMD->md_types().right.at(type_combo->currentText().toStdString());
    if(b.second.first == tID) {
      auto p = std::find(mdIDs1.begin(),mdIDs1.end(),b.first);
      if(p != mdIDs1.end()) flList->addItem(b.second.second.c_str());    
      else lList->addItem(b.second.second.c_str());
    }
  }
  return;
}

void MetadataDialog::onTypeAddClicked() {
  bool ok;
  std::string text = QInputDialog::getText(this, "New Type Entry", "New Metadata Type:",QLineEdit::Normal, "",&ok).toStdString();
  if (ok && !text.empty())  {
    fMD->newType(text);
    updateTypes();
  }
  return;
}

void MetadataDialog::onLabelAddClicked() {
  bool ok;
  std::string text = QInputDialog::getText(this, "New Label Entry", "New Metadata Label:",QLineEdit::Normal, "",&ok).toStdString();
  if (ok && !text.empty())  {
    fMD->newLabel(type_combo->currentText().toStdString(),text);
    updateLabels();
  }
  return;
}

void MetadataDialog::onRightArrowClicked() {
  auto list = lList->selectedItems();
  for(int i = 0; i < fVids.size(); i++) for(auto & item: list)
    fMD->attachToFile(fVids[i],item->text().toStdString());
  updateLabels();
  return;
}

void MetadataDialog::onLeftArrowClicked() {
  auto list = flList->selectedItems();
  for(int i = 0; i < fVids.size(); i++) for(auto & item: list) fMD->removeFromFile(fVids[i],item->text().toStdString());
  updateLabels();
  return;
}
