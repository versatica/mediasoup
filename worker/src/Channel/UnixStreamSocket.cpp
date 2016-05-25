#define MS_CLASS "Channel::UnixStreamSocket"

#include "Channel/UnixStreamSocket.h"
#include "Logger.h"
#include "MediaSoupError.h"
#include <sstream>  // std::ostringstream
#include <cstring>  // std::memmove()
#include <cmath>  // std::ceil()
#include <cstdio>  // sprintf()
extern "C"
{
	#include <netstring.h>
}

// netstring length for a 65536 bytes payload
#define NS_MAX_SIZE      65543
#define MESSAGE_MAX_SIZE 65536

namespace Channel
{
	/* Class variables. */

	uint8_t UnixStreamSocket::writeBuffer[NS_MAX_SIZE];

	/* Instance methods. */

	UnixStreamSocket::UnixStreamSocket(int fd) :
		::UnixStreamSocket::UnixStreamSocket(fd, NS_MAX_SIZE)
	{
		MS_TRACE_STD();

		// Create the JSON reader.
		{
			Json::CharReaderBuilder builder;
			Json::Value settings = Json::nullValue;
			Json::Value invalid_settings;

			builder.strictMode(&settings);

			MS_ASSERT(builder.validate(&invalid_settings), "invalid Json::CharReaderBuilder");

			this->jsonReader = builder.newCharReader();
		}

		// Create the JSON writer.
		{
			Json::StreamWriterBuilder builder;
			Json::Value invalid_settings;

			builder["commentStyle"] = "None";
			builder["indentation"] = "";
			builder["enableYAMLCompatibility"] = false;
			builder["dropNullPlaceholders"] = false;

			MS_ASSERT(builder.validate(&invalid_settings), "invalid Json::StreamWriterBuilder");

			this->jsonWriter = builder.newStreamWriter();
		}
	}

	UnixStreamSocket::~UnixStreamSocket()
	{
		MS_TRACE_STD();

		delete this->jsonReader;
		delete this->jsonWriter;
	}

	void UnixStreamSocket::SetListener(Listener* listener)
	{
		MS_TRACE_STD();

		this->listener = listener;
	}

	void UnixStreamSocket::Send(Json::Value &msg)
	{
		if (this->closed)
			return;

		// MS_TRACE_STD();

		std::ostringstream stream;
		std::string ns_payload;
		size_t ns_payload_len;
		size_t ns_num_len;
		size_t ns_len;

		this->jsonWriter->write(msg, &stream);
		ns_payload = stream.str();
		ns_payload_len = ns_payload.length();

		if (ns_payload_len > MESSAGE_MAX_SIZE)
		{
			MS_ERROR_STD("mesage too big");

			return;
		}

		if (ns_payload_len == 0)
		{
			ns_num_len = 1;
			UnixStreamSocket::writeBuffer[0] = '0';
			UnixStreamSocket::writeBuffer[1] = ':';
			UnixStreamSocket::writeBuffer[2] = ',';
		}
		else
		{
		  ns_num_len = (size_t)std::ceil(std::log10((double)ns_payload_len + 1));
		  std::sprintf((char*)UnixStreamSocket::writeBuffer, "%zu:", ns_payload_len);
		  std::memcpy(UnixStreamSocket::writeBuffer + ns_num_len + 1, ns_payload.c_str(), ns_payload_len);
		  UnixStreamSocket::writeBuffer[ns_num_len + ns_payload_len + 1] = ',';
		}

		ns_len = ns_num_len + ns_payload_len + 2;

		Write(UnixStreamSocket::writeBuffer, ns_len);
	}

	void UnixStreamSocket::SendLog(char* ns_payload, size_t ns_payload_len)
	{
		if (this->closed)
			return;

		// MS_TRACE_STD();

		size_t ns_num_len;
		size_t ns_len;

		if (ns_payload_len > MESSAGE_MAX_SIZE)
		{
			MS_ERROR_STD("mesage too big");

			return;
		}

		if (ns_payload_len == 0)
		{
			ns_num_len = 1;
			UnixStreamSocket::writeBuffer[0] = '0';
			UnixStreamSocket::writeBuffer[1] = ':';
			UnixStreamSocket::writeBuffer[2] = ',';
		}
		else
		{
		  ns_num_len = (size_t)std::ceil(std::log10((double)ns_payload_len + 1));
		  std::sprintf((char*)UnixStreamSocket::writeBuffer, "%zu:", ns_payload_len);
		  std::memcpy(UnixStreamSocket::writeBuffer + ns_num_len + 1, ns_payload, ns_payload_len);
		  UnixStreamSocket::writeBuffer[ns_num_len + ns_payload_len + 1] = ',';
		}

		ns_len = ns_num_len + ns_payload_len + 2;

		Write(UnixStreamSocket::writeBuffer, ns_len);
	}

