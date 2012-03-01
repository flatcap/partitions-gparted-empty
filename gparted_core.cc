#include "win_gparted.h"
#include "gparted_core.h"
#include "proc_partitions_info.h"

#include <set>
#include <cerrno>
#include <cstring>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <mntent.h>
#include <gtkmm/messagedialog.h>
#include <parted/parted.h>

std::vector<Glib::ustring> libparted_messages; //see ped_exception_handler()

/**
 * GParted_Core
 */
GParted_Core::GParted_Core()
{
	lp_device = NULL;
	lp_disk = NULL;
	lp_partition = NULL;
	p_filesystem = NULL;
	thread_status_message = "";

	ped_exception_set_handler (ped_exception_handler);

	//get valid flags...
	for (PedPartitionFlag flag = ped_partition_flag_next (static_cast<PedPartitionFlag> (NULL));
	      flag;
	      flag = ped_partition_flag_next (flag))
		flags.push_back (flag);

	//throw libpartedversion to the stdout to see which version is actually used.
	std::cout << "======================" << std::endl;
	std::cout << "libparted : " << ped_get_version() << std::endl;
	std::cout << "======================" << std::endl;

	//initialize file system list
	find_supported_filesystems();
}

/**
 * find_supported_filesystems
 */
void GParted_Core::find_supported_filesystems()
{
	std::map< FILESYSTEM, FileSystem * >::iterator f;

	// TODO: determine whether it is safe to initialize this only once
	for (f = FILESYSTEM_MAP.begin(); f != FILESYSTEM_MAP.end(); f++) {
		if (f->second)
			delete f->second;
	}

	FILESYSTEM_MAP.clear();

	FILESYSTEM_MAP[FS_LUKS]	= NULL;
	FILESYSTEM_MAP[FS_UNKNOWN]	= NULL;

	FILESYSTEMS.clear();

	FS fs_notsupp;
	for (f = FILESYSTEM_MAP.begin(); f != FILESYSTEM_MAP.end(); f++) {
		if (f->second)
			FILESYSTEMS.push_back (f->second->get_filesystem_support());
		else {
			fs_notsupp.filesystem = f->first;
			FILESYSTEMS.push_back (fs_notsupp);
		}
	}
}

/**
 * set_user_devices
 */
void GParted_Core::set_user_devices (const std::vector<Glib::ustring> & user_devices)
{
	this->device_paths = user_devices;
	this->probe_devices = ! user_devices.size();
}

/**
 * set_devices
 */
void GParted_Core::set_devices (std::vector<Device> & devices)
{
	devices.clear();
	Device temp_device;
	Proc_Partitions_Info pp_info (true);  //Refresh cache of proc partition information

	init_maps();

	//only probe if no devices were specified as arguments..
	if (probe_devices)
	{
		device_paths.clear();

		//FIXME:  When libparted bug 194 is fixed, remove code to read:
		//           /proc/partitions
		//        This was a problem with no floppy drive yet BIOS indicated one existed.
		//        http://parted.alioth.debian.org/cgi-bin/trac.cgi/ticket/194
		//
		//try to find all available devices if devices exist in /proc/partitions
		std::vector<Glib::ustring> temp_devices = pp_info.get_device_paths();
		if (! temp_devices.empty())
		{
			//Try to find all devices in /proc/partitions
			for (unsigned int k=0; k < temp_devices.size(); k++)
			{
				/*TO TRANSLATORS: looks like Scanning /dev/sda */
				set_thread_status_message (String::ucompose ("Scanning %1", temp_devices[k]));
				ped_device_get (temp_devices[k].c_str());
			}

		}
		else
		{
			//No devices found in /proc/partitions so use libparted to probe devices
			ped_device_probe_all();
		}

		lp_device = ped_device_get_next (NULL);
		while (lp_device)
		{
			//only add this device if we can read the first sector (which means it's a real device)
			char * buf = static_cast<char *> (malloc (lp_device->sector_size));
			if (buf)
			{
				/*TO TRANSLATORS: looks like Confirming /dev/sda */
				set_thread_status_message (String::ucompose ("Confirming %1", lp_device->path));
				if (ped_device_open (lp_device))
				{
					//Devices with sector sizes of 512 bytes and higher are supported
					if (ped_device_read (lp_device, buf, 0, 1))
						device_paths.push_back (lp_device->path);
					ped_device_close (lp_device);
				}
				free (buf);
			}
			lp_device = ped_device_get_next (lp_device);
		}
		close_device_and_disk();

		std::sort (device_paths.begin(), device_paths.end());
	}

	for (unsigned int t = 0; t < device_paths.size(); t++)
	{
		/*TO TRANSLATORS: looks like Searching /dev/sda partitions */
		set_thread_status_message (String::ucompose ("Searching %1 partitions", device_paths[t]));
		if (open_device_and_disk (device_paths[t], false))
		{
			temp_device.Reset();

			//device info..
			temp_device.add_path (device_paths[t]);
			temp_device.add_paths (pp_info.get_alternate_paths (temp_device.get_path()));

			temp_device.model	=	lp_device->model;
			temp_device.length	=	lp_device->length;
			temp_device.sector_size	=	lp_device->sector_size;
			temp_device.heads	=	lp_device->bios_geom.heads;
			temp_device.sectors	=	lp_device->bios_geom.sectors;
			temp_device.cylinders	=	lp_device->bios_geom.cylinders;
			temp_device.cylsize	=	temp_device.heads * temp_device.sectors;

			//make sure cylsize is at least 1 MiB
			if (temp_device.cylsize < (MEBIBYTE / temp_device.sector_size))
				temp_device.cylsize = (MEBIBYTE / temp_device.sector_size);

			//normal harddisk
			if (lp_disk)
			{
				temp_device.disktype =	lp_disk->type->name;
				temp_device.max_prims = ped_disk_get_max_primary_partition_count (lp_disk);

				set_device_partitions (temp_device);
				set_mountpoints (temp_device.partitions);
				set_used_sectors (temp_device.partitions);

				if (temp_device.highest_busy)
				{
					temp_device.readonly = ! commit_to_os (1);
					//Clear libparted messages.  Typically these are:
					//  The kernel was unable to re-read the partition table...
					libparted_messages.clear();
				}
			}
			//harddisk without disklabel
			else
			{
				temp_device.disktype =
						/* TO TRANSLATORS:  unrecognized
						 * means that the partition table for this
						 * disk device is unknown or not recognized.
						 */
						"unrecognized";
				temp_device.max_prims = -1;

				Partition partition_temp;
				partition_temp.Set_Unallocated (temp_device.get_path(),
								 0,
								 temp_device.length - 1,
								 temp_device.sector_size,
								 false);
				//Place libparted messages in this unallocated partition
				partition_temp.messages.insert (partition_temp.messages.end(),
								  libparted_messages. begin(),
								  libparted_messages.end());
				libparted_messages.clear();
				temp_device.partitions.push_back (partition_temp);
			}

			devices.push_back (temp_device);

			close_device_and_disk();
		}
	}

	//clear leftover information...
	//NOTE that we cannot clear mountinfo since it might be needed in get_all_mountpoints()
	set_thread_status_message("");
	fstab_info.clear();
}

