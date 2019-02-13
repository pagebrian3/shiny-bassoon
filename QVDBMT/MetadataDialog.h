#include <QDialog>
#include <boost/bimap.hpp>

class QMainWindow;
class QFormLayout;
class QLineEdit;
class video_utils;

class MetadataDialog: public QDialog {

  using QDialog::QDialog;

public:

  MetadataDialog(QMainWindow * parent,std::vector<int> & vids, video_utils * vu);

  void on_accept();

private:
  QFormLayout * formLayout;

};
