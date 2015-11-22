#ifndef MS_VERSION_H
#define	MS_VERSION_H

#include <string>

class Version
{
public:
	static const std::string GetVersion();
	static const std::string GetNameAndVersion();

public:
	static const int versionMajor;
	static const int versionMinor;
	static const int versionTiny;
	static const std::string name;
	static const std::string command;
	static const std::string copyright;
};

#endif
