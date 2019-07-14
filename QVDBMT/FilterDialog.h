#include <QDialog>

class QListView;
class QListWidget;
class QLineEdit;
class qvdb_metadata;

class FilterDialog: public QDialog {

  using QDialog::QDialog;
  
public:

  FilterDialog(QWidget * parent, QListView * listView, qvdb_metadata *  md);  //vids may be redundant as they are already in md

  void on_accept();
  
private:

  qvdb_metadata * fMD;
  QListView * fListView;
  std::vector<QListWidget *> lwPtrs;
  std::vector<QLineEdit *> linePtrs;
  
};
