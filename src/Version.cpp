#define MS_CLASS "Version"

#include "Version.h"
#include "Logger.h"

/* Static variables. */

const int Version::versionMajor = 0;
const int Version::versionMinor = 0;
const int Version::versionTiny = 1;

const std::string Version::name = "MediaSoup";
const std::string Version::command = "mediasoup";
const std::string Version::copyright = "Copyright (c) 2014 Iñaki Baz Castillo | https://github.com/ibc";

/* Static methods .*/

const std::string Version::GetVersion()
{
	MS_TRACE();

	std::string version;

	version.append(std::to_string(Version::versionMajor));
	version.append(".");
	version.append(std::to_string(Version::versionMinor));
	version.append(".");
	version.append(std::to_string(Version::versionTiny));

	return version;
}

const std::string Version::GetNameAndVersion()
{
	MS_TRACE();

	std::string name_and_version;

	name_and_version.append(Version::name);
	name_and_version.append(" v");
	name_and_version.append(Version::GetVersion());

	return name_and_version;
}
