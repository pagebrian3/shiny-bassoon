#include <QApplication>
#include <QVBrowser.h>

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  QVBrowser mainWin(vm);
  mainWin.show();
  return app.exec();
}


 

