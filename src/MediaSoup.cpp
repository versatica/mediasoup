#define MS_CLASS "MediaSoup"

#include "MediaSoup.h"
#include "Settings.h"
#include "Daemon.h"
#include "Dispatcher.h"
#include "Worker.h"
#include "LibUV.h"
#include "OpenSSL.h"
#include "LibSRTP.h"
#include "Utils.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPServer.h"
#include "RTC/DTLSHandler.h"
#include "RTC/SRTPSession.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <vector>
#include <map>
#include <string>
#include <csignal>  // sigaction()
#include <cstdlib>  // std::strtol()
#include <cerrno>
#include <cstdint>  // uint8_t, etc
#include <ctime>  // nanosleep()
#include <sys/resource.h>  // setrlimit(), setrlimit()
#include <unistd.h>  // setgid(), setuid(), initgroups()
#include <pwd.h>
#include <grp.h>
#include <uv.h>

#ifndef MS_RECOMMENDED_NOFILE
#define MS_RECOMMENDED_NOFILE  131072
#endif

/* Static methods. */

void MediaSoup::SetProcess()
{
	MS_TRACE();

	#if defined(MS_LITTLE_ENDIAN)
		MS_DEBUG("detected Little-Endian CPU");
	#elif defined(MS_BIG_ENDIAN)
		MS_DEBUG("detected Big-Endian CPU");
	#endif

	#if defined(INTPTR_MAX) && defined(INT32_MAX) && (INTPTR_MAX == INT32_MAX)
		MS_DEBUG("detected 32 bits architecture");
	#elif defined(INTPTR_MAX) && defined(INT64_MAX) && (INTPTR_MAX == INT64_MAX)
		MS_DEBUG("detected 64 bits architecture");
	#else
		MS_NOTICE("cannot determine whether the architecture is 32 or 64 bits");
	#endif

	// NOTE: Some of these methods can exit if error if detected.
	IgnoreSignals();
	SetKernelLimits();
	SetUserGroup();
}

void MediaSoup::Run()
{
	MS_TRACE();

	// Init main thread stuff.
	MediaSoup::ThreadInit();

	// Init static data.
	MediaSoup::ClassInit();

	int err;

	// A vector for holding the workerId value for each thread.
	std::vector<int> worker_ids;

	for (int workerId=1; workerId <= Settings::configuration.numWorkers; workerId++)
	{
		worker_ids.push_back(workerId);
		Worker::SetWorker(workerId, nullptr);
	}

	// A vector for holding all the worker threads and joining them later.
	std::vector<uv_thread_t> worker_threads;

	// Build the Worker threads.
	for (int i = 0; i < Settings::configuration.numWorkers; i++)
	{
		MS_DEBUG("running a thread for Worker #%d", worker_ids[i]);
		uv_thread_t thread;

		err = uv_thread_create(&thread, MediaSoup::RunWorkerThread, (void*)&worker_ids[i]);
		if (err)
			MS_THROW_ERROR("uv_thread_create() failed: %s", uv_strerror(err));

		worker_threads.push_back(thread);
	}

	// Wait for all the workers to be running.
	// Wait 300 rounds of 0.01 seconds each one (max 3 seconds).
	MS_DEBUG("waiting for all the Workers to be running...");

	bool all_workers_running = false;
	struct timespec waiting;
	waiting.tv_sec = 0;
	waiting.tv_nsec = 10000000;  // 10 ms.

	for (int i=1; i<=300; i++)
	{
		if (Worker::AreAllWorkersRunning())
		{
			all_workers_running = true;
			break;
		}

		err = nanosleep(&waiting, nullptr);
		if (err)
			MS_THROW_ERROR("nanosleep() failed: %s", uv_strerror(err));
	}

	if (!all_workers_running)
		MS_THROW_ERROR("some Worker(s) could not start or died");

	MS_DEBUG("all the Workers running, let's run the Dispatcher");

	try
	{
		Dispatcher dispatcher;
	}
	catch (const MediaSoupError &error)
	{
		MS_THROW_ERROR("%s", error.what());
	}

	MS_DEBUG("Dispatcher ends, waiting for Workers to end");

	for (auto thread : worker_threads)
	{
		err = uv_thread_join(&thread);
		if (err)
			MS_THROW_ERROR("uv_thread_join() failed: %s", uv_strerror(err));
	}

	MS_DEBUG("all the Workers have ended");
}

void MediaSoup::End()
{
	MS_TRACE();

	// Tell the Daemon class to cleanup its resources if needed.
	if (Daemon::IsDaemonized())
		Daemon::End();

	// Free static stuff.
	MediaSoup::ClassDestroy();

	// Free thread stuff.
	MediaSoup::ThreadDestroy();
}

