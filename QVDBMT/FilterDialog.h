#include <QDialog>

class QWidget;
class QCheckBox;
class QStandardItem;
class QLineEdit;
class qvdb_metadata;

class FilterDialog: public QDialog {

  using QDialog::QDialog;

public:

  FilterDialog(QWidget * parent, std::vector<QStandardItem *> & vids, qvdb_metadata *  md);  //vids may be redundant as they are already in md

  void on_accept();
  
private:

  qvdb_metadata * fMD;
  std::vector<QStandardItem *> fVids;
  std::vector<QCheckBox *> checkPtrs;
  std::vector<QLineEdit *> linePtrs;
};
