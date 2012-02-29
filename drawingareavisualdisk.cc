#include "drawingareavisualdisk.h"

#define MAIN_BORDER 5
#define BORDER 4
#define SEP 4
#define HEIGHT 70 + 2 * MAIN_BORDER

/**
 * DrawingAreaVisualDisk
 */
DrawingAreaVisualDisk::DrawingAreaVisualDisk()
{
	selected_vp = NULL;

	//set and allocated some standard colors
	color_used.set (Utils::get_color (FS_USED));
	get_colormap()->alloc_color (color_used);

	color_unused.set (Utils::get_color (FS_UNUSED));
	get_colormap()->alloc_color (color_unused);

	color_text.set ("black");
	get_colormap()->alloc_color (color_text);

	add_events (Gdk::BUTTON_PRESS_MASK);

	set_size_request (-1, HEIGHT);
}

/**
 * load_partitions
 */
void DrawingAreaVisualDisk::load_partitions (const std::vector<Partition> & partitions, Sector device_length)
{
	clear();

	TOT_SEP = get_total_separator_px (partitions);
	set_static_data (partitions, visual_partitions, device_length);

	queue_resize();
}

/**
 * set_selected
 */
void DrawingAreaVisualDisk::set_selected (const Partition & partition)
{
	selected_vp = NULL;
	set_selected (visual_partitions, partition);

	queue_draw();
}

/**
 * clear
 */
void DrawingAreaVisualDisk::clear()
{
	free_colors (visual_partitions);
	visual_partitions.clear();
	selected_vp = NULL;

	queue_resize();
}

/**
 * get_total_separator_px
 */
int DrawingAreaVisualDisk::get_total_separator_px (const std::vector<Partition> & partitions)
{
	for (unsigned int t = 0; t < partitions.size(); t++)
		if (partitions[ t ].type == TYPE_EXTENDED)
			return (partitions[ t ].logicals.size() -1 + partitions.size() -1) * SEP;

	return (partitions.size() -1) * SEP;
}

/**
 * set_static_data
 */
void DrawingAreaVisualDisk::set_static_data (const std::vector<Partition> & partitions,
					     std::vector<visual_partition> & visual_partitions,
					     Sector length)
{
	for (unsigned int t = 0; t < partitions.size(); t++)
	{
		visual_partitions.push_back (visual_partition());

		visual_partitions.back().partition = partitions[ t ];
		visual_partitions.back().fraction = partitions[ t ].get_sector_length() / static_cast<double> (length);

		if (partitions[ t ].type == TYPE_UNALLOCATED || partitions[ t ].type == TYPE_EXTENDED)
			visual_partitions.back().fraction_used = -1;
		else if (partitions[ t ].sectors_used > 0)
			visual_partitions.back().fraction_used =
				partitions[ t ].sectors_used / static_cast<double> (partitions[ t ].get_sector_length());

		visual_partitions.back().color = partitions[ t ].color;
		get_colormap()->alloc_color (visual_partitions.back().color);

		if (partitions[ t ].type == TYPE_EXTENDED)
			set_static_data (partitions[ t ].logicals,
					 visual_partitions.back().logicals,
					 partitions[ t ].get_sector_length());
		else
			visual_partitions.back().pango_layout = create_pango_layout(
				partitions[ t ].get_path() + "\n" + Utils::format_size (partitions[ t ].get_sector_length(), partitions[ t ].sector_size));
	}
}

/**
 * calc_length
 */
int DrawingAreaVisualDisk::calc_length (std::vector<visual_partition> & visual_partitions, int length_px)
{
	int calced_length = 0;

	for (int t = 0; t < static_cast<int> (visual_partitions.size()); t++)
	{
		visual_partitions[ t ].length = Utils::round (length_px * visual_partitions[ t ].fraction);

		if (visual_partitions[ t ].logicals.size() > 0)
			visual_partitions[ t ].length =
				calc_length (visual_partitions[ t ].logicals,
					     visual_partitions[ t ].length - (2 * BORDER)) + (2 * BORDER);
		else if (visual_partitions[ t ].length < MIN_SIZE)
			visual_partitions[ t ].length = MIN_SIZE;

		calced_length += visual_partitions[ t ].length;
	}

	return calced_length + (visual_partitions.size() - 1) * SEP;
}

/**
 * calc_position_and_height
 */
void DrawingAreaVisualDisk::calc_position_and_height (std::vector<visual_partition> & visual_partitions,
						      int start,
						      int border)
{
	for (unsigned int t = 0; t < visual_partitions.size(); t++)
	{
		visual_partitions[ t ].x_start = start;
		visual_partitions[ t ].y_start = border;
		visual_partitions[ t ].height = HEIGHT - (border * 2);

		if (visual_partitions[ t ].logicals.size() > 0)
			calc_position_and_height (visual_partitions[ t ].logicals,
						  visual_partitions[ t ].x_start + BORDER,
						  visual_partitions[ t ].y_start + BORDER);

		start += visual_partitions[ t ].length + SEP;
	}
}

