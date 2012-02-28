#include "win_gparted.h"

#include <gtkmm/messagedialog.h>
#include <gtkmm/main.h>
#include "gparted_core.h"

int main (int argc, char *argv[])
{
	//initialize thread system
	Glib::thread_init();
	gdk_threads_init();
	gdk_threads_enter();
	GParted_Core::mainthread = Glib::Thread::self();

	Gtk::Main kit (argc, argv);

	//deal with arguments..
	std::vector<Glib::ustring> user_devices;

	for (int t = 1; t < argc; t++)
		user_devices.push_back (argv[ t ]);

	Win_GParted win_gparted (user_devices);
	Gtk::Main::run (win_gparted);

	return 0;
}

