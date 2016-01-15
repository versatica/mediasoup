#define MS_CLASS "Loop"

#include "Loop.h"
#include "DepLibUV.h"
#include "Settings.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <string>
#include <utility>  // std::pair()
#include <csignal>  // sigfillset, pthread_sigmask()
#include <cerrno>
#include <iostream>  //  std::cout, std::cerr
#include <json/json.h>

/* Instance methods. */

Loop::Loop(Channel::UnixStreamSocket* channel) :
	channel(channel)
{
	MS_TRACE();

	// Set us as Channel's listener.
	this->channel->SetListener(this);

	// Create the Notifier instance.
	this->notifier = new Channel::Notifier(this->channel);

	// Set the signals handler.
	this->signalsHandler = new SignalsHandler(this);

	// Add signals to handle.
	this->signalsHandler->AddSignal(SIGINT, "INT");
	this->signalsHandler->AddSignal(SIGTERM, "TERM");

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
	if (this->signalsHandler)
		this->signalsHandler->Close();

	// Close all the Rooms.
	// NOTE: Upon Room closure the onRoomClosed() method is called which
	// removes it from the map, so this is the safe way to iterate the mao
	// and remove elements.
	for (auto it = this->rooms.begin(); it != this->rooms.end();)
	{
		RTC::Room* room = it->second;

		it = this->rooms.erase(it);
		room->Close();
	}

	// Close the Notifier.
	this->notifier->Close();

	// Close the Channel socket.
	if (this->channel)
		this->channel->Close();
}

RTC::Room* Loop::GetRoomFromRequest(Channel::Request* request, uint32_t* roomId)
{
	MS_TRACE();

	static const Json::StaticString k_roomId("roomId");

	auto jsonRoomId = request->internal[k_roomId];

	if (!jsonRoomId.isUInt())
		MS_THROW_ERROR("Request has not numeric `internal.roomId`");

	// If given, fill roomId.
	if (roomId)
		*roomId = jsonRoomId.asUInt();

	auto it = this->rooms.find(jsonRoomId.asUInt());
	if (it != this->rooms.end())
	{
		RTC::Room* room = it->second;

		return room;
	}
	else
	{
		return nullptr;
	}
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

void Loop::onChannelRequest(Channel::UnixStreamSocket* channel, Channel::Request* request)
{
	MS_TRACE();

	MS_DEBUG("'%s' request", request->method.c_str());

	switch (request->methodId)
	{
		case Channel::Request::MethodId::worker_dump:
		{
			static const Json::StaticString k_rooms("rooms");

			Json::Value json(Json::objectValue);
			Json::Value json_worker(Json::objectValue);
			Json::Value json_rooms(Json::objectValue);

			for (auto& kv : this->rooms)
			{
				auto room = kv.second;

				json_rooms[std::to_string(room->roomId)] = room->toJson();
			}
			json_worker[k_rooms] = json_rooms;
			json[Logger::id] = json_worker;

			request->Accept(json);

			break;
		}

		case Channel::Request::MethodId::worker_updateSettings:
		{
			Settings::HandleRequest(request);

			break;
		}

		case Channel::Request::MethodId::worker_createRoom:
		{
			RTC::Room* room;
			uint32_t roomId;

			try
			{
				room = GetRoomFromRequest(request, &roomId);
			}
			catch (const MediaSoupError &error)
			{
				request->Reject(error.what());
				return;
			}

			if (room)
			{
				MS_ERROR("Room already exists");

				request->Reject("Room already exists");
				return;
			}

			try
			{
				room = new RTC::Room(this, this->notifier, roomId);
			}
			catch (const MediaSoupError &error)
			{
				request->Reject(error.what());
				return;
			}

			this->rooms[roomId] = room;

			MS_DEBUG("Room created [roomId:%" PRIu32 "]", roomId);
			request->Accept();

			break;
		}

		case Channel::Request::MethodId::room_close:
		case Channel::Request::MethodId::room_dump:
		case Channel::Request::MethodId::room_createPeer:
		case Channel::Request::MethodId::peer_close:
		case Channel::Request::MethodId::peer_dump:
		case Channel::Request::MethodId::peer_createTransport:
		case Channel::Request::MethodId::peer_createAssociatedTransport:
		case Channel::Request::MethodId::peer_createRtpReceiver:
		case Channel::Request::MethodId::transport_close:
		case Channel::Request::MethodId::transport_dump:
		case Channel::Request::MethodId::transport_setRemoteDtlsParameters:
		case Channel::Request::MethodId::rtpReceiver_close:
		case Channel::Request::MethodId::rtpReceiver_dump:
		case Channel::Request::MethodId::rtpReceiver_receive:
		{
			RTC::Room* room;

			try
			{
				room = GetRoomFromRequest(request);
			}
			catch (const MediaSoupError &error)
			{
				request->Reject(error.what());
				return;
			}

			if (!room)
			{
				MS_ERROR("Room does not exist");

				request->Reject("Room does not exist");
				return;
			}

			room->HandleRequest(request);

			break;
		}

		default:
		{
			MS_ERROR("unknown method");

			request->Reject("unknown method");
		}
	}
}

void Loop::onChannelUnixStreamSocketRemotelyClosed(Channel::UnixStreamSocket* socket)
{
	MS_TRACE_STD();

	// When mediasoup Node process ends it sends a SIGTERM to us so we close this
	// pipe and then exit.
	// If the pipe is remotely closed it means that mediasoup Node process
	// abruptly died (SIGKILL?) so we must die.
	MS_ERROR_STD("Channel remotely closed, killing myself");
	this->channel = nullptr;

	Close();
}

void Loop::onRoomClosed(RTC::Room* room)
{
	MS_TRACE();

	this->rooms.erase(room->roomId);
}
