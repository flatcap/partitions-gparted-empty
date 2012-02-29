#ifndef WIN_GPARTED
#define WIN_GPARTED

#include "device.h"
#include "drawingareavisualdisk.h"
#include "partition.h"
#include "treeview_detail.h"
#include "gparted_core.h"
#include "hboxoperations.h"

#include <sigc++/class_slot.h>
#include <gtkmm/paned.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/menubar.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/combobox.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/window.h>
#include <gtkmm/table.h>

class Win_GParted : public Gtk::Window
{
public:
	Win_GParted (const std::vector<Glib::ustring> & user_devices);

private:
	void init_menubar();
	void init_toolbar();
	void init_partition_menu();
	Gtk::Menu * create_format_menu();
	void init_device_info();
	void init_hpaned_main();

	void refresh_combo_devices();
	void show_pulsebar (const Glib::ustring & status_message);

	//Fill txtview_device_info_buffer with some information about the selected device
	void Fill_Label_Device_Info (bool clear = false);

	void Add_Operation (Operation * operation, int index = -1);
	bool Merge_Operations (unsigned int first, unsigned int second);
	void Refresh_Visual();
	bool Quit_Check_Operations();
	void set_valid_operations();

	//convenience functions
	void toggle_item (bool state, int menu_item, int toolbar_item = -1)
        {
                if (menu_item >= 0 && menu_item < static_cast<int> (menu_partition.items().size()))
                        menu_partition.items()[menu_item].set_sensitive (state);

                if (toolbar_item >= 0 && toolbar_item < toolbar_main.get_n_items())
                        toolbar_main.get_nth_item (toolbar_item)->set_sensitive (state);
        }

	void allow_new (bool state) { toggle_item (state, MENU_NEW, TOOLBAR_NEW); }
	void allow_delete (bool state) { toggle_item (state, MENU_DEL, TOOLBAR_DEL); }
	void allow_resize (bool state) { toggle_item (state, MENU_RESIZE_MOVE, TOOLBAR_RESIZE_MOVE); }
	void allow_copy (bool state) { toggle_item (state, MENU_COPY, TOOLBAR_COPY); }
	void allow_paste (bool state) { toggle_item (state, MENU_PASTE, TOOLBAR_PASTE); }
	void allow_format (bool state) { toggle_item (state, MENU_FORMAT); }
	void allow_toggle_swap_mount_state (bool state)	{ toggle_item (state, MENU_TOGGLE_MOUNT_SWAP); }
	void allow_manage_flags (bool state) { toggle_item (state, MENU_FLAGS); }
	void allow_check (bool state) { toggle_item (state, MENU_CHECK); }
	void allow_label_partition (bool state)	{ toggle_item (state, MENU_LABEL_PARTITION); }
	void allow_change_uuid (bool state) { toggle_item (state, MENU_CHANGE_UUID); }
	void allow_info (bool state) { toggle_item (state, MENU_INFO); }

	void allow_undo_clear_apply (bool state)
	{
		toggle_item (state, -1, TOOLBAR_UNDO);
		static_cast<Gtk::CheckMenuItem *> (& menubar_main.items()[1].get_submenu()->items()[0])->set_sensitive (state);
		static_cast<Gtk::CheckMenuItem *> (& menubar_main.items()[1].get_submenu()->items()[1])->set_sensitive (state);
		toggle_item (state, -1, TOOLBAR_APPLY);
		static_cast<Gtk::CheckMenuItem *> (& menubar_main.items()[1].get_submenu()->items()[2])->set_sensitive (state);
	}

	//threads..
	void thread_refresh_devices();
	void thread_unmount_partition (bool * success, Glib::ustring * error);
	void thread_mount_partition (Glib::ustring mountpoint, bool * success, Glib::ustring * error);
	void thread_toggle_swap (bool * success, Glib::ustring * error);
	void thread_guess_partition_table();

	//signal handlers
	void open_operationslist();
	void close_operationslist();
	void clear_operationslist();
	void combo_devices_changed();
	void radio_devices_changed (unsigned int item);
	bool on_delete_event (GdkEventAny*);
	void on_show();

