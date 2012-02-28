#ifndef PARTITION
#define PARTITION

#include "utils.h"

enum PartitionType {
	TYPE_PRIMARY		=	0,
	TYPE_LOGICAL		=	1,
	TYPE_EXTENDED		=	2,
	TYPE_UNALLOCATED	=	3
};

enum PartitionStatus {
	STAT_REAL	=	0,
	STAT_NEW	=	1,
	STAT_COPY	=	2,
	STAT_FORMATTED	=	3
};

enum PartitionAlignment {
	ALIGN_CYLINDER = 0,    //Align to nearest cylinder
	ALIGN_MEBIBYTE = 1,    //Align to nearest mebibyte
	ALIGN_STRICT   = 2     //Strict alignment - no rounding
	                       //  Indicator if start and end sectors must remain unchanged
};

//FIXME: we should make a difference between partition- and file system size.
//especially in the core this can make a difference when giving detailed feedback. With new cairosupport in gtkmm
//it's even possible to make stuff like this visible in the GUI in a nice and clear way
class Partition
{
public:
	Partition();
	Partition( const Glib::ustring & path );
	~Partition();

	void Reset();

	//simple Set-functions.  only for convenience, since most members are public
	void Set(	const Glib::ustring & device_path,
			const Glib::ustring & partition,
			int partition_number,
			PartitionType type,
			FILESYSTEM filesystem,
			Sector sector_start,
			Sector sector_end,
			Byte_Value sector_size,
			bool inside_extended,
			bool busy );

	void Set_Unused( Sector sectors_unused );
	void set_used( Sector sectors_used );

	void Set_Unallocated( const Glib::ustring & device_path,
			      Sector sector_start,
			      Sector sector_end,
			      Byte_Value sector_size,
			      bool inside_extended );

	//update partition number (used when a logical partition is deleted)
	void Update_Number( int new_number );

	void add_path( const Glib::ustring & path, bool clear_paths = false );
	void add_paths( const std::vector<Glib::ustring> & paths, bool clear_paths = false );
	Byte_Value get_byte_length() const;
	Sector get_sector_length() const;
	Glib::ustring get_path() const;
	std::vector<Glib::ustring> get_paths() const;
	void add_mountpoint( const Glib::ustring & mountpoint, bool clear_mountpoints = false );
	void add_mountpoints( const std::vector<Glib::ustring> & mountpoints, bool clear_mountpoints = false );
	Glib::ustring get_mountpoint() const;
	void clear_mountpoints();
	std::vector<Glib::ustring> get_mountpoints() const;
	Sector get_sector() const;
	bool test_overlap( const Partition & partition ) const;

	bool operator==( const Partition & partition ) const;
	bool operator!=( const Partition & partition ) const;

	//some public members
	Glib::ustring device_path;
	int partition_number;
	PartitionType type;// UNALLOCATED, PRIMARY, LOGICAL, etc...
	PartitionStatus status; //STAT_REAL, STAT_NEW, etc..
	PartitionAlignment alignment;   //ALIGN_CYLINDER, ALIGN_STRICT, etc
	FILESYSTEM filesystem;
	Glib::ustring label;
	Glib::ustring uuid;
	Sector sector_start;
	Sector sector_end;
	Sector sectors_used;
	Sector sectors_unused;
	Gdk::Color color;
	bool inside_extended;
	bool busy;
	std::vector<Glib::ustring> messages;
	std::vector<Glib::ustring> flags;

	std::vector<Partition> logicals;

	bool strict_start;	//Indicator if start sector must stay unchanged
	Sector free_space_before;  //Free space preceding partition value

	Byte_Value sector_size;  //Sector size of the disk device needed for converting to/from sectors and bytes.

private:
	void sort_paths_and_remove_duplicates();

	static bool compare_paths( const Glib::ustring & A, const Glib::ustring & B );

	std::vector<Glib::ustring> paths;
	std::vector<Glib::ustring> mountpoints;
};

#endif //PARTITION
