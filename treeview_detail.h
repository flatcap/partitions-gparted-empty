#ifndef TREEVIEW_DETAIL
#define TREEVIEW_DETAIL

#include "partition.h"

#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/entry.h>

#include <gtkmm/stock.h>
#include <gdkmm/pixbuf.h>

#include <vector>

class TreeView_Detail : public Gtk::TreeView
{
public:
	TreeView_Detail();
	void load_partitions( const std::vector<Partition> & partitions );
	void set_selected( const Partition & partition );
	void clear();

	//signals for interclass communication
	sigc::signal< void, const Partition &, bool > signal_partition_selected;
	sigc::signal< void > signal_partition_activated;
	sigc::signal< void, unsigned int, unsigned int > signal_popup_menu;

private:
	void load_partitions( const std::vector<Partition> & partitions,
			      bool & mountpoints,
			      bool & labels,
			      const Gtk::TreeRow & parent_row = Gtk::TreeRow() );
	bool set_selected( Gtk::TreeModel::Children rows, const Partition & partition, bool inside_extended = false );
	void create_row( const Gtk::TreeRow & treerow, const Partition & partition );

	//(overridden) signals
	bool on_button_press_event( GdkEventButton * event );
	void on_row_activated( const Gtk::TreeModel::Path & path, Gtk::TreeViewColumn * column );
	void on_selection_changed();

	Glib::RefPtr<Gtk::TreeStore> treestore_detail;
	Glib::RefPtr<Gtk::TreeSelection> treeselection;

	bool block;

	//columns for this treeview
	struct treeview_detail_Columns : public Gtk::TreeModelColumnRecord
	{
		Gtk::TreeModelColumn<Glib::ustring> path;
		Gtk::TreeModelColumn<Glib::ustring> filesystem;
		Gtk::TreeModelColumn<Glib::ustring> mountpoint;
		Gtk::TreeModelColumn<Glib::ustring> label;
		Gtk::TreeModelColumn<Glib::ustring> size;
		Gtk::TreeModelColumn<Glib::ustring> used;
		Gtk::TreeModelColumn<Glib::ustring> unused;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > color;
		Gtk::TreeModelColumn<Glib::ustring> text_color;
		Gtk::TreeModelColumn<Glib::ustring> mount_text_color;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > icon1;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > icon2;
		Gtk::TreeModelColumn<Glib::ustring> flags;
		Gtk::TreeModelColumn<Partition> partition; //hidden column

		treeview_detail_Columns()
		{
			add( path ); add( filesystem ); add( mountpoint ); add( label );
			add( size ); add( used ); add( unused ); add( color );
			add( text_color ); add( mount_text_color ); add( icon1 );
			add( icon2 ); add( flags ); add( partition );
		}
	};

	treeview_detail_Columns treeview_detail_columns;
};

#endif //TREEVIEW_DETAIL
