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
				 * Initial state.
				 *
				 * @remarks
				 * - Once state changes it will never transition to NEW again.
				 */
				NEW,
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

			constexpr std::string_view AssociationStateToString(AssociationState associationState)
			{
				switch (associationState)
				{
					case AssociationState::NEW:
					{
						return "NEW";
					}

					case AssociationState::CLOSED:
					{
						return "CLOSED";
					}

					case AssociationState::CONNECTING:
					{
						return "CONNECTING";
					}

					case AssociationState::CONNECTED:
					{
						return "CONNECTED";
					}

					case AssociationState::SHUTTING_DOWN:
					{
						return "SHUTTING_DOWN";
					}

						NO_DEFAULT_GCC();
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
				SUCCESS,
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

			constexpr std::string_view ErrorKindToString(ErrorKind errorKind)
			{
				switch (errorKind)
				{
					// NOTE: In dcsctp this is `NO_ERROR` but it fails on MSVC because
					// some Windows headers define `NO_ERROR` macro.
					case ErrorKind::SUCCESS:
					{
						return "SUCCESS";
					}

					case ErrorKind::TOO_MANY_RETRIES:
					{
						return "TOO_MANY_RETRIES";
					}

					case ErrorKind::NOT_CONNECTED:
					{
						return "NOT_CONNECTED";
					}

					case ErrorKind::PARSE_FAILED:
					{
						return "PARSE_FAILED";
					}

					case ErrorKind::WRONG_SEQUENCE:
					{
						return "WRONG_SEQUENCE";
					}

					case ErrorKind::PEER_REPORTED:
					{
						return "PEER_REPORTED";
					}

					case ErrorKind::PROTOCOL_VIOLATION:
					{
						return "PROTOCOL_VIOLATION";
					}

					case ErrorKind::RESOURCE_EXHAUSTION:
					{
						return "RESOURCE_EXHAUSTION";
					}

					case ErrorKind::UNSUPPORTED_OPERATION:
					{
						return "UNSUPPORTED_OPERATION";
					}

						NO_DEFAULT_GCC();
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

						NO_DEFAULT_GCC();
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

			constexpr std::string_view ResetStreamsStatusToString(ResetStreamsStatus status)
			{
				switch (status)
				{
					case ResetStreamsStatus::NOT_CONNECTED:
					{
						return "NOT_CONNECTED";
					}

					case ResetStreamsStatus::PERFORMED:
					{
						return "PERFORMED";
					}

					case ResetStreamsStatus::NOT_SUPPORTED:
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

			constexpr std::string_view SendMessageStatusToString(SendMessageStatus status)
			{
				switch (status)
				{
					case SendMessageStatus::SUCCESS:
					{
						return "SUCCESS";
					}

					case SendMessageStatus::ERROR_MESSAGE_EMPTY:
					{
						return "ERROR_MESSAGE_EMPTY";
					}

					case SendMessageStatus::ERROR_MESSAGE_TOO_LARGE:
					{
						return "ERROR_MESSAGE_TOO_LARGE";
					}

					case SendMessageStatus::ERROR_RESOURCE_EXHAUSTION:
					{
						return "ERROR_RESOURCE_EXHAUSTION";
					}

					case SendMessageStatus::ERROR_SHUTTING_DOWN:
					{
						return "ERROR_SHUTTING_DOWN";
					}
				}
			}
		} // namespace Types
	} // namespace SCTP
} // namespace RTC

#endif