	void UnixStreamSocket::SendBinary(const uint8_t* ns_payload, size_t ns_payload_len)
	{
		if (this->closed)
			return;

		size_t ns_num_len;
		size_t ns_len;

		if (ns_payload_len > MESSAGE_MAX_SIZE)
		{
			MS_ERROR_STD("mesage too big");

			return;
		}

		if (ns_payload_len == 0)
		{
			ns_num_len = 1;
			UnixStreamSocket::writeBuffer[0] = '0';
			UnixStreamSocket::writeBuffer[1] = ':';
			UnixStreamSocket::writeBuffer[2] = ',';
		}
		else
		{
		  ns_num_len = (size_t)std::ceil(std::log10((double)ns_payload_len + 1));
		  std::sprintf((char*)UnixStreamSocket::writeBuffer, "%zu:", ns_payload_len);
		  std::memcpy(UnixStreamSocket::writeBuffer + ns_num_len + 1, ns_payload, ns_payload_len);
		  UnixStreamSocket::writeBuffer[ns_num_len + ns_payload_len + 1] = ',';
		}

		ns_len = ns_num_len + ns_payload_len + 2;

		Write(UnixStreamSocket::writeBuffer, ns_len);
	}

	void UnixStreamSocket::userOnUnixStreamRead()
	{
		MS_TRACE_STD();

		// Be ready to parse more than a single message in a single TCP chunk.
		while (true)
		{
			if (IsClosing())
				return;

			size_t read_len = this->bufferDataLen - this->msgStart;
			char* json_start = nullptr;
			size_t json_len;
			int ns_ret = netstring_read((char*)(this->buffer + this->msgStart), read_len,
				&json_start, &json_len);

			if (ns_ret != 0)
			{
				switch (ns_ret)
				{
					case NETSTRING_ERROR_TOO_SHORT:
						// MS_DEBUG_STD("received netstring is too short, need more data");

						// Check if the buffer is full.
						if (this->bufferDataLen == this->bufferSize)
						{
							// First case: the incomplete message does not begin at position 0 of
							// the buffer, so move the incomplete message to the position 0.
							if (this->msgStart != 0)
							{
								// MS_DEBUG_STD("no more space in the buffer, moving parsed bytes to the beginning of the buffer and waiting for more data");

								std::memmove(this->buffer, this->buffer + this->msgStart, read_len);
								this->msgStart = 0;
								this->bufferDataLen = read_len;
							}
							// Second case: the incomplete message begins at position 0 of the buffer.
							// The message is too big, so discard it.
							else
							{
								MS_ERROR_STD("no more space in the buffer for the unfinished message being parsed, discarding it");

								this->msgStart = 0;
								this->bufferDataLen = 0;
							}
						}
						// Otherwise the buffer is not full, just wait.

						// Exit the parsing loop.
						return;

					case NETSTRING_ERROR_TOO_LONG:
						MS_ERROR_STD("NETSTRING_ERROR_TOO_LONG");
						break;

					case NETSTRING_ERROR_NO_COLON:
						MS_ERROR_STD("NETSTRING_ERROR_NO_COLON");
						break;

					case NETSTRING_ERROR_NO_COMMA:
						MS_ERROR_STD("NETSTRING_ERROR_NO_COMMA");
						break;

					case NETSTRING_ERROR_LEADING_ZERO:
						MS_ERROR_STD("NETSTRING_ERROR_LEADING_ZERO");
						break;

					case NETSTRING_ERROR_NO_LENGTH:
						MS_ERROR_STD("NETSTRING_ERROR_NO_LENGTH");
						break;
				}

				// Error, so reset and exit the parsing loop.
				this->msgStart = 0;
				this->bufferDataLen = 0;

				return;
			}

			// If here it means that json_start points to the beginning of a JSON string
			// with json_len bytes length, so recalculate read_len.
			read_len = (const uint8_t*)json_start - (this->buffer + this->msgStart) + json_len + 1;

			Json::Value json;
			std::string json_parse_error;

			if (this->jsonReader->parse((const char*)json_start, (const char*)json_start + json_len, &json, &json_parse_error))
			{
				Channel::Request* request = nullptr;

				try
				{
					request = new Channel::Request(this, json);
				}
				catch (const MediaSoupError &error)
				{
					MS_ERROR_STD("discarding wrong Channel request");
				}

				if (request)
				{
					// Notify the listener.
					this->listener->onChannelRequest(this, request);

					// Delete the Request.
					delete request;
				}
			}
			else
			{
				MS_ERROR_STD("JSON parsing error: %s", json_parse_error.c_str());
			}

			// If there is no more space available in the buffer and that is because
			// the latest parsed message filled it, then empty the full buffer.
			if ((this->msgStart + read_len) == this->bufferSize)
			{
				// MS_DEBUG_STD("no more space in the buffer, emptying the buffer data");

				this->msgStart = 0;
				this->bufferDataLen = 0;
			}
			// If there is still space in the buffer, set the beginning of the next
			// parsing to the next position after the parsed message.
			else
			{
				this->msgStart += read_len;
			}

			// If there is more data in the buffer after the parsed message
			// then parse again. Otherwise break here and wait for more data.
			if (this->bufferDataLen > this->msgStart)
			{
				// MS_DEBUG_STD("there is more data after the parsed message, continue parsing");

				continue;
			}
			else
			{
				break;
			}
		}
	}

	void UnixStreamSocket::userOnUnixStreamSocketClosed(bool is_closed_by_peer)
	{
		MS_TRACE_STD();

		this->closed = true;

		if (is_closed_by_peer)
		{
			// Notify the listener.
			this->listener->onChannelUnixStreamSocketRemotelyClosed(this);
		}
	}
}
