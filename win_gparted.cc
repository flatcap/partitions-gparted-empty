#include "win_gparted.h"

#include <gtkmm/aboutdialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/radiobuttongroup.h>
#include <gtkmm/main.h>

Win_GParted::Win_GParted( const std::vector<Glib::ustring> & user_devices )
{
	copied_partition .Reset();
	selected_partition .Reset();
	new_count = 1;
	current_device = 0;
	pulse = false;
	OPERATIONSLIST_OPEN = true;
	gparted_core .set_user_devices( user_devices );

	MENU_NEW = TOOLBAR_NEW =
        MENU_DEL = TOOLBAR_DEL =
        MENU_RESIZE_MOVE = TOOLBAR_RESIZE_MOVE =
        MENU_COPY = TOOLBAR_COPY =
        MENU_PASTE = TOOLBAR_PASTE =
        MENU_FORMAT =
        MENU_TOGGLE_MOUNT_SWAP =
        MENU_MOUNT =
        MENU_FLAGS =
        MENU_INFO =
        MENU_LABEL_PARTITION =
        MENU_CHANGE_UUID =
        TOOLBAR_UNDO =
        TOOLBAR_APPLY = -1;

	//==== GUI =========================
	this ->set_title( "GParted" );
	this ->set_default_size( 775, 500 );

	try
	{
		this ->set_default_icon_name( "gparted" );
	}
	catch ( Glib::Exception & e )
	{
		std::cout << e .what() << std::endl;
	}

	//Pack the main box
	this ->add( vbox_main );

	//menubar....
	init_menubar();
	vbox_main .pack_start( menubar_main, Gtk::PACK_SHRINK );

	//toolbar....
	init_toolbar();
	vbox_main.pack_start( hbox_toolbar, Gtk::PACK_SHRINK );

	//drawingarea_visualdisk...  ( contains the visual represenation of the disks )
	drawingarea_visualdisk .signal_partition_selected .connect(
			sigc::mem_fun( this, &Win_GParted::on_partition_selected ) );
	drawingarea_visualdisk .signal_partition_activated .connect(
			sigc::mem_fun( this, &Win_GParted::on_partition_activated ) );
	drawingarea_visualdisk .signal_popup_menu .connect(
			sigc::mem_fun( this, &Win_GParted::on_partition_popup_menu ) );
	vbox_main .pack_start( drawingarea_visualdisk, Gtk::PACK_SHRINK );

	//hpaned_main (NOTE: added to vpaned_main)
	init_hpaned_main();
	vpaned_main .pack1( hpaned_main, true, true );

	//vpaned_main....
	vbox_main .pack_start( vpaned_main );

	//device info...
	init_device_info();

	//operationslist...
	hbox_operations .signal_undo .connect( sigc::mem_fun( this, &Win_GParted::activate_undo ) );
	hbox_operations .signal_clear .connect( sigc::mem_fun( this, &Win_GParted::clear_operationslist ) );
	hbox_operations .signal_apply .connect( sigc::mem_fun( this, &Win_GParted::activate_apply ) );
	hbox_operations .signal_close .connect( sigc::mem_fun( this, &Win_GParted::close_operationslist ) );
	vpaned_main .pack2( hbox_operations, true, true );

	//statusbar...
	pulsebar .set_pulse_step( 0.01 );
	statusbar .add( pulsebar );
	vbox_main .pack_start( statusbar, Gtk::PACK_SHRINK );

	this ->show_all_children();

	//make sure harddisk information is closed..
	hpaned_main .get_child1() ->hide();
}

