#include "QVBConfig.h"
#include <QDialog>
#include <QWidget>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QScrollArea>
#include <boost/variant.hpp>

class ConfigDialog: public QDialog {

  using QDialog::QDialog;

public:

ConfigDialog(QWidget * parent, qvdb_config * cfg)  {
  cfgPtr = cfg;
  config = cfg->get_data();
  QWidget * flob = new QWidget;
  QScrollArea * scrollArea = new QScrollArea;
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  formLayout = new QFormLayout(flob);
  for(auto & a: config) {
    int type = a.second.which();
    QLineEdit *  tempEdit = new QLineEdit();
    std::string tempS;
    if(type == 0) tempS = std::to_string(boost::get<int>(a.second));
    else if(type == 1) tempS = std::to_string(boost::get<float>(a.second));
    else tempS = boost::get<std::string>(a.second);
    tempEdit->setPlaceholderText(tempS.c_str());
    editPtrs.push_back(tempEdit);
    formLayout->addRow(a.first.c_str(),tempEdit);
  }
  scrollArea->setWidget(flob);
  QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox,&QDialogButtonBox::accepted,this,&ConfigDialog::on_accept);
  connect(buttonBox,&QDialogButtonBox::accepted,this,&QDialog::accept);
  connect(buttonBox,&QDialogButtonBox::rejected,this,&QDialog::reject);
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(scrollArea);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);
  setWindowTitle(tr("Video Browser Configuration"));
  scrollArea->show();
};

void on_accept() {
  int i = 0;
  for(auto & cfg:config) {
    std::string key(cfg.first);
    QLineEdit * leHolder = editPtrs[i];
    i++;
    if(leHolder->isModified()) {
      int index = cfg.second.which();
      if(index == 0) {
	cfgPtr->set(key,leHolder->text().toInt());
      }
      else if(index == 1) {
	cfgPtr->set(key,leHolder->text().toFloat());
      }
      else if(index == 2) {
	cfgPtr->set(key,leHolder->text().toStdString());
      }
      else continue;
    }
    else continue;
  }
  return;
};

  private:

  qvdb_config * cfgPtr;
  QFormLayout * formLayout;
  std::vector<QLineEdit *> editPtrs;
  std::map<std::string,boost::variant<int, float, std::string> > config;
};

