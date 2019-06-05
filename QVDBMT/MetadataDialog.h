#include <QDialog>

class QWidget;
class QComboBox;
class QListWidget;
class qvdb_metadata;

class MetadataDialog: public QDialog {

  using QDialog::QDialog;

public:

  MetadataDialog(QWidget * parent,std::vector<int> & vids, qvdb_metadata *  md);  //vids may be redundant as they are already in md

  void on_accept();

  void onTypeAddClicked();

  void onLabelAddClicked();

   void onRightArrowClicked(); 

  void onLeftArrowClicked(); 

  void updateTypes();

  void updateLabels();
  
private:

  bool firstRun;
  qvdb_metadata *  fMD;
  QComboBox * type_combo;
  QListWidget * lList;
  QListWidget * flList;
  std::vector<int> fVids;
};