void Win_GParted::init_menubar()
{
	//fill menubar_main and connect callbacks
	//gparted
	menu = manage( new Gtk::Menu() );
	image = manage( new Gtk::Image( Gtk::Stock::REFRESH, Gtk::ICON_SIZE_MENU ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem(
		"_Refresh Devices",
		Gtk::AccelKey("<control>r"),
		*image,
		sigc::mem_fun(*this, &Win_GParted::menu_gparted_refresh_devices) ) );

	image = manage( new Gtk::Image( Gtk::Stock::HARDDISK, Gtk::ICON_SIZE_MENU ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem( "_Devices", *image ) );

	menu ->items() .push_back( Gtk::Menu_Helpers::SeparatorElem( ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::StockMenuElem(
		Gtk::Stock::QUIT, sigc::mem_fun(*this, &Win_GParted::menu_gparted_quit) ) );
	menubar_main .items() .push_back( Gtk::Menu_Helpers::MenuElem( "_GParted", *menu ) );

	//edit
	menu = manage( new Gtk::Menu() );
	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem(
		"_Undo Last Operation",
		Gtk::AccelKey("<control>z"),
		* manage( new Gtk::Image( Gtk::Stock::UNDO, Gtk::ICON_SIZE_MENU ) ),
		sigc::mem_fun(*this, &Win_GParted::activate_undo) ) );

	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem(
		"_Clear All Operations",
		* manage( new Gtk::Image( Gtk::Stock::CLEAR, Gtk::ICON_SIZE_MENU ) ),
		sigc::mem_fun(*this, &Win_GParted::clear_operationslist) ) );

	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem(
		"_Apply All Operations",
		* manage( new Gtk::Image( Gtk::Stock::APPLY, Gtk::ICON_SIZE_MENU ) ),
		sigc::mem_fun(*this, &Win_GParted::activate_apply) ) );
	menubar_main .items() .push_back( Gtk::Menu_Helpers::MenuElem( "_Edit", *menu ) );

	//view
	menu = manage( new Gtk::Menu() );
	menu ->items() .push_back( Gtk::Menu_Helpers::CheckMenuElem(
		"Device _Information", sigc::mem_fun(*this, &Win_GParted::menu_view_harddisk_info) ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::CheckMenuElem(
		"Pending _Operations", sigc::mem_fun(*this, &Win_GParted::menu_view_operations) ) );
	menubar_main .items() .push_back( Gtk::Menu_Helpers::MenuElem( "_View", *menu ) );

	menu ->items() .push_back( Gtk::Menu_Helpers::SeparatorElem( ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::MenuElem(
		"_File System Support", sigc::mem_fun( *this, &Win_GParted::menu_gparted_features ) ) );

	//device
	menu = manage( new Gtk::Menu() );
	menu ->items() .push_back( Gtk::Menu_Helpers::MenuElem( Glib::ustring( "_Create Partition Table" ) + "...",
								sigc::mem_fun(*this, &Win_GParted::activate_disklabel) ) );

	menu ->items() .push_back( Gtk::Menu_Helpers::MenuElem( Glib::ustring( "_Attempt Data Rescue" ) + "...",
								sigc::mem_fun(*this, &Win_GParted::activate_attempt_rescue_data) ) );

	menubar_main .items() .push_back( Gtk::Menu_Helpers::MenuElem( "_Device", *menu ) );

	//partition
	init_partition_menu();
	menubar_main .items() .push_back( Gtk::Menu_Helpers::MenuElem( "_Partition", menu_partition ) );

	//help
	menu = manage( new Gtk::Menu() );
	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem(
		"_Contents",
		Gtk::AccelKey("F1"),
		* manage( new Gtk::Image( Gtk::Stock::HELP, Gtk::ICON_SIZE_MENU ) ),
		sigc::mem_fun(*this, &Win_GParted::menu_help_contents) ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::SeparatorElem( ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::StockMenuElem(
		Gtk::Stock::ABOUT, sigc::mem_fun(*this, &Win_GParted::menu_help_about) ) );

	menubar_main.items() .push_back( Gtk::Menu_Helpers::MenuElem("_Help", *menu ) );
}

void Win_GParted::init_toolbar()
{
	int index = 0;
	//initialize and pack toolbar_main
	hbox_toolbar.pack_start( toolbar_main );

	//NEW and DELETE
	image = manage( new Gtk::Image( Gtk::Stock::NEW, Gtk::ICON_SIZE_BUTTON ) );
	/*TO TRANSLATORS: "New" is a tool bar item for partition actions. */
	Glib::ustring str_temp = "New";
	toolbutton = Gtk::manage(new Gtk::ToolButton( *image, str_temp ));
	toolbutton ->signal_clicked() .connect( sigc::mem_fun( *this, &Win_GParted::activate_new ) );
	toolbar_main .append( *toolbutton );
	TOOLBAR_NEW = index++;
	toolbutton ->set_tooltip(tooltips, "Create a new partition in the selected unallocated space" );
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::DELETE));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_delete) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_DEL = index++;
	toolbutton ->set_tooltip(tooltips, "Delete the selected partition" );
	toolbar_main.append( *(Gtk::manage(new Gtk::SeparatorToolItem)) );
	index++;

	//RESIZE/MOVE
	image = manage( new Gtk::Image( Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_BUTTON ) );
	str_temp = "Resize/Move";
	//Condition string split and Undo button.
	//  for longer translated string, split string in two and skip the Undo button to permit full toolbar to display
	//  FIXME:  Is there a better way to do this, perhaps without the conditional?  At the moment this seems to be the best compromise.
	bool display_undo = true;
	if( str_temp .length() > 14 ) {
		size_t index = str_temp .find( "/" );
		if ( index != Glib::ustring::npos ) {
			str_temp .replace( index, 1, "\n/" );
			display_undo = false;
		}
	}
	toolbutton = Gtk::manage(new Gtk::ToolButton( *image, str_temp ));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_resize) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_RESIZE_MOVE = index++;
	toolbutton ->set_tooltip(tooltips, "Resize/Move the selected partition" );
	toolbar_main.append( *(Gtk::manage(new Gtk::SeparatorToolItem)) );
	index++;

	//COPY and PASTE
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::COPY));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_copy) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_COPY = index++;
	toolbutton ->set_tooltip(tooltips, "Copy the selected partition to the clipboard" );
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::PASTE));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_paste) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_PASTE = index++;
	toolbutton ->set_tooltip(tooltips, "Paste the partition from the clipboard" );
	toolbar_main.append( *(Gtk::manage(new Gtk::SeparatorToolItem)) );
	index++;

	//UNDO and APPLY
	if ( display_undo ) {
		//Undo button is displayed only if translated language "Resize/Move" is not too long.  See above setting of this condition.
		toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::UNDO));
		toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_undo) );
		toolbar_main.append(*toolbutton);
		TOOLBAR_UNDO = index++;
		toolbutton ->set_sensitive( false );
		toolbutton ->set_tooltip(tooltips, "Undo Last Operation" );
	}

	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::APPLY));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_apply) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_APPLY = index++;
	toolbutton ->set_sensitive( false );
	toolbutton ->set_tooltip(tooltips, "Apply All Operations" );

	//initialize and pack combo_devices
	liststore_devices = Gtk::ListStore::create( treeview_devices_columns );
	combo_devices .set_model( liststore_devices );

	combo_devices .pack_start( treeview_devices_columns .icon, false );
	combo_devices .pack_start( treeview_devices_columns .device );
	combo_devices .pack_start( treeview_devices_columns .size, false );

	combo_devices .signal_changed() .connect( sigc::mem_fun(*this, &Win_GParted::combo_devices_changed) );

	hbox_toolbar .pack_start( combo_devices, Gtk::PACK_SHRINK );
}

