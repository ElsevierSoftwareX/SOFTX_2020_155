#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string>
#include <unistd.h>
#include <daqmap.h>

extern "C" {
#include "param.h"
}

/// noop callback we need here; local to this source code file
static int noop(char *channel_name, struct CHAN_PARAM *params, void *user){ return 1;};

/// Check on a file's modification time and then the CRC .
/// This can be used to periodically check on some config files to ascertain they
/// are not changed from the underneath of a running application.
class file_checker {
public:
	/// Save the file name and the initial CRC
	file_checker(std::string name, unsigned long incrc, unsigned long intag = 0)
		: file_name(name), crc(incrc), mtime(0), tag(intag)
 	{
		mtime = fmtime();
	}
	std::string file_name;	// File name
	unsigned long crc;	// The original CRC
	unsigned long mtime;	// File modification time during the construction time
	unsigned long tag;	// Tag given during the construction time
public:
	/// Recalculate the CRC on the file; returns 1 if matches or zero if not
	bool match() {
	  unsigned long new_crc;
	  // See if the file was touched and parse then
	  time_t mt = fmtime();
	  if (mt == mtime) return 1; // Same mtime, assume no change
	  // Parse and compute the CRC sum
	  int res = parseConfigFile(const_cast<char *>(file_name.c_str()), &new_crc, noop, 0, 0, 0);
	  // If parsed correctly and CRC is the same, return 1, no change (match)
	  // Record new modification time this avoid re-parsing the file all the time
	  if (res && new_crc == crc) {
		mtime = mt;
	  	return 1;
	  }
	  return 0;
	}
private:
	/// Determine my file's mod time
	time_t fmtime() {
	    struct stat sb;
	    // Open the file read-only
            int fd = open(file_name.c_str(), O_RDONLY);
            if (fd == -1) {
              system_log(1, "failed to open file %s in file_checker; errno %d", file_name.c_str(), errno);
	      return 0;
            } else if (fstat(fd, &sb) == -1) { // File stat to get the modification time
              system_log(1, "failed to fstat file %s in file_checker; errno %d", file_name.c_str(), errno);
	      close(fd);
	      return 0;
            } else {
	      close(fd);
	      return sb.st_mtime; // Return the modification time
	    }
	    return 0;
	}
};

#if 0
int main(int argc, char *argv[]) {
	file_checker f(argv[1], atoi(argv[2]));
	sleep(2);
	return f.match();
}
#endif
