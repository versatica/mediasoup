#ifndef MS_RTC_SCTP_ASSOCIATION_HPP
#define MS_RTC_SCTP_ASSOCIATION_HPP

#include "common.hpp"
#include "RTC/SCTP/association/AssociationDeferredListener.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/association/PacketSender.hpp"
#include "RTC/SCTP/association/StateCookie.hpp"
#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include "RTC/SCTP/packet/chunks/AnyDataChunk.hpp"
#include "RTC/SCTP/packet/chunks/AnyForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/AnyInitChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieEchoChunk.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/chunks/OperationErrorChunk.hpp"
#include "RTC/SCTP/packet/chunks/ReConfigChunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/packet/chunks/UnknownChunk.hpp"
#include "RTC/SCTP/public/AssociationInterface.hpp"
#include "RTC/SCTP/public/AssociationListener.hpp"
#include "RTC/SCTP/public/AssociationMetrics.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "handles/BackoffTimerHandle.hpp"
#include <FBS/sctpParameters.h>
#include <span>
#include <string_view>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * This is the implementation of the AssociationInterface.
		 */
		class Association : public AssociationInterface,
		                    public PacketSender::Listener,
		                    public BackoffTimerHandle::Listener
		{
		public:
			/**
			 * Internal SCTP association state. This is different from the public SCTP
			 * Association state (`SCTP::Types::AssociationState`).
			 */
			enum class State : uint8_t
			{
				NEW,
				CLOSED,
				COOKIE_WAIT,
				// NOTE: TCB is valid in these states:
				COOKIE_ECHOED,
				ESTABLISHED,
				SHUTDOWN_PENDING,
				SHUTDOWN_SENT,
				SHUTDOWN_RECEIVED,
				SHUTDOWN_ACK_SENT
			};

			static constexpr std::string_view StateToString(State state)
			{
				// NOTE: We cannot use MS_TRACE() here because clang in Linux will
				// complain about "read of non-constexpr variable 'configuration' is not
				// allowed in a constant expression".

				switch (state)
				{
					case State::NEW:
					{
						return "NEW";
					}

					case State::CLOSED:
					{
						return "CLOSED";
					}

					case State::COOKIE_WAIT:
					{
						return "COOKIE_WAIT";
					}

					case State::COOKIE_ECHOED:
					{
						return "COOKIE_ECHOED";
					}

					case State::ESTABLISHED:
					{
						return "ESTABLISHED";
					}

					case State::SHUTDOWN_PENDING:
					{
						return "SHUTDOWN_PENDING";
					}

					case State::SHUTDOWN_SENT:
					{
						return "SHUTDOWN_SENT";
					}

					case State::SHUTDOWN_RECEIVED:
					{
						return "SHUTDOWN_RECEIVED";
					}

					case State::SHUTDOWN_ACK_SENT:
					{
						return "SHUTDOWN_ACK_SENT";
					}

						NO_DEFAULT_GCC();
				}
			}

			/**
			 * Struct holding local verification tag and initial TSN between having
			 * sent the INIT Chunk until the connection is established (there is no
			 * TCB in between).
			 *
			 * @remarks
			 * - This is how dcSCTP does, despite RFC 9260 states that the TCB should
			 *   also be created when an INIT Chunk is sent.
			 */
			struct PreTransmissionControlBlock
			{
				uint32_t localVerificationTag{ 0 };
				uint32_t localInitialTsn{ 0 };
			};

			/**
			 * Metrics that are directly filled by the Association class.
			 *
			 * @remarks
			 * - This struct is a subset of the public AssociationMetrics struct.
			 */
			struct AssociationPrivateMetrics
			{
				uint64_t txPacketsCount{ 0 };
				uint64_t txMessagesCount{ 0 };
				uint64_t rxPacketsCount{ 0 };
				uint64_t rxMessagesCount{ 0 };
				Types::SctpImplementation peerImplementation{ Types::SctpImplementation::UNKNOWN };
				uint16_t negotiatedMaxOutboundStreams{ 0 };
				uint16_t negotiatedMaxInboundStreams{ 0 };
				bool usesPartialReliability{ false };
				bool usesMessageInterleaving{ false };
				bool usesReConfig{ false };
				bool usesZeroChecksum{ false };
			};

		public:
			explicit Association(const SctpOptions& sctpOptions, AssociationListener* listener);

			~Association() override;

		public:
			void Dump(int indentation = 0) const override;

			flatbuffers::Offset<FBS::SctpParameters::SctpParameters> FillBuffer(
			  flatbuffers::FlatBufferBuilder& builder) const override;

			Types::AssociationState GetAssociationState() const override;

			/**
			 * Initiate the SCTP association with the remote peer. It sends an INIT
			 * Chunk.
			 *
			 * @remarks
			 * - The SCTP association must be in Closed state.
			 */
			void Connect() override;

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
			void Shutdown() override;

			/**
			 * Closes the Association non-gracefully. Will send ABORT if the connection
			 * is not already closed. No callbacks will be made after Close() has
			 * returned. However, before Close() returns, it may have called
			 * `OnAssociationClosed()` or `OnAssociationAborted()` callbacks.
			 */
			void Close() override;

			/**
			 * Retrieves the latest metrics. If the Association is not fully connected,
			 * `std::nullopt` will be returned.
			 */
			std::optional<AssociationMetrics> GetMetrics() const override;

			/**
			 * Returns the currently set priority for an outgoing stream. The initial
			 * value, when not set, is `SctpOptions::defaultStreamPriority`.
			 */
			uint16_t GetStreamPriority(uint16_t streamId) const override;

			/**
			 * Sets the priority of an outgoing stream. The initial value, when not
			 * set, is `SctpOptions::defaultStreamPriority`.
			 */
			void SetStreamPriority(uint16_t streamId, uint16_t priority) override;

			/**
			 * Sets the maximum size of sent messages. The initial value, when not
			 * set, is `SctpOptions::maxSendMessageSize`.
			 */
			void SetMaxSendMessageSize(size_t maxMessageSize) override;

			/**
			 * Returns the number of bytes of data currently queued to be sent on a
			 * given stream.
			 */
			size_t GetStreamBufferedAmount(uint16_t streamId) const override;

			/**
			 * Returns the number of buffered outgoing bytes that is considered "low"
			 * for a given stream. See `SetStreamBufferedAmountLowThreshold()`.
			 */
			size_t GetStreamBufferedAmountLowThreshold(uint16_t streamId) const override;

			/**
			 * Specifies the number of bytes of buffered outgoing data that is
			 * considered "low" for a given stream, which will trigger
			 * `OnAssociationStreamBufferedAmountLow()` event. The default value is 0.
			 */
			void SetBufferedAmountLowThreshold(uint16_t streamId, size_t bytes) override;

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
			Types::ResetStreamsStatus ResetStreams(std::span<const uint16_t> outboundStreamIds) override;

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
			Types::SendMessageStatus SendMessage(
			  Message message, const SendMessageOptions& sendMessageOptions) override;

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
			std::vector<Types::SendMessageStatus> SendManyMessages(
			  std::span<Message> messages, const SendMessageOptions& sendMessageOptions) override;

			/**
			 * Receives SCTP data (hopefully an SCTP Packet) from the remote peer.
			 */
			void ReceiveSctpData(const uint8_t* data, size_t len) override;

		private:
			void InternalClose(Types::ErrorKind errorKind, const std::string_view& message);

			void SetState(State state, const std::string_view& message);

			void AddCapabilitiesParametersToInitOrInitAckChunk(AnyInitChunk* chunk) const;

			void CreateTransmissionControlBlock(
			  uint32_t localVerificationTag,
			  uint32_t remoteVerificationTag,
			  uint32_t localInitialTsn,
			  uint32_t remoteInitialTsn,
			  uint32_t remoteAdvertisedReceiverWindowCredit,
			  uint64_t tieTag,
			  const NegotiatedCapabilities& negotiatedCapabilities);

			std::unique_ptr<Packet> CreatePacket() const;

			std::unique_ptr<Packet> CreatePacketWithVerificationTag(uint32_t verificationTag) const;

			void SendInitChunk();

			void SendShutdownChunk();

			void SendShutdownAckChunk();

			/**
			 * Sends SHUTDOWN or SHUTDOWN-ACK if the Association is shutting down and
			 * if all outstanding data has been acknowledged.
			 */
			void MaySendShutdownOrShutdownAckChunk();

			/**
			 * If the Association is shutting down, responds SHUTDOWN to any incoming
			 * DATA.
			 */
			void MaySendShutdownOnPacketReceived(const Packet* receivedPacket);

			/**
			 * If there are streams pending to be reset, send a request to reset them.
			 */
			void MaySendResetStreamsRequest();

			/**
			 * Called whenever data has been received, or the cumulative acknowledgment
			 * TSN has moved, that may result in delivering messages.
			 */
			void MayDeliverMessages();

			Types::SendMessageStatus InternalSendMessage(
			  const Message& message, const SendMessageOptions& sendMessageOptions);

			bool ValidateReceivedPacket(const Packet* receivedPacket);

			bool ProcessReceivedChunk(const Packet* receivedPacket, const Chunk* receivedChunk);

			void ProcessReceivedInitChunk(const Packet* receivedPacket, const InitChunk* receivedInitChunk);

			void ProcessReceivedInitAckChunk(
			  const Packet* receivedPacket, const InitAckChunk* receivedInitAckChunk);

			void ProcessReceivedCookieEchoChunk(
			  const Packet* receivedPacket, const CookieEchoChunk* receivedCookieEchoChunk);

			bool ProcessReceivedCookieEchoChunkWithTcb(const Packet* receivedPacket, const StateCookie* cookie);

			void ProcessReceivedCookieAckChunk(
			  const Packet* receivedPacket, const CookieAckChunk* receivedCookieAckChunk);

			void ProcessReceivedShutdownChunk(
			  const Packet* receivedPacket, const ShutdownChunk* receivedShutdownChunk);

			void ProcessReceivedShutdownAckChunk(
			  const Packet* receivedPacket, const ShutdownAckChunk* receivedShutdownAckChunk);

			void ProcessReceivedShutdownCompleteChunk(
			  const Packet* receivedPacket, const ShutdownCompleteChunk* receivedShutdownCompleteChunk);

			void ProcessReceivedOperationErrorChunk(
			  const Packet* receivedPacket, const OperationErrorChunk* receivedOperationErrorChunk);

			void ProcessReceivedAbortAssociationChunk(
			  const Packet* receivedPacket, const AbortAssociationChunk* receivedAbortAssociationChunk);

			void ProcessReceivedHeartbeatRequestChunk(
			  const Packet* receivedPacket, const HeartbeatRequestChunk* receivedHeartbeatRequestChunk);

			void ProcessReceivedHeartbeatAckChunk(
			  const Packet* receivedPacket, const HeartbeatAckChunk* receivedHeartbeatAckChunk);

			void ProcessReceivedReConfigChunk(
			  const Packet* receivedPacket, const ReConfigChunk* receivedReConfigChunk);

			void ProcessReceivedForwardTsnChunk(
			  const Packet* receivedPacket, const ForwardTsnChunk* receivedForwardTsnChunk);

			void ProcessReceivedIForwardTsnChunk(
			  const Packet* receivedPacket, const IForwardTsnChunk* receivedIForwardTsnChunk);

			void ProcessReceivedAnyForwardTsnChunk(
			  const Packet* receivedPacket, const AnyForwardTsnChunk* receivedAnyForwardTsnChunk);

			void ProcessReceivedDataChunk(const Packet* receivedPacket, const DataChunk* receivedDataChunk);

			void ProcessReceivedIDataChunk(const Packet* receivedPacket, const IDataChunk* receivedIDataChunk);

			void ProcessReceivedAnyDataChunk(
			  const Packet* receivedPacket, const AnyDataChunk* receivedAnyDataChunk);

			void ProcessReceivedSackChunk(const Packet* receivedPacket, const SackChunk* receivedSackChunk);

			bool ProcessReceivedUnknownChunk(
			  const Packet* receivedPacket, const UnknownChunk* receivedUnknownChunk);

			void OnT1InitTimer(uint64_t& baseTimeoutMs, bool& stop);

			void OnT1CookieTimer(uint64_t& baseTimeoutMs, bool& stop);

			void OnT2ShutdownTimer(uint64_t& baseTimeoutMs, bool& stop);

			template<typename... States>
			void AssertState(States... expectedStates) const;

			template<typename... States>
			void AssertNotState(States... unexpectedStates) const;

			/**
			 * Returns true if there is a TCB, and false otherwise (and reports an
			 * error).
			 */
			bool ValidateHasTcb();

			void AssertHasTcb() const;

			void AssertStateIsConsistent() const;

			/* Pure virtual methods inherited from PacketSender::Listener. */
		public:
			void OnPacketSenderPacketSent(PacketSender* packetSender, const Packet* packet, bool sent) override;

			/* Pure virtual methods inherited from BackoffTimerHandle::Listener. */
		public:
			void OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeoutMs, bool& stop) override;

		private:
			// SCTP options given in the constructor.
			SctpOptions sctpOptions;
			// Listener. It's not an AssociationListener but an
			// AssociationDeferredListener which inherits from AssociationListener.
			AssociationDeferredListener listener;
			// SCTP association internal state.
			State state{ State::NEW };
			// Private metrics.
			AssociationPrivateMetrics privateMetrics{};
			// The actual send queue implementation. As data can be sent before the
			// connection is established, this component is not in the TCB.
			// TODO: Implement this class.
			// RRSendQueue sendQueue;
			// To keep settings between sending of INIT Chunk and establishment of
			// the connection.
			PreTransmissionControlBlock preTcb;
			// Once the SCTP association is established a Transmission Control Block
			// is created.
			std::unique_ptr<TransmissionControlBlock> tcb;
			// Packet sender.
			PacketSender packetSender;
			// T1-init timer.
			const std::unique_ptr<BackoffTimerHandle> t1InitTimer;
			// T1-cookie timer.
			const std::unique_ptr<BackoffTimerHandle> t1CookieTimer;
			// T2-shutdown timer.
			const std::unique_ptr<BackoffTimerHandle> t2ShutdownTimer;
		};
	} // namespace SCTP
} // namespace RTC

#endif