void Win_GParted::init_partition_menu()
{
	int index = 0;

	//fill menu_partition
	image = manage( new Gtk::Image( Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU ) );
	menu_partition .items() .push_back(
			/*TO TRANSLATORS: "_New" is a sub menu item for the partition menu. */
			Gtk::Menu_Helpers::ImageMenuElem( "_New",
							  *image,
							  sigc::mem_fun(*this, &Win_GParted::activate_new) ) );
	MENU_NEW = index++;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::DELETE,
							  Gtk::AccelKey( GDK_Delete, Gdk::BUTTON1_MASK ),
							  sigc::mem_fun(*this, &Win_GParted::activate_delete) ) );
	MENU_DEL = index++;

	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() );
	index++;

	image = manage( new Gtk::Image( Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_MENU ) );
	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::ImageMenuElem( "_Resize/Move",
							  *image,
							  sigc::mem_fun(*this, &Win_GParted::activate_resize) ) );
	MENU_RESIZE_MOVE = index++;

	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() );
	index++;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::COPY,
							  sigc::mem_fun(*this, &Win_GParted::activate_copy) ) );
	MENU_COPY = index++;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::PASTE,
							  sigc::mem_fun(*this, &Win_GParted::activate_paste) ) );
	MENU_PASTE = index++;

	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() );
	index++;

	image = manage( new Gtk::Image( Gtk::Stock::CONVERT, Gtk::ICON_SIZE_MENU ) );
	/*TO TRANSLATORS: menuitem which holds a submenu with file systems.. */
	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::ImageMenuElem( "_Format to",
							  *image,
							  * create_format_menu() ) );
	MENU_FORMAT = index++;

	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() );
	index++;

	menu_partition .items() .push_back(
			//This is a placeholder text. It will be replaced with some other text before it is used
			Gtk::Menu_Helpers::MenuElem( "--placeholder--",
						     sigc::mem_fun( *this, &Win_GParted::toggle_swap_mount_state ) ) );
	MENU_TOGGLE_MOUNT_SWAP = index++;

	/*TO TRANSLATORS: menuitem which holds a submenu with mount points.. */
	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::MenuElem( "_Mount on", * manage( new Gtk::Menu() ) ) );
	MENU_MOUNT = index++;

	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() );
	index++;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::MenuElem( "M_anage Flags",
						     sigc::mem_fun( *this, &Win_GParted::activate_manage_flags ) ) );
	MENU_FLAGS = index++;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::MenuElem( "C_heck",
						     sigc::mem_fun( *this, &Win_GParted::activate_check ) ) );
	MENU_CHECK = index++;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::MenuElem( "_Label",
						     sigc::mem_fun( *this, &Win_GParted::activate_label_partition ) ) );
	MENU_LABEL_PARTITION = index++;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::MenuElem( "New UU_ID",
						     sigc::mem_fun( *this, &Win_GParted::activate_change_uuid ) ) );
	MENU_CHANGE_UUID = index++;

	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() );
	index++;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::DIALOG_INFO,
							  sigc::mem_fun(*this, &Win_GParted::activate_info) ) );
	MENU_INFO = index++;

	menu_partition .accelerate( *this );
}

Gtk::Menu * Win_GParted::create_format_menu()
{
	menu = manage( new Gtk::Menu() );

	for ( unsigned int t =0; t < gparted_core .get_filesystems() .size(); t++ )
	{
		//Skip luks, lvm2, and unknown because these are not file systems
		if (
		     gparted_core .get_filesystems()[ t ] .filesystem == FS_LUKS    ||
		     gparted_core .get_filesystems()[ t ] .filesystem == FS_LVM2_PV ||
		     gparted_core .get_filesystems()[ t ] .filesystem == FS_UNKNOWN
		   )
			continue;

		hbox = manage( new Gtk::HBox() );

		//the colored square
		hbox ->pack_start( * manage( new Gtk::Image(
					Utils::get_color_as_pixbuf(
						gparted_core .get_filesystems()[ t ] .filesystem, 16, 16 ) ) ),
				   Gtk::PACK_SHRINK );

		//the label...
		hbox ->pack_start( * Utils::mk_label(
					" " +
					Utils::get_filesystem_string( gparted_core .get_filesystems()[ t ] .filesystem ) ),
				   Gtk::PACK_SHRINK );

		menu ->items() .push_back( * manage( new Gtk::MenuItem( *hbox ) ) );
		if ( gparted_core .get_filesystems()[ t ] .create )
			menu ->items() .back() .signal_activate() .connect(
				sigc::bind<FILESYSTEM>(sigc::mem_fun(*this, &Win_GParted::activate_format),
				gparted_core .get_filesystems()[ t ] .filesystem ) );
		else
			menu ->items() .back() .set_sensitive( false );
	}

	return menu;
}

void Win_GParted::init_device_info()
{
	vbox_info.set_spacing( 5 );
	int top = 0, bottom = 1;

	//title
	vbox_info .pack_start(
		* Utils::mk_label( " <b>" + static_cast<Glib::ustring>( "Device Information" ) + "</b>" ),
		Gtk::PACK_SHRINK );

	//GENERAL DEVICE INFO
	table = manage( new Gtk::Table() );
	table ->set_col_spacings( 10 );

	//model
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( "Model:" ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) );
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL );

	//size
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( "Size:" ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) );
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL );

	//path
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( "Path:" ) + "</b>",
					   true,
					   Gtk::ALIGN_LEFT,
					   Gtk::ALIGN_TOP ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) );
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL );

	vbox_info .pack_start( *table, Gtk::PACK_SHRINK );

	//DETAILED DEVICE INFO
	top = 0; bottom = 1;
	table = manage( new Gtk::Table() );
	table ->set_col_spacings( 10 );

	//one blank line
	table ->attach( * Utils::mk_label( "" ), 1, 2, top++, bottom++, Gtk::FILL );

	//disktype
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( "Partition table:" ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) );
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL );

	//heads
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( "Heads:" ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) );
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL );

	//sectors/track
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( "Sectors/track:" ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) );
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL );

	//cylinders
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( "Cylinders:" ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) );
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL );

	//total sectors
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( "Total sectors:" ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) );
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL );

	//sector size
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( "Sector size:" ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) );
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL );

	vbox_info .pack_start( *table, Gtk::PACK_SHRINK );
}

