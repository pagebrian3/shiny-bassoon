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

class NameDialog: public QDialog {

  using QDialog::QDialog;

public:
  
  NameDialog(QWidget * parent, qvdb_metadata * md)  : fMD(md) {
  firstRun = true;
  QGroupBox * flob = new QGroupBox;
  QGridLayout *  hlo = new QGridLayout;
  flob->setLayout(hlo);
  fList = new QListWidget(this);
  //fList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  QPushButton * addName = new QPushButton("+");
  addName->setMaximumWidth(20);
  QLabel * h1 = new QLabel("Available Names");
  hlo->setColumnMinimumWidth(1,25);
  hlo->setColumnMinimumWidth(3,25);
  hlo->setColumnStretch(0,1);
  hlo->setColumnStretch(4,1);
  hlo->setRowStretch(1,1);
  hlo->setRowStretch(7,1);
  hlo->addWidget(h1,0,0);
  hlo->addWidget(fList,1,0,7,1);
  hlo->addWidget(addName,4,2,1,1);
  QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  updateLabels();
  connect(buttonBox, &QDialogButtonBox::accepted, this, &NameDialog::on_accept);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(addName, &QPushButton::clicked,this,&NameDialog::onNameAddClicked);
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(flob);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);
  setWindowTitle(tr("Select Name for Face"));
  firstRun=false;
  };

void on_accept() {
  fMD->saveMetadata();
  return;
};

void updateLabels() {
  if(!firstRun) {
    fList->clear();
  }
  std::set<std::string> lSet;
  for(auto &b: fMD->md_lookup()) { 
    if(b.second.first == 1) {   //need to fix the Person/Performer type ID.
      lSet.insert(b.second.second);
    }
  }
  for(auto & c: lSet) fList->addItem(c.c_str());
  return;
};

void onNameAddClicked() {
  bool ok;
  std::string text = QInputDialog::getText(this, "New Name Entry", "New Name:",QLineEdit::Normal, "",&ok).toStdString();
  if (ok && !text.empty())  {
    updateLabels();
  }
  return;
};


private:

  bool firstRun;
  qvdb_metadata *  fMD;
  QListWidget * fList;
};
