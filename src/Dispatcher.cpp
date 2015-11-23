#define MS_CLASS "Dispatcher"

#include "Dispatcher.h"
#include "Settings.h"
#include "Daemon.h"
#include "Worker.h"
#include "Utils.h"
#include "DepLibUV.h"
#include "ControlProtocol/messages.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <string>
#include <csignal>  // sigaction(), pthread_sigmask()
#include <cerrno>

/* Instance methods. */

Dispatcher::Dispatcher()
{
	MS_TRACE();

	// Set the signals handler.
	try
	{
		this->signalsHandler = new SignalsHandler(this);
		// Add signals to handle.
		this->signalsHandler->AddSignal(SIGUSR1, "USR1");
		this->signalsHandler->AddSignal(SIGINT, "INT");
		this->signalsHandler->AddSignal(SIGTERM, "TERM");
	}
	catch (const MediaSoupError &error)
	{
		MS_THROW_ERROR("error creating the SignalsHandler: %s", error.what());
	}
	MS_DEBUG("SignalsHandler ready");

	// Create a ControlProtocol TCP server.
	std::string ip = Settings::configuration.ControlProtocol.listenIP;
	MS_PORT port = Settings::configuration.ControlProtocol.listenPort;
	try
	{
		this->controlProtocolTCPServer = new ControlProtocol::TCPServer(this, this, ip, port);
	}
	catch (const MediaSoupError &error)
	{
		MS_THROW_ERROR("error creating a ControlProtocol TCP server in %s : %d: %s", ip.c_str(), port, error.what());
	}
	MS_DEBUG("ControlProtocol TCP server listening into %s : %d", this->controlProtocolTCPServer->GetLocalIP().c_str(), this->controlProtocolTCPServer->GetLocalPort());

	// Create the ControlProtocol UnixStream sockets.
	int num_workers = Worker::CountWorkers();

	// Insert a nullptr value in position 0 of the vector of ControlProtocol::UnixStreamSocket*
	// since the first entry will be in position 1.
	this->controlProtocolUnixStreamSockets.push_back(nullptr);

	for (int workerId = 1; workerId <= num_workers; workerId++)
	{
		int fd = Worker::GetControlProtocolRemoteSocket(workerId);
		ControlProtocol::UnixStreamSocket* controlProtocolUnixStreamSocket;

		try
		{
			controlProtocolUnixStreamSocket = new ControlProtocol::UnixStreamSocket(this, fd);

			// Attach the workerId as user data.
			int* data = new int(workerId);
			controlProtocolUnixStreamSocket->SetUserData((void*)data);
		}
		catch (const MediaSoupError &error)
		{
			MS_THROW_ERROR("error creating a ControlProtocol Unix socket for Worker %d: %s", workerId, error.what());
		}

		// Add to the vector. Note that workerId 1 will be placed in position 1 of the vector.
		this->controlProtocolUnixStreamSockets.push_back(controlProtocolUnixStreamSocket);

		MS_DEBUG("ControlProtocol Unix socket for Worker #%d ready", workerId);
	}

	MS_DEBUG("Dispatcher running");

	// Write the success status on the pipe if daemonized to tell the ancestor
	// process that the daemonized process is properly running.
	if (Settings::arguments.daemonize)
		Daemon::SendOKStatusToAncestor();

	// Run the loop.
	DepLibUV::RunLoop();

	MS_DEBUG("libuv loop ends");
}

Dispatcher::~Dispatcher()
{
	MS_TRACE();
}

void Dispatcher::Close()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_ERROR("already closed");
		return;
	}
	this->closed = true;

	MS_DEBUG("closing all the services running in the Dispatcher");

	// First block all the signals to not be interrupted while closing.
	int err;
	sigset_t signal_mask;

	// Block all the signals.
	sigfillset(&signal_mask);

	err = pthread_sigmask(SIG_BLOCK, &signal_mask, nullptr);
	if (err)
		MS_ERROR("pthread_sigmask() failed: %s", std::strerror(errno));

	// Close the SignalsHandler.
	this->signalsHandler->Close();

	// Close the ControlProtocol TCP server.
	this->controlProtocolTCPServer->Close();

	// Close the ControlProtocol UnixStream sockets.
	int num_workers = Worker::CountWorkers();
	for (int workerId = 1; workerId <= num_workers; workerId++)
		this->controlProtocolUnixStreamSockets[workerId]->Close();
}

void Dispatcher::onSignal(SignalsHandler* signalsHandler, int signum)
{
	MS_TRACE();

	switch (signum)
	{
		// SIGUSR1 means configuration reload.
		case SIGUSR1:
			MS_NOTICE("signal USR1 received, reloading configuration");
			if (Settings::ReloadConfigurationFile())
			{
				// Print new configuration.
				Settings::PrintConfiguration();
			}
			break;

		case SIGINT:
			MS_NOTICE("signal INT received, exiting");
			Close();
			break;

		case SIGTERM:
			MS_NOTICE("signal TERM received, exiting");
			Close();
			break;

		default:
			MS_ERROR("received a signal (with signum %d) for which there is no handling code", signum);
	}
}