void Win_GParted::init_hpaned_main()
{
	//left scrollwindow (holds device info)
	scrollwindow = manage( new Gtk::ScrolledWindow() );
	scrollwindow ->set_shadow_type( Gtk::SHADOW_ETCHED_IN );
	scrollwindow ->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );

	hpaned_main .pack1( *scrollwindow, true, true );
	scrollwindow ->add( vbox_info );

	//right scrollwindow (holds treeview with partitions)
	scrollwindow = manage( new Gtk::ScrolledWindow() );
	scrollwindow ->set_shadow_type( Gtk::SHADOW_ETCHED_IN );
	scrollwindow ->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );

	//connect signals and add treeview_detail
	treeview_detail .signal_partition_selected .connect( sigc::mem_fun( this, &Win_GParted::on_partition_selected ) );
	treeview_detail .signal_partition_activated .connect( sigc::mem_fun( this, &Win_GParted::on_partition_activated ) );
	treeview_detail .signal_popup_menu .connect( sigc::mem_fun( this, &Win_GParted::on_partition_popup_menu ) );
	scrollwindow ->add( treeview_detail );
	hpaned_main .pack2( *scrollwindow, true, true );
}

void Win_GParted::refresh_combo_devices()
{
	liststore_devices ->clear();

	menu = manage( new Gtk::Menu() );
	Gtk::RadioButtonGroup radio_group;

	for ( unsigned int i = 0; i < devices .size( ); i++ )
	{
		//combo...
		treerow = *( liststore_devices ->append() );
		treerow[ treeview_devices_columns .icon ] =
			render_icon( Gtk::Stock::HARDDISK, Gtk::ICON_SIZE_LARGE_TOOLBAR );
		treerow[ treeview_devices_columns .device ] = devices[ i ] .get_path();
		treerow[ treeview_devices_columns .size ] = "(" + Utils::format_size( devices[ i ] .length, devices[ i ] .sector_size ) + ")";

		//devices submenu....
		hbox = manage( new Gtk::HBox() );
		hbox ->pack_start( * Utils::mk_label( devices[ i ] .get_path() ), Gtk::PACK_SHRINK );
		hbox ->pack_start( * Utils::mk_label( "   (" + Utils::format_size( devices[ i ] .length, devices[ i ] .sector_size ) + ")",
					              true,
						      Gtk::ALIGN_RIGHT ),
				   Gtk::PACK_EXPAND_WIDGET );

		menu ->items() .push_back( * manage( new Gtk::RadioMenuItem( radio_group ) ) );
		menu ->items() .back() .add( *hbox );
		menu ->items() .back() .signal_activate() .connect(
			sigc::bind<unsigned int>( sigc::mem_fun(*this, &Win_GParted::radio_devices_changed), i ) );
	}

	menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .remove_submenu();

	if ( menu ->items() .size() )
	{
		menu ->show_all();
		menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .set_submenu( *menu );
	}

	combo_devices .set_active( current_device );
}

void Win_GParted::show_pulsebar( const Glib::ustring & status_message )
{
	pulsebar .show();
	statusbar .push( status_message);

	//disable all input stuff
	toolbar_main .set_sensitive( false );
	menubar_main .set_sensitive( false );
	combo_devices .set_sensitive( false );
	menu_partition .set_sensitive( false );
	treeview_detail .set_sensitive( false );
	drawingarea_visualdisk .set_sensitive( false );

	//the actual 'pulsing'
	while ( pulse )
	{
		pulsebar .pulse();
		while ( Gtk::Main::events_pending() )
			Gtk::Main::iteration();
		gdk_threads_leave();
		usleep( 100000 );
		gdk_threads_enter();
		Glib::ustring tmp_msg = gparted_core .get_thread_status_message();
		if ( tmp_msg != "" )
			statusbar .push( tmp_msg );
	}

	thread ->join();

	pulsebar .hide();
	statusbar .pop();

	//enable all disabled stuff
	toolbar_main .set_sensitive( true );
	menubar_main .set_sensitive( true );
	combo_devices .set_sensitive( true );
	menu_partition .set_sensitive( true );
	treeview_detail .set_sensitive( true );
	drawingarea_visualdisk .set_sensitive( true );
}

void Win_GParted::Fill_Label_Device_Info( bool clear )
{
	if ( clear )
		for ( unsigned int t = 0; t < device_info .size( ); t++ )
			device_info[ t ] ->set_text( "" );

	else
	{
		short t = 0;

		//global info...
		device_info[ t++ ] ->set_text( devices[ current_device ] .model );
		device_info[ t++ ] ->set_text( Utils::format_size( devices[ current_device ] .length, devices[ current_device ] .sector_size ) );
		device_info[ t++ ] ->set_text( Glib::build_path( "\n", devices[ current_device ] .get_paths() ) );

		//detailed info
		device_info[ t++ ] ->set_text( devices[ current_device ] .disktype );
		device_info[ t++ ] ->set_text( Utils::num_to_str( devices[ current_device ] .heads ) );
		device_info[ t++ ] ->set_text( Utils::num_to_str( devices[ current_device ] .sectors ) );
		device_info[ t++ ] ->set_text( Utils::num_to_str( devices[ current_device ] .cylinders ) );
		device_info[ t++ ] ->set_text( Utils::num_to_str( devices[ current_device ] .length ) );
		device_info[ t++ ] ->set_text( Utils::num_to_str( devices[ current_device ] .sector_size ) );
	}
}

bool Win_GParted::on_delete_event( GdkEventAny *event )
{
	return ! Quit_Check_Operations();
}

