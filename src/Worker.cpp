#define MS_CLASS "Worker"

#include "Worker.h"
#include "Utils.h"
#include "LibUV.h"
#include "ControlProtocol/messages.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <string>
#include <csignal>  // sigaction(), pthread_sigmask()
#include <cerrno>

/* Static variables. */

Worker::Workers Worker::workers;
int Worker::numWorkers = 0;
std::atomic<int> Worker::numWorkersRunning {0};

/* Static methods. */

void Worker::ThreadInit(int workerId)
{
	Logger::ThreadInit("worker #" + std::to_string(workerId));
	MS_TRACE();

	LibUV::ThreadInit();

	Utils::Crypto::ThreadInit();
}

void Worker::ThreadDestroy()
{
	MS_TRACE();

	LibUV::ThreadDestroy();

	Utils::Crypto::ThreadDestroy();
}

void Worker::SetWorker(int workerId, Worker* worker)
{
	MS_TRACE();

	// Main thread calls this method with nullptr as worker. In that case the
	// workerId entry MUST NOT already exit.
	if (!worker)
	{
		if (Worker::workers.find(workerId) != Worker::workers.end())
			MS_ABORT("entry with key %d already in the map", workerId);

		Worker::workers[workerId] = nullptr;
	}
	// Each Worker (in its own thread) calls this method to insert itself in
	// the map. In that case the key MUST exist.
	else
	{
		if (Worker::workers.find(workerId) == Worker::workers.end())
			MS_ABORT("entry with key %d not present in the map", workerId);

		Worker::workers[workerId] = worker;
	}
}

int Worker::CountWorkers()
{
	MS_TRACE();

	if (!Worker::numWorkers)
	{
		Worker::numWorkers = workers.size();
	}

	return Worker::numWorkers;
}

bool Worker::AreAllWorkersRunning()
{
	MS_TRACE();

	if (Worker::numWorkersRunning.load(std::memory_order_relaxed) == Worker::CountWorkers())
		return true;
	else
		return false;
}

int Worker::GetControlProtocolRemoteSocket(int workerId)
{
	MS_TRACE();

	Workers::iterator it = Worker::workers.find(workerId);

	// Ensure it does exist in the map of Workers.
	if (it == Worker::workers.end())
		MS_ABORT("Worker with workerId %d not present in the map", workerId);

	Worker* worker = it->second;

	return worker->GetControlProtocolRemoteSocket();
}

/* Instance methods. */

Worker::Worker(int workerId) :
	workerId(workerId)
{
	MS_TRACE();

	// Block signals that must just be delivered to Dispatcher thread.
	BlockSignals();

	// Add this worker to the map.
	// NOTE: This is thread-safe given that I don't alter the map (but just
	// the values of already allocated entries in the map).
	Worker::SetWorker(this->workerId, this);

	// Build a pair of connected UnixSocket in stream mode.
	int fds[2] {-1,-1};
	try
	{
		Utils::Socket::BuildSocketPair(AF_UNIX, SOCK_STREAM, fds);
	}
	catch (const MediaSoupError &error)
	{
		MS_THROW_ERROR("error building a socket pair for the ControlProtocol Unix socket: %s", error.what());
	}

	// Store the first paired socket for Dispatcher's usage.
	this->controlProtocolRemoteSocket = fds[0];

	try
	{
		this->controlProtocolUnixStreamSocket = new ControlProtocol::UnixStreamSocket(this, fds[1]);
	}
	catch (const MediaSoupError &error)
	{
		MS_THROW_ERROR("error creating a ControlProtocol Unix socket for Dispatcher: %s", error.what());
	}
	MS_DEBUG("ControlProtocol Unix socket for Dispatcher ready");

	// TODO: TMP
	if (this->workerId == 1)
	{
		try
		{
			this->room = new RTC::Room();
		}
		catch (const MediaSoupError &error)
		{
			MS_ERROR("---- TEST: error creating a RTC Room: %s", error.what());
		}
	}

	MS_DEBUG("Worker #%d running", this->workerId);

    // Increase the static variable numWorkersRunning.
    Worker::numWorkersRunning++;

	// Run the loop.
	LibUV::RunLoop();

	MS_DEBUG("libuv loop ends");
}

Worker::~Worker()
{
	MS_TRACE();
}

void Worker::Close()
{
	MS_TRACE();

	// TMP: close RTC::Room.
	if (this->room)
		this->room->Close();
}

int Worker::GetControlProtocolRemoteSocket()
{
	return this->controlProtocolRemoteSocket;
}

void Worker::BlockSignals()
{
	MS_TRACE();

	int err;
	sigset_t signal_mask;

	sigemptyset(&signal_mask);

	// Block some signals to ensure that they are just delivered to the Dispatcher thread.
	sigaddset(&signal_mask, SIGUSR1);
	sigaddset(&signal_mask, SIGINT);
	sigaddset(&signal_mask, SIGTERM);

	err = pthread_sigmask(SIG_BLOCK, &signal_mask, nullptr);
	if (err)
		MS_ABORT("pthread_sigmask() failed: %s", std::strerror(errno));
}

void Worker::onControlProtocolMessage(ControlProtocol::UnixStreamSocket* unixSocket, ControlProtocol::Message* msg, const MS_BYTE* raw, size_t len)
{
	MS_TRACE();

	MS_DEBUG("ControlProtocol message received from Dispatcher");
	msg->Dump();


// TMP
	bool sendBackToDispatcher = false;

	if (sendBackToDispatcher) {
		MS_INFO("sending msg back to Dispatcher...");
		unixSocket->Write(raw, len);
	}

	// TODO: process the ControlProtocol::Message.
}

void Worker::onControlProtocolUnixStreamSocketClosed(ControlProtocol::UnixStreamSocket* unixSocket, bool is_closed_by_peer)
{
	MS_TRACE();

	if (is_closed_by_peer)
		MS_DEBUG("ControlProtocol Unix socket for Dispatcher remotely closed");
	else
		MS_DEBUG("ControlProtocol Unix socket for Dispatcher locally closed");

	// Call Close() to terminate all the active handles.
	Close();
}
