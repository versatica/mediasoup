#define MS_CLASS "Logger"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include <uv.h>
#include "Channel/ChannelNotifier.hpp"

/* Class variables. */

std::string Logger::levelPrefix;
const int64_t Logger::pid{ static_cast<int64_t>(uv_os_getpid()) };
thread_local char Logger::buffer[Logger::bufferSize];
thread_local std::string Logger::backupBuffer;
thread_local std::string Logger::appdataBuffer;
std::string Logger::logfilename = "";
std::FILE* Logger::logfd {nullptr};
bool Logger::openLogFile {false};

/* Class methods. */

/*
  {mslogname: "path_to_mslog"};
*/
bool Logger::MSlogopen(json& data)
{
	auto jsonLognameIt = data.find("mslogname");
	if (jsonLognameIt != data.end() && jsonLognameIt->is_string())
	{
		Logger::logfilename = jsonLognameIt->get<std::string>();
		Logger::logfd = std::fopen(Logger::logfilename.c_str(), "a");
		if (Logger::logfd)
		{
			Logger::openLogFile = true;
			return true;
		}
		// tell nodejs
		json msg = json::object();
		msg["source"] = "open";
		msg["error"] = strerror(errno);
		msg["file"] = Logger::logfilename;
		msg["data"] = data.dump();

		if (Channel::ChannelNotifier::channel)
			Channel::ChannelNotifier::Emit(std::to_string(Logger::pid), "failedlog", msg);
	}
	else
	{
		json msg = json::object();
		msg["source"] = "open";
		msg["error"] = "no valid mslogname";
		msg["file"] = "";
		msg["data"] = data.dump();
	
		if (Channel::ChannelNotifier::channel)
			Channel::ChannelNotifier::Emit(std::to_string(Logger::pid), "failedlog", msg);
	}
	// unsuccessful 
	Logger::logfd = nullptr;
	return false;
}


/*
* Log rotation: on external signal
* or in case MSlogwrite() failed.
*/
void Logger::MSlogrotate()
{
	MS_TRACE_STD();
	
	if (Logger::openLogFile)
	{
		// close and reopen to refresh fd
		Logger::MSlogclose();

		Logger::logfd = std::fopen(Logger::logfilename.c_str(), "a");
		if(Logger::logfd)
		{
			return;
		}
	}

	// tell nodejs
	json data = json::object();
	data["source"] = "rotate";
	data["error"] = strerror(errno);
	data["file"] = Logger::logfilename;
	data["data"] = Logger::openLogFile ? "failed logrotate" : "skip logrotate";

	if (Channel::ChannelNotifier::channel)
		Channel::ChannelNotifier::Emit(std::to_string(Logger::pid), "failedlog", data);
}


void Logger::MSlogwrite(int written)
{
	// Write backed up buffer first, the one that we failed to write on a previous attempt.
	if (!Logger::backupBuffer.empty())
	{
		if (!Logger::openLogFile
			|| !Logger::logfd 
			|| (EOF == std::fputs(Logger::backupBuffer.c_str(), Logger::logfd)) 
			|| (EOF == std::fputc('\n', Logger::logfd)))
		{
			// If failed to write previously saved log msg, send it to nodejs and tell we failed
			json data = json::object();
			data["source"] = "write";
			data["error"] = strerror(errno);
			data["file"] = Logger::logfilename;
			data["data"] = Logger::backupBuffer;

			// Back up a new log msg, try writing it out next time
			Logger::backupBuffer.assign(Logger::buffer, written);

			if (Channel::ChannelNotifier::channel)
				Channel::ChannelNotifier::Emit(std::to_string(Logger::pid), "failedlog", data);
			return;
		}

		Logger::backupBuffer.erase();
	}
	// Write a new log message. If failed, save it for another time
	if (!Logger::openLogFile
		|| !Logger::logfd 
		|| (EOF == std::fputs(Logger::buffer, Logger::logfd)) 
		|| (EOF == std::fputs("\"\n", Logger::logfd)))
	{
	  Logger::backupBuffer.assign(Logger::buffer, written);
		
		// Try refreshing file descriptor and hope file write succeeds next time
		if (!Logger::logfilename.empty())
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