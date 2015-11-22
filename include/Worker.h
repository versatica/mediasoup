#ifndef MS_WORKER_H
#define	MS_WORKER_H

#include "common.h"
#include "ControlProtocol/UnixStreamSocket.h"
#include "ControlProtocol/Message.h"
#include <map>
#include <atomic>
// TMP
#include "RTC/Room.h"

class Worker :
	public ControlProtocol::UnixStreamSocket::Listener
{
public:
	static void ThreadInit(int workerId);
	static void ThreadDestroy();
	static void SetWorker(int workerId, Worker* worker);
	static int CountWorkers();
	static bool AreAllWorkersRunning();
	static int GetControlProtocolRemoteSocket(int workerId);

private:
	typedef std::map <int, Worker*> Workers;
	static Workers workers;
	static int numWorkers;
	static std::atomic<int> numWorkersRunning;

public:
	Worker(int workerId);
	~Worker();

	void Close();
	int GetControlProtocolRemoteSocket();

private:
	void BlockSignals();

/* Methods inherited from ControlProtocol::UnixStreamSocket::Listener. */
public:
	virtual void onControlProtocolMessage(ControlProtocol::UnixStreamSocket* unixSocket, ControlProtocol::Message* msg, const MS_BYTE* raw, size_t len) override;
	virtual void onControlProtocolUnixStreamSocketClosed(ControlProtocol::UnixStreamSocket* unixSocket, bool is_closed_by_peer) override;

private:
	// Allocated by this:
	ControlProtocol::UnixStreamSocket* controlProtocolUnixStreamSocket = nullptr;
	// Others:
	int workerId = 0;
	int controlProtocolRemoteSocket = 0;
	// TMP
	RTC::Room* room = nullptr;
};

#endif