	void menu_gparted_refresh_devices();
	void menu_gparted_features();
	void menu_gparted_quit();
	void menu_view_harddisk_info();
	void menu_view_operations();
	void show_disklabel_unrecognized (Glib::ustring device_name);
	void show_help_dialog (const Glib::ustring & filename, const Glib::ustring & link_id);
	void menu_help_contents();
	void menu_help_about();

	void on_partition_selected (const Partition & partition, bool src_is_treeview);
	void on_partition_activated();
	void on_partition_popup_menu (unsigned int button, unsigned int time);

	bool max_amount_prim_reached();

	void activate_resize();
	void activate_copy();
	void activate_paste();
	void activate_new();
	void activate_delete();
	void activate_info();
	void activate_format (FILESYSTEM new_fs);
	void toggle_swap_mount_state();
	void activate_mount_partition (unsigned int index);
	void activate_disklabel();
	void activate_attempt_rescue_data();
	void activate_manage_flags();
	void activate_check();
	void activate_change_uuid();
	void activate_label_partition();

	void activate_undo();
	void remove_operation (int index = -1, bool remove_all = false);
	int  partition_in_operation_queue_count (const Partition & partition);
	int  active_partitions_on_device_count (const Device & device);
	void activate_apply();

	//private variables
	unsigned int current_device;
	Partition selected_partition, copied_partition;
	std::vector<Device> devices;
	std::vector<Operation *> operations;

	//gui stuff
	Gtk::HPaned hpaned_main;
	Gtk::VPaned vpaned_main;
	Gtk::VBox vbox_main,vbox_info;
	Gtk::HBox hbox_toolbar, *hbox;
	Gtk::Toolbar toolbar_main;
	Gtk::MenuBar menubar_main;
	Gtk::ComboBox combo_devices;
	Gtk::Menu menu_partition, *menu;
	Gtk::ToolButton *toolbutton;
	Gtk::Statusbar statusbar;
	Gtk::Image *image;
	Gtk::ScrolledWindow *scrollwindow;
	Gtk::Label label_device_info1, label_device_info2, *label;
	Gtk::Tooltips tooltips;
	Gtk::Table *table;
	Gtk::MenuItem *menu_item;
	Gtk::ProgressBar pulsebar;
	Gtk::TreeRow treerow;

	DrawingAreaVisualDisk drawingarea_visualdisk;
	TreeView_Detail treeview_detail;
	HBoxOperations hbox_operations;

	//device combo
	Glib::RefPtr<Gtk::ListStore> liststore_devices;

	struct treeview_devices_Columns : public Gtk::TreeModelColumnRecord
	{
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
		Gtk::TreeModelColumn<Glib::ustring> device;
		Gtk::TreeModelColumn<Glib::ustring> size;

		treeview_devices_Columns()
		{
			add (icon);
			add (device);
			add (size);
		}
	};
	treeview_devices_Columns treeview_devices_columns;

	//indices for partitionmenu and toolbar
        int
        MENU_NEW, TOOLBAR_NEW,
        MENU_DEL, TOOLBAR_DEL,
        MENU_RESIZE_MOVE, TOOLBAR_RESIZE_MOVE,
        MENU_COPY, TOOLBAR_COPY,
        MENU_PASTE, TOOLBAR_PASTE,
        MENU_FORMAT,
        MENU_TOGGLE_MOUNT_SWAP,
        MENU_MOUNT,
        MENU_FLAGS,
        MENU_CHECK,
	MENU_LABEL_PARTITION,
	MENU_CHANGE_UUID,
        MENU_INFO,
        TOOLBAR_UNDO,
        TOOLBAR_APPLY;

	//usefull variables which are used by many different functions...
	int index_extended; //position of the extended partition (-1 means there isn't one)
	unsigned short primary_count;//primary_count checks for max. of 4 pimary partitions
	unsigned short new_count;//new_count keeps track of the new created partitions
	FS fs;
	bool OPERATIONSLIST_OPEN;
	Glib::ustring gpart_output;//Output of gpart command

	GParted_Core gparted_core;
	std::vector<Gtk::Label *> device_info;

	//stuff for progress overview and pulsebar
	Glib::Thread *thread;
	bool pulse;
};

#endif //WIN_GPARTED
