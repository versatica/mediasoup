#define MS_CLASS "main"

#include "MediaSoup.h"
#include "Settings.h"
#include "Version.h"
#include "Daemon.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <string>
#include <cstdlib>  // std::_Exit()

int main(int argc, char* argv[])
{
	// Set this thread as main thread.
	Logger::ThreadInit("main");

	// Read command line arguments.
	Settings::ReadArguments(argc, argv);

	MS_NOTICE("starting %s", Version::GetNameAndVersion().c_str());

	// Set default settings.
	Settings::SetDefaultConfiguration();

	// Read configuration file (if given).
	if (!Settings::arguments.configFile.empty())
	{
		MS_INFO("reading configuration file '%s'", Settings::arguments.configFile.c_str());
		Settings::ReadConfigurationFile();
	}

	// Print configuration.
	Settings::PrintConfiguration();

	// Verify final configuration.
	Settings::ConfigurationPostCheck();

	// Set process stuff.
	MediaSoup::SetProcess();

	try
	{
		// Daemonize if requested.
		if (Settings::arguments.daemonize)
		{
			MS_NOTICE("daemonizing %s", Version::name.c_str());
			Daemon::Daemonize();
		}

		// Run MediaSoup.
		MediaSoup::Run();

		// End.
		MediaSoup::End();

		MS_EXIT_SUCCESS("%s ends", Version::name.c_str());
	}
	catch (const MediaSoupError &error)
	{
		// End.
		MediaSoup::End();

		// If this is the daemonized process don't log FAILURE EXIT as the
		// ancestor process will do it.
		if (Daemon::IsDaemonized())
		{
			MS_CRIT("%s", error.what());
			// Notify the ancestor process that the daemonized process failed.
			Daemon::SendErrorStatusToAncestor();
			// And silently die.
			std::_Exit(EXIT_FAILURE);
		}
		// Otherwise this is the ancestor process so log FAILURE EXIT.
		else
		{
			MS_EXIT_FAILURE("%s", error.what());
		}
	}
}
