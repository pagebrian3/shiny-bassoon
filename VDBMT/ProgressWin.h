#include <gtkmm-3.0/gtkmm.h>

class ProgressWin : public Gtk::Window
{
 public:
  ProgressWin() {
    this->set_default_size(200, 100);
    this->set_title("Progress");
    this->add(bar);
    this->show_all_children();
  };
  
  void update_progress(float progress) {
    bar.set_fraction(progress);
    return;
  };

  Gtk::ProgressBar bar;
};
