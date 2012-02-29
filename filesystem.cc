#include "filesystem.h"

#include <cerrno>

/**
 * FileSystem
 */
FileSystem::FileSystem()
{
}

/**
 * get_custom_text
 */
const Glib::ustring FileSystem::get_custom_text (CUSTOM_TEXT ttype, int index)
{
	return get_generic_text (ttype, index);
}

/**
 * get_generic_text
 */
const Glib::ustring FileSystem::get_generic_text (CUSTOM_TEXT ttype, int index)
{
	/*TO TRANSLATORS: these labels will be used in the partition menu */
	static const Glib::ustring activate_text =  "_Mount" ;
	static const Glib::ustring deactivate_text =  "_Unmount" ;

	switch (ttype) {
		case CTEXT_ACTIVATE_FILESYSTEM :
			return index == 0 ? activate_text : "";
		case CTEXT_DEACTIVATE_FILESYSTEM :
			return index == 0 ? deactivate_text : "";
		default :
			return "";
	}
}

/**
 * execute_command
 */
int FileSystem::execute_command (const Glib::ustring & command, OperationDetail & operationdetail)
{
	return 0;
}

/**
 * execute_command_timed
 * Time command, add results to operation detail and by default set successs or failure
 */
int FileSystem::execute_command_timed (const Glib::ustring & command
                                     , OperationDetail & operationdetail
                                     , bool check_status)
{
	return 0;
}

/**
 * mk_temp_dir
 * Create uniquely named temporary directory and add results to operation detail
 */
Glib::ustring FileSystem::mk_temp_dir (const Glib::ustring & infix, OperationDetail & operationdetail)
{
	return "";
}

/**
 * rm_temp_dir
 * Remove directory and add results to operation detail
 */
void FileSystem::rm_temp_dir (const Glib::ustring dir_name, OperationDetail & operationdetail)
{
}

