#ifndef DEVICE
#define DEVICE

#include "partition.h"

class Device
{

public:
	Device();
	~Device();

	void add_path (const Glib::ustring & path, bool clear_paths = false);
	void add_paths (const std::vector<Glib::ustring> & paths, bool clear_paths = false);
	Glib::ustring get_path() const;
	std::vector<Glib::ustring> get_paths() const;

	bool operator== (const Device & device) const;
	bool operator!= (const Device & device) const;

	void Reset();
	std::vector<Partition> partitions;
	Sector length;
	Sector heads;
	Sector sectors;
	Sector cylinders;
	Sector cylsize;
	Glib::ustring model;
	Glib::ustring disktype;
	int sector_size;
	int max_prims;
	int highest_busy;
	bool readonly;

private:
	void sort_paths_and_remove_duplicates();

	static bool compare_paths (const Glib::ustring & A, const Glib::ustring & B);

	std::vector<Glib::ustring> paths;

};

#endif //DEVICE