/**
 * guess_partition_table
 * runs gpart on the specified parameter
 */
void GParted_Core::guess_partition_table(const Device & device, Glib::ustring &buff)
{
	int pid, stdoutput, stderror;
	std::vector<std::string> argvproc, envpproc;
	gunichar tmp;

	//Get the char string of the sector_size
	std::ostringstream ssIn;
    ssIn << device.sector_size;
    Glib::ustring str_ssize = ssIn.str();

	//Build the command line
	argvproc.push_back("gpart");
	argvproc.push_back(device.get_path());
	argvproc.push_back("-s");
	argvproc.push_back(str_ssize);

	envpproc.push_back ("LC_ALL=C");
	envpproc.push_back ("PATH=" + Glib::getenv ("PATH"));

	Glib::spawn_async_with_pipes(Glib::get_current_dir(), argvproc,
		envpproc, Glib::SPAWN_SEARCH_PATH, sigc::slot<void>(),
		&pid, NULL, &stdoutput, &stderror);

	this->iocOutput=Glib::IOChannel::create_from_fd(stdoutput);

	while(this->iocOutput->read(tmp)==Glib::IO_STATUS_NORMAL)
	{
		buff+=tmp;
	}
	this->iocOutput->close();

	return;
}

/**
 * set_thread_status_message
 */
void GParted_Core::set_thread_status_message (Glib::ustring msg)
{
	//Remember to clear status message when finished with thread.
	thread_status_message = msg;
}

/**
 * get_thread_status_message
 */
Glib::ustring GParted_Core::get_thread_status_message()
{
	return thread_status_message;
}

/**
 * snap_to_cylinder
 */
bool GParted_Core::snap_to_cylinder (const Device & device, Partition & partition, Glib::ustring & error)
{
	Sector diff = 0;

	//Determine if partition size is less than half a disk cylinder
	bool less_than_half_cylinder = false;
	if ((partition.sector_end - partition.sector_start) < (device.cylsize / 2))
		less_than_half_cylinder = true;

	if (partition.type == TYPE_LOGICAL ||
	     partition.sector_start == device.sectors
	  )
	{
		//Must account the relative offset between:
		// (A) the Extended Boot Record sector and the next track of the
		//     logical partition (usually 63 sectors), and
		// (B) the Master Boot Record sector and the next track of the first
		//     primary partition
		diff = (partition.sector_start - device.sectors) % device.cylsize;
	}
	else if (partition.sector_start == 34)
	{
		// (C) the GUID Partition Table (GPT) and the start of the data
		//     partition at sector 34
		diff = (partition.sector_start - 34) % device.cylsize;
	}
	else
	{
		diff = partition.sector_start % device.cylsize;
	}
	if (diff && ! partition.strict_start )
	{
		if (diff < (device.cylsize / 2) || less_than_half_cylinder)
			partition.sector_start -= diff;
		else
			partition.sector_start += (device.cylsize - diff);
	}

	diff = (partition.sector_end +1) % device.cylsize;
	if (diff)
	{
		if (diff < (device.cylsize / 2) && ! less_than_half_cylinder)
			partition.sector_end -= diff;
		else
			partition.sector_end += (device.cylsize - diff);
	}

	return true;
}

/**
 * snap_to_mebibyte
 */
bool GParted_Core::snap_to_mebibyte (const Device & device, Partition & partition, Glib::ustring & error)
{
	Sector diff = 0;
	if (partition.sector_start < 2 || partition.type == TYPE_LOGICAL)
	{
		//Must account the relative offset between:
		// (A) the Master Boot Record sector and the first primary/extended partition, and
		// (B) the Extended Boot Record sector and the logical partition

		//If strict_start is set then do not adjust sector start.
		//If this partition is not simply queued for a reformat then
		//  add space minimum to force alignment to next mebibyte.
		if ( (! partition.strict_start)
		    && (partition.free_space_before == 0)
		    && (partition.status != STAT_FORMATTED)
		  )
		{
			//Unless specifically told otherwise, the Linux kernel considers extended
			//  boot records to be two sectors long, in order to "leave room for LILO".
			partition.sector_start += 2;
		}
	}

	//Align start sector
	diff = Sector(partition.sector_start % (MEBIBYTE / partition.sector_size));
	if (diff && ( (! partition.strict_start)
	              || (  partition.strict_start
	                  && (  partition.status == STAT_NEW
	                      || partition.status == STAT_COPY
	                    )
	                )
	            )
	  )
		partition.sector_start += ((MEBIBYTE / partition.sector_size) - diff);

	//If this is a logical partition not at end of drive then check to see if space is
	//  required for a following logical partition Extended Boot Record
	if (partition.type == TYPE_LOGICAL)
	{
		//Locate the extended partition that contains the logical partitions.
		int index_extended = -1;
		for (unsigned int t = 0; t < device.partitions.size(); t++)
		{
			if (device.partitions[t].type == TYPE_EXTENDED)
				index_extended = t;
		}

		//If there is a following logical partition that starts a mebibyte or less
		//  from the end of this partition, then reserve a mebibyte for the EBR.
		if (index_extended != -1)
		{
			for (unsigned int t = 0; t < device.partitions[index_extended].logicals.size(); t++)
			{
				if ( (device.partitions[index_extended].logicals[t].type == TYPE_LOGICAL)
				    && (device.partitions[index_extended].logicals[t].sector_start > partition.sector_end)
				    && ((device.partitions[index_extended].logicals[t].sector_start - partition.sector_end)
				         < (MEBIBYTE / device.sector_size)
				      )
				  )
					partition.sector_end -= 1;
			}
		}
	}

	//If this is a GPT partition table then reserve a mebibyte at the end of the device
	//  for the backup partition table
	if (   device.disktype == "gpt"
	    && ((device.length - partition.sector_end) <= (MEBIBYTE / device.sector_size))
	  )
	{
		partition.sector_end -= 1;
	}

	//Align end sector
	diff = (partition.sector_end + 1) % (MEBIBYTE / partition.sector_size);
	if (diff)
		partition.sector_end -= diff;

	return true;
}

