#include "operation.h"

/**
 * Operation
 */
Operation::Operation()
{
}

/**
 * find_index_original
 */
int Operation::find_index_original (const std::vector<Partition> & partitions)
{
	for (unsigned int t = 0; t < partitions.size(); t++)
		if (partition_original.sector_start >= partitions[t].sector_start &&
		     partition_original.sector_end <= partitions[t].sector_end)
			return t;

	return -1;
}

/**
 * find_index_extended
 */
int Operation::find_index_extended (const std::vector<Partition> & partitions)
{
	for (unsigned int t = 0; t < partitions.size(); t++)
		if (partitions[t].type == TYPE_EXTENDED)
			return t;

	return -1;
}

/**
 * insert_unallocated
 */
void Operation::insert_unallocated (std::vector<Partition> & partitions, Sector start, Sector end, Byte_Value sector_size, bool inside_extended)
{
	Partition UNALLOCATED;
	UNALLOCATED.Set_Unallocated (device.get_path(), 0, 0, sector_size, inside_extended);

	//if there are no partitions at all..
	if (partitions.empty())
	{
		UNALLOCATED.sector_start = start;
		UNALLOCATED.sector_end = end;

		partitions.push_back (UNALLOCATED);

		return;
	}

	//start <---> first partition start
	if ((partitions.front().sector_start - start) > (MEBIBYTE / sector_size))
	{
		UNALLOCATED.sector_start = start;
		UNALLOCATED.sector_end = partitions.front().sector_start -1;

		partitions.insert (partitions.begin(), UNALLOCATED);
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
			UNALLOCATED.sector_start = partitions[t].sector_end +1;
			UNALLOCATED.sector_end = partitions[t +1].sector_start -1;

			partitions.insert (partitions.begin() + ++t, UNALLOCATED);
		}
	}

	//last partition end <---> end
	if ((end - partitions.back().sector_end) >= (MEBIBYTE / sector_size))
	{
		UNALLOCATED.sector_start = partitions.back().sector_end +1;
		UNALLOCATED.sector_end = end;

		partitions.push_back (UNALLOCATED);
	}
}