void Dispatcher::onSignalsHandlerClosed(SignalsHandler* signalsHandler)
{
	MS_TRACE();
}

void Dispatcher::onControlProtocolNewTCPConnection(ControlProtocol::TCPServer* tcpServer, ControlProtocol::TCPConnection* connection)
{
	MS_TRACE();

	MS_DEBUG("new ControlProtocol TCP connection:");
	tcpServer->Dump();
	connection->Dump();
}

void Dispatcher::onControlProtocolTCPConnectionClosed(ControlProtocol::TCPServer* tcpServer, ControlProtocol::TCPConnection* connection, bool is_closed_by_peer)
{
	MS_TRACE();

	if (is_closed_by_peer)
		MS_DEBUG("ControlProtocol TCP connection closed by peer:");
	else
		MS_DEBUG("ControlProtocol TCP connection closed locally:");
	connection->Dump();
	tcpServer->Dump();
}

void Dispatcher::onControlProtocolMessage(ControlProtocol::TCPConnection* connection, ControlProtocol::Message* msg, const MS_BYTE* raw, size_t len)
{
	MS_TRACE();

	MS_DEBUG("ControlProtocol message received from TCP %s : %u", connection->GetPeerIP().c_str(), (unsigned int)connection->GetPeerPort());

	// TODO: process the ControlProtocol::Message.

	// TEST
	bool relayToRandomWorker = true;
	bool relayToAllWorkers = false;
	bool replyOK = true;

	if (relayToRandomWorker)
	{
		int workerId = (int)Utils::Crypto::GetRandomUInt(1, (unsigned int)Worker::CountWorkers());

		MS_INFO("relaying ControlProtocol message to Worker #%d ...", workerId);

		ControlProtocol::UnixStreamSocket* controlProtocolUnixStreamSocket = controlProtocolUnixStreamSockets[workerId];

		controlProtocolUnixStreamSocket->Write(raw, len);
	}

	if (relayToAllWorkers)
	{
		int num_workers = Worker::CountWorkers();
		for (int workerId = 1; workerId <= num_workers; workerId++)
		{
			ControlProtocol::UnixStreamSocket* controlProtocolUnixStreamSocket = controlProtocolUnixStreamSockets[workerId];

			MS_INFO("relaying ControlProtocol message to Worker #%d ...", workerId);
			controlProtocolUnixStreamSocket->Write(raw, len);
		}
	}

	if (replyOK)
	{
		MS_INFO("replying 'OK, message received' via TCP ...");

		// TEST: Write(std::string &data)
		std::string ok_reply_begin = "OK, message received with transaction ";
		connection->Write(ok_reply_begin);

		// TEST: Write(std::string &data)
		std::string transaction_str = std::to_string(msg->GetTransaction());
		connection->Write(transaction_str);

		// TEST: Write(std::string &data)
		std::string ok_reply_end = ":\n---------------------\n";
		connection->Write(ok_reply_end);

		// TEST: Write(const MS_BYTE* data, size_t len)
		connection->Write(raw, len);

		// TEST: Write(std::string &data)
		std::string ok_reply_real_end = "---------------------\n";
		connection->Write(ok_reply_real_end);
	}
}

void Dispatcher::onControlProtocolMessage(ControlProtocol::UnixStreamSocket* unixSocket, ControlProtocol::Message* msg, const MS_BYTE* raw, size_t len)
{
	MS_TRACE();

	// Get the workerId associated to this socket.
	int workerId = *(int*)unixSocket->GetUserData();

	// TMP
	MS_INFO("ControlProtocol message received from Worker #%d", workerId);

	// TMP send back to the worker (loop !!!)
	unixSocket->Write((MS_BYTE*)raw, len);

	// TODO: process the ControlProtocol::Message.
}

void Dispatcher::onControlProtocolUnixStreamSocketClosed(ControlProtocol::UnixStreamSocket* unixSocket, bool is_closed_by_peer)
{
	MS_TRACE();

	// Get the workerId associated to this socket.
	int* userData = (int*)unixSocket->GetUserData();
	int workerId = *userData;

	// Free the socket's user data and remove the socket from the map.
	delete userData;
	controlProtocolUnixStreamSockets[workerId] = nullptr;

	// A Worker MUST NEVER close the socket. If it does it is due to a parsing error
	// that should NEVER happen, so abort.
	if (is_closed_by_peer)
		MS_ABORT("ControlProtocol Unix socket for Worker #%d remotely closed", workerId);
	else
		MS_DEBUG("ControlProtocol Unix socket for Worker #%d locally closed", workerId);
}
