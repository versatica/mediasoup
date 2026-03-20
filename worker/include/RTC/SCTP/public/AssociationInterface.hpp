#ifndef MS_RTC_SCTP_ASSOCIATION_INTERFACE_HPP
#define MS_RTC_SCTP_ASSOCIATION_INTERFACE_HPP

#include "common.hpp"
#include "RTC/SCTP/public/AssociationMetrics.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include <FBS/sctpParameters.h>
#include <span>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * The SCTP Association class represents the mediasoup side of an SCTP
		 * association with a remote peer.
		 *
		 * It manages all Packet and Chunk dispatching and the connection flow.
		 */
		class AssociationInterface
		{
		public:
			virtual ~AssociationInterface() = default;

			virtual void Dump(int indentation = 0) const = 0;

			virtual flatbuffers::Offset<FBS::SctpParameters::SctpParameters> FillBuffer(
			  flatbuffers::FlatBufferBuilder& builder) const = 0;

			virtual Types::AssociationState GetAssociationState() const = 0;

			/**
			 * Initiate the SCTP association with the remote peer. It sends an INIT
			 * Chunk.
			 *
			 * @remarks
			 * - The SCTP association must be in Closed state.
			 */
			virtual void Connect() = 0;

			/**
			 * Gracefully shutdowns the Association and sends all outstanding data.
			 * This is an asynchronous operation and `OnAssociationClosed()` will be
			 * called on success.
			 *
			 * @remarks
			 * - libwebrtc never calls the corresponding DcSctpSocket::Shutdown()
			 *   method due to a bug and hence we shouldn't either.
			 *
			 * @see https://issues.webrtc.org/issues/42222897
			 */
			virtual void Shutdown() = 0;

			/**
			 * Closes the Association non-gracefully. Will send ABORT if the connection
			 * is not already closed. No callbacks will be made after Close() has
			 * returned. However, before Close() returns, it may have called
			 * `OnAssociationClosed()` or `OnAssociationAborted()` callbacks.
			 */
			virtual void Close() = 0;

			/**
			 * Retrieves the latest metrics. If the Association is not fully connected,
			 * `std::nullopt` will be returned.
			 */
			virtual std::optional<AssociationMetrics> GetMetrics() const = 0;

			/**
			 * Returns the currently set priority for an outgoing stream. The initial
			 * value, when not set, is `SctpOptions::defaultStreamPriority`.
			 */
			virtual uint16_t GetStreamPriority(uint16_t streamId) const = 0;

			/**
			 * Sets the priority of an outgoing stream. The initial value, when not
			 * set, is `SctpOptions::defaultStreamPriority`.
			 */
			virtual void SetStreamPriority(uint16_t streamId, uint16_t priority) = 0;

			/**
			 * Sets the maximum size of sent messages. The initial value, when not
			 * set, is `SctpOptions::maxSendMessageSize`.
			 */
			virtual void SetMaxSendMessageSize(size_t maxMessageSize) = 0;

			/**
			 * Returns the number of bytes of data currently queued to be sent on a
			 * given stream.
			 */
			virtual size_t GetStreamBufferedAmount(uint16_t streamId) const = 0;

			/**
			 * Returns the number of buffered outgoing bytes that is considered "low"
			 * for a given stream. See `SetStreamBufferedAmountLowThreshold()`.
			 */
			virtual size_t GetStreamBufferedAmountLowThreshold(uint16_t streamId) const = 0;

			/**
			 * Specifies the number of bytes of buffered outgoing data that is
			 * considered "low" for a given stream, which will trigger
			 * `OnAssociationStreamBufferedAmountLow()` event. The default value is 0.
			 */
			virtual void SetBufferedAmountLowThreshold(uint16_t streamId, size_t bytes) = 0;

			/**
			 * Resetting streams is an asynchronous operation and the results will be
			 * notified using `OnAssociationStreamsResetPerformed()` on success and
			 * `OnAssociationStreamsResetFailed()` on failure.
			 *
			 * When it's known that the peer has reset its own outgoing streams,
			 * `OnAssociationInboundStreamsReset()` is called.
			 *
			 * Resetting streams can only be done on an established association that
			 * supports stream resetting. Calling this method on e.g. a closed SCTP
			 * association or streams that don't support resetting will not perform
			 * any operation.
			 *
			 * @remarks
			 * - Only outbound streams can be reset.
			 * - Resetting a stream will also remove all queued messages on those
			 *   streams, but will ensure that the currently sent message (if any) is
			 *   fully sent before closing the stream.
			 */
			virtual Types::ResetStreamsStatus ResetStreams(std::span<const uint16_t> outboundStreamIds) = 0;

			/**
			 * Sends an SCTP message using the provided send options. Sending a message
			 * is an asynchronous operation, and the `OnAssociationError()` callback
			 * may be invoked to indicate any errors in sending the message.
			 *
			 * The association does not have to be established before calling this
			 * method. If it's called before there is an established association, the
			 * message will be queued.
			 *
			 * @remarks
			 * - Copy constructor is disabled and there is move constructor. That's why
			 *   we don't pass a reference here. We could pass `Message&&` but that's
			 *   worse opens the door to bugs.
			 */
			virtual Types::SendMessageStatus SendMessage(
			  Message message, const SendMessageOptions& sendMessageOptions) = 0;

			/**
			 * Sends SCTP messages using the provided send options. Sending a message
			 * is an asynchronous operation, and the `OnAssociationError()` callback
			 * may be invoked to indicate any errors in sending a message.
			 *
			 * The association does not have to be established before calling this
			 * method. If it's called before there is an established association, the
			 * message will be queued.
			 *
			 * This has identical semantics to `SendMessage()', except that it may
			 * coalesce many messages into a single SCTP Packet if they would fit.
			 *
			 * @remarks
			 * - Same as in `SendMessage()`.
			 */
			virtual std::vector<Types::SendMessageStatus> SendManyMessages(
			  std::span<Message> messages, const SendMessageOptions& sendMessageOptions) = 0;

			/**
			 * Receives SCTP data (hopefully an SCTP Packet) from the remote peer.
			 */
			virtual void ReceiveSctpData(const uint8_t* data, size_t len) = 0;
		};
	} // namespace SCTP
} // namespace RTC

#endif
