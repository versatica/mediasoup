#ifndef MS_RTC_SCTP_TYPES_HPP
#define MS_RTC_SCTP_TYPES_HPP

#include "common.hpp"
#include <string_view>

namespace RTC
{
	namespace SCTP
	{
		namespace Types
		{
			/**
			 * Publicly exposed SCTP socket state.
			 */
			enum class SocketState
			{
				/**
				 * The socket is closed.
				 */
				CLOSED,
				/**
				 * The Socket has initiated a connection, which is not yet established.
				 *
				 * @remarks
				 * - For incoming connections and for reconnections when the Socket is
				 *   already connected, the Socket will not transition to this state.
				 */
				CONNECTING,
				/**
				 * The Socket is connected and the connection is established.
				 */
				CONNECTED,
				/**
				 * The Socket is shutting down, and the connection is not yet closed.
				 */
				SHUTTING_DOWN,
			};

			constexpr std::string_view SocketStateToString(SocketState socketState)
			{
				switch (socketState)
				{
					case SocketState::CLOSED:
					{
						return "Closed";
					}

					case SocketState::CONNECTING:
					{
						return "Connecting";
					}

					case SocketState::CONNECTED:
					{
						return "Connected";
					}

					case SocketState::SHUTTING_DOWN:
					{
						return "ShuttingDown";
					}
				}
			}

			/**
			 * Kinds of errors that are exposed in the Socket API.
			 */
			enum class ErrorKind
			{
				/**
				 * Indicates that no error has occurred. This will never be the case when
				 * OnError() or OnAborted() is called.
				 */
				NoError,
				/**
				 * There have been too many retries or timeouts, and the library has given
				 * up.
				 */
				TooManyRetries,
				/**
				 * A command was received that is only possible to execute when the Socket
				 * is connected, which it is not.
				 */
				NotConnected,
				/**
				 * Parsing of the command or its parameters failed.
				 */
				ParseFailed,
				/**
				 * Commands are received in the wrong sequence, which indicates a
				 * synchronisation mismatch between the peers.
				 */
				WrongSequence,
				/**
				 * The peer has reported an issue using ERROR or ABORT command.
				 */
				PeerReported,
				/**
				 * The peer has performed a protocol violation.
				 */
				ProtocolViolation,
				/**
				 * The receive or send buffers have been exhausted.
				 */
				ResourceExhaustion,
				/**
				 * The application has performed an invalid operation.
				 */
				UnsupportedOperation,
			};

			constexpr std::string_view ErrorKindToString(ErrorKind errorKind)
			{
				switch (errorKind)
				{
					case ErrorKind::NoError:
					{
						return "NO_ERROR";
					}

					case ErrorKind::TooManyRetries:
					{
						return "TOO_MANY_RETRIES";
					}

					case ErrorKind::NotConnected:
					{
						return "NOT_CONNECTED";
					}

					case ErrorKind::ParseFailed:
					{
						return "PARSE_FAILED";
					}

					case ErrorKind::WrongSequence:
					{
						return "WRONG_SEQUENCE";
					}

					case ErrorKind::PeerReported:
					{
						return "PEER_REPORTED";
					}

					case ErrorKind::ProtocolViolation:
					{
						return "PROTOCOL_VIOLATION";
					}

					case ErrorKind::ResourceExhaustion:
					{
						return "RESOURCE_EXHAUSTION";
					}

					case ErrorKind::UnsupportedOperation:
					{
						return "UNSUPPORTED_OPERATION";
					}
				}
			}

			/**
			 * SCTP implementation determined by first 8 bytes of the State Cookie
			 * sent by the remote peer.
			 */
			enum class SctpImplementation
			{
				UNKNOWN,
				MEDIASOUP,
				DCSCTP,
				USRSCTP,
			};

			constexpr std::string_view SctpImplementationToString(SctpImplementation sctpImplementation)
			{
				switch (sctpImplementation)
				{
					case SctpImplementation::UNKNOWN:
					{
						return "unknown";
					}

					case SctpImplementation::MEDIASOUP:
					{
						return "mediasoup";
					}

					case SctpImplementation::DCSCTP:
					{
						return "dcsctp";
					}

					case SctpImplementation::USRSCTP:
					{
						return "usrsctp";
					}
				}
			}
		} // namespace Types
	} // namespace SCTP
} // namespace RTC

#endif
