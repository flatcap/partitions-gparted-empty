#include "device.h"

Device::Device()
{
	Reset();
}

void Device::Reset()
{
	paths .clear();
	partitions .clear();
	length = cylsize = 0;
	heads = sectors = cylinders = 0;
	model = disktype = "";
	sector_size = max_prims = highest_busy = 0;
	readonly = false;
}

void Device::add_path( const Glib::ustring & path, bool clear_paths )
{
	if ( clear_paths )
		paths .clear();

	paths .push_back( path );

	sort_paths_and_remove_duplicates();
}

void Device::add_paths( const std::vector<Glib::ustring> & paths, bool clear_paths )
{
	if ( clear_paths )
		this ->paths .clear();

	this ->paths .insert( this ->paths .end(), paths .begin(), paths .end() );

	sort_paths_and_remove_duplicates();
}

Glib::ustring Device::get_path() const
{
	if ( paths .size() > 0 )
		return paths .front();

	return "";
}

std::vector<Glib::ustring> Device::get_paths() const
{
	return paths;
}

bool Device::operator==( const Device & device ) const
{
	return this ->get_path() == device .get_path();
}

bool Device::operator!=( const Device & device ) const
{
	return ! ( *this == device );
}

void Device::sort_paths_and_remove_duplicates()
{
	//remove duplicates
	std::sort( paths .begin(), paths .end() );
	paths .erase( std::unique( paths .begin(), paths .end() ), paths .end() );

	//sort on length
	std::sort( paths .begin(), paths .end(), compare_paths );
}

bool Device::compare_paths( const Glib::ustring & A, const Glib::ustring & B )
{
	return A .length() < B .length();
}

Device::~Device()
{
}

