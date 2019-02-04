#include <QDialog>

class QMainWindow;
class QFormLayout;
class QLineEdit;

class MetadataDialog: public QDialog {

  using QDialog::QDialog;

public:

  MetadataDialog(QMainWindow * parent);

  void on_accept();

private:

  
  std::map<std::string,boost::variant<int, float, std::string> > config;
};
