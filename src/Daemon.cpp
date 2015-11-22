#define MS_CLASS "Daemon"

#include "Daemon.h"
#include "Settings.h"
#include "Version.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <string>
#include <cerrno>
#include <cstdlib>  // std::_Exit()
#include <cstdio>  // std::setbuf(), std::freopen(), std::fopen(), std::fscanf(), std::fprintf(), std::remove()
#include <csignal>  // kill()
#include <unistd.h>  // pipe(), fork(), close(), setsid(), getpid()

/* Static variables. */

int Daemon::statusPipe[2] {-1, -1};
bool Daemon::isDaemonized = false;
bool Daemon::hasWrittenPIDFile = false;

/* Static methods. */

void Daemon::Daemonize()
{
	MS_TRACE();

	/**
	 * - ancestor process (running in foreground)
	 *   \- intermediary process
	 *       \- daemonized process (main process running in background)
	 */

	int ret;
	int pid;
	char status;

	// Flush std file descriptors to avoid flushes after fork (same message
	// appearing multiple times) and switch to unbuffered mode.
	std::setbuf(stdout, 0);
	std::setbuf(stderr, 0);

	// Set the pipe for the daemonized process to send its status to the ancestor
	// process (that waits for it in the foreground).
	do
	{
		ret = pipe(Daemon::statusPipe);
	}
	while (ret < 0 && errno == EINTR);

	if (ret < 0)
		MS_THROW_ERROR("pipe() failed: %s", std::strerror(errno));

	// NOTE: Don't need to update thread name after fork() calls. fork() copies
	// the memory of the calling thread.

	// First fork: don't be group leader.
	if ((pid = fork()) < 0)
	{
		MS_THROW_ERROR("first fork() failed: %s", std::strerror(errno));
	}
	else if (pid > 0)
	{
		// When fork() returns a positive number, we are in the ancestor process
		// and the return value is the PID of the newly created child process (the
		// intermediary process).

		// Ancestor process wait for the daemonized process to write into the pipe (or to die
		// before) so it can exit with the proper exit status.
		if (Daemon::WaitForDaemonizedStatus(&status))
		{
			if ((int)status == EXIT_SUCCESS)
			{
				MS_NOTICE("%s running as daemon", Version::name.c_str());
				std::_Exit(EXIT_SUCCESS);
			}
			else
			{
				MS_EXIT_FAILURE("daemonized process failed to start (check Syslog)");
			}
		}
		// The daemonized process died before writing its status on the pipe.
		else
		{
			MS_EXIT_FAILURE("daemonized process died before writing its status (check Syslog)");
		}
	}

	// This is the intermediary process.

	// We are now daemonized (even if this is not the definitive forked process).
	Daemon::isDaemonized = true;

	// Clean unused fds.
	if (Daemon::statusPipe[0] != -1)
	{
		close(Daemon::statusPipe[0]);
		Daemon::statusPipe[0] = -1;
	}

	// Become session leader to drop the ctrl terminal.
	if (setsid() < 0)
	{
		MS_THROW_ERROR("setsid() failed: %s", std::strerror(errno));
	}

	// Fork again to drop group leadership.
	if ((pid = fork()) < 0)
	{
		MS_THROW_ERROR("second fork() failed: %s", std::strerror(errno));
	}
	else if (pid > 0) {
		// Intermediary process just exits.
		std::_Exit(EXIT_SUCCESS);
	}

	// This is the daemonized process.

	// Create a PID file if requested.
	if (!Settings::arguments.pidFile.empty())
	{
		if (!Daemon::WritePIDFile())
			MS_THROW_ERROR("error writing the PID file '%s'", Settings::arguments.pidFile.c_str());
	}

	// Replace stdin, stdout & stderr with /dev/null.
	if (std::freopen("/dev/null", "r", stdin) == nullptr)
	{
		MS_ERROR("error replacing stdin with /dev/null: %s", std::strerror(errno));
	}
	if (std::freopen("/dev/null", "w", stdout) == nullptr)
	{
		MS_ERROR("error replacing stdout with /dev/null: %s", std::strerror(errno));
	}
	if (std::freopen("/dev/null", "w", stderr) == nullptr)
	{
		MS_ERROR("error replacing stderr with /dev/null: %s", std::strerror(errno));
	}

	// Close all the possible fds inherited from the ancestor process (all but
	// the write side of the pipe as the daemonized process must use it).
	for (int fd = 3; fd < 32; fd++)
	{
		if (fd != Daemon::statusPipe[1])
		close(fd);
	}

	// Enable Syslog logging.
	Logger::EnableSyslog();

	// First log in Syslog.
	MS_NOTICE("%s running as daemon", Version::name.c_str());
}