void Win_GParted::Add_Operation( Operation * operation, int index )
{
	if ( operation )
	{
		Glib::ustring error;
		//FIXME: this is becoming a mess.. maybe it's better to check if partition_new > 0
		if ( operation ->type == OPERATION_DELETE ||
		     operation ->type == OPERATION_FORMAT ||
		     operation ->type == OPERATION_CHECK ||
		     operation ->type == OPERATION_CHANGE_UUID ||
		     operation ->type == OPERATION_LABEL_PARTITION ||
		     gparted_core .snap_to_alignment( operation ->device, operation ->partition_new, error )
		   )
		{
			operation ->create_description();

			if ( index >= 0 && index < static_cast<int>( operations .size() ) )
				operations .insert( operations .begin() + index, operation );
			else
				operations .push_back( operation );

			allow_undo_clear_apply( true );
			Refresh_Visual();

			if ( operations .size() == 1 ) //first operation, open operationslist
				open_operationslist();

			//FIXME:  A slight flicker may be introduced by this extra display refresh.
			//An extra display refresh seems to prevent the disk area visual disk from
			//  disappearing when enough operations are added to require a scrollbar
			//  (about 4 operations with default window size).
			//  Note that commenting out the code to
			//  "//make scrollwindow focus on the last operation in the list"
			//  in HBoxOperations::load_operations() prevents this problem from occurring as well.
			//  See also Win_GParted::activate_undo().
			drawingarea_visualdisk .queue_draw();
		}
		else
		{
			Gtk::MessageDialog dialog( *this,
				   "Could not add this operation to the list.",
				   false,
				   Gtk::MESSAGE_ERROR,
				   Gtk::BUTTONS_OK,
				   true );
			dialog .set_secondary_text( error );

			dialog .run();
		}
	}
}

bool Win_GParted::Merge_Operations( unsigned int first, unsigned int second )
{
	if( first >= operations .size() || second >= operations .size() )
		return false;

	// Two resize operations of the same partition
	if ( operations[ first ]->type == OPERATION_RESIZE_MOVE &&
	     operations[ second ]->type == OPERATION_RESIZE_MOVE &&
	     operations[ first ]->partition_new == operations[ second ]->partition_original
	   )
	{
		operations[ first ]->partition_new = operations[ second ]->partition_new;
		operations[ first ]->create_description();
		remove_operation( second );

		Refresh_Visual();

		return true;
	}
	// Two label change operations on the same partition
	else if ( operations[ first ]->type == OPERATION_LABEL_PARTITION &&
	          operations[ second ]->type == OPERATION_LABEL_PARTITION &&
	          operations[ first ]->partition_new == operations[ second ]->partition_original
	        )
	{
		operations[ first ]->partition_new.label = operations[ second ]->partition_new.label;
		operations[ first ]->create_description();
		remove_operation( second );

		Refresh_Visual();

		return true;
	}
	// Two change-uuid change operations on the same partition
	else if ( operations[ first ]->type == OPERATION_CHANGE_UUID &&
	          operations[ second ]->type == OPERATION_CHANGE_UUID &&
	          operations[ first ]->partition_new == operations[ second ]->partition_original
	        )
	{
		// Changing half the UUID should not override changing all of it
		if ( operations[ first ]->partition_new.uuid == UUID_RANDOM_NTFS_HALF ||
		     operations[ second ]->partition_new.uuid == UUID_RANDOM )
			operations[ first ]->partition_new.uuid = operations[ second ]->partition_new.uuid;
		operations[ first ]->create_description();
		remove_operation( second );

		Refresh_Visual();

		return true;
	}
	// Two check operations of the same partition
	else if ( operations[ first ]->type == OPERATION_CHECK &&
	          operations[ second ]->type == OPERATION_CHECK &&
	          operations[ first ]->partition_original == operations[ second ]->partition_original
	        )
	{
		remove_operation( second );

		Refresh_Visual();

		return true;
	}
	// Two format operations of the same partition
	else if ( operations[ first ]->type == OPERATION_FORMAT &&
	          operations[ second ]->type == OPERATION_FORMAT &&
	          operations[ first ]->partition_new == operations[ second ]->partition_original
	        )
	{
		operations[ first ]->partition_new = operations[ second ]->partition_new;
		operations[ first ]->create_description();
		remove_operation( second );

		Refresh_Visual();

		return true;
	}

	return false;
}

void Win_GParted::Refresh_Visual()
{
	std::vector<Partition> partitions = devices[ current_device ] .partitions;

	//make all operations visible
	for ( unsigned int t = 0; t < operations .size(); t++ )
		if ( operations[ t ] ->device == devices[ current_device ] )
			operations[ t ] ->apply_to_visual( partitions );

	hbox_operations .load_operations( operations );

	//set new statusbartext
	statusbar .pop();
	statusbar .push( String::ucompose(  "%1 operation pending"
	                                           , "%1 operations pending"
	                                           , operations .size()
	                                           )
	                                 , operations .size()
	                                 );

	if ( ! operations .size() )
		allow_undo_clear_apply( false );

	//count primary's and check for extended
	index_extended = -1;
	primary_count = 0;
	for ( unsigned int t = 0; t < partitions .size(); t++ )
	{
		if ( partitions[ t ] .get_path() == copied_partition .get_path() )
			copied_partition = partitions[ t ];

		switch ( partitions[ t ] .type )
		{
			case TYPE_PRIMARY	:
				primary_count++;
				break;

			case TYPE_EXTENDED	:
				index_extended = t;
				primary_count++;
				break;

			default				:
				break;
		}
	}

	//frame visualdisk
	drawingarea_visualdisk .load_partitions( partitions, devices[ current_device ] .length );

	//treeview details
	treeview_detail .load_partitions( partitions );

	//no partition can be selected after a refresh..
	selected_partition .Reset();
	set_valid_operations();

	while ( Gtk::Main::events_pending() )
		Gtk::Main::iteration();
}

bool Win_GParted::Quit_Check_Operations()
{
	if ( operations .size() )
	{
		Gtk::MessageDialog dialog( *this,
					   "Quit GParted?",
					   false,
					   Gtk::MESSAGE_QUESTION,
					   Gtk::BUTTONS_NONE,
					   true );

		dialog .set_secondary_text( String::ucompose( "%1 operation is currently pending."
		                                                      , "%1 operations are currently pending."
		                                                      , operations .size()
		                                            , operations .size()
		                                            )
		                          );

		dialog .add_button( Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE );
		dialog .add_button( Gtk::Stock::CANCEL,Gtk::RESPONSE_CANCEL );

		if ( dialog .run() == Gtk::RESPONSE_CANCEL )
			return false;//don't close GParted
	}

	return true; //close GParted
}

