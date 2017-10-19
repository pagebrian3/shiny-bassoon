#include <VBrowser.h>

int main (int argc, char *argv[]) {
  int c = 1;
  auto app = Gtk::Application::create(c, argv, "org.gtkmm.example");

  VBrowser vbrowser(argc, argv);

  //Shows the window and returns when it is closed.
  return app->run(vbrowser);
}
