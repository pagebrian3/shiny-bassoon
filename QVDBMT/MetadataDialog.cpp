#include "MetadataDialog.h"
#include "QVBMetadata.h"
#include <QMainWindow>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>

MetadataDialog::MetadataDialog(QMainWindow * parent,std::vector<int> & vids, qvdb_metadata * md)  {
  fMD = md;
  QComboBox * type_combo = new QComboBox();
  for(auto &a: fMD->md_types().left) {
    type_combo->addItem(a.second.c_str());
  }
  type_combo->setCurrentIndex(0);
  QGroupBox * flob = new QGroupBox;
  QHBoxLayout *  hlo = new QHBoxLayout;
  flob->setLayout(hlo);
  QListWidget * lList = new QListWidget();
  int row = 1;
  for(auto &b: fMD->md_lookup()) {
    if(b.second.first == type_combo->currentIndex()) {
      lList->insertItem(row,b.second.second.c_str());
      row++;
    }
  }
  hlo->addWidget(lList);
  QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &MetadataDialog::on_accept);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(type_combo);
    mainLayout->addWidget(flob);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    setWindowTitle(tr("Edit Metadata")); 
}

void MetadataDialog::on_accept() {
  int i = 0;
  return;
}
