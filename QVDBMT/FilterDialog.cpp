#include "FilterDialog.h"
#include "QVBMetadata.h"
#include <QWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QInputDialog>
#include <QGridLayout>
#include <QStandardItem>
#include <QFormLayout>

FilterDialog::FilterDialog(QWidget * parent, std::vector<QStandardItem *> & vids, qvdb_metadata * md)  : fMD(md),fVids(vids) { 
  QGroupBox * filterBox = new QGroupBox;
  QVBoxLayout * mainLayout = new QVBoxLayout;
  QFormLayout * formLayout = new QFormLayout;
  std::vector<std::string> labels = { "Minimum Length:", "Maximum Length:", "Minimum Size:", "Maximum Size:", "Filename Regex:"};
  for(auto & l: labels) {
    QLineEdit * tempEntry = new QLineEdit;
    formLayout->addRow(l.c_str(),tempEntry);
    editPtrs->push_back(tempEntry);
  }
  filterBox->addLayout(formLayout);
  mainLayout->addWidget(filterBox);
  auto mdLookup = fMD->md_lookup();
  int columns = 3;
  for(auto & a: fMD->md_types().left) {
    std::string tempS(a.second);
    QGroupBox *  tempGroup = new QGroupBox(tempS.c_str());
    QGridLayout * grid = new QGridLayout();
    for(auto & x : mdLookup) {
      int index = 0;
      if(a.first == x.second.first) {
	QCheckBox * tagCheck = new QCheckBox(x.second.second.c_str());
	tagCheck->setTristate();
        grid->addItem(tagCheck,index/columns,index%columns);
	checkPtrs->push_back(tagCheck);
	index++;
      }
    }
    tempGroup->addLayout(grid);
    mainLayout->addWidget(tempGroup);
  }
  QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &FilterDialog::on_accept);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);
  setWindowTitle(tr("Edit Filter"));
}

void FilterDialog::on_accept() {
  std::vector<int> vids;
  if(editPtrs[0]->isModified()){
    std::vector<string> strTemp;
    boost::split(editPtrs[0]->text().toString(),strTemp,boost::is_any_of("\t"));
    for(auto &tok: strTemp) vids.push_back(atoi(tok));
  }
  float lengthMin = -1;
  if(editPtrs[1]->isModified()) lengthMin = editPtrs[1]->text().toFloat();
  float lengthMax = -1;
  if(editPtrs[2]->isModified()) lengthMax = editPtrs[2]->text().toFloat();
  float sizeMin = -1;
  if(editPtrs[3]->isModified()) sizeMin = editPtrs[3]->text().toFloat();
  float sizeMax = -1;
  if(editPtrs[4]->isModified()) sizeMax = editPtrs[4]->text().toFloat();
  std::string regex("");
  if(editPtrs[5]->isModified()) regex = editPtrs[5]->text().toString();
  std::vector<std::string> acceptTags;
  std::vector<std::string> rejectTags;
  for(auto & b: checkPtrs) {
    if(b->checkState() == Qt::Checked) acceptTags.push_back(b->text().toString());
    else if(b->checkState() == Qt::PartiallyChecked) rejectTags.push_back(b->text().toString());		 
  }
  for(auto & a: fVids) {
    if(vids.size() > 0 && std::find(vids.begin(), vids.end(), a->data(Qt::UserRole+4).toInt()) == vids.end()) {
      std::cout << "Make vid " << a->data(Qt::UserRole+4).toInt()) << " invisible." << std::endl;
    }
  //if(lengthMin > 0 && a->data(Qt::UserRole
  }
  return;
}

