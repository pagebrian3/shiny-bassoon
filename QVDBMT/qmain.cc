#include <QApplication>
#include <QVBrowser.h>

int main(int argc, char *argv[])
{
  QApplication * app = new QApplication(argc, argv);
  QVBrowser * mainWin = new QVBrowser();
  mainWin->show();
  return app->exec();
}


 

