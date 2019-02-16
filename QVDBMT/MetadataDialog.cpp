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
  std::cout << vids[0] << std::endl;
  type_combo = new QComboBox();
  connect(type_combo, &QComboBox::currentTextChanged,this,&MetadataDialog::updateLabels);
  updateTypes();
  type_combo->setCurrentIndex(0);
  QPushButton * addType = new QPushButton("+");
  connect(addType, &QPushButton::clicked,this,&MetadataDialog::onTypeAddClicked);
  addType->setMaximumWidth(20);
  QGroupBox * typeBox = new QGroupBox;
  QHBoxLayout * typelo = new QHBoxLayout;
  typelo->addWidget(type_combo);
  typelo->addWidget(addType);
  typeBox->setLayout(typelo);
  QGroupBox * flob = new QGroupBox;
  QGridLayout *  hlo = new QGridLayout;
  flob->setLayout(hlo);
  lList = new QListWidget();
  flList = new QListWidget();
  updateLabels();
  QPushButton * rightArrow = new QPushButton(">");
  QPushButton * addLabel = new QPushButton("+");
  QPushButton * leftArrow = new QPushButton("<");
  connect(addLabel, &QPushButton::clicked,this,&MetadataDialog::onLabelAddClicked);
  connect(rightArrow, &QPushButton::clicked,this,&MetadataDialog::onRightArrowClicked);
  connect(leftArrow, &QPushButton::clicked,this,&MetadataDialog::onLeftArrowClicked);
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
  QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
						      | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &MetadataDialog::on_accept);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(typeBox);
  mainLayout->addWidget(flob);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);
  setWindowTitle(tr("Edit Metadata")); 
}

void MetadataDialog::on_accept() {
  fMD->saveMetadata();
  return;
}

void MetadataDialog::updateTypes() {
  type_combo->clear();
  for(auto &a: fMD->md_types().left) type_combo->addItem(a.second.c_str());
  return;
}

void MetadataDialog::updateLabels() {
  lList->clear();
  flList->clear();
  int lrow = 1;
  int flrow = 1;
  std::vector<int>mdIDs = fMD->mdForFile(fVids[0]);   //right now just one file
  for(auto &b: fMD->md_lookup()) {
    if(b.second.first == type_combo->currentIndex()) {
      auto p = std::find(mdIDs.begin(),mdIDs.end(),b.first);
      if( p != mdIDs.end() ) {
        flList->insertItem(flrow,b.second.second.c_str());
	flrow++;
      }
      else {
	lList->insertItem(lrow,b.second.second.c_str());
	lrow++;
      }
    }
  }
  return;
}

void MetadataDialog::onTypeAddClicked() {
  bool ok;
  std::string text = QInputDialog::getText(this, "New Type Entry",
					   "New Metadata Type:",QLineEdit::Normal, "",&ok).toStdString();
  if (ok && !text.empty())  {
    fMD->newType(text);
    updateTypes();
  }
  return;
}

void MetadataDialog::onLabelAddClicked() {
  bool ok;
  std::string text = QInputDialog::getText(this, "New Label Entry",
					   "New Metadata Label:",QLineEdit::Normal, "",&ok).toStdString();
  if (ok && !text.empty())  {
    fMD->newLabel(type_combo->currentIndex(),text);
    updateLabels();
  }
  return;
}

void MetadataDialog::onRightArrowClicked() {
  auto list = lList->selectedItems();
  for(auto & item: list)
    fMD->attachToFile(fVids[0],item->text().toStdString());
  return;
  updateLabels();
}

void MetadataDialog::onLeftArrowClicked() {
  auto list = flList->selectedItems();
  for(auto & item: list)
    fMD->removeFromFile(fVids[0],item->text().toStdString());
  return;
  updateLabels();
}
