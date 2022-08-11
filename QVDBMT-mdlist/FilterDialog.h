#include "QVBMetadata.h"
#include <QDialog>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QInputDialog>
#include <QListView>
#include <QHeaderView>
#include <QFormLayout>
#include <QRegularExpression>

class FilterDialog: public QDialog {

  using QDialog::QDialog;
  
public:
  
  FilterDialog(QWidget * parent, QListView * listView, qvdb_metadata * md)  : fMD(md),fListView(listView) { 
    QGroupBox * filterBox = new QGroupBox;
    QVBoxLayout * mainLayout = new QVBoxLayout;
    QFormLayout * formLayout = new QFormLayout;
    std::vector<std::string> labels = { "VIDs:","Minimum Length (s):", "Maximum Length (s):", "Minimum Size (MB):", "Maximum Size(MB):", "Filename Regex:"};
    for(auto & l: labels) {
      QLineEdit * tempEntry = new QLineEdit;
      formLayout->addRow(l.c_str(),tempEntry);
      linePtrs.push_back(tempEntry);
    }
    filterBox->setLayout(formLayout);
    mainLayout->addWidget(filterBox);
    auto mdLookup = fMD->md_lookup();
    std::map<int,std::map<int,int> >typeMap=md->typeCountMap();
    for(auto & a: fMD->md_types().left) {  //One table for each label type
      std::string tempS(a.second);
      int numRows = fMD->typeCount(a.first)+1;
      QGroupBox *  tempGroup = new QGroupBox(tempS.c_str());
      QTableWidget * tempTable = new QTableWidget(numRows,2,this);
      tempTable->verticalHeader()->setVisible(false);
      tempTable->setColumnWidth(0,220);
      tempTable->setColumnWidth(1,40);
      tempTable->setSortingEnabled(true);
      tempTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
      QTableWidgetItem * tempItem = new QTableWidgetItem("Unknown");
      tempItem->setData(Qt::UserRole,-1);
      tempItem->setFlags(tempItem->flags() | Qt::ItemIsUserCheckable |Qt::ItemIsUserTristate);
      tempItem->setCheckState(Qt::Unchecked);
      tempTable->setItem(0,0,tempItem);
      QTableWidgetItem * countItem = new QTableWidgetItem(0);
      countItem->setData(Qt::DisplayRole,typeMap[a.first][-1]);
      tempTable->setItem(0,1,countItem);
      int row = 1;
      for(auto & x : mdLookup) {  
	if(a.first == x.second.first) {
	  int count = typeMap[a.first][x.first];
	  QTableWidgetItem * countItem = new QTableWidgetItem(0);
	  countItem->setData(Qt::DisplayRole,count);
	  tempTable->setItem(row,1,countItem);
	  QTableWidgetItem * tempItem = new QTableWidgetItem(x.second.second.c_str());
	  tempItem->setFlags(tempItem->flags() | Qt::ItemIsUserCheckable |Qt::ItemIsUserTristate);
	  tempItem->setCheckState(Qt::Unchecked);
	  tempItem->setData(Qt::UserRole,x.first);
	  tempTable->setItem(row,0,tempItem);
	  row++;
	}
      }
      tempTable->sortByColumn(1,Qt::DescendingOrder);
      lwPtrs.push_back(tempTable);
      QVBoxLayout * tempLayout = new QVBoxLayout;
      tempLayout->addWidget(tempTable);
      tempGroup->setLayout(tempLayout);
      mainLayout->addWidget(tempGroup);
    }
    QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &FilterDialog::on_accept);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    setWindowTitle(tr("Edit Filter"));
  };

  void on_accept() {
    std::vector<int> vids;
    auto mdLookup = fMD->md_lookup();
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
    for(auto & mPtr: lwPtrs) {
      int nRows = mPtr->rowCount();
      for(int i = 0; i < nRows; i++) {
	QTableWidgetItem * iPtr = mPtr->item(i,0);
	int mdIndex = iPtr->data(Qt::UserRole).toInt();
	auto state = iPtr->checkState();
	if(state == Qt::Unchecked) continue;
	else if(state == Qt::Checked) {
	  acceptTags.insert(mdIndex);
	}
	else rejectTags.insert(mdIndex);
      }
    }
    auto fModel = fListView->model();
    auto root_index = fListView->rootIndex();
    auto rows = fModel->rowCount(root_index);
    int performerTID = fMD->md_types().right.at("Performer");
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
      bool performerKnown = false;
      for(auto & tagID: vidMD)
	if(fMD->md_lookup()[tagID].first == performerTID) {
	  performerKnown=true;
	  break;
	}
      if(!performerKnown) vidMD.insert(-1);   
      if(acceptTags.size() > 0) {
	bool hasAny = false;
	for(auto & element: acceptTags) {
	  if(vidMD.count(element) != 0) {  //c++20 contains is better
	    hasAny=true;
	    break;
	  }
	}
	if(!hasAny) {
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
  };

private:

  qvdb_metadata * fMD;
  QListView * fListView;
  std::vector<QTableWidget *> lwPtrs;
  std::vector<QLineEdit *> linePtrs;
  
};

