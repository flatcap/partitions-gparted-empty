#ifndef HBOX_OPERATIONS
#define HBOX_OPERATIONS

#include "operation.h"

#include <gtkmm/box.h>
#include <gtkmm/liststore.h>
#include <gtkmm/menu.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>

class HBoxOperations : public Gtk::HBox
{
public:
	HBoxOperations();
	~HBoxOperations();

	void load_operations( const std::vector<Operation *> operations );
	void clear();

	sigc::signal< void > signal_undo;
	sigc::signal< void > signal_clear;
	sigc::signal< void > signal_apply;
	sigc::signal< void > signal_close;

private:
	bool on_signal_button_press_event( GdkEventButton * event );
	void on_undo();
	void on_clear();
	void on_apply();
	void on_close();

	Gtk::Menu menu_popup;
	Gtk::ScrolledWindow scrollwindow;
	Gtk::TreeView treeview_operations;
	Glib::RefPtr<Gtk::ListStore> liststore_operations;

	struct treeview_operations_Columns : public Gtk::TreeModelColumnRecord
	{
		Gtk::TreeModelColumn<Glib::ustring> operation_description;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > operation_icon;

		treeview_operations_Columns()
		{
			add( operation_description );
			add( operation_icon );
		}
	};
	treeview_operations_Columns treeview_operations_columns;
};

#endif //HBOX_OPERATIONS
