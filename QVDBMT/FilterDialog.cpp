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
#include <QListView>
#include <QStandardItemModel>
#include <QFormLayout>
#include <QRegularExpression>
#include <QScrollArea>

FilterDialog::FilterDialog(QWidget * parent, QListView * listView, qvdb_metadata * md)  : fMD(md),fListView(listView) { 
  QGroupBox * filterBox = new QGroupBox;
  QVBoxLayout * mainLayout = new QVBoxLayout;
  QFormLayout * formLayout = new QFormLayout;
  QVBoxLayout * scrollLayout = new QVBoxLayout;
  QWidget *viewport = new QWidget;
  viewport->setMinimumSize(20,20);
  viewport->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  QScrollArea *scrollArea = new QScrollArea;
  std::vector<std::string> labels = { "VIDs:","Minimum Length (s):", "Maximum Length (s):", "Minimum Size (MB):", "Maximum Size(MB):", "Filename Regex:"};
  for(auto & l: labels) {
    QLineEdit * tempEntry = new QLineEdit;
    formLayout->addRow(l.c_str(),tempEntry);
    linePtrs.push_back(tempEntry);
  }
  filterBox->setLayout(formLayout);
  scrollLayout->addWidget(filterBox);
  auto mdLookup = fMD->md_lookup();
  int columns = 3;
  for(auto & a: fMD->md_types().left) {
    std::string tempS(a.second);
    QGroupBox *  tempGroup = new QGroupBox(tempS.c_str());
    QGridLayout * grid = new QGridLayout();
    int index = 0;
    for(auto & x : mdLookup) {  
      if(a.first == x.second.first) {
	md_lookup.push_back(x.first);
	QCheckBox * tagCheck = new QCheckBox(x.second.second.c_str());
	tagCheck->setTristate();
        grid->addWidget(tagCheck,index/columns,index%columns);
	checkPtrs.push_back(tagCheck);
	index++;
      }
    }
    tempGroup->setLayout(grid);
    scrollLayout->addWidget(tempGroup);
  }
  viewport->setLayout(scrollLayout);
  scrollArea->setWidget(viewport);
  QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &FilterDialog::on_accept);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  mainLayout->addWidget(viewport);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);
  setWindowTitle(tr("Edit Filter"));
}

void FilterDialog::on_accept() {
  std::vector<int> vids;
  if(linePtrs[0]->isModified() && linePtrs[0]->text().toStdString().length()>0) {
    char * line = strdup(linePtrs[0]->text().toStdString().c_str());
    char * pch = strtok (line," ,");    
    while (pch != NULL)
      {
	vids.push_back(atoi(pch));
	pch = strtok (NULL, " ,");
      }
  }
  float lengthMin = -1;
  if(linePtrs[1]->isModified()) lengthMin = linePtrs[1]->text().toFloat();
  float lengthMax = -1;
  if(linePtrs[2]->isModified()) lengthMax = linePtrs[2]->text().toFloat();
  float sizeMin = -1;
  if(linePtrs[3]->isModified()) sizeMin = linePtrs[3]->text().toFloat();
  float sizeMax = -1;
  if(linePtrs[4]->isModified()) sizeMax = linePtrs[4]->text().toFloat();
  std::string regex("");
  if(linePtrs[5]->isModified()) regex = linePtrs[5]->text().toStdString();
  QRegularExpression re(regex.c_str());
  std::set<int> acceptTags;
  std::set<int> rejectTags;
  for(uint i = 0; i < checkPtrs.size(); i++) {
    auto state = checkPtrs[i]->checkState();
    if(state == Qt::Unchecked) continue;
    else if(state == Qt::Checked) acceptTags.insert(md_lookup[i]);
    else rejectTags.insert(md_lookup[i]);		 
  }
  auto fModel = fListView->model();
  auto root_index = fListView->rootIndex();
  auto rows = fModel->rowCount(root_index);
  for(int i = 0; i < rows; i++) {
    fListView->setRowHidden(i,false);
    auto index = fModel->index(i,0,root_index);
    int vid = fModel->data(index, Qt::UserRole+4).toInt();
    if(vids.size() > 0 && std::find(vids.begin(), vids.end(), vid) == vids.end()) {
      fListView->setRowHidden(i,true);
      continue;
    }
    if(lengthMin != -1 && fModel->data(index, Qt::UserRole+2).toFloat() < lengthMin) {
      fListView->setRowHidden(i,true);
      continue;
    }
    if(lengthMax != -1 && fModel->data(index, Qt::UserRole+2).toFloat() > lengthMax) {
      fListView->setRowHidden(i,true);
      continue;
    }
    float mbConv=1.0e-6;
    if(sizeMin != -1 && mbConv*fModel->data(index, Qt::UserRole+1).toFloat() < sizeMin) {
      fListView->setRowHidden(i,true);
      continue;
    }
    if(sizeMax != -1 && mbConv*fModel->data(index, Qt::UserRole+1).toFloat() > sizeMax) {
      fListView->setRowHidden(i,true);
      continue;
    }
    if(regex.length() > 0)  {
      QRegularExpressionMatch match = re.match(fModel->data(index, Qt::UserRole+3).toString());
      if(!match.hasMatch()) {
	fListView->setRowHidden(i,true);
	continue;
      }
    }
    std::set<int> vidMD = fMD->mdForFile(vid);
    if(acceptTags.size() > 0) {
      bool hasAll = true;
      for(auto & element: acceptTags) {
	if(vidMD.count(element) == 0) {  //c++20 contains is better
	  hasAll = false;
	  break;
	}
      }
      if(!hasAll) {
	fListView->setRowHidden(i,true);
	continue;
      }
    }
    if(rejectTags.size() > 0) {
      bool hasAny = false;
      for(auto & element: rejectTags) {
	if(vidMD.count(element) > 0) { //c++20 contains is better
	  hasAny = true;
	  break;
	}
      }
      if(hasAny) {
	fListView->setRowHidden(i,true);
	continue;
      }
    }
  }
  return;
}

