#define MS_CLASS "Logger"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include <uv.h>
#include "Channel/Notifier.hpp"
#include <json.hpp>

/* Class variables. */

const int64_t Logger::pid{ static_cast<int64_t>(uv_os_getpid()) };
char Logger::buffer[Logger::bufferSize];
std::string Logger::logfilename;
std::FILE* Logger::logfd {nullptr};
std::string Logger::backupBuffer;

/* Class methods. */

/*
  {mslogname: "path_to_mslog"};
*/
bool Logger::MSlogopen(json& data)
{
	MS_TRACE_STD();

	auto jsonLognameIt = data.find("mslogname");
	if (jsonLognameIt != data.end() && jsonLognameIt->is_string())
	{
		Logger::logfilename = jsonLognameIt->get<std::string>();
		Logger::logfd = std::fopen(Logger::logfilename.c_str(), "a");
		if (Logger::logfd)
		{
			return true;
		}
		// tell nodejs
		json msg = json::object();
		msg["source"] = "open";
		msg["error"] = strerror(errno);
		msg["file"] = Logger::logfilename;
		msg["data"] = data.dump();
		Channel::Notifier::Emit(std::to_string(Logger::pid), "failedlog", msg);
	}
	else
	{
		json msg = json::object();
		msg["source"] = "open";
		msg["error"] = "no valid mslogname";
		msg["file"] = "";
		msg["data"] = data.dump();
		Channel::Notifier::Emit(std::to_string(Logger::pid), "failedlog", msg);
	}
	// unsuccessful 
	Logger::logfd = nullptr;
	return false;
}

/*
* Log rotation: on external signal
* or in case MSlogwrite() failed.
* TODO: see that we don't call it excessively i.e. request from nodejs came in,
* and also this worker process has just called rotation recently because it failed logwrite?
* TBD: can I not signal at all and just rely on failed writes to refresh file descriptors?
* TBD: the other way around, just rely on logrotate message from nodejs
*/
void Logger::MSlogrotate()
{
	MS_TRACE_STD();
	
	// close and reopen to refresh fd
	Logger::MSlogclose();

	Logger::logfd = std::fopen(Logger::logfilename.c_str(), "a");
	if(Logger::logfd)
	{
		return;
	}

	// tell nodejs
	json data = json::object();
	data["source"] = "rotate";
	data["error"] = strerror(errno);
	data["file"] = Logger::logfilename;
	data["data"] = "";

	Channel::Notifier::Emit(std::to_string(Logger::pid), "failedlog", data);
}

void Logger::MSlogwrite(int written)
{
	MS_TRACE_STD();

	// Write backed up buffer first, the one that we failed to write on a previous attempt.
	if (!Logger::backupBuffer.empty())
	{
		if (!Logger::logfd || (EOF == std::fputs(Logger::backupBuffer.c_str(), Logger::logfd)) || (EOF == std::fputc('\n', Logger::logfd)))
		{
			// If failed to write previously saved log msg, send it to nodejs and tell we failed
			json data = json::object();
			data["source"] = "write";
			data["error"] = strerror(errno);
			data["file"] = Logger::logfilename;
			data["data"] = Logger::backupBuffer;

			Channel::Notifier::Emit(std::to_string(Logger::pid), "failedlog", data);

			// Back up a new log msg, try writing it out next time
			Logger::backupBuffer.assign(Logger::buffer, written);
			return;
		}

		Logger::backupBuffer.erase();
	}

	// Write a new log message. If failed, save it for another time
	if (!Logger::logfd || (EOF == std::fputs(Logger::buffer, Logger::logfd)) || (EOF == std::fputc('\n', Logger::logfd)))
	{
    Logger::backupBuffer.assign(Logger::buffer, written);
		
		// Try refreshing file descriptor and hope file write succeeds next time
		Logger::MSlogrotate();
	}
}

void Logger::MSlogclose()
{
	MS_TRACE_STD();

	if (Logger::logfd)
	{
		std::fclose(Logger::logfd);
		Logger::logfd = nullptr;
	}
}