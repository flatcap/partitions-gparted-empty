/* Proc_Partitions_Info
 *
 * A persistent cache of information from the file /proc/partitions
 * that helps to minimize the number of required disk reads.
 */

#ifndef PROC_PARTITONS_INFO_H_
#define PROC_PARTITONS_INFO_H_

#include "utils.h"

class Proc_Partitions_Info
{
public:
	Proc_Partitions_Info();
	Proc_Partitions_Info( bool do_refresh );
	~Proc_Partitions_Info();
	std::vector<Glib::ustring> get_device_paths();
	std::vector<Glib::ustring> get_alternate_paths( const Glib::ustring & path );
private:
	void load_proc_partitions_info_cache();
	static bool proc_partitions_info_cache_initialized;
	static std::vector<Glib::ustring> device_paths_cache;
	static std::map< Glib::ustring, Glib::ustring > alternate_paths_cache;
};

#endif /*PROC_PARTITONS_INFO_H_*/