void MediaSoup::IgnoreSignals()
{
	MS_TRACE();

	int err;
	struct sigaction act;
	// NOTE: Here we ignore also signals that will be later handled by the Dispatcher
	// (as the libuv signal handler overrides it).
	std::map<std::string, int> ignored_signals =
	{
		{ "INT",  SIGINT  },
		{ "TERM", SIGTERM },
		{ "HUP",  SIGHUP  },
		{ "ALRM", SIGALRM },
		{ "USR1", SIGUSR2 },
		{ "USR2", SIGUSR1 }
	};

	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	err = sigfillset(&act.sa_mask);
	if (err)
		MS_EXIT_FAILURE("sigfillset() failed: %s", std::strerror(errno));

	for (auto it = ignored_signals.begin(); it != ignored_signals.end(); ++it)
	{
		std::string sig_name = it->first;
		int sig_id = it->second;

		err = sigaction(sig_id, &act, nullptr);
		if (err)
			MS_EXIT_FAILURE("sigaction() failed for signal %s: %s", sig_name.c_str(), std::strerror(errno));
	}
}

void MediaSoup::SetKernelLimits()
{
	MS_TRACE();

	struct rlimit current_limit;
	struct rlimit new_limit;

	rlim_t recommended_NOFILE = MS_RECOMMENDED_NOFILE;
	int err;

	// Get current NOFILE limits.
	err = getrlimit(RLIMIT_NOFILE, &current_limit);
	if (err)
	{
		MS_EXIT_FAILURE("getrlimit(RLIMIT_NOFILE) failed: %s", std::strerror(errno));
		return;
	}

	MS_DEBUG("getrlimit(RLIMIT_NOFILE) [soft:%llu, hard:%llu]", (unsigned long long)current_limit.rlim_cur, (unsigned long long)current_limit.rlim_max);

	// If soft limit of "open files" is equal or bigger than recommended_NOFILE we are done.
	if (current_limit.rlim_cur >= recommended_NOFILE)
		goto end;

	// If hard limit of "open files" is equal or bigger than recommended_NOFILE then we should
	// be able to set the soft limit to recommended_NOFILE.
	// NOTE: This is not true on OSX in which hard limit is infinity but it does
	// not let the user chaning its soft limit.
	if (current_limit.rlim_max >= recommended_NOFILE)
	{
		new_limit.rlim_cur = recommended_NOFILE;
		new_limit.rlim_max = current_limit.rlim_max;

		err = setrlimit(RLIMIT_NOFILE, &new_limit);
		if (err) {
			if (errno == EINVAL)
			{
				MS_DEBUG("setrlimit(RLIMIT_NOFILE) failed for limits [soft:%llu, hard:%llu]: %s", (unsigned long long)new_limit.rlim_cur, (unsigned long long)new_limit.rlim_max, std::strerror(errno));
			}
			else
			{
				MS_WARN("setrlimit(RLIMIT_NOFILE) failed for limits [soft:%llu, hard:%llu]: %s", (unsigned long long)new_limit.rlim_cur, (unsigned long long)new_limit.rlim_max, std::strerror(errno));
			}
		}
	}
	// Otherwise try to set both hard and soft limits to recommended_NOFILE.
	else
	{
		new_limit.rlim_cur = recommended_NOFILE;
		new_limit.rlim_max = recommended_NOFILE;

		err = setrlimit(RLIMIT_NOFILE, &new_limit);
		if (err)
			MS_WARN("setrlimit(RLIMIT_NOFILE) failed for limits [soft:%llu, hard:%llu]: %s", (unsigned long long)new_limit.rlim_cur, (unsigned long long)new_limit.rlim_max, std::strerror(errno));
	}

end:
	// Get the resulting NOFILE limits.
	err = getrlimit(RLIMIT_NOFILE, &current_limit);
	if (err)
	{
		MS_EXIT_FAILURE("getrlimit(RLIMIT_NOFILE) failed: %s", std::strerror(errno));
		return;
	}

	// If the soft limit is equal or bigger than recommended_NOFILE then ok.
	if (current_limit.rlim_cur >= recommended_NOFILE)
	{
		MS_DEBUG("RLIMIT_NOFILE soft limit (max open files) set to %llu", (unsigned long long)current_limit.rlim_cur);
	}
	// Otherwise warn the user.
	else
	{
		MS_WARN("RLIMIT_recommended_NOFILE soft limit (max open files) set to %llu, less than the recommended value (%llu)", (unsigned long long)current_limit.rlim_cur, (unsigned long long)recommended_NOFILE);
	}
}

