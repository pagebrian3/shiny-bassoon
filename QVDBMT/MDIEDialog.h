#include <QDialog>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFileDialog>
#include <fstream>

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
    if(fBGrp->checkedId() == 0) dialog.setFileMode(QFileDialog::ExistingFile);
    else if(fBGrp->checkedId() == 1) dialog.setFileMode(QFileDialog::AnyFile);
    if(dialog.exec()) {
      QString qfile = dialog.selectedFiles()[0];
      fFile = qfile.toStdString();
      fnDisplay->setText(qfile);
      fBBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    return;
  };

  void on_accept() {
    if(fBGrp->checkedId() == 0) importMD();
    else exportMD();
    return;
  };

  void importMD() {
    //Would be good to add some checks/failsafes on formatting issue.
    std::string STRING;
    std::string label;
    std::ifstream infile;
    infile.open(fFile);
    size_t spacePos;
    int currentType;
    std::string currentTypeLabel;
    std::filesystem::path filename;
    while(!infile.eof()) {
      getline(infile,STRING);
      if(STRING.length() < 2) continue;      
      spacePos = STRING.find_first_of(' ');
      if(spacePos == std::string::npos) {
	if(fMD->md_types().right.count(STRING) == 0) fMD->newType(STRING);
	currentType=fMD->md_types().right.at(STRING);
	currentTypeLabel=STRING;
      }
      else {
	label = STRING.substr(0,spacePos);
	filename = STRING.substr(spacePos+1);
	if(!fDB->video_exists(filename)) continue;
	if(!fMD->labelExists(currentType,label)) fMD->newLabel(currentTypeLabel,label);
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
      for(auto & tag: labelMap) {
	if(tag.second.first != typeID) continue;
	int tagID = tag.first;
	for(auto & vPath: vidMap) if(fMD->mdForFile(vPath.first).count(tagID) != 0) outfile << tag.second.second << " "<<vPath.second.native()<<"\n";
      }      
    }
    outfile.close();
    return;
  };

private:
  qvdb_metadata * fMD;
  DbConnector * fDB;
  QButtonGroup * fBGrp;
  QDialogButtonBox * fBBox;
  QLabel * fnDisplay;
  std::filesystem::path fFile;
  
};
