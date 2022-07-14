#include "QVBMetadata.h"
#include <QDialog>
#include <QWidget>
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

class MetadataDialog: public QDialog {

  using QDialog::QDialog;

public:
  
  MetadataDialog(QWidget * parent,std::vector<int> & vids, qvdb_metadata * md)  : fMD(md),fVids(vids) {
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
  lList->setSortingEnabled(true);
  lList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  flList = new QListWidget(this);
  flList->setSortingEnabled(true);
  flList->setSelectionMode(QAbstractItemView::ExtendedSelection);
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
  };

void on_accept() {
  fMD->saveMetadata();
  return;
};

void updateTypes() {
  if(!firstRun) type_combo->clear();
  for(auto &a: fMD->md_types().left) type_combo->addItem(a.second.c_str());
  return;
}

void updateLabels() {
  if(!firstRun) {
    lList->clear();
    flList->clear();
  }
  if(type_combo->currentText().isEmpty()) return;
  std::set<int> mdIDs1 = fMD->mdForFile(fVids[0]); //Get IDs for first file
  std::set<int> mdIsx;  //List of common mdIds
  if(mdIDs1.size() > 0) 
    for(uint i = 1; i < fVids.size(); i++){ 
      std::set<int> mdIDs2 = fMD->mdForFile(fVids[i]);
      for(auto & a:mdIDs1) if(mdIDs2.count(a)==1) mdIsx.insert(a);  
    mdIDs1 = mdIsx;
  }
  std::set<std::string> lSet;
  std::set<std::string> flSet;
  for(auto &b: fMD->md_lookup()) { 
    int tID = fMD->md_types().right.at(type_combo->currentText().toStdString());
    if(b.second.first == tID) {
      if(mdIDs1.count(b.first)==1) flSet.insert(b.second.second);    
      else lSet.insert(b.second.second);
    }
  }
  for(auto & c: lSet) lList->addItem(c.c_str());
  for(auto & d: flSet) flList->addItem(d.c_str());
  return;
};

void onTypeAddClicked() {
  bool ok;
  std::string text = QInputDialog::getText(this, "New Type Entry", "New Metadata Type:",QLineEdit::Normal, "",&ok).toStdString();
  if (ok && !text.empty())  {
    fMD->newType(text);
    updateTypes();
  } 
  return;
};

void onLabelAddClicked() {
  bool ok;
  std::string text = QInputDialog::getText(this, "New Label Entry", "New Metadata Label:",QLineEdit::Normal, "",&ok).toStdString();
  if (ok && !text.empty())  {
    fMD->newLabel(type_combo->currentText().toStdString(),text);
    updateLabels();
    lList->findItems(QString(text.c_str()),Qt::MatchCaseSensitive)[0]->setSelected(true);
    onRightArrowClicked();
  }
  return;
};

void onRightArrowClicked() {
  auto list = lList->selectedItems();
  for(uint i = 0; i < fVids.size(); i++) for(auto & item: list)
    fMD->attachToFile(fVids[i],item->text().toStdString());
  updateLabels();
  return;
};

void onLeftArrowClicked() {
  auto list = flList->selectedItems();
  for(uint i = 0; i < fVids.size(); i++) for(auto & item: list) fMD->removeFromFile(fVids[i],item->text().toStdString());
  updateLabels();
  return;
};

private:

  bool firstRun;
  qvdb_metadata *  fMD;
  QComboBox * type_combo;
  QListWidget * lList;  //Available labels
  QListWidget * flList;  //Applied labels
  std::vector<int> fVids;
};
