#define MS_CLASS "Utils::File"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
#include <cerrno>
#include <sys/stat.h> // stat()
#include <unistd.h>   // access(), R_OK

namespace Utils
{
	void Utils::File::CheckFile(const char* file)
	{
		MS_TRACE();

		// clang-format off
		struct stat fileStat{};
		// clang-format on
		int err;

		// Ensure the given file exists.
		err = stat(file, &fileStat);
		if (err != 0)
			MS_THROW_ERROR("cannot read file '%s': %s", file, std::strerror(errno));

		// Ensure it is a regular file.
		if (!S_ISREG(fileStat.st_mode))
			MS_THROW_ERROR("'%s' is not a regular file", file);

		// Ensure it is readable.
		err = access(file, R_OK);
		if (err != 0)
			MS_THROW_ERROR("cannot read file '%s': %s", file, std::strerror(errno));
	}
} // namespace Utils
