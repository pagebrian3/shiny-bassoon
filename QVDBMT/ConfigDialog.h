#include <QDialog>
#include <boost/variant.hpp>  //can't fwd declare  its a templated type

class QWidget;
class QFormLayout;
class QLineEdit;
class qvdb_config;

class ConfigDialog: public QDialog {

  using QDialog::QDialog;

public:

  ConfigDialog(QWidget * parent, qvdb_config * cfg);

  void on_accept();

private:

  qvdb_config * cfgPtr;
  QFormLayout * formLayout;
  std::vector<QLineEdit *> editPtrs;
  std::map<std::string,boost::variant<int, float, std::string> > config;
};
