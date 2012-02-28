#ifndef GPARTED_CORE
#define GPARTED_CORE

#include "filesystem.h"
#include "operation.h"

#include <parted/parted.h>
#include <vector>
#include <fstream>

class GParted_Core
{
public:
	static Glib::Thread *mainthread;
	GParted_Core();
	~GParted_Core();

	void find_supported_filesystems();
	void set_user_devices( const std::vector<Glib::ustring> & user_devices );
	void set_devices( std::vector<Device> & devices );
	void guess_partition_table(const Device & device, Glib::ustring &buff);

	bool snap_to_cylinder( const Device & device, Partition & partition, Glib::ustring & error );
	bool snap_to_mebibyte( const Device & device, Partition & partition, Glib::ustring & error );
	bool snap_to_alignment( const Device & device, Partition & partition, Glib::ustring & error );
	bool apply_operation_to_disk( Operation * operation );

	bool set_disklabel( const Glib::ustring & device_path, const Glib::ustring & disklabel );

	bool toggle_flag( const Partition & partition, const Glib::ustring & flag, bool state );

	const std::vector<FS> & get_filesystems() const;
	const FS & get_fs( FILESYSTEM filesystem ) const;
	std::vector<Glib::ustring> get_disklabeltypes();
	std::vector<Glib::ustring> get_all_mountpoints();
	std::map<Glib::ustring, bool> get_available_flags( const Partition & partition );
	Glib::ustring get_libparted_version();
	Glib::ustring get_thread_status_message();

	FileSystem * get_filesystem_object( const FILESYSTEM & filesystem );
private:
	//detectionstuff..
	void init_maps();
	void set_thread_status_message( Glib::ustring msg );
	void read_mountpoints_from_file( const Glib::ustring & filename,
					 std::map< Glib::ustring, std::vector<Glib::ustring> > & map );
	void read_mountpoints_from_file_swaps(
		const Glib::ustring & filename,
		std::map< Glib::ustring, std::vector<Glib::ustring> > & map );
	Glib::ustring get_partition_path( PedPartition * lp_partition );
	void set_device_partitions( Device & device );
	FILESYSTEM get_filesystem();
	void read_label( Partition & partition );
	void read_uuid( Partition & partition );
	void insert_unallocated( const Glib::ustring & device_path,
				 std::vector<Partition> & partitions,
				 Sector start,
				 Sector end,
				 Byte_Value sector_size,
				 bool inside_extended );
	void set_mountpoints( std::vector<Partition> & partitions );
	void set_used_sectors( std::vector<Partition> & partitions );
	void set_flags( Partition & partition );

	//operationstuff...
	bool create( const Device & device, Partition & new_partition, OperationDetail & operationdetail );
	bool create_partition( Partition & new_partition, OperationDetail & operationdetail, Sector min_size = 0 );
	bool create_filesystem( const Partition & partition, OperationDetail & operationdetail );

	bool format( const Partition & partition, OperationDetail & operationdetail );

	bool Delete( const Partition & partition, OperationDetail & operationdetail );

	bool label_partition( const Partition & partition, OperationDetail & operation_detail );

	bool change_uuid( const Partition & partition, OperationDetail & operation_detail );

	bool resize_move( const Device & device,
			  const Partition & partition_old,
			  Partition & partition_new,
			  OperationDetail & operationdetail );
	bool move( const Device & device,
		   const Partition & partition_old,
		   const Partition & partition_new,
		   OperationDetail & operationdetail );
	bool move_filesystem( const Partition & partition_old,
			      const Partition & partition_new,
			      OperationDetail & operationdetail );
	bool resize( const Partition & partition_old,
		     const Partition & partition_new,
		     OperationDetail & operationdetail );
	bool resize_move_partition( const Partition & partition_old,
				    const Partition & partition_new,
				    OperationDetail & operationdetail );
	bool resize_filesystem( const Partition & partition_old,
				const Partition & partition_new,
				OperationDetail & operationdetail,
				bool fill_partition = false );
	bool maximize_filesystem( const Partition & partition, OperationDetail & operationdetail );

