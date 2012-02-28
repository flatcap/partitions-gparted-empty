#include "proc_partitions_info.h"

#include <fstream>

//Initialize static data elements
bool Proc_Partitions_Info::proc_partitions_info_cache_initialized = false;
std::vector<Glib::ustring> Proc_Partitions_Info::device_paths_cache;
std::map< Glib::ustring, Glib::ustring > Proc_Partitions_Info::alternate_paths_cache;

Proc_Partitions_Info::Proc_Partitions_Info()
{
	//Ensure that cache has been loaded at least once
	if (! proc_partitions_info_cache_initialized)
	{
		proc_partitions_info_cache_initialized = true;
		load_proc_partitions_info_cache();
	}
}

Proc_Partitions_Info::Proc_Partitions_Info (bool do_refresh)
{
	//Ensure that cache has been loaded at least once
	if (! proc_partitions_info_cache_initialized)
	{
		proc_partitions_info_cache_initialized = true;
		if (do_refresh == false)
			load_proc_partitions_info_cache();
	}

	if (do_refresh)
		load_proc_partitions_info_cache();
}

Proc_Partitions_Info::~Proc_Partitions_Info()
{
}

std::vector<Glib::ustring> Proc_Partitions_Info::get_device_paths()
{
	return device_paths_cache;
}

std::vector<Glib::ustring> Proc_Partitions_Info::get_alternate_paths (const Glib::ustring & path)
{
	std::vector<Glib::ustring> paths;
	std::map< Glib::ustring, Glib::ustring >::iterator iter;

	iter = alternate_paths_cache.find (path);
	if (iter != alternate_paths_cache.end())
		paths.push_back (iter->second);

	return paths;
}

//Private Methods
void Proc_Partitions_Info::load_proc_partitions_info_cache()
{
	alternate_paths_cache.clear();
	device_paths_cache.clear();

	//Initialize alternate_paths
	std::ifstream proc_partitions ("/proc/partitions");
	if (proc_partitions)
	{
		std::string line;
		std::string device;
		char c_str[4096+1];

		while (getline (proc_partitions, line))
		{
			//Build cache of disk devices.
			//Whole disk devices are the ones we want.
			//Device names without a trailing digit refer to the whole disk.
			device = Utils::regexp_label(line, "^[\t ]+[0-9]+[\t ]+[0-9]+[\t ]+[0-9]+[\t ]+([^0-9]+)$");
			//Recognize /dev/mmcblk* devices.
			//E.g., device = /dev/mmcblk0, partition = /dev/mmcblk0p1
			if (device == "")
				device = Utils::regexp_label(line, "^[\t ]+[0-9]+[\t ]+[0-9]+[\t ]+[0-9]+[\t ]+(mmcblk[0-9]+)$");
			//Device names that end with a #[^p]# are HP Smart Array Devices (disks)
			//E.g., device = /dev/cciss/c0d0, partition = /dev/cciss/c0d0p1
			if (device == "")
				device = Utils::regexp_label(line, "^[\t ]+[0-9]+[\t ]+[0-9]+[\t ]+[0-9]+[\t ]+(.*[0-9]+[^p]{1}[0-9]+)$");
			if (device != "")
			{
				//add potential device to the list
				device = "/dev/" + device;
				device_paths_cache.push_back (device);
			}

			//Build cache of potential alternate paths
			if (sscanf (line.c_str(), "%*d %*d %*d %4096s", c_str) == 1)
			{
				line = "/dev/";
				line += c_str;

				//FIXME: it seems realpath is very unsafe to use (manpage)...
				if (  file_test (line, Glib::FILE_TEST_EXISTS)
				    && realpath (line.c_str(), c_str)
				    //&& line != c_str
				  )
				{
					//Because we can make no assumption about which path libparted will
					//detect, we add all combinations.
					alternate_paths_cache[ c_str ] = line;
					alternate_paths_cache[ line ] = c_str;
				}
			}

		}
		proc_partitions.close();
	}
}

