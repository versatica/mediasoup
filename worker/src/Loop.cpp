#define MS_CLASS "Loop"

#include "Loop.h"
#include "DepLibUV.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <string>
#include <csignal>  // sigfillset, pthread_sigmask()
#include <cstdlib>  // std::genenv()
#include <cerrno>

/* Instance methods. */

Loop::Loop()
{
	MS_TRACE();

	int controlFD = std::stoi(std::getenv("MEDIASOUP_CONTROL_FD"));

	// Set the signals handler.
	this->signalsHandler = new SignalsHandler(this);

	// Add signals to handle.
	this->signalsHandler->AddSignal(SIGINT, "INT");
	this->signalsHandler->AddSignal(SIGTERM, "TERM");

	// Set the Control socket.
	this->controlSocket = new Control::UnixStreamSocket(this, controlFD);

	MS_DEBUG("starting libuv loop");
	DepLibUV::RunLoop();
	MS_DEBUG("libuv loop ended");
}

Loop::~Loop()
{
	MS_TRACE();
}

void Loop::Close()
{
	MS_TRACE();

	int err;
	sigset_t signal_mask;

	if (this->closed)
	{
		MS_ERROR("already closed");
		return;
	}
	this->closed = true;

	// First block all the signals to not be interrupted while closing.
	sigfillset(&signal_mask);
	err = pthread_sigmask(SIG_BLOCK, &signal_mask, nullptr);
	if (err)
		MS_ERROR("pthread_sigmask() failed: %s", std::strerror(errno));

	// Close the SignalsHandler.
	this->signalsHandler->Close();

	// Close the Control socket
	this->controlSocket->Close();
}

void Loop::onSignal(SignalsHandler* signalsHandler, int signum)
{
	MS_TRACE();

	switch (signum)
	{
		case SIGINT:
			MS_DEBUG("signal INT received, exiting");
			Close();
			break;

		case SIGTERM:
			MS_DEBUG("signal TERM received, exiting");
			Close();
			break;

		default:
			MS_WARN("received a signal (with signum %d) for which there is no handling code", signum);
	}
}

void Loop::onSignalsHandlerClosed(SignalsHandler* signalsHandler)
{
	MS_TRACE();
}
