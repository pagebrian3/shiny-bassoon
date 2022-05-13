#include <QDialog>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFileDialog>
#include <fstream>
#include <iostream>

class MDIEDialog: public QDialog {

  using QDialog::QDialog;

public:
  
  MDIEDialog(QWidget * parent, qvdb_metadata * md, DbConnector * dbCon) : fMD(md), fDB(dbCon) {
    setWindowTitle("Export/Import Metadata");
    QVBoxLayout * mainlo = new QVBoxLayout(this);
    QHBoxLayout * dlo = new QHBoxLayout;
    QGroupBox * eIBox = new QGroupBox;
    QRadioButton * exportBtn = new QRadioButton("export");
    QRadioButton * importBtn = new QRadioButton("import");
    exportBtn->setChecked(true);
    fBGrp = new QButtonGroup;
    fBGrp->addButton(exportBtn);
    fBGrp->addButton(importBtn);
    dlo->addWidget(exportBtn);
    dlo->addWidget(importBtn);
    eIBox->setLayout(dlo);
    mainlo->addWidget(eIBox);
    QGroupBox * fileBox = new QGroupBox;
    QHBoxLayout * flo = new QHBoxLayout;
    QPushButton * selFileBtn = new QPushButton("Select File");
    connect(selFileBtn, &QPushButton::clicked, this, &MDIEDialog::selFile_clicked);
    fnDisplay = new QLabel("");
    flo->addWidget(selFileBtn);
    flo->addWidget(fnDisplay);
    fileBox->setLayout(flo);
    mainlo->addWidget(fileBox);
    fBBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    fBBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    connect(fBBox, &QDialogButtonBox::accepted, this, &MDIEDialog::on_accept);
    connect(fBBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(fBBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainlo->addWidget(fBBox);
  };

  void selFile_clicked() {
    QFileDialog dialog(this, tr("Please Choose a Folder or Folders"));
    if(is_import()) dialog.setFileMode(QFileDialog::ExistingFile);
    else dialog.setFileMode(QFileDialog::AnyFile);
    if(dialog.exec()) {
      QString qfile = dialog.selectedFiles()[0];
      fFile = qfile.toStdString();
      fnDisplay->setText(qfile);
      fBBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    return;
  };

  void on_accept() {
    if(is_import()) importMD();
    else exportMD();
    return;
  };

  void importMD() {
    //Would be good to add some checks/failsafes on formatting issue.
    std::cout << "importing" << std::endl;
    std::string STRING;
    std::string label;
    std::ifstream infile;
    infile.open(fFile);
    size_t spacePos;
    int currentType = 0;
    std::string currentTypeLabel;
    std::filesystem::path filename;
    while(!infile.eof()) {
      getline(infile,STRING);
      if(STRING.length() < 2) continue;      
      spacePos = STRING.find_first_of('\t');
      if(spacePos == std::string::npos) {
	if(!fMD->typeExists(STRING)) currentType=fMD->newType(STRING);
	else currentType=fMD->mdType(STRING);
	currentTypeLabel=STRING;
	std::cout << "Type: " <<currentType << " " << currentTypeLabel;
      }
      else {
	label = STRING.substr(0,spacePos);
	filename = STRING.substr(spacePos+1);
	if(!fDB->video_exists(filename)) continue;
	if(!fMD->labelExists(currentType,label)) {
	  int id = fMD->newLabel(currentTypeLabel,label);
	  std::cout << "Added label " << label << " ID " << id << std::endl;
	}
	fMD->attachToFile(fDB->fileVid(filename),label);
      }
    }
    infile.close();
    fMD->saveMetadata();
    return;
  };

  void exportMD() {
    std::ofstream outfile;
    outfile.open(fFile);
    auto typeMap = fMD->types();
    auto labelMap = fMD->md_lookup();
    std::map<int,std::filesystem::path> vidMap;
    fDB->load_fileVIDs(vidMap);
    std::vector<int> VIDs;
    for(auto & a : vidMap) VIDs.push_back(a.first);
    fMD->load_file_md(VIDs);
    for(auto & a: typeMap) {
      int typeID = a.first;
      std::string typeLabel(a.second);
      outfile << typeLabel << "\n";
      std::cout << typeLabel << std::endl;
      for(auto & tag: labelMap) {
	if(tag.second.first != typeID) continue;
	int tagID = tag.first;
	for(auto & vPath: vidMap) if(fMD->mdForFile(vPath.first).count(tagID) != 0) outfile << tag.second.second << "\n"<<vPath.second.native()<<"\n";
      }      
    }
    outfile.close();
    return;
  };

  bool is_import() {
    if( strcmp(fBGrp->checkedButton()->text().toStdString().c_str(),"import")==0) return true;
    else return false;
  };

private:
  qvdb_metadata * fMD;
  DbConnector * fDB;
  QButtonGroup * fBGrp;
  QDialogButtonBox * fBBox;
  QLabel * fnDisplay;
  std::filesystem::path fFile;
  
};