	bool copy( const Partition & partition_src,
		   Partition & partition_dst,
		   Byte_Value min_size,
		   OperationDetail & operationdetail );
	bool copy_filesystem_simulation( const Partition & partition_src,
					 const Partition & partition_dst,
					 OperationDetail & operationdetail );
	bool copy_filesystem( const Partition & partition_src,
			      const Partition & partition_dst,
			      OperationDetail & operationdetail,
			      bool readonly = false );
	bool copy_filesystem( const Partition & partition_src,
			      const Partition & partition_dst,
			      OperationDetail & operationdetail,
			      Byte_Value & total_done );
	bool copy_filesystem( const Glib::ustring & src_device,
			      const Glib::ustring & dst_device,
			      Sector src_start,
			      Sector dst_start,
			      Byte_Value src_sector_size,
			      Byte_Value dst_sector_size,
			      Byte_Value src_length,
			      OperationDetail & operationdetail,
			      bool readonly,
			      Byte_Value & total_done );
	void rollback_transaction( const Partition & partition_src,
				   const Partition & partition_dst,
				   OperationDetail & operationdetail,
				   Byte_Value total_done );

	bool check_repair_filesystem( const Partition & partition, OperationDetail & operationdetail );

	bool set_partition_type( const Partition & partition, OperationDetail & operationdetail );

	void set_progress_info( Sector total, Sector done, const Glib::Timer & timer, OperationDetail & operationdetail, bool readonly );

	bool copy_blocks( const Glib::ustring & src_device,
			  const Glib::ustring & dst_device,
			  Sector src_start,
			  Sector dst_start,
			  Byte_Value length,
			  Byte_Value blocksize,
			  OperationDetail & operationdetail,
			  bool readonly,
			  Byte_Value & total_done );

	bool copy_block( PedDevice * lp_device_src,
			 PedDevice * lp_device_dst,
			 Sector offset_src,
			 Sector offset_dst,
			 Byte_Value blocksize,
			 Glib::ustring & error_message,
			 bool readonly );
	bool calibrate_partition( Partition & partition, OperationDetail & operationdetail );
	bool calculate_exact_geom( const Partition & partition_old,
			           Partition & partition_new,
				   OperationDetail & operationdetail );
	bool set_proper_filesystem( const FILESYSTEM & filesystem );
	bool update_bootsector( const Partition & partition, OperationDetail & operationdetail );

	//general..
	bool open_device( const Glib::ustring & device_path );
	bool open_device_and_disk( const Glib::ustring & device_path, bool strict = true );
	void close_disk();
	void close_device_and_disk();
	bool commit();
	bool commit_to_os( std::time_t timeout );
	void settle_device( std::time_t timeout );

	static PedExceptionOption ped_exception_handler( PedException * e );

	std::vector<FS> FILESYSTEMS;
	std::map< FILESYSTEM, FileSystem * > FILESYSTEM_MAP;
	FileSystem * p_filesystem;
	std::vector<PedPartitionFlag> flags;
	Glib::ustring temp;
	Partition partition_temp;
	FS fs;
	std::vector<Glib::ustring> device_paths;
	bool probe_devices;
	Glib::ustring thread_status_message;  //Used to pass data to show_pulsebar method
	Glib::RefPtr<Glib::IOChannel> iocInput, iocOutput; // Used to send data to gpart command

	std::map< Glib::ustring, std::vector<Glib::ustring> > mount_info;
	std::map< Glib::ustring, std::vector<Glib::ustring> > fstab_info;
	std::map< Glib::ustring, std::vector<Glib::ustring> >::iterator iter_mp;

	PedDevice *lp_device;
	PedDisk *lp_disk;
	PedPartition *lp_partition;

	char * buf;
};

#endif //GPARTED_CORE
