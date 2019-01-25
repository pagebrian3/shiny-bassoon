#include <QMainWindow>
#include <QDialog>
#include <QVBConfig.h>

class ConfigDialog: public QDialog {

 public:

  ConfigDialog(QMainWindow * parent, qvdb_config * cfg);

 private:

};
