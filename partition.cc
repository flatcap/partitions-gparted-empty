#include "partition.h"

/**
 * Partition
 */
Partition::Partition()
{
	Reset();
}

/**
 * Partition
 */
Partition::Partition (const Glib::ustring & path)
{
	Reset();

	paths.push_back (path);
}

/**
 * Reset
 */
void Partition::Reset()
{
	paths.clear();
	messages.clear();
	status = STAT_REAL;
	type = TYPE_UNALLOCATED;
	filesystem = FS_UNALLOCATED;
	label.clear();
	uuid.clear();
	partition_number = sector_start = sector_end = sectors_used = sectors_unused = -1;
	free_space_before = -1;
	sector_size = 0;
	color.set ("black");
	inside_extended = busy = strict_start = false;
	logicals.clear();
	flags.clear();
	mountpoints.clear();
}

/**
 * Set
 */
void Partition::Set(	const Glib::ustring & device_path,
			const Glib::ustring & partition,
			int partition_number,
			PartitionType type,
			FILESYSTEM filesystem,
			Sector sector_start,
			Sector sector_end,
			Byte_Value sector_size,
			bool inside_extended,
			bool busy)
{
	this->device_path = device_path;

	paths.push_back (partition);

	this->partition_number = partition_number;
	this->type = type;
	this->filesystem = filesystem;
	this->sector_start = sector_start;
	this->sector_end = sector_end;
	this->sector_size = sector_size;
	this->inside_extended = inside_extended;
	this->busy = busy;

	this->color.set (Utils::get_color (filesystem));
}

/**
 * Set_Unused
 */
void Partition::Set_Unused (Sector sectors_unused)
{
	if (sectors_unused <= get_sector_length())
	{
		this->sectors_unused = sectors_unused;
		this->sectors_used = (sectors_unused == -1) ? -1 : get_sector_length() - sectors_unused;
	}
}

/**
 * set_used
 */
void Partition::set_used (Sector sectors_used)
{
	if (sectors_used < get_sector_length())
	{
		this->sectors_used = sectors_used;
		this->sectors_unused = (sectors_used == -1) ? -1 : get_sector_length() - sectors_used;
	}
}

/**
 * Set_Unallocated
 */
void Partition::Set_Unallocated (const Glib::ustring & device_path,
				 Sector sector_start,
				 Sector sector_end,
				 Byte_Value sector_size,
				 bool inside_extended)
{
	Reset();

	Set (device_path,
	     Utils::get_filesystem_string (FS_UNALLOCATED),
	     -1,
	     TYPE_UNALLOCATED,
	     FS_UNALLOCATED,
	     sector_start,
	     sector_end,
	     sector_size,
	     inside_extended,
	     false);

	status = STAT_REAL;
}

/**
 * Update_Number
 */
void Partition::Update_Number (int new_number)
{
	unsigned int index;
	for (unsigned int t = 0; t < paths.size(); t++)
	{
		index = paths[ t ].rfind (Utils::num_to_str (partition_number));

		if (index < paths[ t ].length())
			paths[ t ].replace (index,
				       Utils::num_to_str (partition_number).length(),
				       Utils::num_to_str (new_number));
	}

	partition_number = new_number;
}

/**
 * add_path
 */
void Partition::add_path (const Glib::ustring & path, bool clear_paths)
{
	if (clear_paths)
		paths.clear();

	paths.push_back (path);

	sort_paths_and_remove_duplicates();
}

/**
 * add_paths
 */
void Partition::add_paths (const std::vector<Glib::ustring> & paths, bool clear_paths)
{
	if (clear_paths)
		this->paths.clear();

	this->paths.insert (this->paths.end(), paths.begin(), paths.end());

	sort_paths_and_remove_duplicates();
}

/**
 * get_byte_length
 */
Byte_Value Partition::get_byte_length() const
{
	if (get_sector_length() >= 0)
		return get_sector_length() * sector_size;
	else
		return -1;
}

/**
 * get_sector_length
 */
Sector Partition::get_sector_length() const
{
	if (sector_start >= 0 && sector_end >= 0)
		return sector_end - sector_start + 1;
	else
		return -1;
}

/**
 * get_path
 */
Glib::ustring Partition::get_path() const
{
	if (paths.size() > 0)
		return paths.front();

	return "";
}

/**
 * get_paths
 */
std::vector<Glib::ustring> Partition::get_paths() const
{
	return paths;
}

/**
 * operator==
 */
bool Partition::operator== (const Partition & partition) const
{
	return device_path == partition.device_path &&
	       partition_number == partition.partition_number &&
	       sector_start == partition.sector_start &&
	       type == partition.type;
}

/**
 * operator!=
 */
bool Partition::operator!= (const Partition & partition) const
{
	return ! (*this == partition);
}

/**
 * sort_paths_and_remove_duplicates
 */
void Partition::sort_paths_and_remove_duplicates()
{
	//remove duplicates
	std::sort (paths.begin(), paths.end());
	paths.erase (std::unique (paths.begin(), paths.end()), paths.end());

	//sort on length
	std::sort (paths.begin(), paths.end(), compare_paths);
}

/**
 * add_mountpoint
 */
void Partition::add_mountpoint (const Glib::ustring & mountpoint, bool clear_mountpoints)
{
	if (clear_mountpoints)
		this->mountpoints.clear();

	this->mountpoints.push_back (mountpoint);
}

/**
 * add_mountpoints
 */
void Partition::add_mountpoints (const std::vector<Glib::ustring> & mountpoints, bool clear_mountpoints)
{
	if (clear_mountpoints)
		this->mountpoints.clear();

	this->mountpoints.insert (this->mountpoints.end(), mountpoints.begin(), mountpoints.end());
}

/**
 * get_mountpoint
 */
Glib::ustring Partition::get_mountpoint() const
{
	if (mountpoints.size() > 0)
		return mountpoints.front();

	return "";
}

/**
 * get_mountpoints
 */
std::vector<Glib::ustring> Partition::get_mountpoints() const
{
	return mountpoints;
}

/**
 * get_sector
 */
Sector Partition::get_sector() const
{
	return (sector_start + sector_end) / 2;
}

/**
 * test_overlap
 */
bool Partition::test_overlap (const Partition & partition) const
{
	return ((partition.sector_start >= sector_start && partition.sector_start <= sector_end)
		 ||
		 (partition.sector_end >= sector_start && partition.sector_end <= sector_end)
		 ||
		 (partition.sector_start < sector_start && partition.sector_end > sector_end));
}

/**
 * clear_mountpoints
 */
void Partition::clear_mountpoints()
{
	mountpoints.clear();
}

/**
 * compare_paths
 */
bool Partition::compare_paths (const Glib::ustring & A, const Glib::ustring & B)
{
	return A.length() < B.length();
}

/**
 * ~Partition
 */
Partition::~Partition()
{
}

