#include <QDialog>
#include <boost/bimap.hpp>

class QMainWindow;
class QFormLayout;
class QLineEdit;
class qvdb_metadata;

class MetadataDialog: public QDialog {

  using QDialog::QDialog;

public:

  MetadataDialog(QMainWindow * parent,std::vector<int> & vids, qvdb_metadata *  md);  //vids may be redundant as they are already in md

  void on_accept();

private:
  QFormLayout * formLayout;
  qvdb_metadata *  fMD;
};
