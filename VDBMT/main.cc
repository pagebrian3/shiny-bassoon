#include <VBrowser.h>

int main (int argc, char *argv[])
{
  auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

  VBrowser vbrowser(argc, argv);

  //Shows the window and returns when it is closed.
  return app->run(vbrowser);
}