/**
 * snap_to_alignment
 */
bool GParted_Core::snap_to_alignment (const Device & device, Partition & partition, Glib::ustring & error)
{
	bool rc = true;

	if (partition.alignment == ALIGN_CYLINDER)
		rc = snap_to_cylinder (device, partition, error);
	else if (partition.alignment == ALIGN_MEBIBYTE)
		rc = snap_to_mebibyte (device, partition, error);

	//Ensure that partition start and end are not beyond the ends of the disk device
	if (partition.sector_start < 0)
		partition.sector_start = 0;
	if (partition.sector_end > device.length)
		partition.sector_end = device.length - 1;

	//do some basic checks on the partition
	if (partition.get_sector_length() <= 0)
	{
		error = String::ucompose(
				/* TO TRANSLATORS:  looks like   A partition cannot have a length of -1 sectors */
				"A partition cannot have a length of %1 sectors",
				partition.get_sector_length());
		return false;
	}

	if (partition.get_sector_length() < partition.sectors_used)
	{
		error = String::ucompose(
				/* TO TRANSLATORS: looks like   A partition with used sectors (2048) greater than its length (1536) is not valid */
				"A partition with used sectors (%1) greater than its length (%2) is not valid",
				partition.sectors_used,
				partition.get_sector_length());
		return false;
	}

	//FIXME: it would be perfect if we could check for overlapping with adjacent partitions as well,
	//however, finding the adjacent partitions is not as easy as it seems and at this moment all the dialogs
	//already perform these checks. A perfect 'fixme-later';)

	return rc;
}

/**
 * apply_operation_to_disk
 */
bool GParted_Core::apply_operation_to_disk (Operation * operation)
{
	return 0;
}

/**
 * set_disklabel
 */
bool GParted_Core::set_disklabel (const Glib::ustring & device_path, const Glib::ustring & disklabel)
{
	return 0;
}

/**
 * toggle_flag
 */
bool GParted_Core::toggle_flag (const Partition & partition, const Glib::ustring & flag, bool state)
{
	bool success = false;

	if (open_device_and_disk (partition.device_path))
	{
		lp_partition = NULL;
		if (partition.type == TYPE_EXTENDED)
			lp_partition = ped_disk_extended_partition (lp_disk);
		else
			lp_partition = ped_disk_get_partition_by_sector (lp_disk, partition.get_sector());

		if (lp_partition)
		{
			PedPartitionFlag lp_flag = ped_partition_flag_get_by_name (flag.c_str());

			if (lp_flag > 0 && ped_partition_set_flag (lp_partition, lp_flag, state))
				success = commit();
		}

		close_device_and_disk();
	}

	return success;
}

/**
 * get_filesystems
 */
const std::vector<FS> & GParted_Core::get_filesystems() const
{
	return FILESYSTEMS;
}

/**
 * get_fs
 */
const FS & GParted_Core::get_fs (FILESYSTEM filesystem) const
{
	unsigned int unknown;

	unknown = FILESYSTEMS.size();
	for (unsigned int t = 0; t < FILESYSTEMS.size(); t++)
	{
		if (FILESYSTEMS[t].filesystem == filesystem)
			return FILESYSTEMS[t];
		else if (FILESYSTEMS[t].filesystem == FS_UNKNOWN)
			unknown = t;
	}

	if (unknown == FILESYSTEMS.size()) {
		// This shouldn't happen, but just in case...
		static FS fs;
		fs.filesystem = FS_UNKNOWN;
		return fs;
	} else
		return FILESYSTEMS[unknown];
}

/**
 * get_disklabeltypes
 */
std::vector<Glib::ustring> GParted_Core::get_disklabeltypes()
{
	std::vector<Glib::ustring> disklabeltypes;

	//msdos should be first in the list
	disklabeltypes.push_back ("msdos");

	 PedDiskType *disk_type;
	 for (disk_type = ped_disk_type_get_next (NULL); disk_type; disk_type = ped_disk_type_get_next (disk_type))
		 if (Glib::ustring (disk_type->name) != "msdos")
			disklabeltypes.push_back (disk_type->name);

	 return disklabeltypes;
}

/**
 * get_all_mountpoints
 */
std::vector<Glib::ustring> GParted_Core::get_all_mountpoints()
{
	std::vector<Glib::ustring> mountpoints;

	for (iter_mp = mount_info.begin(); iter_mp != mount_info.end(); ++iter_mp)
		mountpoints.insert (mountpoints.end(), iter_mp->second.begin(), iter_mp->second.end());

	return mountpoints;
}

/**
 * get_available_flags
 */
std::map<Glib::ustring, bool> GParted_Core::get_available_flags (const Partition & partition)
{
	std::map<Glib::ustring, bool> flag_info;

	if (open_device_and_disk (partition.device_path))
	{
		lp_partition = NULL;
		if (partition.type == TYPE_EXTENDED)
			lp_partition = ped_disk_extended_partition (lp_disk);
		else
			lp_partition = ped_disk_get_partition_by_sector (lp_disk, partition.get_sector());

		if (lp_partition)
		{
			for (unsigned int t = 0; t < flags.size(); t++)
				if (ped_partition_is_flag_available (lp_partition, flags[t]))
					flag_info[ped_partition_flag_get_name (flags[t])] =
						ped_partition_get_flag (lp_partition, flags[t]);
		}

		close_device_and_disk();
	}

	return flag_info;
}

/**
 * get_libparted_version
 */
Glib::ustring GParted_Core::get_libparted_version()
{
	return ped_get_version();
}

//private functions...

/**
 * init_maps
 */
void GParted_Core::init_maps()
{
	mount_info.clear();
	fstab_info.clear();

	//sort the mount points and remove duplicates.. (no need to do this for fstab_info)
	for (iter_mp = mount_info.begin(); iter_mp != mount_info.end(); ++iter_mp)
	{
		std::sort (iter_mp->second.begin(), iter_mp->second.end());

		iter_mp->second.erase(
				std::unique (iter_mp->second.begin(), iter_mp->second.end()),
				iter_mp->second.end());
	}
}

