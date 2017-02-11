#define CATCH_CONFIG_RUNNER

#include "include/catch.hpp"
#include "Settings.h"
#include "LogLevel.h"

int main(int argc, char* argv[])
{
	Settings::configuration.logLevel = LogLevel::LOG_DEBUG;
	Settings::configuration.logTags.rtp = true;
	Settings::configuration.logTags.rtcp = true;

	int ret = Catch::Session().run(argc, argv);

	return ret;
}