void Win_GParted::set_valid_operations()
{
	allow_new( false ); allow_delete( false ); allow_resize( false ); allow_copy( false );
	allow_paste( false ); allow_format( false ); allow_toggle_swap_mount_state( false );
	allow_manage_flags( false ); allow_check( false ); allow_label_partition( false );
	allow_change_uuid( false ); allow_info( false );

	dynamic_cast<Gtk::Label*>( menu_partition .items()[ MENU_TOGGLE_MOUNT_SWAP ] .get_child() )
		->set_label( FileSystem::get_generic_text ( CTEXT_DEACTIVATE_FILESYSTEM ) );

	menu_partition .items()[ MENU_TOGGLE_MOUNT_SWAP ] .show();
	menu_partition .items()[ MENU_MOUNT ] .hide();

	//no partition selected...
	if ( ! selected_partition .get_paths() .size() )
		return;

	//if there's something, there's some info;)
	allow_info( true );

	//flag managing..
	if ( selected_partition .type != TYPE_UNALLOCATED && selected_partition .status == STAT_REAL )
		allow_manage_flags( true );

	//Activate / deactivate
	if ( gparted_core .get_filesystem_object ( selected_partition .filesystem ) )
		dynamic_cast<Gtk::Label*>( menu_partition .items()[ MENU_TOGGLE_MOUNT_SWAP ] .get_child() )
			->set_label( gparted_core .get_filesystem_object ( selected_partition .filesystem )
			             ->get_custom_text (  selected_partition .busy
			                                ? CTEXT_DEACTIVATE_FILESYSTEM
			                                : CTEXT_ACTIVATE_FILESYSTEM
			                               )
			           );
	else
		dynamic_cast<Gtk::Label*>( menu_partition .items()[ MENU_TOGGLE_MOUNT_SWAP ] .get_child() )
			->set_label( FileSystem::get_generic_text (  selected_partition .busy
			                                           ? CTEXT_DEACTIVATE_FILESYSTEM
			                                           : CTEXT_ACTIVATE_FILESYSTEM )
			                                          );

	//Only permit mount/unmount, swapon/swapoff, ... if action is available
	if (    selected_partition .status == STAT_REAL
	     && selected_partition .type != TYPE_EXTENDED
	     && selected_partition .filesystem != FS_LVM2_PV
	     && (    selected_partition .busy
	          || selected_partition .get_mountpoints() .size() /* Have mount point(s) */
	          || selected_partition .filesystem == FS_LINUX_SWAP
	        )
	   )
		allow_toggle_swap_mount_state( true );

	//only unmount/swapoff/... is allowed if busy
	if ( selected_partition .busy )
		return;

	//UNALLOCATED
	if ( selected_partition .type == TYPE_UNALLOCATED )
	{
		allow_new( true );

		//find out if there is a copied partition and if it fits inside this unallocated space
		if ( ! copied_partition .get_path() .empty() && ! devices[ current_device ] .readonly )
		{
			Byte_Value required_size;
			if ( copied_partition .filesystem == FS_XFS )
				required_size = copied_partition .sectors_used * copied_partition .sector_size;
			else
				required_size = copied_partition .get_byte_length();

			//Determine if space is needed for the Master Boot Record or
			//  the Extended Boot Record.  Generally an an additional track or MEBIBYTE
			//  is required so for our purposes reserve a MEBIBYTE in front of the partition.
			//  NOTE:  This logic also contained in Dialog_Base_Partition::MB_Needed_for_Boot_Record
			if (   (   selected_partition .inside_extended
			        && selected_partition .type == TYPE_UNALLOCATED
			       )
			    || ( selected_partition .type == TYPE_LOGICAL )
			                                     /* Beginning of disk device */
			    || ( selected_partition .sector_start <= (MEBIBYTE / selected_partition .sector_size) )
			   )
				required_size += MEBIBYTE;

			//Determine if space is needed for the Extended Boot Record for a logical partition
			//  after this partition.  Generally an an additional track or MEBIBYTE
			//  is required so for our purposes reserve a MEBIBYTE in front of the partition.
			if (   (   (   selected_partition .inside_extended
			            && selected_partition .type == TYPE_UNALLOCATED
			           )
			        || ( selected_partition .type == TYPE_LOGICAL )
			       )
			    && ( selected_partition .sector_end
			         < ( devices[ current_device ] .length
			             - ( 2 * MEBIBYTE / devices[ current_device ] .sector_size )
			           )
			       )
			   )
				required_size += MEBIBYTE;

			//Determine if space is needed for the backup partition on a GPT partition table
			if (   ( devices[ current_device ] .disktype == "gpt" )
			    && ( ( devices[ current_device ] .length - selected_partition .sector_end )
			         < ( MEBIBYTE / devices[ current_device ] .sector_size )
			       )
			   )
				required_size += MEBIBYTE;

			if ( required_size <= selected_partition .get_byte_length() )
				allow_paste( true );
		}

		return;
	}

	//EXTENDED
	if ( selected_partition .type == TYPE_EXTENDED )
	{
		//deletion is only allowed when there are no logical partitions inside.
		if ( selected_partition .logicals .size() == 1 &&
		     selected_partition .logicals .back() .type == TYPE_UNALLOCATED )
			allow_delete( true );

		if ( ! devices[ current_device ] .readonly )
			allow_resize( true );

		return;
	}

	//PRIMARY and LOGICAL
	if (  selected_partition .type == TYPE_PRIMARY || selected_partition .type == TYPE_LOGICAL )
	{
		fs = gparted_core .get_fs( selected_partition .filesystem );

		allow_delete( true );
		allow_format( true );

		//find out if resizing/moving is possible
		if ( (fs .grow || fs .shrink || fs .move ) && ! devices[ current_device ] .readonly )
			allow_resize( true );

		//only allow copying of real partitions
		if ( selected_partition .status == STAT_REAL && fs .copy )
			allow_copy( true );

		//only allow labelling of real partitions that support labelling
		if ( selected_partition .status == STAT_REAL && fs .write_label )
			allow_label_partition( true );

		//only allow changing UUID of real partitions that support it
		if ( selected_partition .status == STAT_REAL && fs .write_uuid )
			allow_change_uuid( true );

		//Generate Mount on submenu, except for LVM2 PVs
		//  borrowing mount point to display the VGNAME
		if (   selected_partition .filesystem != FS_LVM2_PV
		    && selected_partition .get_mountpoints() .size()
		   )
		{
			menu = menu_partition .items()[ MENU_MOUNT ] .get_submenu();
			menu ->items() .clear();
			for ( unsigned int t = 0; t < selected_partition .get_mountpoints() .size(); t++ )
			{
				menu ->items() .push_back(
					Gtk::Menu_Helpers::MenuElem(
						selected_partition .get_mountpoints()[ t ],
						sigc::bind<unsigned int>( sigc::mem_fun(*this, &Win_GParted::activate_mount_partition), t ) ) );

				dynamic_cast<Gtk::Label*>( menu ->items() .back() .get_child() ) ->set_use_underline( false );
			}

			menu_partition .items()[ MENU_TOGGLE_MOUNT_SWAP ] .hide();
			menu_partition .items()[ MENU_MOUNT ] .show();
		}

		//see if there is an copied partition and if it passes all tests
		if ( ! copied_partition .get_path() .empty() &&
		     copied_partition .get_byte_length() <= selected_partition .get_byte_length() &&
		     selected_partition .status == STAT_REAL &&
		     copied_partition != selected_partition )
		     allow_paste( true );

		//see if we can somehow check/repair this file system....
		if ( fs .check && selected_partition .status == STAT_REAL )
			allow_check( true );
	}
}

