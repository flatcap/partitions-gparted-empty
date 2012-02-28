#ifndef OPERATION
#define OPERATION

#include "device.h"

class OperationDetail {
public:
	OperationDetail() { }
};

enum OperationType {
	OPERATION_DELETE	= 0,
	OPERATION_CHECK		= 1,
	OPERATION_CREATE	= 2,
	OPERATION_RESIZE_MOVE	= 3,
	OPERATION_FORMAT	= 4,
	OPERATION_COPY		= 5,
	OPERATION_LABEL_PARTITION = 6,
	OPERATION_CHANGE_UUID	= 7
};

class Operation
{

public:
	Operation();
	virtual ~Operation() {}

	virtual void apply_to_visual( std::vector<Partition> & partitions ) = 0;
	virtual void create_description() = 0;

	//public variables
	Device device;
	OperationType type;
	Partition partition_original;
	Partition partition_new;

	Glib::RefPtr<Gdk::Pixbuf> icon;
	Glib::ustring description;

	OperationDetail operation_detail;

protected:
	int find_index_original( const std::vector<Partition> & partitions );
	int find_index_extended( const std::vector<Partition> & partitions );
	void insert_unallocated( std::vector<Partition> & partitions, Sector start, Sector end, Byte_Value sector_size, bool inside_extended );

	int index;
	int index_extended;
};

#endif //OPERATION