/**
 * calc_used_unused
 */
void DrawingAreaVisualDisk::calc_used_unused (std::vector<visual_partition> & visual_partitions)
{
	for (unsigned int t = 0; t < visual_partitions.size(); t++)
	{
		if (visual_partitions[ t ].fraction_used > -1)
		{
			//used
			visual_partitions[ t ].x_used_start = visual_partitions[ t ].x_start + BORDER;

			if (visual_partitions[ t ].fraction_used)
				visual_partitions[ t ].used_length =
					Utils::round ((visual_partitions[ t ].length - (2*BORDER)) * visual_partitions[ t ].fraction_used);

			//unused
			visual_partitions[ t ].x_unused_start =
				visual_partitions[ t ].x_used_start + visual_partitions[ t ].used_length ;
			visual_partitions[ t ].unused_length =
				visual_partitions[ t ].length - (2 * BORDER) - visual_partitions[ t ].used_length;

			//y position and height
			visual_partitions[ t ].y_used_unused_start = visual_partitions[ t ].y_start + BORDER;
			visual_partitions[ t ].used_unused_height = visual_partitions[ t ].height - (2 * BORDER);
		}

		if (visual_partitions[ t ].logicals.size() > 0)
			calc_used_unused (visual_partitions[ t ].logicals);
	}
}

/**
 * calc_text
 */
void DrawingAreaVisualDisk::calc_text (std::vector<visual_partition> & visual_partitions)
{
	int length, height;

	for (unsigned int t = 0; t < visual_partitions.size(); t++)
	{
		if (visual_partitions[ t ].pango_layout)
		{
			//see if the text fits in the partition... (and if so, center the text..)
			visual_partitions[ t ].pango_layout->get_pixel_size (length, height);
			if (length < visual_partitions[ t ].length - (2 * BORDER) - 2)
			{
				visual_partitions[ t ].x_text = visual_partitions[ t ].x_start +
					Utils::round ((visual_partitions[ t ].length / 2) - (length / 2));

				visual_partitions[ t ].y_text = visual_partitions[ t ].y_start +
					Utils::round ((visual_partitions[ t ].height / 2) - (height / 2));
			}
			else
				visual_partitions[ t ].x_text = visual_partitions[ t ].y_text = 0;
		}

		if (visual_partitions[ t ].logicals.size() > 0)
			calc_text (visual_partitions[ t ].logicals);
	}
}

/**
 * draw_partition
 */
void DrawingAreaVisualDisk::draw_partition (const visual_partition & vp)
{
	//partition...
	gc->set_foreground (vp.color);
	get_window()->draw_rectangle (gc,
				       true,
				       vp.x_start,
				       vp.y_start,
				       vp.length,
				       vp.height);

	//used..
	if (vp.used_length > 0)
	{
		gc->set_foreground (color_used);
		get_window()->draw_rectangle (gc,
					       true,
					       vp.x_used_start,
					       vp.y_used_unused_start,
					       vp.used_length,
					       vp.used_unused_height);
	}

	//unused
	if (vp.unused_length > 0)
	{
		gc->set_foreground (color_unused);
		get_window()->draw_rectangle (gc,
					       true,
					       vp.x_unused_start,
					       vp.y_used_unused_start,
					       vp.unused_length,
					       vp.used_unused_height);
	}

	//text
	if (vp.x_text > 0)
	{
		gc->set_foreground (color_text);
		get_window()->draw_layout (gc,
					    vp.x_text,
					    vp.y_text,
					    vp.pango_layout);
	}
}

/**
 * draw_partitions
 */
void DrawingAreaVisualDisk::draw_partitions (const std::vector<visual_partition> & visual_partitions)
{
	for (unsigned int t = 0; t < visual_partitions.size(); t++)
	{
		draw_partition (visual_partitions[ t ]);

		if (visual_partitions[ t ].logicals.size() > 0)
			draw_partitions (visual_partitions[ t ].logicals);
	}
}

/**
 * set_selected
 */
void DrawingAreaVisualDisk::set_selected (const std::vector<visual_partition> & visual_partitions, int x, int y)
{
	for (unsigned int t = 0; t < visual_partitions.size() && ! selected_vp; t++)
	{
		if (visual_partitions[ t ].logicals.size() > 0 )
			set_selected (visual_partitions[ t ].logicals, x, y);

		if (! selected_vp &&
		     visual_partitions[ t ].x_start <= x &&
		     x < visual_partitions[ t ].x_start + visual_partitions[ t ].length &&
		     visual_partitions[ t ].y_start <= y &&
		     y < visual_partitions[ t ].y_start + visual_partitions[ t ].height)
		{
			selected_vp = & visual_partitions[ t ];
		}
	}
}

/**
 * set_selected
 */