void Win_GParted::open_operationslist()
{
	if ( ! OPERATIONSLIST_OPEN )
	{
		OPERATIONSLIST_OPEN = true;
		hbox_operations .show();

		for ( int t = vpaned_main .get_height(); t > ( vpaned_main .get_height() - 100 ); t -= 5 )
		{
			vpaned_main .set_position( t );
			while ( Gtk::Main::events_pending() )
				Gtk::Main::iteration();
		}

		static_cast<Gtk::CheckMenuItem *>( & menubar_main .items()[ 2 ] .get_submenu() ->items()[ 1 ] )
			->set_active( true );
	}
}

void Win_GParted::close_operationslist()
{
	if ( OPERATIONSLIST_OPEN )
	{
		OPERATIONSLIST_OPEN = false;

		for ( int t = vpaned_main .get_position(); t < vpaned_main .get_height(); t += 5 )
		{
			vpaned_main .set_position( t );

			while ( Gtk::Main::events_pending() )
				Gtk::Main::iteration();
		}

		hbox_operations .hide();

		static_cast<Gtk::CheckMenuItem *>( & menubar_main .items()[ 2 ] .get_submenu() ->items()[ 1 ] )
			->set_active( false );
	}
}

void Win_GParted::clear_operationslist()
{
	remove_operation( -1, true );
	close_operationslist();

	Refresh_Visual();
}

void Win_GParted::combo_devices_changed()
{
	unsigned int old_current_device = current_device;
	//set new current device
	current_device = combo_devices .get_active_row_number();
	if ( current_device == (unsigned int) -1 )
		current_device = old_current_device;
	if ( current_device >= devices .size() )
		current_device = 0;
	set_title( String::ucompose( "%1 - GParted", devices[ current_device ] .get_path() ) );

	//refresh label_device_info
	Fill_Label_Device_Info();

	//rebuild visualdisk and treeview
	Refresh_Visual();

	//uodate radiobuttons..
	if ( menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .get_submenu() )
		static_cast<Gtk::RadioMenuItem *>(
			& menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .get_submenu() ->
			items()[ current_device ] ) ->set_active( true );
}

void Win_GParted::radio_devices_changed( unsigned int item )
{
	if ( static_cast<Gtk::RadioMenuItem *>(
		& menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .get_submenu() ->
		items()[ item ] ) ->get_active() )
	{
		combo_devices .set_active( item );
	}
}

void Win_GParted::on_show()
{
	Gtk::Window::on_show();

	vpaned_main .set_position( vpaned_main .get_height() );
	close_operationslist();

	menu_gparted_refresh_devices();
}

void Win_GParted::thread_refresh_devices()
{
	gparted_core .set_devices( devices );
	pulse = false;
}

void Win_GParted::menu_gparted_refresh_devices()
{
	pulse = true;
	thread = Glib::Thread::create( sigc::mem_fun( *this, &Win_GParted::thread_refresh_devices ), true );

	show_pulsebar( "Scanning all devices..." );

	//check if current_device is still available (think about hotpluggable stuff like usbdevices)
	if ( current_device >= devices .size() )
		current_device = 0;

	//see if there are any pending operations on non-existent devices
	//NOTE that this isn't 100% foolproof since some stuff (e.g. sourcedevice of copy) may slip through.
	//but anyone who removes the sourcedevice before applying the operations gets what he/she deserves :-)
	//FIXME: this actually sucks;) see if we can use STL predicates here..
	unsigned int i;
	for ( unsigned int t = 0; t < operations .size(); t++ )
	{
		for ( i = 0; i < devices .size() && devices[ i ] != operations[ t ] ->device; i++ ) {}

		if ( i >= devices .size() )
			remove_operation( t-- );
	}

	//if no devices were detected we disable some stuff and show a message in the statusbar
	if ( devices .empty() )
	{
		this ->set_title( "GParted" );
		combo_devices .hide();

		menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .set_sensitive( false );
		menubar_main .items()[ 1 ] .set_sensitive( false );
		menubar_main .items()[ 2 ] .set_sensitive( false );
		menubar_main .items()[ 3 ] .set_sensitive( false );
		menubar_main .items()[ 4 ] .set_sensitive( false );
		toolbar_main .set_sensitive( false );
		drawingarea_visualdisk .set_sensitive( false );
		treeview_detail .set_sensitive( false );

		Fill_Label_Device_Info( true );

		drawingarea_visualdisk .clear();
		treeview_detail .clear();

		//hmzz, this is really paranoid, but i think it's the right thing to do;)
		hbox_operations .clear();
		close_operationslist();
		remove_operation( -1, true );

		statusbar .pop();
		statusbar .push(  "No devices detected"  );
	}
	else //at least one device detected
	{
		combo_devices .show();

		menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .set_sensitive( true );
		menubar_main .items()[ 1 ] .set_sensitive( true );
		menubar_main .items()[ 2 ] .set_sensitive( true );
		menubar_main .items()[ 3 ] .set_sensitive( true );
		menubar_main .items()[ 4 ] .set_sensitive( true );

		toolbar_main .set_sensitive( true );
		drawingarea_visualdisk .set_sensitive( true );
		treeview_detail .set_sensitive( true );

		refresh_combo_devices();
	}
}

