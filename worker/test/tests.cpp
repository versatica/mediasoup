#define CATCH_CONFIG_RUNNER

#include "include/catch.hpp"
#include "Settings.hpp"
#include "LogLevel.hpp"
#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "Utils.hpp"

static void init();
static void destroy();

int main(int argc, char* argv[])
{
	Settings::configuration.logLevel = LogLevel::LOG_DEBUG;
	Settings::configuration.logTags.rtp = true;
	Settings::configuration.logTags.rtcp = true;

	init();

	int ret = Catch::Session().run(argc, argv);

	destroy();

	return ret;
}

void init()
{
	// Initialize static stuff.
	DepLibUV::ClassInit();
	DepOpenSSL::ClassInit();
	Utils::Crypto::ClassInit();
}

void destroy()
{
	// Free static stuff.
	Utils::Crypto::ClassDestroy();
	DepOpenSSL::ClassDestroy();
	DepLibUV::ClassDestroy();
}
