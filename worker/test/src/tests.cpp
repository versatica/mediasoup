#define CATCH_CONFIG_RUNNER

#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "LogLevel.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "catch.hpp"

int main(int argc, char* argv[])
{
	Settings::configuration.logLevel = LogLevel::LOG_NONE;

	// Initialize static stuff.
	DepLibUV::ClassInit();
	DepOpenSSL::ClassInit();
	Utils::Crypto::ClassInit();

	int status = Catch::Session().run(argc, argv);

	// Free static stuff.
	DepLibUV::ClassDestroy();
	Utils::Crypto::ClassDestroy();

	return status;
}
