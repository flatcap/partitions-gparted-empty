#include "utils.h"

#include <sstream>
#include <fstream>
#include <iomanip>
#include <glibmm/regex.h>
#include <locale.h>
#include <uuid/uuid.h>

/**
 * round
 */
Sector Utils::round (double double_value)
{
	 return static_cast<Sector> (double_value + 0.5);
}

/**
 * mk_label
 */
Gtk::Label * Utils::mk_label (const Glib::ustring & text,
			      bool use_markup,
			      Gtk::AlignmentEnum x_align,
			      Gtk::AlignmentEnum y_align,
			      bool wrap,
			      bool selectable,
			      const Glib::ustring & text_color)
{

	Gtk::Label * label = manage (new Gtk::Label (text, x_align, y_align));

	label->set_use_markup (use_markup);
	label->set_line_wrap (wrap);
	label->set_selectable (selectable);

	if (text_color != "black")
	{
		Gdk::Color color (text_color);
		label->modify_fg (label->get_state(), color);
	}

	return label;
}

/**
 * num_to_str
 */
Glib::ustring Utils::num_to_str (Sector number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

/**
 * get_color
 * use palette from http://developer.gnome.org/hig-book/2.32/design-color.html.en as a starting point.
 */
Glib::ustring Utils::get_color (FILESYSTEM filesystem)
{
	switch (filesystem)
	{
		case FS_UNALLOCATED	: return "#A9A9A9";	// ~ medium grey
		case FS_UNKNOWN		: return "#000000";	//black
		case FS_UNFORMATTED	: return "#000000";	//black
		case FS_EXTENDED	: return "#7DFCFE";	// ~ light blue
		case FS_BTRFS		: return "#FF9955";	//orange
		case FS_EXT2		: return "#9DB8D2";	//blue hilight
		case FS_EXT3		: return "#7590AE";	//blue medium
		case FS_EXT4		: return "#4B6983";	//blue dark
		case FS_LINUX_SWAP	: return "#C1665A";	//red medium
		case FS_FAT16		: return "#00FF00";	//green
		case FS_FAT32		: return "#18D918";	// ~ medium green
		case FS_EXFAT		: return "#2E8B57";	// ~ sea green
		case FS_NILFS2		: return "#826647";	//face skin dark
		case FS_NTFS		: return "#42E5AC";	// ~ light turquoise
		case FS_REISERFS	: return "#ADA7C8";	//purple hilight
		case FS_REISER4		: return "#887FA3";	//purple medium
		case FS_XFS			: return "#EED680";	//accent yellow
		case FS_JFS			: return "#E0C39E";	//face skin medium
		case FS_HFS			: return "#E0B6AF";	//red hilight
		case FS_HFSPLUS		: return "#C0A39E";	// ~ serene red
		case FS_UFS			: return "#D1940C";	//accent yellow dark
		case FS_USED		: return "#F8F8BA";	// ~ light tan yellow
		case FS_UNUSED		: return "#FFFFFF";	//white
		case FS_LVM2_PV		: return "#CC9966";	// ~ medium brown
		case FS_LUKS		: return "#625B81";	//purple dark

		default				: return "#000000";
	}
}

/**
 * get_color_as_pixbuf
 */
Glib::RefPtr<Gdk::Pixbuf> Utils::get_color_as_pixbuf (FILESYSTEM filesystem, int width, int height)
{
	Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create (Gdk::COLORSPACE_RGB, false, 8, width, height);

	if (pixbuf)
	{
		std::stringstream hex (get_color (filesystem).substr (1) + "00");
		unsigned long dec;
		hex >> std::hex >> dec;

		pixbuf->fill (dec);
	}

	return pixbuf;
}

/**
 * get_filesystem_string
 */
Glib::ustring Utils::get_filesystem_string (FILESYSTEM filesystem)
{
	switch (filesystem)
	{
		case FS_UNALLOCATED	: return
				/* TO TRANSLATORS:  unallocated
				 * means that this space on the disk device does
				 * not contain a recognized file system, and is in
				 * other words unallocated.
				 */
				"unallocated";
		case FS_UNKNOWN		: return
				/* TO TRANSLATORS:  unknown
				 * means that this space within this partition does
				 * not contain a file system known to GParted, and
				 * is in other words unknown.
				 */
				"unknown";
		case FS_UNFORMATTED	: return
				/* TO TRANSLATORS:  unformatted
				 * means that the space within this partition will not
				 * be formatted with a known file system by GParted.
				 */
				"unformatted";
		case FS_EXTENDED	: return "extended";
		case FS_BTRFS		: return "btrfs";
		case FS_EXT2		: return "ext2";
		case FS_EXT3		: return "ext3";
		case FS_EXT4		: return "ext4";
		case FS_LINUX_SWAP	: return "linux-swap";
		case FS_FAT16		: return "fat16";
		case FS_FAT32		: return "fat32";
		case FS_EXFAT		: return "exfat";
		case FS_NILFS2		: return "nilfs2";
		case FS_NTFS		: return "ntfs";
		case FS_REISERFS	: return "reiserfs";
		case FS_REISER4		: return "reiser4";
		case FS_XFS		: return "xfs";
		case FS_JFS		: return "jfs";
		case FS_HFS		: return "hfs";
		case FS_HFSPLUS		: return "hfs+";
		case FS_UFS		: return "ufs";
		case FS_USED		: return "used";
		case FS_UNUSED		: return "unused";
		case FS_LVM2_PV		: return "lvm2 pv";
		case FS_LUKS		: return "crypt-luks";

		default			: return "";
	}
}

/**
 * get_filesystem_software
 */
Glib::ustring Utils::get_filesystem_software (FILESYSTEM filesystem)
{
	switch (filesystem)
	{
		case FS_BTRFS       : return "btrfs-tools";
		case FS_EXT2        : return "e2fsprogs";
		case FS_EXT3        : return "e2fsprogs";
		case FS_EXT4        : return "e2fsprogs v1.41+";
		case FS_FAT16       : return "dosfstools, mtools";
		case FS_FAT32       : return "dosfstools, mtools";
		case FS_HFS         : return "hfsutils";
		case FS_HFSPLUS     : return "hfsprogs";
		case FS_JFS         : return "jfsutils";
		case FS_LINUX_SWAP  : return "util-linux";
		case FS_LVM2_PV     : return "lvm2";
		case FS_NILFS2      : return "nilfs-utils";
		case FS_NTFS        : return "ntfsprogs / ntfs-3g";
		case FS_REISER4     : return "reiser4progs";
		case FS_REISERFS    : return "reiserfsprogs";
		case FS_UFS         : return "";
		case FS_XFS         : return "xfsprogs, xfsdump";

		default             : return "";
	}
}

/**
 * kernel_supports_fs
 * Report whether or not the kernel supports a particular file system
 */
bool Utils::kernel_supports_fs (const Glib::ustring & fs)
{
	bool fs_supported = false;

	//Read /proc/filesystems and check for the file system name.
	//  Will succeed for compiled in drivers and already loaded
	//  moduler drivers.  If not found, try loading the driver
	//  as a module and re-checking /proc/filesystems.
	std::ifstream input;
	std::string line;
	input.open ("/proc/filesystems");
	if (input)
	{
		while (input >> line)
			if (line == fs)
			{
				fs_supported = true;
				break;
			}
		input.close();
	}
	if (fs_supported)
		return true;

	Glib::ustring output, error;
	execute_command ("modprobe " + fs, output, error, true);

	input.open ("/proc/filesystems");
	if (input)
	{
		while (input >> line)
			if (line == fs)
			{
				fs_supported = true;
				break;
			}
		input.close();
	}

	return fs_supported;
}

/**
 * kernel_version_at_least
 * Report if kernel version is >= (major, minor, patch)
 */
bool Utils::kernel_version_at_least (int major_ver, int minor_ver, int patch_ver)
{
	int actual_major_ver, actual_minor_ver, actual_patch_ver;
	if (! get_kernel_version (actual_major_ver, actual_minor_ver, actual_patch_ver))
		return false;
	bool result = (actual_major_ver > major_ver)
	              || (actual_major_ver == major_ver && actual_minor_ver > minor_ver)
	              || (actual_major_ver == major_ver && actual_minor_ver == minor_ver && actual_patch_ver >= patch_ver);
	return result;
}

/**
 * format_size
 */
Glib::ustring Utils::format_size (Sector sectors, Byte_Value sector_size)
{
	std::stringstream ss;
	ss << std::setiosflags (std::ios::fixed) << std::setprecision (2);

	if ((sectors * sector_size) < KIBIBYTE)
	{
		ss << sector_to_unit (sectors, sector_size, UNIT_BYTE);
		return String::ucompose ("%1 B", ss.str());
	}
	else if ((sectors * sector_size) < MEBIBYTE)
	{
		ss << sector_to_unit (sectors, sector_size, UNIT_KIB);
		return String::ucompose ("%1 KiB", ss.str());
	}
	else if ((sectors * sector_size) < GIBIBYTE)
	{
		ss << sector_to_unit (sectors, sector_size, UNIT_MIB);
		return String::ucompose ("%1 MiB", ss.str());
	}
	else if ((sectors * sector_size) < TEBIBYTE)
	{
		ss << sector_to_unit (sectors, sector_size, UNIT_GIB);
		return String::ucompose ("%1 GiB", ss.str());
	}
	else
	{
		ss << sector_to_unit (sectors, sector_size, UNIT_TIB);
		return String::ucompose ("%1 TiB", ss.str());
	}
}

/**
 * format_time
 */
Glib::ustring Utils::format_time (std::time_t seconds)
{
	Glib::ustring time;

	int unit = static_cast<int> (seconds / 3600);
	if (unit < 10)
		time += "0";
	time += num_to_str (unit) + ":";
	seconds %= 3600;

	unit = static_cast<int> (seconds / 60);
	if (unit < 10)
		time += "0";
	time += num_to_str (unit) + ":";
	seconds %= 60;

	if (seconds < 10)
		time += "0";
	time += num_to_str (seconds);

	return time;
}

/**
 * sector_to_unit
 */
double Utils::sector_to_unit (Sector sectors, Byte_Value sector_size, SIZE_UNIT size_unit)
{
	switch (size_unit)
	{
		case UNIT_BYTE	:
			return sectors * sector_size;

		case UNIT_KIB	:
			return sectors / (static_cast<double> (KIBIBYTE) / sector_size);
		case UNIT_MIB	:
			return sectors / (static_cast<double> (MEBIBYTE) / sector_size);
		case UNIT_GIB	:
			return sectors / (static_cast<double> (GIBIBYTE) / sector_size);
		case UNIT_TIB	:
			return sectors / (static_cast<double> (TEBIBYTE) / sector_size);

		default:
			return sectors;
	}
}

/**
 * execute_command
 */
int Utils::execute_command (const Glib::ustring & command)
{
	Glib::ustring dummy;
	return execute_command (command, dummy, dummy);
}

/**
 * execute_command
 */
int Utils::execute_command (const Glib::ustring & command,
			    Glib::ustring & output,
			    Glib::ustring & error,
			    bool use_C_locale)
{
	int exit_status = -1;
	std::string std_out, std_error;

	try
	{
		std::vector<std::string>argv;
		argv.push_back ("sh");
		argv.push_back ("-c");
		argv.push_back (command);

		if (use_C_locale)
		{
			//Spawn command using the C language environment
			std::vector<std::string> envp;
			envp.push_back ("LC_ALL=C");
			envp.push_back ("PATH=" + Glib::getenv ("PATH"));

			Glib::spawn_sync ("."
			                , argv
			                , envp
			                , Glib::SPAWN_SEARCH_PATH
			                , sigc::slot<void>()
			                , &std_out
			                , &std_error
			                , &exit_status
			               );
		}
		else
		{
			//Spawn command inheriting the parent's environment
			Glib::spawn_sync ("."
			                , argv
			                , Glib::SPAWN_SEARCH_PATH
			                , sigc::slot<void>()
			                , &std_out
			                , &std_error
			                , &exit_status
			               );
		}
	}
	catch (Glib::Exception & e)
	{
		 error = e.what();

		 return -1;
	}

	output = Utils::cleanup_cursor (std_out);
	error = std_error;

	return exit_status;
}

/**
 * regexp_label
 */
Glib::ustring Utils::regexp_label (const Glib::ustring & text
                                 , const Glib::ustring & pattern
                                )
{
	//Extract text from a regular sub-expression or pattern.
	//  E.g., "text we don't want (text we want)"
	std::vector<Glib::ustring> results;
	Glib::RefPtr<Glib::Regex> myregexp =
		Glib::Regex::create (pattern
		                   , Glib::REGEX_CASELESS | Glib::REGEX_MULTILINE
		                  );

	results = myregexp->split (text);

	if (results.size() >= 2)
		return results[1];
	else
		return "";
}

/**
 * fat_compliant_label
 */
Glib::ustring Utils::fat_compliant_label (const Glib::ustring & label)
{
	//Limit volume label to 11 characters
	Glib::ustring text = label;
	if (text.length() > 11)
		text.resize (11);
	return text;
}

/**
 * create_mtoolsrc_file
 */
Glib::ustring Utils::create_mtoolsrc_file (char file_name[], const char drive_letter,
		const Glib::ustring & device_path)
{
	//Create mtools config file
	//NOTE:  file_name will be changed by the mkstemp() function call.
	Glib::ustring err_msg = "";
	int fd;
	fd = mkstemp (file_name);
	if (fd != -1) {
		Glib::ustring fcontents =
			/* TO TRANSLATORS:  # Temporary file created by gparted.  It may be deleted.
			 * means that this file is only used while gparted is applying operations.
			 * If for some reason this file exists at any other time, then the message is
			 * meant to inform a user that the file can be deleted with no harmful effects.
			 * This file is typically created, exists for less than a few seconds, and is
			 * then deleted by gparted.  Under normal circumstances a user should never
			 * see this file.
			 */
			"# Temporary file created by gparted.  It may be deleted.\n";

		//The following file contents are mtools keywords (see man mtools.conf)
		fcontents = String::ucompose(
						"drive %1: file=\"%2\"\nmtools_skip_check=1\n", drive_letter, device_path
					);

		if (write (fd, fcontents.c_str(), fcontents.size()) == -1) {
			err_msg = String::ucompose(
							/* TO TRANSLATORS: looks like
							 * Label operation failed:  Unable to write to temporary file /tmp/Y56ZZ3M13LM.
							 */
							"Label operation failed:  Unable to write to temporary file %1.\n"
							, file_name
						);
		}
		close (fd);
	}
	else
	{
		err_msg = String::ucompose(
						/* TO TRANSLATORS: looks like
						 * Label operation failed:  Unable to create temporary file /tmp/Y56ZZ3M13LM.
						 */
						"Label operation failed:  Unable to create temporary file %1.\n"
						, file_name
					);
	}
	return err_msg;
}

/**
 * delete_mtoolsrc_file
 */
Glib::ustring Utils::delete_mtoolsrc_file (const char file_name[])
{
	//Delete mtools config file
	Glib::ustring err_msg = "";
	remove (file_name);
	return err_msg;
}

/**
 * trim
 */
Glib::ustring Utils::trim (const Glib::ustring & src, const Glib::ustring & c /* = " \t\r\n" */)
{
	//Trim leading and trailing whitespace from string
	Glib::ustring::size_type p2 = src.find_last_not_of(c);
	if (p2 == Glib::ustring::npos) return Glib::ustring();
	Glib::ustring::size_type p1 = src.find_first_not_of(c);
	if (p1 == Glib::ustring::npos) p1 = 0;
	return src.substr(p1, (p2-p1)+1);
}

/**
 * cleanup_cursor
 */
Glib::ustring Utils::cleanup_cursor (const Glib::ustring & text)
{
	std::istringstream in(text);
	std::ostringstream out;
	char ch;
	std::streampos startofline = out.tellp();

	while (in.get(ch))
	{
		switch(ch)
		{
			case '\r':
				if ('\n' != in.peek()) // for windows CRLF
					out.seekp(startofline);
				else
					out.put(ch);
				break;
			case '\b':
				if (out.tellp() > startofline)
					out.seekp(out.tellp() - std::streamoff(1));
				break;
			default:
				out.put(ch);
		}
		if (ch == '\n')
			startofline = out.tellp();
	}

	return out.str();
}

/**
 * get_lang
 */
Glib::ustring Utils::get_lang()
{
	//Extract base language from string that may look like "en_CA.UTF-8"
	//  and return in the form "en-CA"
	Glib::ustring lang = setlocale (LC_CTYPE, NULL);

	//Strip off anything after the period "." or at sign "@"
	lang = Utils::regexp_label (lang.c_str(), "^([^.@]*)");

	//Convert the underscore "_" to a hyphen "-"
	Glib::ustring sought = "_";
	Glib::ustring replacement = "-";
	//NOTE:  Application crashes if string replace is called and sought is not found,
	//       so we need to only perform replace if the sought is found.
	if (lang.find(sought) != Glib::ustring::npos)
		lang.replace (lang.find(sought), sought.size(), replacement);

	return lang;
}

/**
 * tokenize
 * Extract a list of tokens from any number of background separator characters
 * E.g., tokenize(str="  word1   word2   ", tokens, delimiters=" ")
 * -> tokens=["word1","word2]
 * The tokenize method copied and adapted from:
 * http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
 */
void Utils::tokenize (const Glib::ustring& str,
                      std::vector<Glib::ustring>& tokens,
                      const Glib::ustring& delimiters = " ")
{
	// Skip delimiters at beginning.
	Glib::ustring::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	Glib::ustring::size_type pos     = str.find_first_of(delimiters, lastPos);

	while (Glib::ustring::npos != pos || Glib::ustring::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}

/**
 * split
 * Split string on every delimiter, appending to the vector.  Inspired by:
 *   http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c/3616605#3616605
 *   E.g. using Utils::split(str, result, ":") for str-> result
 *   ""-> []   "a"-> ["a"]   "::"-> ["","",""]   ":a::bb"-> ["","a","","bb"]
 */
void Utils::split (const Glib::ustring& str,
                   std::vector<Glib::ustring>& result,
                   const Glib::ustring& delimiters    )
{
	//Special case zero length string to empty vector
	if (str == "")
		return;
	Glib::ustring::size_type fromPos  = 0;
	Glib::ustring::size_type delimPos = str.find_first_of (delimiters);
	while (Glib::ustring::npos != delimPos)
	{
		Glib::ustring word (str, fromPos, delimPos - fromPos);
		result.push_back (word);
		fromPos = delimPos + 1;
		delimPos = str.find_first_of (delimiters, fromPos);
	}
	Glib::ustring word (str, fromPos);
	result. push_back (word);
}

/**
 * convert_to_int
 * Converts a Glib::ustring into a int
 */
int Utils::convert_to_int(const Glib::ustring & src)
{
	int ret_val;
	std::istringstream stream(src);
	stream >> ret_val;

	return ret_val;
}

/**
 * generate_uuid
 * Create a new UUID
 */
Glib::ustring Utils::generate_uuid(void)
{
	uuid_t uuid;
	char uuid_str[UUID_STRING_LENGTH+1];

	uuid_generate(uuid);
	uuid_unparse(uuid,uuid_str);

	return uuid_str;
}

//private functions...

/**
 * get_kernel_version
 * Read kernel version, reporting successs or failure
 */
bool Utils::get_kernel_version (int & major_ver, int & minor_ver, int & patch_ver)
{
	static bool read_file = false;
	static int read_major_ver = -1;
	static int read_minor_ver = -1;
	static int read_patch_ver = -1;

	bool successs = false;
	if (! read_file)
	{
		std::ifstream input ("/proc/version");
		std::string line;
		if (input)
		{
			getline (input, line);
			sscanf (line.c_str(), "Linux version %d.%d.%d",
			        &read_major_ver, &read_minor_ver, &read_patch_ver);
			input.close();
		}
		read_file = true;
	}
	if (read_major_ver > -1 && read_minor_ver > -1 && read_patch_ver > -1)
	{
		major_ver = read_major_ver;
		minor_ver = read_minor_ver;
		patch_ver = read_patch_ver;
		successs = true;
	}
	return successs;
}

