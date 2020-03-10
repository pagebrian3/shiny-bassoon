#include "QVBMetadata.h"
#include <QDialog>
#include <QWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QInputDialog>
#include <QLabel>

class NameDialog: public QDialog {

  using QDialog::QDialog;

public:
  
  NameDialog(QWidget * parent, qvdb_metadata * md)  : fMD(md) {
    firstRun = true;
    QGroupBox * flob = new QGroupBox;
    QVBoxLayout *  hlo = new QVBoxLayout;
    flob->setLayout(hlo);
    fList = new QListWidget(this);
    QPushButton * addName = new QPushButton("New Name");
    QLabel * h1 = new QLabel("Available Names");
    hlo->addWidget(addName);
    hlo->addWidget(h1);
    hlo->addWidget(fList);
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
    if(fList->selectedItems().size() > 0) selected_name.assign(fList->selectedItems()[0]->text().toStdString());
    return;
  };

  void updateLabels() {
    if(!firstRun) fList->clear();   
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
      fMD->newLabel(fMD->typeLabel(1),text);
      updateLabels();
    }
    return;
  };

  std::string get_name() {
    return selected_name;
  };


private:

  bool firstRun;
  qvdb_metadata *  fMD;
  QListWidget * fList;
  std::string selected_name;
};
