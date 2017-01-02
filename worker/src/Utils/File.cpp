#define MS_CLASS "Utils::File"
// #define MS_LOG_DEV

#include "Utils.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <cerrno>
#include <unistd.h> // access(), R_OK
#include <sys/stat.h> // stat()

namespace Utils
{
	void Utils::File::CheckFile(const char* file)
	{
		MS_TRACE();

		struct stat file_stat;
		int err;

		// Ensure the given file exists.
		err = stat(file, &file_stat);
		if (err)
			MS_THROW_ERROR("cannot read file '%s': %s", file, std::strerror(errno));

		// Ensure it is a regular file.
		if (!S_ISREG(file_stat.st_mode))
			MS_THROW_ERROR("'%s' is not a regular file", file);

		// Ensure it is readable.
		err = access(file, R_OK);
		if (err)
			MS_THROW_ERROR("cannot read file '%s': %s", file, std::strerror(errno));
	}
}