/**
 * read_mountpoints_from_file
 */
void GParted_Core::read_mountpoints_from_file(
	const Glib::ustring & filename,
	std::map< Glib::ustring, std::vector<Glib::ustring> > & map)
{
	FILE* fp = setmntent (filename.c_str(), "r");

	if (fp == NULL)
		return;

	struct mntent* p = NULL;

	while ((p = getmntent(fp)) != NULL)
	{
		Glib::ustring node = p->mnt_fsname;

		if (! node.empty())
		{
			Glib::ustring mountpoint = p->mnt_dir;

			//Only add node path(s) if mount point exists
			if (file_test (mountpoint, Glib::FILE_TEST_EXISTS))
			{
				map[node].push_back (mountpoint);

				//If node is a symbolic link (e.g., /dev/root)
				//  then find real path and add entry
				if (file_test (node, Glib::FILE_TEST_IS_SYMLINK))
				{
					char c_str[4096+1];
					//FIXME: it seems realpath is very unsafe to use (manpage)...
					if (realpath (node.c_str(), c_str) != NULL)
						map[c_str].push_back (mountpoint);
				}
			}
		}
	}

	endmntent (fp);
}

/**
 * read_mountpoints_from_file_swaps
 */
void GParted_Core::read_mountpoints_from_file_swaps(
	const Glib::ustring & filename,
	std::map< Glib::ustring, std::vector<Glib::ustring> > & map)
{
	std::string line;
	std::string node;

	std::ifstream file (filename.c_str());
	if (file)
	{
		while (getline (file, line))
		{
			node = Utils::regexp_label (line, "^(/[^]+)");
			if (node.size() > 0)
				map[node].push_back ("" /* no mountpoint for swap */);
		}
		file.close();
	}
}

/**
 * get_partition_path
 */
Glib::ustring GParted_Core::get_partition_path (PedPartition * lp_partition)
{
	char * lp_path;  //we have to free the result of ped_partition_get_path()
	Glib::ustring partition_path = "Partition path not found";

	lp_path = ped_partition_get_path(lp_partition);
	if (lp_path != NULL)
	{
		partition_path = lp_path;
		free(lp_path);
	}

	return partition_path;
}

/**
 * set_device_partitions
 */
void GParted_Core::set_device_partitions (Device & device)
{
	int EXT_INDEX = -1;
	Proc_Partitions_Info pp_info; //Use cache of proc partitions information

	//clear partitions
	device.partitions.clear();

	lp_partition = ped_disk_next_partition (lp_disk, NULL);
	while (lp_partition)
	{
		libparted_messages.clear();
		partition_temp.Reset();
		bool partition_is_busy = false;
		FILESYSTEM filesystem;

		//Retrieve partition path
		Glib::ustring partition_path = get_partition_path (lp_partition);

		switch (lp_partition->type)
		{
			case PED_PARTITION_NORMAL:
			case PED_PARTITION_LOGICAL:
				filesystem = get_filesystem();
					partition_is_busy = ped_partition_is_busy (lp_partition);

				partition_temp.Set (device.get_path(),
						     partition_path,
						     lp_partition->num,
						     lp_partition->type == 0 ?	TYPE_PRIMARY : TYPE_LOGICAL,
						     filesystem,
						     lp_partition->geom.start,
						     lp_partition->geom.end,
						     device.sector_size,
						     lp_partition->type,
						     partition_is_busy);

				partition_temp.add_paths (pp_info.get_alternate_paths (partition_temp.get_path()));
				set_flags (partition_temp);

				if (partition_temp.busy && partition_temp.partition_number > device.highest_busy)
					device.highest_busy = partition_temp.partition_number;

				break;

			case PED_PARTITION_EXTENDED:
					partition_is_busy = ped_partition_is_busy (lp_partition);

				partition_temp.Set (device.get_path(),
						     partition_path,
						     lp_partition->num,
						     TYPE_EXTENDED,
						     FS_EXTENDED,
						     lp_partition->geom.start,
						     lp_partition->geom.end,
						     device.sector_size,
						     false,
						     partition_is_busy);

				partition_temp.add_paths (pp_info.get_alternate_paths (partition_temp.get_path()));
				set_flags (partition_temp);

				EXT_INDEX = device.partitions.size();
				break;

			default:
				break;
		}

		//Avoid reading additional file system information if there is no path
		if (partition_temp.get_path() != "")
		{
			//Retrieve file system label
			read_label (partition_temp);
			if (partition_temp.label.empty())
			{
			}

			//Retrieve file system UUID
			read_uuid (partition_temp);
			if (partition_temp.uuid.empty())
			{
			}
		}

		partition_temp.messages.insert (partition_temp.messages.end(),
						  libparted_messages. begin(),
						  libparted_messages.end());

		//if there's an end, there's a partition;)
		if (partition_temp.sector_end > -1)
		{
			if (! partition_temp.inside_extended)
				device.partitions.push_back (partition_temp);
			else
				device.partitions[EXT_INDEX].logicals.push_back (partition_temp);
		}

		//next partition (if any)
		lp_partition = ped_disk_next_partition (lp_disk, lp_partition);
	}

	if (EXT_INDEX > -1)
		insert_unallocated (device.get_path(),
				    device.partitions[EXT_INDEX].logicals,
				    device.partitions[EXT_INDEX].sector_start,
				    device.partitions[EXT_INDEX].sector_end,
				    device.sector_size,
				    true);

	insert_unallocated (device.get_path(), device.partitions, 0, device.length -1, device.sector_size, false);
}

/**
 * get_filesystem
 */
