#ifndef DRAWINGAREA_VISUALDISK
#define DRAWINGAREA_VISUALDISK

#include "partition.h"

#include <gtkmm/drawingarea.h>

class DrawingAreaVisualDisk : public Gtk::DrawingArea
{
public:
	DrawingAreaVisualDisk();
	~DrawingAreaVisualDisk();

	void load_partitions (const std::vector<Partition> & partitions, Sector device_length);
	void set_selected (const Partition & partition);
	void clear();

	//public signals for interclass communication
	sigc::signal< void, const Partition &, bool > signal_partition_selected;
	sigc::signal< void > signal_partition_activated;
	sigc::signal< void, unsigned int, unsigned int > signal_popup_menu;

private:
	struct visual_partition;

	//private functions
	int get_total_separator_px (const std::vector<Partition> & partitions);

	void set_static_data (const std::vector<Partition> & partitions,
			      std::vector<visual_partition> & visual_partitions,
			      Sector length);
	int calc_length (std::vector<visual_partition> & visual_partitions, int length_px);
	void calc_position_and_height (std::vector<visual_partition> & visual_partitions, int start, int border);
	void calc_used_unused (std::vector<visual_partition> & visual_partitions);
	void calc_text (std::vector<visual_partition> & visual_partitions);

	void draw_partition (const visual_partition & vp);
	void draw_partitions (const std::vector<visual_partition> & visual_partitions);

	void set_selected (const std::vector<visual_partition> & visual_partitions, int x, int y);
	void set_selected (const std::vector<visual_partition> & visual_partitions, const Partition & partition);

	int spreadout_leftover_px (std::vector<visual_partition> & visual_partitions, int pixels);
	void free_colors (std::vector<visual_partition> & visual_partitions);

	//overridden signalhandlers
	void on_realize();
	bool on_expose_event (GdkEventExpose * event);
	bool on_button_press_event (GdkEventButton * event);
	void on_size_allocate (Gtk::Allocation & allocation);

	//variables
	struct visual_partition
	{
		double fraction;
		double fraction_used;

		int x_start, length;
		int y_start, height;
		int x_used_start, used_length;
		int x_unused_start, unused_length;
		int y_used_unused_start, used_unused_height;
		int x_text, y_text;

		Gdk::Color color;
		Glib::RefPtr<Pango::Layout> pango_layout;

		//real partition
		Partition partition;

		std::vector<visual_partition> logicals;

		visual_partition()
		{
			fraction = fraction_used =
			x_start = length =
			y_start = height =
			x_used_start = used_length =
			x_unused_start = unused_length =
			y_used_unused_start = used_unused_height =
			x_text = y_text = 0;

			pango_layout.clear();
			logicals.clear();
		}

		~visual_partition()
		{
			pango_layout.clear();
			logicals.clear();
		}
	};

	std::vector<visual_partition> visual_partitions;
	const visual_partition * selected_vp;
	int TOT_SEP, MIN_SIZE;

	Glib::RefPtr<Gdk::GC> gc;
	Gdk::Color color_used, color_unused, color_text;
};

#endif //DRAWINGAREA_VISUALDISK
