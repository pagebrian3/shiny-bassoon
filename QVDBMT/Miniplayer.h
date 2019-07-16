#include <QDialog>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QVBoxLayout>
#include <QUrl>
#include <QKeyEvent>

class Miniplayer: public QDialog {

  using QDialog::QDialog;

public:
  
  Miniplayer(std::string & vidFile, int height, int width) {
    player = new QMediaPlayer;
    QVBoxLayout * mainLayout = new QVBoxLayout;
    player->setMedia(QUrl::fromLocalFile(vidFile.c_str()));
    QVideoWidget * videoWidget = new QVideoWidget;
    videoWidget->setMinimumHeight(height);
    videoWidget->setMinimumWidth(width);
    player->setVideoOutput(videoWidget);
    mainLayout->addWidget(videoWidget);
    setLayout(mainLayout);
    videoWidget->show();
    player->play();
  };
  
private:

  QMediaPlayer * player;

protected:

  void keyPressEvent(QKeyEvent *event) override {
    int change = 15000;
    int position = player->position();
    switch(event->key()) {
    case Qt::Key_Right:
      player->setPosition(position+change);
      break;
    case Qt::Key_Left:
      player->setPosition(position-change);
      break;
    case Qt::Key_Up:
      player->setPosition(position+20*change);
      break;
    case Qt::Key_Down:
      player->setPosition(position-20*change);
      break;
    default:
      QDialog::keyPressEvent(event);
    }
  };
};