FILESYSTEM GParted_Core::get_filesystem()
{
	char magic1[16] = "";
	char magic2[16] = "";

	//Check for LUKS encryption prior to libparted file system detection.
	//  Otherwise encrypted file systems such as ext3 will be detected by
	//  libparted as 'ext3'.

	//LUKS encryption
	char * buf = static_cast<char *> (malloc (lp_device->sector_size));
	if (buf)
	{
		ped_device_open (lp_device);
		ped_geometry_read (& lp_partition->geom, buf, 0, 1);
		memcpy(magic1, buf+0, 6);  //set binary magic data
		ped_device_close (lp_device);
		free (buf);

		if (0 == memcmp (magic1 , "LUKS\xBA\xBE", 6))
		{
			temp =  "Linux Unified Key Setup encryption is not yet supported." ;
			temp += "\n";
			partition_temp.messages.push_back (temp);
			return FS_LUKS;
		}
	}

	Glib::ustring fs_type = "";

	//Standard libparted file system detection
	if (lp_partition && lp_partition->fs_type)
	{
		fs_type = lp_partition->fs_type->name;

		//TODO:  Temporary code to detect ext4.
		//       Replace when libparted >= 1.9.0 is chosen as minimum required version.
	}

	//FS_Info (blkid) file system detection because current libparted (v2.2) does not
	//  appear to detect file systems for sector sizes other than 512 bytes.
	if (fs_type.empty())
	{
		//TODO: blkid does not return anything for an "extended" partition.  Need to handle this somehow
	}

	if (! fs_type.empty())
	{
		if (fs_type == "extended")
			return FS_EXTENDED;
		else if (fs_type == "btrfs")
			return FS_BTRFS;
		else if (fs_type == "exfat")
			return FS_EXFAT;
		else if (fs_type == "ext2")
			return FS_EXT2;
		else if (fs_type == "ext3")
			return FS_EXT3;
		else if (fs_type == "ext4" ||
		          fs_type == "ext4dev")
			return FS_EXT4;
		else if (fs_type == "linux-swap" ||
		          fs_type == "linux-swap(v1)" ||
		          fs_type == "linux-swap(new)" ||
		          fs_type == "linux-swap(v0)" ||
		          fs_type == "linux-swap(old)" ||
		          fs_type == "swap")
			return FS_LINUX_SWAP;
		else if (fs_type == "LVM2_member")
			return FS_LVM2_PV;
		else if (fs_type == "fat16")
			return FS_FAT16;
		else if (fs_type == "fat32")
			return FS_FAT32;
		else if (fs_type == "nilfs2")
			return FS_NILFS2;
		else if (fs_type == "ntfs")
			return FS_NTFS;
		else if (fs_type == "reiserfs")
			return FS_REISERFS;
		else if (fs_type == "xfs")
			return FS_XFS;
		else if (fs_type == "jfs")
			return FS_JFS;
		else if (fs_type == "hfs")
			return FS_HFS;
		else if (fs_type == "hfs+" ||
		          fs_type == "hfsplus")
			return FS_HFSPLUS;
		else if (fs_type == "ufs")
			return FS_UFS;
	}

	//other file systems libparted couldn't detect (i've send patches for these file systems to the parted guys)
	// - no patches sent to parted for lvm2, or luks

	//reiser4
	buf = static_cast<char *> (malloc (lp_device->sector_size));
	if (buf)
	{
		ped_device_open (lp_device);
		ped_geometry_read (& lp_partition->geom
		                 , buf
		                 , (65536 / lp_device->sector_size)
		                 , 1
		                );
		memcpy(magic1, buf+0, 7); //set binary magic data
		ped_device_close (lp_device);
		free (buf);

		if (0 == memcmp (magic1, "ReIsEr4", 7))
			return FS_REISER4;
	}

	//lvm2
	//NOTE: lvm2 is not a file system but we do wish to recognize the Physical Volume
	buf = static_cast<char *> (malloc (lp_device->sector_size));
	if (buf)
	{
		ped_device_open (lp_device);
		if (lp_device->sector_size == 512)
		{
			ped_geometry_read (& lp_partition->geom, buf, 1, 1);
			memcpy(magic1, buf+ 0, 8); // set binary magic data
			memcpy(magic2, buf+24, 4); // set binary magic data
		}
		else
		{
			ped_geometry_read (& lp_partition->geom, buf, 0, 1);
			memcpy(magic1, buf+ 0+512, 8); // set binary magic data
			memcpy(magic2, buf+24+512, 4); // set binary magic data
		}
		ped_device_close (lp_device);
		free (buf);

		if (   0 == memcmp (magic1, "LABELONE", 8)
		     && 0 == memcmp (magic2, "LVM2", 4))
		{
			return FS_LVM2_PV;
		}
	}

	//btrfs
	const Sector BTRFS_SUPER_INFO_SIZE   = 4096;
	const Sector BTRFS_SUPER_INFO_OFFSET = (64 * 1024);
	const char* const BTRFS_SIGNATURE  = "_BHRfS_M";

	char    buf_btrfs[BTRFS_SUPER_INFO_SIZE];

	ped_device_open (lp_device);
	ped_geometry_read (& lp_partition->geom
	                 , buf_btrfs
	                 , (BTRFS_SUPER_INFO_OFFSET / lp_device->sector_size)
	                 , (BTRFS_SUPER_INFO_SIZE / lp_device->sector_size)
	                );
	memcpy(magic1, buf_btrfs+64, strlen(BTRFS_SIGNATURE));  //set binary magic data
	ped_device_close (lp_device);

	if (0 == memcmp (magic1, BTRFS_SIGNATURE, strlen(BTRFS_SIGNATURE)))
	{
		return FS_BTRFS;
	}

	//no file system found....
	temp =  "Unable to detect file system! Possible reasons are:" ;
	temp += "\n- ";
	temp +=  "The file system is damaged" ;
	temp += "\n- ";
	temp +=  "The file system is unknown to GParted" ;
	temp += "\n- ";
	temp +=  "There is no file system available (unformatted)" ;
	temp += "\n- ";
	/* TO TRANSLATORS: looks like  The device entry /dev/sda5 is missing */
	temp += String::ucompose ( "The device entry %1 is missing" , get_partition_path (lp_partition));

	partition_temp.messages.push_back (temp);

	return FS_UNKNOWN;
}

/**
 * read_label
 */
void GParted_Core::read_label (Partition & partition)
{
	if (partition.type != TYPE_EXTENDED)
	{
		switch (get_fs (partition.filesystem).read_label)
		{
			case FS::EXTERNAL:
				if (set_proper_filesystem (partition.filesystem))
					p_filesystem->read_label (partition);
				break;
			default:
				break;
		}
	}
}

/**
 * read_uuid
 */
void GParted_Core::read_uuid (Partition & partition)
{
	if (partition.type != TYPE_EXTENDED)
	{
		switch (get_fs (partition.filesystem).read_uuid)
		{
			case FS::EXTERNAL:
				if (set_proper_filesystem (partition.filesystem))
					p_filesystem->read_uuid (partition);
				break;

			default:
				break;
		}
	}
}

/**
 * insert_unallocated
 */