void MediaSoup::SetUserGroup()
{
	MS_TRACE();

	struct group *grp = nullptr;
	struct passwd *pwd = nullptr;

	// Set the group if requested.
	if (!Settings::arguments.group.empty())
	{
		char *endptr;
		const char* group = Settings::arguments.group.c_str();
		gid_t g, gid;

		// Allow passing a numeric string representing the user's gid.
		g = std::strtol(group, &endptr, 10);
		if (*endptr == '\0')
		{
			gid = (gid_t)g;
			grp = getgrgid(gid);
			if (!grp)
				MS_EXIT_FAILURE("cannot get information for group with gid '%d': %s", (int)gid, std::strerror(errno));
		}
		// Otherwise convert from name to gid.
		else
		{
			grp = getgrnam(group);
			if (!grp)
				MS_EXIT_FAILURE("group '%s' does not exist", group);

			gid = grp->gr_gid;
		}

		// Set gid as process gid.
		MS_DEBUG("setting '%s' (%d) as process group", group, (int)gid);
		if (setgid(gid) < 0)
			MS_EXIT_FAILURE("cannot change gid to %s (%d): %s", group, (int)gid, std::strerror(errno));
	}

	// Set the user if requested.
	if (!Settings::arguments.user.empty())
	{
		char *endptr;
		const char* user = Settings::arguments.user.c_str();
		uid_t u, uid;

		// Allow passing a numeric string representing the group's uid.
		u = std::strtol(user, &endptr, 10);
		if (*endptr == '\0')
		{
			uid = (uid_t)u;
			pwd = getpwuid(uid);
			if (!pwd)
				MS_EXIT_FAILURE("cannot get information for user with uid '%d': %s", (int)uid, std::strerror(errno));
		}
		// Otherwise convert from name to uid.
		else
		{
			pwd = getpwnam(user);
			if (!pwd)
			{
				MS_EXIT_FAILURE("user '%s' does not exist", user);
			}
			uid = pwd->pw_uid;
		}

		// Set access to all the groups of which the user is member.
		gid_t additional_gid = grp ? grp->gr_gid : pwd->pw_gid;
		MS_DEBUG("setting supplementary groups for uid %d (using %d as additional gid)", (int)uid, (int)additional_gid);
		if (initgroups(pwd->pw_name, additional_gid) < 0)
			MS_EXIT_FAILURE("initgroups() for uid %d and gid %d failed: %s", (int)uid, (int)additional_gid, std::strerror(errno));

		// Set uid as process uid.
		MS_DEBUG("setting '%s' (%d) as process user", user, (int)uid);
		if (setuid(uid) < 0)
			MS_EXIT_FAILURE("cannot change uid to %s (%d): %s", user, (int)uid, std::strerror(errno));
	}
}

void MediaSoup::ThreadInit()
{
	MS_TRACE();

	// Set (again, don't worry) this thread as main thread.
	Logger::ThreadInit("main");

	// Load libuv stuff.
	LibUV::ThreadInit();

	// Load the crypto utils.
	Utils::Crypto::ThreadInit();
}

void MediaSoup::ThreadDestroy()
{
	MS_TRACE();

	LibUV::ThreadDestroy();

	Utils::Crypto::ThreadDestroy();
}

void MediaSoup::ClassInit()
{
	MS_TRACE();

	// Initialize static stuff.
	LibUV::ClassInit();
	OpenSSL::ClassInit();
	LibSRTP::ClassInit();
	RTC::UDPSocket::ClassInit();
	RTC::TCPServer::ClassInit();
	RTC::DTLSHandler::ClassInit();
	RTC::SRTPSession::ClassInit();
}

void MediaSoup::ClassDestroy()
{
	MS_TRACE();

	// Free static stuff.
	RTC::DTLSHandler::ClassDestroy();
	OpenSSL::ClassDestroy();
	LibSRTP::ClassDestroy();
}

/* This method is called within a new thread and creates a Worker. */
void MediaSoup::RunWorkerThread(void* data)
{
	int workerId = *(int*)data;

	// Init thread stuff.
	Worker::ThreadInit(workerId);

	try
	{
		MS_DEBUG("initializing Worker #%d", workerId);
		Worker worker(workerId);
		MS_DEBUG("Worker #%d exits", workerId);
	}
	catch (const MediaSoupError &error)
	{
		MS_CRIT("error happened in Worker #%d: %s | Worker ended", workerId, error.what());

		Worker::ThreadDestroy();
		throw(error);
	}

	Worker::ThreadDestroy();
}
