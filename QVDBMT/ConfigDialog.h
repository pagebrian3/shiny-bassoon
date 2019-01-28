#include <QDialog>
#include<boost/variant.hpp>

class QMainWindow;
class QFormLayout;
class QLineEdit;
class qvdb_config;

class ConfigDialog: public QDialog {

  using QDialog::QDialog;

public:

  ConfigDialog(QMainWindow * parent, qvdb_config * cfg);

  void on_accept();

private:

  qvdb_config * cfgPtr;
  QFormLayout * formLayout;
  std::vector<QLineEdit *> editPtrs;
  std::map<std::string,boost::variant<int, float, std::string> > config;
};