void GParted_Core::insert_unallocated (const Glib::ustring & device_path,
				       std::vector<Partition> & partitions,
				       Sector start,
				       Sector end,
				       Byte_Value sector_size,
				       bool inside_extended)
{
	partition_temp.Reset();
	partition_temp.Set_Unallocated (device_path, 0, 0, sector_size, inside_extended);

	//if there are no partitions at all..
	if (partitions.empty())
	{
		partition_temp.sector_start = start;
		partition_temp.sector_end = end;

		partitions.push_back (partition_temp);

		return;
	}

	//start <---> first partition start
	if ((partitions.front().sector_start - start) > (MEBIBYTE / sector_size))
	{
		partition_temp.sector_start = start;
		partition_temp.sector_end = partitions.front().sector_start -1;

		partitions.insert (partitions.begin(), partition_temp);
	}

	//look for gaps in between
	for (unsigned int t =0; t < partitions.size() -1; t++)
	{
		if ( ((partitions[t + 1].sector_start - partitions[t].sector_end - 1) > (MEBIBYTE / sector_size))
		    || ( (partitions[t + 1].type != TYPE_LOGICAL)  // Only show exactly 1 MiB if following partition is not logical.
		        && ((partitions[t + 1].sector_start - partitions[t].sector_end - 1) == (MEBIBYTE / sector_size))
		      )
		  )
		{
			partition_temp.sector_start = partitions[t].sector_end +1;
			partition_temp.sector_end = partitions[t +1].sector_start -1;

			partitions.insert (partitions.begin() + ++t, partition_temp);
		}
	}

	//last partition end <---> end
	if ((end - partitions.back().sector_end) >= (MEBIBYTE / sector_size))
	{
		partition_temp.sector_start = partitions.back().sector_end +1;
		partition_temp.sector_end = end;

		partitions.push_back (partition_temp);
	}
}

/**
 * set_mountpoints
 */
void GParted_Core::set_mountpoints (std::vector<Partition> & partitions)
{
	for (unsigned int t = 0; t < partitions.size(); t++)
	{
		if ((partitions[t].type == TYPE_PRIMARY ||
		       partitions[t].type == TYPE_LOGICAL
		    ) &&
		     partitions[t].filesystem != FS_LINUX_SWAP &&
		     partitions[t].filesystem != FS_LVM2_PV    &&
		     partitions[t].filesystem != FS_LUKS
		  )
		{
			if (partitions[t].busy)
			{
					//Normal device, not DMRaid device
					for (unsigned int i = 0; i < partitions[t].get_paths().size(); i++)
					{
						iter_mp = mount_info.find (partitions[t].get_paths()[i]);
						if (iter_mp != mount_info.end())
						{
							partitions[t].add_mountpoints (iter_mp->second);
							break;
						}
					}

				if (partitions[t].get_mountpoints().empty())
					partitions[t].messages.push_back ("Unable to find mount point");
			}
			else
			{
				iter_mp = fstab_info.find (partitions[t].get_path());
				if (iter_mp != fstab_info.end())
					partitions[t].add_mountpoints (iter_mp->second);
			}
		}
		else if (partitions[t].type == TYPE_EXTENDED)
			set_mountpoints (partitions[t].logicals);
		else if (partitions[t].filesystem == FS_LVM2_PV)
		{
		}
	}
}

/**
 * set_used_sectors
 */
void GParted_Core::set_used_sectors (std::vector<Partition> & partitions)
{
	struct statvfs sfs;

	for (unsigned int t = 0; t < partitions.size(); t++)
	{
		if (partitions[t].filesystem != FS_LINUX_SWAP &&
		     partitions[t].filesystem != FS_LUKS       &&
		     partitions[t].filesystem != FS_UNKNOWN
		  )
		{
			if (partitions[t].type == TYPE_PRIMARY ||
			     partitions[t].type == TYPE_LOGICAL)
			{
				if (partitions[t].busy && partitions[t].filesystem != FS_LVM2_PV)
				{
					if (partitions[t].get_mountpoints().size() > 0 )
					{
						if (statvfs (partitions[t].get_mountpoint().c_str(), &sfs) == 0)
							partitions[t].Set_Unused (sfs.f_bfree * (sfs.f_bsize / partitions[t].sector_size));
						else
							partitions[t].messages.push_back(
								"statvfs (" +
								partitions[t].get_mountpoint() +
								"): " +
								Glib::strerror (errno));
					}
				}
				else
				{
					switch (get_fs (partitions[t].filesystem).read)
					{
						case FS::EXTERNAL	:
							if (set_proper_filesystem (partitions[t].filesystem))
								p_filesystem->set_used_sectors (partitions[t]);
							break;

						default:
							break;
					}
				}

				if (partitions[t].sectors_used == -1)
				{
					temp = "Unable to read the contents of this file system!";
					temp += "\n";
					temp += "Because of this some operations may be unavailable.";
					if (! Utils::get_filesystem_software (partitions[t].filesystem).empty())
					{
						temp += "\n\n";
						temp +=  "The cause might be a missing software package.";
						temp += "\n";
						/*TO TRANSLATORS: looks like The following list of software packages is required for NTFS file system support:  ntfsprogs. */
						temp += String::ucompose ("The following list of software packages is required for %1 file system support:  %2.",
						                          Utils::get_filesystem_string (partitions[t].filesystem),
						                          Utils::get_filesystem_software (partitions[t].filesystem)
						                       );
					}
					partitions[t].messages.push_back (temp);
				}
			}
			else if (partitions[t].type == TYPE_EXTENDED)
				set_used_sectors (partitions[t].logicals);
		}
	}
}

/**
 * set_flags
 */
void GParted_Core::set_flags (Partition & partition)
{
	for (unsigned int t = 0; t < flags.size(); t++)
		if (ped_partition_is_flag_available (lp_partition, flags[t]) &&
		     ped_partition_get_flag (lp_partition, flags[t]))
			partition.flags.push_back (ped_partition_flag_get_name (flags[t]));
}

/**
 * create
 */
bool GParted_Core::create (const Device & device, Partition & new_partition, OperationDetail & operationdetail)
{
	if (new_partition.type == TYPE_EXTENDED)
	{
		return create_partition (new_partition, operationdetail);
	}
	else if (create_partition (new_partition, operationdetail, (get_fs (new_partition.filesystem).MIN / new_partition.sector_size)))
	{
		if (new_partition.filesystem == FS_UNFORMATTED)
			return true;
		else
			return set_partition_type (new_partition, operationdetail) &&
			       create_filesystem (new_partition, operationdetail);
	}

	return false;
}

