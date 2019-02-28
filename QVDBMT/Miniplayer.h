#include <QDialog>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QVBoxLayout>
#include <QUrl>

class Miniplayer: public QDialog {

  using QDialog::QDialog;

 public:
  
  Miniplayer(QMainWindow * parent,std::string & vidFile) {
    player = new QMediaPlayer;
    QVBoxLayout * mainLayout = new QVBoxLayout;
    player->setMedia(QUrl::fromLocalFile(vidFile.c_str()));
    QVideoWidget * videoWidget = new QVideoWidget;
    player->setVideoOutput(videoWidget);
    mainLayout->addWidget(videoWidget);
    setLayout(mainLayout);
    videoWidget->show();
    player->play();
  };
  
 private:

  QMediaPlayer * player;
};
