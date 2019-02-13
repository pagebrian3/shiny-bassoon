#include "MetadataDialog.h"
#include "VideoUtils.h"
#include <QMainWindow>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>

MetadataDialog::MetadataDialog(QMainWindow * parent,std::vector<int> & vids, video_utils *  vu)  {
  QGroupBox * flob = new QGroupBox;
  formLayout = new QFormLayout;
  /*for(auto & a: config) {
    int type = a.second.which();
    QLineEdit *  tempEdit = new QLineEdit();
    std::string tempS;
    if(type == 0) tempS = std::to_string(boost::get<int>(a.second));
    else if(type == 1) tempS = std::to_string(boost::get<float>(a.second));
    else tempS = boost::get<std::string>(a.second);
    tempEdit->setPlaceholderText(tempS.c_str());
    formLayout->addRow(a.first.c_str(),tempEdit);
    }*/
  flob->setLayout(formLayout);
  QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &MetadataDialog::on_accept);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(flob);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    setWindowTitle(tr("Video Browser Configuration")); 
}

void MetadataDialog::on_accept() {
  int i = 0;
  return;
}