/**
 * create_partition
 */
bool GParted_Core::create_partition (Partition & new_partition, OperationDetail & operationdetail, Sector min_size)
{
	return 0;
}

/**
 * create_filesystem
 */
bool GParted_Core::create_filesystem (const Partition & partition, OperationDetail & operationdetail)
{
	return 0;
}

/**
 * format
 */
bool GParted_Core::format (const Partition & partition, OperationDetail & operationdetail)
{
	return set_partition_type (partition, operationdetail) && create_filesystem (partition, operationdetail);
}

/**
 * Delete
 */
bool GParted_Core::Delete (const Partition & partition, OperationDetail & operationdetail)
{
	return 0;
}

/**
 * label_partition
 */
bool GParted_Core::label_partition (const Partition & partition, OperationDetail & operationdetail)
{
	return 0;
}

/**
 * change_uuid
 */
bool GParted_Core::change_uuid (const Partition & partition, OperationDetail & operationdetail)
{
	return 0;
}

/**
 * resize_move
 */
bool GParted_Core::resize_move (const Device & device,
				const Partition & partition_old,
				Partition & partition_new,
				OperationDetail & operationdetail)
{
	if ( (partition_new.alignment == ALIGN_STRICT)
	    || (partition_new.alignment == ALIGN_MEBIBYTE)
	    || partition_new.strict_start
	    || calculate_exact_geom (partition_old, partition_new, operationdetail)
	  )
	{
		if (partition_old.type == TYPE_EXTENDED)
			return resize_move_partition (partition_old, partition_new, operationdetail);

		if (partition_new.sector_start == partition_old.sector_start)
			return resize (partition_old, partition_new, operationdetail);

		if (partition_new.get_sector_length() == partition_old.get_sector_length())
			return move (device, partition_old, partition_new, operationdetail);

		Partition temp;
		if (partition_new.get_sector_length() > partition_old.get_sector_length())
		{
			//first move, then grow. Since old.length < new.length and new.start is valid, temp is valid.
			temp = partition_new;
			temp.sector_end = temp.sector_start + partition_old.get_sector_length() -1;
		}

		if (partition_new.get_sector_length() < partition_old.get_sector_length())
		{
			//first shrink, then move. Since new.length < old.length and old.start is valid, temp is valid.
			temp = partition_old;
			temp.sector_end = partition_old.sector_start + partition_new.get_sector_length() -1;
		}

		PartitionAlignment previous_alignment = temp.alignment;
		temp.alignment = ALIGN_STRICT;
		bool success = resize_move (device, partition_old, temp, operationdetail);
		temp.alignment = previous_alignment;

		return success && resize_move (device, temp, partition_new, operationdetail);
	}

	return false;
}

/**
 * move
 */
bool GParted_Core::move (const Device & device,
			 const Partition & partition_old,
			 const Partition & partition_new,
			 OperationDetail & operationdetail)
{
	return 0;
}

/**
 * move_filesystem
 */
bool GParted_Core::move_filesystem (const Partition & partition_old,
				    const Partition & partition_new,
				    OperationDetail & operationdetail)
{
	return 0;
}

/**
 * resize
 */
bool GParted_Core::resize (const Partition & partition_old,
			   const Partition & partition_new,
			   OperationDetail & operationdetail)
{
	return 0;
}

/**
 * resize_move_partition
 */
bool GParted_Core::resize_move_partition (const Partition & partition_old,
					  const Partition & partition_new,
					  OperationDetail & operationdetail)
{
	return 0;
}

/**
 * resize_filesystem
 */
bool GParted_Core::resize_filesystem (const Partition & partition_old,
				      const Partition & partition_new,
				      OperationDetail & operationdetail,
				      bool fill_partition)
{
	return 0;
}

/**
 * maximize_filesystem
 */
bool GParted_Core::maximize_filesystem (const Partition & partition, OperationDetail & operationdetail)
{
	return 0;
}

/**
 * copy
 */
bool GParted_Core::copy (const Partition & partition_src,
			 Partition & partition_dst,
			 Byte_Value min_size,
			 OperationDetail & operationdetail)
{
	return 0;
}

/**
 * copy_filesystem_simulation
 */
bool GParted_Core::copy_filesystem_simulation (const Partition & partition_src,
					       const Partition & partition_dst,
					       OperationDetail & operationdetail)
{
	return 0;
}

/**
 * copy_filesystem
 */
bool GParted_Core::copy_filesystem (const Partition & partition_src,
				    const Partition & partition_dst,
				    OperationDetail & operationdetail,
				    bool readonly)
{
	Sector dummy;
	return copy_filesystem (partition_src.device_path,
				partition_dst.device_path,
				partition_src.sector_start,
				partition_dst.sector_start,
				partition_src.sector_size,
				partition_dst.sector_size,
				partition_src.get_byte_length(),
				operationdetail,
				readonly,
				dummy);
}

/**
 * copy_filesystem
 */
bool GParted_Core::copy_filesystem (const Partition & partition_src,
				    const Partition & partition_dst,
				    OperationDetail & operationdetail,
				    Byte_Value & total_done)
{
	return copy_filesystem (partition_src.device_path,
				partition_dst.device_path,
				partition_src.sector_start,
				partition_dst.sector_start,
				partition_src.sector_size,
				partition_dst.sector_size,
				partition_src.get_byte_length(),
				operationdetail,
				false,
				total_done);
}

/**
 * copy_filesystem
 */
bool GParted_Core::copy_filesystem (const Glib::ustring & src_device,
				    const Glib::ustring & dst_device,
				    Sector src_start,
				    Sector dst_start,
				    Byte_Value src_sector_size,
				    Byte_Value dst_sector_size,
				    Byte_Value src_length,
				    OperationDetail & operationdetail,
				    bool readonly,
				    Byte_Value & total_done)
{
	return 0;
}

/**
 * rollback_transaction
 */
void GParted_Core::rollback_transaction (const Partition & partition_src,
					 const Partition & partition_dst,
					 OperationDetail & operationdetail,
					 Byte_Value total_done)
{
}

/**
 * check_repair_filesystem
 */
bool GParted_Core::check_repair_filesystem (const Partition & partition, OperationDetail & operationdetail)
{
	return 0;
}