void Daemon::SendOKStatusToAncestor()
{
	MS_TRACE();

	MS_DEBUG("sending OK status to the ancestor process");

	SendStatusToAncestor(EXIT_SUCCESS);
}

void Daemon::SendErrorStatusToAncestor()
{
	MS_TRACE();

	MS_DEBUG("sending error status to the ancestor process");

	SendStatusToAncestor(EXIT_FAILURE);
}

bool Daemon::IsDaemonized()
{
	MS_TRACE();

	return Daemon::isDaemonized;
}

void Daemon::End()
{
	MS_TRACE();

	// Delete PID file.
	if (Daemon::hasWrittenPIDFile)
	{
		int err;

		err = std::remove(Settings::arguments.pidFile.c_str());
		if (err)
			MS_ERROR("cannot delete the PID file '%s': %s", Settings::arguments.pidFile.c_str(), std::strerror(errno));
	}
}

bool Daemon::WritePIDFile()
{
	MS_TRACE();

	FILE *pid_file;
	const char* pidFile = Settings::arguments.pidFile.c_str();
	unsigned int existing_pid = 0;
	pid_t pid;

	pid_file = std::fopen(pidFile, "r");
	// PID file already exists. Check it.
	if (pid_file)
	{
		MS_DEBUG("a PID file '%s' already exists", pidFile);

		if (std::fscanf(pid_file, "%9u", &existing_pid) < 0)
		{
			MS_ERROR("could not parse existing PID file '%s'", pidFile);
			std::fclose(pid_file);
			return false;
		}
		fclose(pid_file);

		if (existing_pid == 0)
		{
			MS_ERROR("existing PID file '%s' does not contain a valid value", pidFile);
			return false;
		}

		if (kill((pid_t)existing_pid, 0) == 0 || errno == EPERM)
		{
			MS_ERROR("there is a running process with the same PID as in the existing PID file '%s'",
				pidFile);
			return false;
		}
		else
		{
			MS_NOTICE("existing PID file '%s' contains an old PID, replacing its value", pidFile);
		}
	}

	// Get our PID.
	pid = getpid();
	MS_DEBUG("PID of the daemonized process: %u", (unsigned int)pid);

	// Write into the PID file.
	pid_file = std::fopen(pidFile, "w");

	if (!pid_file)
	{
		MS_ERROR("cannot open PID file '%s' for writing: %s", pidFile, std::strerror(errno));
		return false;
	}

	std::fprintf(pid_file, "%u\n", (unsigned int)pid);
	std::fclose(pid_file);

	Daemon::hasWrittenPIDFile = true;

	MS_DEBUG("PID file '%s' created with our PID value", pidFile);

	return true;
}

bool Daemon::WaitForDaemonizedStatus(char* status)
{
	MS_TRACE();

	int ret;

	MS_DEBUG("waiting for the daemonized process to write its status");

	// Close the output side of the pipe.
	if (Daemon::statusPipe[1] != -1)
	{
		close(Daemon::statusPipe[1]);
		Daemon::statusPipe[1] = -1;
	}
	// If the read side of the pipe is closed the daemonized process died even before
	// sending status.
	if (Daemon::statusPipe[0] == -1)
	{
		return false;
	}

	do
	{
		ret = read(Daemon::statusPipe[0], status, 1);
	}
	while (ret < 0 && errno == EINTR);

	// If a byte has been read return true and the ancestor process will inspect the
	// status value.
	return (ret == 1) ? true : false;
}

void Daemon::SendStatusToAncestor(char status)
{
	MS_TRACE();

	int ret;

	if (Daemon::statusPipe[1] == -1)
	{
		MS_ERROR("cannot send status to ancestor process, pipe is closed");
		return;
	}

	do
	{
		ret = write(Daemon::statusPipe[1], &status, 1);
	}
	while (ret < 0 && errno == EINTR);

	if (ret != 1)
		MS_ERROR("cannot send status to ancestor process: %s", std::strerror(errno));

	close(Daemon::statusPipe[1]);
	Daemon::statusPipe[1] = -1;
}
