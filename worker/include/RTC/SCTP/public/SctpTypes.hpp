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
			 * Publicly exposed SCTP Association state.
			 */
			enum class AssociationState : uint8_t
			{
				/**
				 * The Association is closed.
				 */
				CLOSED,
				/**
				 * The Association has initiated a connection, which is not yet
				 * established.
				 *
				 * @remarks
				 * - For incoming connections and for reconnections when the Association
				 *   is already connected, the Association will not transition to this
				 *   state.
				 */
				CONNECTING,
				/**
				 * The Association is connected and the connection is established.
				 */
				CONNECTED,
				/**
				 * The Association is shutting down, and the connection is not yet closed.
				 */
				SHUTTING_DOWN
			};

			constexpr std::string_view AssociationStateToString(Types::AssociationState associationState)
			{
				switch (associationState)
				{
					case Types::AssociationState::CLOSED:
					{
						return "Closed";
					}

					case Types::AssociationState::CONNECTING:
					{
						return "Connecting";
					}

					case Types::AssociationState::CONNECTED:
					{
						return "Connected";
					}

					case Types::AssociationState::SHUTTING_DOWN:
					{
						return "ShuttingDown";
					}
				}
			}

			/**
			 * Kinds of errors that are exposed in the API.
			 */
			enum class ErrorKind : uint8_t
			{
				/**
				 * Indicates that no error has occurred. This will never be the case when
				 * OnError() or OnAborted() is called.
				 */
				NO_ERROR,
				/**
				 * There have been too many retries or timeouts, and the library has given
				 * up.
				 */
				TOO_MANY_RETRIES,
				/**
				 * A command was received that is only possible to execute when the
				 * Association is connected, which it is not.
				 */
				NOT_CONNECTED,
				/**
				 * Parsing of the command or its parameters failed.
				 */
				PARSE_FAILED,
				/**
				 * Commands are received in the wrong sequence, which indicates a
				 * synchronisation mismatch between the peers.
				 */
				WRONG_SEQUENCE,
				/**
				 * The peer has reported an issue using ERROR or ABORT command.
				 */
				PEER_REPORTED,
				/**
				 * The peer has performed a protocol violation.
				 */
				PROTOCOL_VIOLATION,
				/**
				 * The receive or send buffers have been exhausted.
				 */
				RESOURCE_EXHAUSTION,
				/**
				 * The application has performed an invalid operation.
				 */
				UNSUPPORTED_OPERATION
			};

			constexpr std::string_view ErrorKindToString(Types::ErrorKind errorKind)
			{
				switch (errorKind)
				{
					case Types::ErrorKind::NO_ERROR:
					{
						return "NO_ERROR";
					}

					case Types::ErrorKind::TOO_MANY_RETRIES:
					{
						return "TOO_MANY_RETRIES";
					}

					case Types::ErrorKind::NOT_CONNECTED:
					{
						return "NOT_CONNECTED";
					}

					case Types::ErrorKind::PARSE_FAILED:
					{
						return "PARSE_FAILED";
					}

					case Types::ErrorKind::WRONG_SEQUENCE:
					{
						return "WRONG_SEQUENCE";
					}

					case Types::ErrorKind::PEER_REPORTED:
					{
						return "PEER_REPORTED";
					}

					case Types::ErrorKind::PROTOCOL_VIOLATION:
					{
						return "PROTOCOL_VIOLATION";
					}

					case Types::ErrorKind::RESOURCE_EXHAUSTION:
					{
						return "RESOURCE_EXHAUSTION";
					}

					case Types::ErrorKind::UNSUPPORTED_OPERATION:
					{
						return "UNSUPPORTED_OPERATION";
					}
				}
			}

			/**
			 * SCTP implementation determined by first 8 bytes of the State Cookie
			 * sent by the remote peer.
			 */
			enum class SctpImplementation : uint8_t
			{
				UNKNOWN,
				MEDIASOUP,
				DCSCTP,
				USRSCTP
			};

			constexpr std::string_view SctpImplementationToString(Types::SctpImplementation sctpImplementation)
			{
				switch (sctpImplementation)
				{
					case Types::SctpImplementation::UNKNOWN:
					{
						return "unknown";
					}

					case Types::SctpImplementation::MEDIASOUP:
					{
						return "mediasoup";
					}

					case Types::SctpImplementation::DCSCTP:
					{
						return "dcsctp";
					}

					case Types::SctpImplementation::USRSCTP:
					{
						return "usrsctp";
					}
				}
			}

			/**
			 * Return value of Association::ResetStreams().
			 */
			enum class ResetStreamsStatus : uint8_t
			{
				/**
				 * If the connection is not yet established, this will be returned.
				 */
				NOT_CONNECTED,

				/**
				 * Indicates that ResetStreams operation has been successfully
				 * initiated.
				 */
				PERFORMED,

				/**
				 * Indicates that resetting streams has failed as it's not supported by
				 * the peer.
				 */
				NOT_SUPPORTED
			};

			constexpr std::string_view ResetStreamsStatusToString(Types::ResetStreamsStatus status)
			{
				switch (status)
				{
					case Types::ResetStreamsStatus::NOT_CONNECTED:
					{
						return "NOT_CONNECTED";
					}

					case Types::ResetStreamsStatus::PERFORMED:
					{
						return "PERFORMED";
					}

					case Types::ResetStreamsStatus::NOT_SUPPORTED:
					{
						return "NOT_SUPPORTED";
					}
				}
			}

			/**
			 * Return value of Association::SendMessage() and
			 * Association::SendManyMessages().
			 */
			enum class SendMessageStatus : uint8_t
			{
				/**
				 * The message was enqueued successfully. As sending the message is done
				 * asynchronously, this is no guarantee that the message has been
				 * actually sent.
				 */
				SUCCESS,
				/**
				 * The message was rejected as the payload was empty (which is not
				 * allowed in SCTP).
				 */
				ERROR_MESSAGE_EMPTY,

				/**
				 * The message was rejected as the payload was larger than what has been
				 * set as `SctpOptions.maxMessageSize`.
				 */
				ERROR_MESSAGE_TOO_LARGE,

				/**
				 * The message could not be enqueued as the Association is out of
				 * resources. This mainly indicates that the send queue is full.
				 */
				ERROR_RESOURCE_EXHAUSTION,

				/**
				 * The message could not be sent as the Association is shutting down.
				 */
				ERROR_SHUTTING_DOWN
			};

			constexpr std::string_view SendMessageStatusToString(Types::SendMessageStatus status)
			{
				switch (status)
				{
					case Types::SendMessageStatus::SUCCESS:
					{
						return "SUCCESS";
					}

					case Types::SendMessageStatus::ERROR_MESSAGE_EMPTY:
					{
						return "ERROR_MESSAGE_EMPTY";
					}

					case Types::SendMessageStatus::ERROR_MESSAGE_TOO_LARGE:
					{
						return "ERROR_MESSAGE_TOO_LARGE";
					}

					case Types::SendMessageStatus::ERROR_RESOURCE_EXHAUSTION:
					{
						return "ERROR_RESOURCE_EXHAUSTION";
					}

					case Types::SendMessageStatus::ERROR_SHUTTING_DOWN:
					{
						return "ERROR_SHUTTING_DOWN";
					}
				}
			}
		} // namespace Types
	} // namespace SCTP
} // namespace RTC

#endif