/**
 * set_partition_type
 */
bool GParted_Core::set_partition_type (const Partition & partition, OperationDetail & operationdetail)
{
	return 0;
}

/**
 * set_progress_info
 */
void GParted_Core::set_progress_info (Byte_Value total,
				      Byte_Value done,
				      const Glib::Timer & timer,
				      OperationDetail & operationdetail,
				      bool readonly)
{
}

/**
 * copy_blocks
 */
bool GParted_Core::copy_blocks (const Glib::ustring & src_device,
				const Glib::ustring & dst_device,
				Sector src_start,
				Sector dst_start,
				Byte_Value length,
				Byte_Value blocksize,
				OperationDetail & operationdetail,
				bool readonly,
				Byte_Value & total_done)
{
	return 0;
}

/**
 * copy_block
 */
bool GParted_Core::copy_block (PedDevice * lp_device_src,
			       PedDevice * lp_device_dst,
			       Sector offset_src,
			       Sector offset_dst,
			       Byte_Value block_length,
			       Glib::ustring & error_message,
			       bool readonly)
{
	Byte_Value sector_size_src = lp_device_src->sector_size;
	Byte_Value sector_size_dst = lp_device_dst->sector_size;

	//Handle case where src and dst sector sizes are different.
	//    E.g.,  5 sectors x 512 bytes/sector = ??? 2048 byte sectors
	Sector num_blocks_src = (llabs(block_length) + (sector_size_src - 1)) / sector_size_src;
	Sector num_blocks_dst = (llabs(block_length) + (sector_size_dst - 1)) / sector_size_dst;

	//Handle situation where we are performing copy operation beginning
	//  with the end of the partition and finishing with the start.
	if (block_length < 0)
	{
		block_length = llabs (block_length);
		offset_src -= ((block_length / sector_size_src) - 1);
		/* Handle situation where src sector size is smaller than dst sector size and an additional partial dst sector is required. */
		offset_dst -= (((block_length + (sector_size_dst - 1)) / sector_size_dst) - 1);
	}

	if (block_length != 0)
	{
		if (ped_device_read (lp_device_src, buf, offset_src, num_blocks_src))
		{
			if (readonly || ped_device_write (lp_device_dst, buf, offset_dst, num_blocks_dst))
				return true;
			else
				error_message = String::ucompose ("Error while writing block at sector %1", offset_dst);
		}
		else
			error_message = String::ucompose ("Error while reading block at sector %1", offset_src);
	}

	return false;
}

/**
 * calibrate_partition
 */
bool GParted_Core::calibrate_partition (Partition & partition, OperationDetail & operationdetail)
{
	return 0;
}

/**
 * calculate_exact_geom
 */
bool GParted_Core::calculate_exact_geom (const Partition & partition_old,
				         Partition & partition_new,
				         OperationDetail & operationdetail)
{
	return 0;
}

/**
 * set_proper_filesystem
 */
bool GParted_Core::set_proper_filesystem (const FILESYSTEM & filesystem)
{
	p_filesystem = get_filesystem_object (filesystem);

	return p_filesystem;
}

/**
 * get_filesystem_object
 */
FileSystem * GParted_Core::get_filesystem_object (const FILESYSTEM & filesystem)
{
	if (FILESYSTEM_MAP.count (filesystem))
	    return FILESYSTEM_MAP[filesystem];
	else
	    return NULL;
}

/**
 * update_bootsector
 */
bool GParted_Core::update_bootsector (const Partition & partition, OperationDetail & operationdetail)
{
	return 0;
}

/**
 * open_device
 */
bool GParted_Core::open_device (const Glib::ustring & device_path)
{
	lp_device = ped_device_get (device_path.c_str());

	return lp_device;
}

/**
 * open_device_and_disk
 */
bool GParted_Core::open_device_and_disk (const Glib::ustring & device_path, bool strict)
{
	lp_device = NULL;
	lp_disk = NULL;

	if (open_device (device_path))
	{
		lp_disk = ped_disk_new (lp_device);

		//if ! disk and writeable it's probably a HD without disklabel.
		//We return true here and deal with them in GParted_Core::get_devices
		if (lp_disk || (! strict && ! lp_device->read_only))
			return true;

		close_device_and_disk();
	}

	return false;
}

/**
 * close_disk
 */
void GParted_Core::close_disk()
{
	if (lp_disk)
		ped_disk_destroy (lp_disk);

	lp_disk = NULL;
}

/**
 * close_device_and_disk
 */
void GParted_Core::close_device_and_disk()
{
	close_disk();

	if (lp_device)
		ped_device_destroy (lp_device);

	lp_device = NULL;
}

/**
 * commit
 */
bool GParted_Core::commit()
{
	return true;
}

/**
 * commit_to_os
 */
bool GParted_Core::commit_to_os (std::time_t timeout)
{
	return true;
}

/**
 * settle_device
 */
void GParted_Core::settle_device (std::time_t timeout)
{
	if (! Glib::find_program_in_path ("udevsettle").empty())
		Utils::execute_command ("udevsettle --timeout=" + Utils::num_to_str (timeout));
	else if (! Glib::find_program_in_path ("udevadm").empty())
		Utils::execute_command ("udevadm settle --timeout=" + Utils::num_to_str (timeout));
	else
		sleep (timeout);
}

/**
 * ped_exception_handler
 */
PedExceptionOption GParted_Core::ped_exception_handler (PedException * e)
{
	PedExceptionOption ret = PED_EXCEPTION_UNHANDLED;
        std::cout << e->message << std::endl;

        libparted_messages.push_back (e->message);
	char optcount = 0;
	int opt = 0;
	for (char c = 0; c < 10; c++)
		if (e->options & (1 << c)) {
			opt = (1 << c);
			optcount++;
		}
	// if only one option was given, choose it without popup
	if (optcount == 1 && e->type != PED_EXCEPTION_BUG && e->type != PED_EXCEPTION_FATAL)
		return (PedExceptionOption)opt;
	if (Glib::Thread::self() != GParted_Core::mainthread)
		gdk_threads_enter();
	if (Glib::Thread::self() != GParted_Core::mainthread)
		gdk_threads_leave();
	if (ret < 0)
		ret = PED_EXCEPTION_UNHANDLED;
	return ret;
}

/**
 * ~GParted_Core
 */
GParted_Core::~GParted_Core()
{
	delete p_filesystem;
}

Glib::Thread *GParted_Core::mainthread;

