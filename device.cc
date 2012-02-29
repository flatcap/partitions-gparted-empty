#include "device.h"

/**
 * Device
 */
Device::Device()
{
	Reset();
}

/**
 * Reset
 */
void Device::Reset()
{
	paths.clear();
	partitions.clear();
	length = cylsize = 0;
	heads = sectors = cylinders = 0;
	model = disktype = "";
	sector_size = max_prims = highest_busy = 0;
	readonly = false;
}

/**
 * add_path
 */
void Device::add_path (const Glib::ustring & path, bool clear_paths)
{
	if (clear_paths)
		paths.clear();

	paths.push_back (path);

	sort_paths_and_remove_duplicates();
}

/**
 * add_paths
 */
void Device::add_paths (const std::vector<Glib::ustring> & paths, bool clear_paths)
{
	if (clear_paths)
		this->paths.clear();

	this->paths.insert (this->paths.end(), paths.begin(), paths.end());

	sort_paths_and_remove_duplicates();
}

/**
 * get_path
 */
Glib::ustring Device::get_path() const
{
	if (paths.size() > 0)
		return paths.front();

	return "";
}

/**
 * get_paths
 */
std::vector<Glib::ustring> Device::get_paths() const
{
	return paths;
}

/**
 * operator==
 */
bool Device::operator== (const Device & device) const
{
	return this->get_path() == device.get_path();
}

/**
 * operator!=
 */
bool Device::operator!= (const Device & device) const
{
	return ! (*this == device);
}

/**
 * sort_paths_and_remove_duplicates
 */
void Device::sort_paths_and_remove_duplicates()
{
	//remove duplicates
	std::sort (paths.begin(), paths.end());
	paths.erase (std::unique (paths.begin(), paths.end()), paths.end());

	//sort on length
	std::sort (paths.begin(), paths.end(), compare_paths);
}

/**
 * compare_paths
 */
bool Device::compare_paths (const Glib::ustring & A, const Glib::ustring & B)
{
	return A.length() < B.length();
}

/**
 * ~Device
 */
Device::~Device()
{
}