void Win_GParted::menu_gparted_features()
{
}

void Win_GParted::menu_gparted_quit()
{
	if ( Quit_Check_Operations() )
		this ->hide();
}

void Win_GParted::menu_view_harddisk_info()
{
}

void Win_GParted::menu_view_operations()
{
	if ( static_cast<Gtk::CheckMenuItem *>( & menubar_main .items()[ 2 ] .get_submenu() ->items()[ 1 ] ) ->get_active() )
		open_operationslist();
	else
		close_operationslist();
}

void Win_GParted::show_disklabel_unrecognized ( Glib::ustring device_name )
{
}

void Win_GParted::show_help_dialog( const Glib::ustring & filename /* E.g., gparted */
                                  , const Glib::ustring & link_id  /* For context sensitive help */
                                  )
{
}

void Win_GParted::menu_help_contents()
{
}

void Win_GParted::menu_help_about()
{
}

void Win_GParted::on_partition_selected( const Partition & partition, bool src_is_treeview )
{
	selected_partition = partition;

	set_valid_operations();

	if ( src_is_treeview )
		drawingarea_visualdisk .set_selected( partition );
	else
		treeview_detail .set_selected( partition );
}

void Win_GParted::on_partition_activated()
{
	activate_info();
}

void Win_GParted::on_partition_popup_menu( unsigned int button, unsigned int time )
{
	menu_partition .popup( button, time );
}

bool Win_GParted::max_amount_prim_reached()
{
	return false;
}

void Win_GParted::activate_resize()
{
}

void Win_GParted::activate_copy()
{
	copied_partition = selected_partition;
}

void Win_GParted::activate_paste()
{
}

void Win_GParted::activate_new()
{
}

void Win_GParted::activate_delete()
{
}

void Win_GParted::activate_info()
{
}

void Win_GParted::activate_format( FILESYSTEM new_fs )
{
}

void Win_GParted::thread_unmount_partition( bool * succes, Glib::ustring * error )
{
}

void Win_GParted::thread_mount_partition( Glib::ustring mountpoint, bool * succes, Glib::ustring * error )
{
}

void Win_GParted::thread_toggle_swap( bool * succes, Glib::ustring * error )
{
}

void Win_GParted::thread_guess_partition_table()
{
	this->gpart_output="";
	this->gparted_core.guess_partition_table(devices[ current_device ], this->gpart_output);
	pulse=false;
}

void Win_GParted::toggle_swap_mount_state()
{
}

void Win_GParted::activate_mount_partition( unsigned int index )
{
}

void Win_GParted::activate_disklabel()
{
}

void Win_GParted::activate_attempt_rescue_data()
{
}

void Win_GParted::activate_manage_flags()
{
}

void Win_GParted::activate_check()
{
}

void Win_GParted::activate_label_partition()
{
}

void Win_GParted::activate_change_uuid()
{
}

void Win_GParted::activate_undo()
{
}

void Win_GParted::remove_operation( int index, bool remove_all )
{
	if ( remove_all )
	{
		for ( unsigned int t = 0; t < operations .size(); t++ )
			delete operations[ t ];

		operations .clear();
	}
	else if ( index == -1  && operations .size() > 0 )
	{
		delete operations .back();
		operations .pop_back();
	}
	else if ( index > -1 && index < static_cast<int>( operations .size() ) )
	{
		delete operations[ index ];
		operations .erase( operations .begin() + index );
	}
}

int Win_GParted::partition_in_operation_queue_count( const Partition & partition )
{
	int operation_count = 0;

	for ( unsigned int t = 0; t < operations .size(); t++ )
	{
		if ( partition .get_path() == operations[ t ] ->partition_original .get_path() )
			operation_count++;
	}

	return operation_count;
}

int  Win_GParted::active_partitions_on_device_count( const Device & device )
{
	int active_count = 0;

	//Count the active partitions on the device
	for ( unsigned int k=0; k < device .partitions .size(); k++ )
	{
		//FIXME:  Should also count other types of active partitions, such as LVM2 when we know how.

		//Count the active primary partitions
		if (   device .partitions[ k ] .busy
		    && device .partitions[ k ] .type != TYPE_EXTENDED
		    && device .partitions[ k ] .type != TYPE_UNALLOCATED
		   )
			active_count++;

		//Count the active logical partitions
		if (   device .partitions[ k ] .busy
		    && device .partitions[ k ] .type == TYPE_EXTENDED
		   )
		{
			for ( unsigned int j=0; j < device .partitions[ k ] .logicals .size(); j++ )
			{
				if (   device .partitions[ k ] .logicals [ j ] .busy
				    && device .partitions[ k ] .logicals [ j ] .type != TYPE_UNALLOCATED
				   )
					active_count++;
			}
		}
	}

	return active_count;
}

void Win_GParted::activate_apply()
{
}