void DrawingAreaVisualDisk::set_selected (const std::vector<visual_partition> & visual_partitions,
					  const Partition & partition)
{
	for (unsigned int t = 0; t < visual_partitions.size() && ! selected_vp; t++)
	{
		if (visual_partitions[ t ].logicals.size() > 0)
			set_selected (visual_partitions[ t ].logicals, partition);

		if (! selected_vp && visual_partitions[ t ].partition == partition)
			selected_vp = & visual_partitions[ t ];
	}
}

/**
 * on_realize
 */
void DrawingAreaVisualDisk::on_realize()
{
	Gtk::DrawingArea::on_realize();

	gc = Gdk::GC::create (get_window());
	gc->set_line_attributes (2,
				  Gdk::LINE_ON_OFF_DASH,
				  Gdk::CAP_BUTT,
				  Gdk::JOIN_MITER);
}

/**
 * on_expose_event
 */
bool DrawingAreaVisualDisk::on_expose_event (GdkEventExpose * event)
{
	bool ret_val = Gtk::DrawingArea::on_expose_event (event);

	draw_partitions (visual_partitions);

	//selection
	if (selected_vp)
	{
		gc->set_foreground (color_used);
		get_window()->draw_rectangle (gc,
					       false,
					       selected_vp->x_start + BORDER/2 ,
					       selected_vp->y_start + BORDER/2 ,
					       selected_vp->length - BORDER,
					       selected_vp->height - BORDER);
	}

	return ret_val;
}

/**
 * on_button_press_event
 */
bool DrawingAreaVisualDisk::on_button_press_event (GdkEventButton * event)
{
	bool ret_val = Gtk::DrawingArea::on_button_press_event (event);

	selected_vp = NULL;
	set_selected (visual_partitions, static_cast<int> (event->x), static_cast<int> (event->y));
	queue_draw();

	if (selected_vp)
	{
		signal_partition_selected.emit (selected_vp->partition, false);

		if (event->type == GDK_2BUTTON_PRESS)
			signal_partition_activated.emit();
		else if (event->button == 3)
			signal_popup_menu.emit (event->button, event->time);
	}

	return ret_val;
}

/**
 * on_size_allocate
 */
void DrawingAreaVisualDisk::on_size_allocate (Gtk::Allocation & allocation)
{
	Gtk::DrawingArea::on_size_allocate (allocation);

	MIN_SIZE = BORDER * 2 + 2;

	int available_size = allocation.get_width() - (2 * MAIN_BORDER),
	calced = 0,
	px_left;

	do
	{
		px_left = available_size - TOT_SEP;
		calced = available_size; //for first time :)
		do
		{
			px_left -= (calced - available_size);
			calced = calc_length (visual_partitions, px_left);
		}
		while (calced > available_size && px_left > 0);

		MIN_SIZE--;
	}
	while (px_left <= 0 && MIN_SIZE > 0);

	//due to rounding a few px may be lost. here we salvage them..
	if (visual_partitions.size() && calced > 0)
	{
		px_left = available_size - calced;

		while (px_left > 0)
			px_left = spreadout_leftover_px (visual_partitions, px_left);
	}

	//and calculate the rest..
	calc_position_and_height (visual_partitions, MAIN_BORDER, MAIN_BORDER);//0, 0);
	calc_used_unused (visual_partitions);
	calc_text (visual_partitions);
	queue_draw();
}

/**
 * spreadout_leftover_px
 */
int DrawingAreaVisualDisk::spreadout_leftover_px (std::vector<visual_partition> & visual_partitions, int pixels)
{
	int extended = -1;

	for (unsigned int t = 0; t < visual_partitions.size() && pixels > 0; t++)
		if (! visual_partitions[ t ].logicals.size())
		{
			visual_partitions[ t ].length++;
			pixels--;
		}
		else
			extended = t;

	if (extended > -1 && pixels > 0)
	{
		int actually_used = pixels - spreadout_leftover_px (visual_partitions[ extended ].logicals, pixels);
		visual_partitions[ extended ].length += actually_used;
		pixels -= actually_used;
	}

	return pixels;
}

/**
 * free_colors
 */
void DrawingAreaVisualDisk::free_colors (std::vector<visual_partition> & visual_partitions)
{
	for (unsigned int t = 0; t < visual_partitions.size(); t++)
	{
		get_colormap()->free_colors (visual_partitions[ t ].color, 1);

		if (visual_partitions[ t ].logicals.size() > 0)
			free_colors (visual_partitions[ t ].logicals);
	}
}

/**
 * ~DrawingAreaVisualDisk
 */
DrawingAreaVisualDisk::~DrawingAreaVisualDisk()
{
	clear();

	//free the allocated colors
	get_colormap()->free_colors (color_used, 1);
	get_colormap()->free_colors (color_unused, 1);
	get_colormap()->free_colors (color_text, 1);
}

