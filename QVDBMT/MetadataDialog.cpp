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

MetadataDialog::MetadataDialog(QMainWindow * parent,std::vector<int> & vids, qvdb_metadata * md)  {
  fMD = md;
  fVids = vids;
  firstRun = true;
  type_combo = new QComboBox();
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
    std::cout <<"type: " ;
    std::cout << a.second.c_str() << std::endl;
    type_combo->addItem(a.second.c_str());
  }
  return;
}

void MetadataDialog::updateLabels() {
  if(!firstRun) {
    lList->clear();
    flList->clear();
  }
  std::vector<int>mdIDs = fMD->mdForFile(fVids[0]);   //right now just one file
  std::cout << fMD->md_lookup().size() <<" "<<type_combo->currentText() .toStdString()<< std::endl;
  for(int i = 0; i < mdIDs.size(); i++) std::cout << mdIDs[i] <<std::endl;
  for(auto &b: fMD->md_lookup()) { //loop over all metadata
    int tID = fMD->md_types().right.at(type_combo->currentText().toStdString());
    if(b.second.first == tID) {
      std::cout << "HERE " << b.first << " " << b.second .second<< std::endl;
      auto p = std::find(mdIDs.begin(),mdIDs.end(),b.first);
      if(p != mdIDs.end()) flList->addItem(b.second.second.c_str());    
      else {
	std::cout << "adding item:  ";
	std::cout <<b.second.second.c_str() << std::endl;
	lList->addItem(b.second.second.c_str());
      }
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
  for(auto & item: list)
    fMD->attachToFile(fVids[0],item->text().toStdString());
  updateLabels();
  return;
}

void MetadataDialog::onLeftArrowClicked() {
  auto list = flList->selectedItems();
  for(auto & item: list) fMD->removeFromFile(fVids[0],item->text().toStdString());
  updateLabels();
  return;
}
