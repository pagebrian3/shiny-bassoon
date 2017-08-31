#include <gtk/gtk.h>

class VBrowser
{
 public:
  VBrowser(GtkWidget * window);
  ~VBrowser();
 private:
  GtkWidget * fWindow;
  std::string sort_by;
  bool sort_desc;
  GtkButton * browse_button;
  GtkButton * fdupe_button;
  GtkComboBoxText * 
  
  
};
  
