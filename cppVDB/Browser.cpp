#include "Browser.h"
#include <gtkmm/box.h>
#include <gtkmm/liststore.h>
#include <gtkmm/label.h>

MyWindow::MyWindow() {
        this->set_default_size(width,height);
	Gtk::Box * box_outer = new Gtk::Box(Gtk::ORIENTATION_VERTICAL,=6);
        this->add(box_outer);
        fSortBy="size"; //size, name, length
        fSortDesc=true; //True, False
	Gtk::Button * browse_button = new Gtk::Button("...");
        browse_button.connect("clicked",browse_clicked);
	Gtk::Button * fdupe_button = Gtk::Button("Find Dupes");
        fdupe_button.connect("clicked",fdupe_clicked);
	Gtk::Box * sort_opt = new Gtk::Box();
        Gtk::Label sort_label("Sort by:");
        fSortCombo = new Gtk::ComboBoxText();
	fSortCombo->append(opts,"size");
	fSortCombo->append(opts,"length");
        fSortCombo->append(opts,"name");
        Gtk::CellRendererText renderer_text();
        fSortCombo.pack_start(renderer_text, true);
        fSortCombo.add_attribute(renderer_text, "text", 0);
        fSortCombo.set_active(0);
        fSortCombo.connect("changed",on_sort_changed);
        if (fSortDesc) stock_icon=Gtk::STOCK_SORT_DESCENDING;
        else stock_icon=Gtk::STOCK_SORT_ASCENDING;
        fAscButton = new Gtk::Button(None,Gtk::Image(stock_icon) );
        fAscBbutton.connect("clicked", asc_clicked);
        this->connect("destroy", on_delete);
        sort_opt.add(sort_label);
        sort_opt.add(fSortCombo);
        sort_opt.add(fAscButton);
        sort_opt.pack_end(fdupe_button,  False,  False,  0);
        sort_opt.pack_end(browse_button,  False, False, 0 ); 
        box_outer.pack_start(sort_opt, False, True, 0);
        fScrollWin = new Gtk::ScrolledWindow();
        box_outer.pack_start(fScrollWin, True, True, 0);
        populate_icons();
}
