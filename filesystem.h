#ifndef DEFINE_FILESYSTEM
#define DEFINE_FILESYSTEM

#include "operation.h"

#include <fstream>
#include <sys/stat.h>

class FileSystem
{
public:
	FileSystem();
	virtual ~FileSystem() {}

	virtual const Glib::ustring get_custom_text( CUSTOM_TEXT ttype, int index = 0 );
	static const Glib::ustring get_generic_text( CUSTOM_TEXT ttype, int index = 0 );

	virtual FS get_filesystem_support() = 0;
	virtual void set_used_sectors( Partition & partition ) = 0;
	virtual void read_label( Partition & partition ) = 0;
	virtual bool write_label( const Partition & partition, OperationDetail & operationdetail ) = 0;
	virtual void read_uuid( Partition & partition ) = 0;
	virtual bool write_uuid( const Partition & partition, OperationDetail & operationdetail ) = 0;
	virtual bool create( const Partition & new_partition, OperationDetail & operationdetail ) = 0;
	virtual bool resize( const Partition & partition_new,
			     OperationDetail & operationdetail,
			     bool fill_partition = false ) = 0;
	virtual bool move( const Partition & partition_new
	                 , const Partition & partition_old
	                 , OperationDetail & operationdetail
	                 ) = 0;
	virtual bool copy( const Glib::ustring & src_part_path,
			   const Glib::ustring & dest_part_path,
			   OperationDetail & operationdetail ) = 0;
	virtual bool check_repair( const Partition & partition, OperationDetail & operationdetail ) = 0;

protected:
	int execute_command( const Glib::ustring & command, OperationDetail & operationdetail );
	int execute_command_timed( const Glib::ustring & command
	                         , OperationDetail & operationdetail
	                         , bool check_status = true );
	Glib::ustring mk_temp_dir( const Glib::ustring & infix, OperationDetail & operationdetail );
	void rm_temp_dir( const Glib::ustring dir_name, OperationDetail & operationdetail );

	//those are used in several places..
	Glib::ustring output, error;
	Sector N, S;
	int exit_status;
	unsigned int index;

private:

};

#endif //DEFINE_FILESYSTEM
