#include "common.hpp"
#include "RTC/SCTP/association/Association.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieEchoChunk.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "mocks/include/MockShared.hpp"
#include "mocks/include/RTC/SCTP/association/MockAssociationListener.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <span>
#include <string_view>
#include <vector>

// NOTE: This test mirrors libwebrtc's dcsctp/socket/dcsctp_socket_test.cc. The
// mediasoup SCTP::Association class is the equivalent of dcsctp's DcSctpSocket.
// Instead of relying on gmock matchers (as dcsctp does) we use the inspection
// API already exposed by SCTP::Packet and SCTP::Chunk.

namespace
{
	// Initial value of the simulated clock (ms).
	constexpr uint64_t InitialNowMs{ 1000000 };

	// All backoff timer labels an SCTP Association may create. Used by RunTimers()
	// to fire whichever timers have expired after advancing time, mirroring
	// dcsctp's RunTimers() test helper.
	constexpr std::array<std::string_view, 8> SctpTimerLabels{
		"sctp-t1-init",          "sctp-t1-cookie",   "sctp-t2-shutdown", "sctp-t3-rtx",
		"sctp-delayed-ack",      "sctp-heartbeat-interval", "sctp-heartbeat-timeout",
		"sctp-re-config",
	};

	/**
	 * Builds the default SctpOptions used in tests. Tweaked to make timers
	 * predictable, as dcsctp's FixupOptions() does.
	 */
	RTC::SCTP::SctpOptions MakeSctpOptions()
	{
		RTC::SCTP::SctpOptions sctpOptions;

		// To make the heartbeat interval more predictable in tests.
		sctpOptions.heartbeatIntervalIncludeRtt = false;
		sctpOptions.maxBurst                    = 4;

		return sctpOptions;
	}

	/**
	 * An SCTP Association under test, together with its simulated clock and
	 * listener. This is the equivalent of dcsctp's SocketUnderTest.
	 */
	class AssociationUnderTest
	{
	public:
		/**
		 * `mayConnectOnReceivedSctpData` defaults to false so that, just like
		 * dcsctp, the connection is only initiated by explicitly calling `Connect()`.
		 * In production mediasoup associations auto-initiate the connection upon
		 * receiving data (both peers are active), but for tests we want to mimic
		 * dcsctp's asymmetric handshake (one active peer, one passive peer) so that
		 * packet sequences and counts match.
		 */
		explicit AssociationUnderTest(
		  RTC::SCTP::SctpOptions sctpOptions = MakeSctpOptions(), bool mayConnectOnReceivedSctpData = false)
		  // NOTE: The order in which these members are initialized is **critical**.
		  : sctpOptions(sctpOptions),
		    shared(/*getTimeMs*/
		           [this]()
		           {
			           return this->nowMs;
		           }),
		    association(
		      this->sctpOptions,
		      std::addressof(this->listener),
		      std::addressof(this->shared),
		      /*isDataChannel*/ true,
		      mayConnectOnReceivedSctpData)
		{
		}

		/**
		 * Advances the simulated clock of this association by `incrementMs`.
		 */
		void AdvanceTimeMs(uint64_t incrementMs)
		{
			this->nowMs += incrementMs;
		}

	public:
		uint64_t nowMs{ InitialNowMs };
		RTC::SCTP::SctpOptions sctpOptions;
		mocks::RTC::SCTP::MockAssociationListener listener;
		mocks::MockShared shared;
		RTC::SCTP::Association association;
	};

	/**
	 * Parses a previously consumed sent packet buffer into a SCTP::Packet.
	 */
	std::unique_ptr<RTC::SCTP::Packet> ParsePacket(const std::vector<uint8_t>& buffer)
	{
		return std::unique_ptr<RTC::SCTP::Packet>(RTC::SCTP::Packet::Parse(buffer.data(), buffer.size()));
	}

	/**
	 * Returns true if the packet contains a single chunk of the given type.
	 */
	template<typename ChunkType>
	bool PacketHasSingleChunkOfType(const std::vector<uint8_t>& buffer)
	{
		const auto packet = ParsePacket(buffer);

		return packet && packet->GetChunksCount() == 1 &&
		       packet->GetFirstChunkOfType<ChunkType>() != nullptr;
	}

	/**
	 * Returns true if the packet contains a DATA or I-DATA chunk.
	 */
	bool PacketHasDataChunk(const std::vector<uint8_t>& buffer)
	{
		const auto packet = ParsePacket(buffer);

		return packet && (packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>() != nullptr ||
		                  packet->GetFirstChunkOfType<RTC::SCTP::IDataChunk>() != nullptr);
	}

	/**
	 * Delivers a single sent packet from `from` to `to`. The packet must exist.
	 */
	void DeliverFirstSentPacket(AssociationUnderTest& from, AssociationUnderTest& to)
	{
		const auto packet = from.listener.ConsumeFirstSentPacket();

		REQUIRE(!packet.empty());

		to.association.ReceiveSctpData(packet.data(), packet.size());
	}

	/**
	 * Delivers all currently queued packets between `a` and `z` back and forth
	 * until neither has anything more to send.
	 */
	void ExchangeMessages(AssociationUnderTest& a, AssociationUnderTest& z)
	{
		bool deliveredPacket{ false };

		do
		{
			deliveredPacket = false;

			auto packetFromA = a.listener.ConsumeFirstSentPacket();

			if (!packetFromA.empty())
			{
				deliveredPacket = true;
				z.association.ReceiveSctpData(packetFromA.data(), packetFromA.size());
			}

			auto packetFromZ = z.listener.ConsumeFirstSentPacket();

			if (!packetFromZ.empty())
			{
				deliveredPacket = true;
				a.association.ReceiveSctpData(packetFromZ.data(), packetFromZ.size());
			}
		} while (deliveredPacket);
	}

	/**
	 * Fires every Association backoff timer that has expired, looping until none
	 * remain expired. This is the equivalent of dcsctp's RunTimers().
	 */
	void RunTimers(AssociationUnderTest& s)
	{
		bool fired{ false };

		do
		{
			fired = false;

			for (const auto label : SctpTimerLabels)
			{
				auto* timer = s.shared.GetBackoffTimer(label);

				// NOTE: We must check IsRunning() because MockBackoffTimerHandle's
				// EvaluateHasExpired() only compares times: a stopped timer whose
				// expiry time is in the past would otherwise be fired again and again.
				if (timer && timer->IsRunning() && timer->EvaluateHasExpired())
				{
					fired = true;
				}
			}
		} while (fired);
	}

	/**
	 * Advances the simulated clock of both associations by `durationMs` and fires
	 * any timer that has expired. This is the equivalent of dcsctp's AdvanceTime().
	 */
	void AdvanceTime(AssociationUnderTest& a, AssociationUnderTest& z, uint64_t durationMs)
	{
		a.AdvanceTimeMs(durationMs);
		z.AdvanceTimeMs(durationMs);

		RunTimers(a);
		RunTimers(z);
	}

	/**
	 * Calls Connect() on `a` (the active peer) and drives the handshake to
	 * completion against `z` (the passive peer). This is the equivalent of
	 * dcsctp's ConnectSockets().
	 */
	void ConnectAssociations(AssociationUnderTest& a, AssociationUnderTest& z)
	{
		a.association.Connect();
		ExchangeMessages(a, z);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
	}

	/**
	 * Enqueues a message to be sent on the given association.
	 */
	RTC::SCTP::Types::SendMessageStatus SendMessage(
	  AssociationUnderTest& s,
	  uint16_t streamId,
	  uint32_t ppid,
	  std::vector<uint8_t> payload,
	  const RTC::SCTP::SendMessageOptions& sendMessageOptions = {})
	{
		return s.association.SendMessage(
		  RTC::SCTP::Message(streamId, ppid, std::move(payload)), sendMessageOptions);
	}
} // namespace

SCENARIO("SCTP Association", "[sctp][association]")
{
	SECTION("establishes connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();
		// Z reads INIT, produces INIT-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		DeliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		DeliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(a.listener.IsConnected() == true);
		REQUIRE(z.listener.IsConnected() == true);
		REQUIRE(a.listener.HasRestarted() == false);
		REQUIRE(z.listener.HasRestarted() == false);
	}

	SECTION("establishes connection with setup collision")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();
		z.association.Connect();

		ExchangeMessages(a, z);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(a.listener.HasRestarted() == false);
		REQUIRE(z.listener.HasRestarted() == false);
	}

	SECTION("establishes simultaneous connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		// INIT isn't received by Z, as it wasn't ready yet.
		a.listener.ConsumeFirstSentPacket();

		z.association.Connect();

		// A reads INIT, produces INIT-ACK.
		DeliverFirstSentPacket(z, a);
		// Z reads INIT-ACK, sends COOKIE-ECHO.
		DeliverFirstSentPacket(a, z);
		// A reads COOKIE-ECHO, establishes connection.
		DeliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);

		// Proceed with the remaining packets.
		ExchangeMessages(a, z);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(a.listener.HasRestarted() == false);
		REQUIRE(z.listener.HasRestarted() == false);
	}

	SECTION("establishes connection with lost COOKIE-ACK")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();
		// Z reads INIT, produces INIT-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		DeliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		DeliverFirstSentPacket(a, z);
		// COOKIE-ACK is lost.
		z.listener.ConsumeFirstSentPacket();

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTING);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);

		// This will make A re-send the COOKIE-ECHO.
		AdvanceTime(a, z, a.sctpOptions.t1CookieTimeoutMs);

		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		DeliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
	}

	SECTION("resends INIT and establishes connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		// INIT is never received by Z.
		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::InitChunk>(a.listener.ConsumeFirstSentPacket()) == true);

		AdvanceTime(a, z, a.sctpOptions.t1InitTimeoutMs);

		// Z reads INIT, produces INIT-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		DeliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		DeliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
	}

	SECTION("resending INIT too many times aborts")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		// INIT is never received by Z.
		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::InitChunk>(a.listener.ConsumeFirstSentPacket()) == true);

		const auto maxInitRetransmissions = a.sctpOptions.maxInitRetransmissions.value();

		for (size_t i = 0; i < maxInitRetransmissions; ++i)
		{
			AdvanceTime(a, z, a.sctpOptions.t1InitTimeoutMs * (1u << i));

			// INIT is resent.
			REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::InitChunk>(a.listener.ConsumeFirstSentPacket()) == true);
		}

		// Another timeout, after the max init retransmits.
		AdvanceTime(a, z, a.sctpOptions.t1InitTimeoutMs * (1u << maxInitRetransmissions));

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.HasFailed() == true);
		REQUIRE(a.listener.GetFailedErrorKind() == RTC::SCTP::Types::ErrorKind::TOO_MANY_RETRIES);
	}

	SECTION("resends COOKIE-ECHO and establishes connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		// Z reads INIT, produces INIT-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		DeliverFirstSentPacket(z, a);

		// COOKIE-ECHO is never received by Z.
		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::CookieEchoChunk>(a.listener.ConsumeFirstSentPacket()) == true);

		AdvanceTime(a, z, a.sctpOptions.t1CookieTimeoutMs);

		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		DeliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
	}

	SECTION("resending COOKIE-ECHO too many times aborts")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		// Z reads INIT, produces INIT-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		DeliverFirstSentPacket(z, a);

		// COOKIE-ECHO is never received by Z.
		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::CookieEchoChunk>(a.listener.ConsumeFirstSentPacket()) == true);

		const auto maxInitRetransmissions = a.sctpOptions.maxInitRetransmissions.value();

		for (size_t i = 0; i < maxInitRetransmissions; ++i)
		{
			AdvanceTime(a, z, a.sctpOptions.t1CookieTimeoutMs * (1u << i));

			// COOKIE-ECHO is resent.
			REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::CookieEchoChunk>(a.listener.ConsumeFirstSentPacket()) == true);
		}

		// Another timeout, after the max init retransmits.
		AdvanceTime(a, z, a.sctpOptions.t1CookieTimeoutMs * (1u << maxInitRetransmissions));

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.HasFailed() == true);
		REQUIRE(a.listener.GetFailedErrorKind() == RTC::SCTP::Types::ErrorKind::TOO_MANY_RETRIES);
	}

	SECTION("shutting down while establishing connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		// Z reads INIT, produces INIT-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		DeliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		DeliverFirstSentPacket(a, z);
		// Drop COOKIE-ACK, just to more easily verify shutdown protocol.
		z.listener.ConsumeFirstSentPacket();

		// As Association A has received INIT-ACK, it has a TCB and is connected,
		// while Association Z needs to receive COOKIE-ECHO to get there. A still
		// has timers running at this point.
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTING);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);

		// Association A is now shut down, which should make it stop those timers.
		a.association.Shutdown();

		// Z reads SHUTDOWN, produces SHUTDOWN-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads SHUTDOWN-ACK, produces SHUTDOWN-COMPLETE.
		DeliverFirstSentPacket(z, a);
		// Z reads SHUTDOWN-COMPLETE.
		DeliverFirstSentPacket(a, z);

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);
		REQUIRE(z.listener.ConsumeFirstSentPacket().empty() == true);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
	}

	SECTION("shuts down connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		a.association.Shutdown();
		// Z reads SHUTDOWN, produces SHUTDOWN-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads SHUTDOWN-ACK, produces SHUTDOWN-COMPLETE.
		DeliverFirstSentPacket(z, a);
		// Z reads SHUTDOWN-COMPLETE.
		DeliverFirstSentPacket(a, z);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.IsClosed() == true);
		REQUIRE(z.listener.IsClosed() == true);
	}

	SECTION("shutdown timer expiring too many times closes connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		a.association.Shutdown();
		// Drop the first SHUTDOWN packet.
		a.listener.ConsumeFirstSentPacket();

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		const auto maxRetransmissions = a.sctpOptions.maxRetransmissions.value();

		for (size_t i = 0; i < maxRetransmissions; ++i)
		{
			AdvanceTime(a, z, a.sctpOptions.initialRtoMs * (1u << i));

			// SHUTDOWN is resent (and dropped).
			REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::ShutdownChunk>(a.listener.ConsumeFirstSentPacket()) == true);
		}

		// The last expiry makes it abort the connection.
		AdvanceTime(a, z, a.sctpOptions.initialRtoMs * (1u << maxRetransmissions));

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.IsClosed() == true);
		REQUIRE(a.listener.GetClosedErrorKind() == RTC::SCTP::Types::ErrorKind::TOO_MANY_RETRIES);
		// An ABORT chunk is sent.
		REQUIRE(
		  PacketHasSingleChunkOfType<RTC::SCTP::AbortAssociationChunk>(a.listener.ConsumeFirstSentPacket()) == true);
	}

	SECTION("T2-shutdown timer expiry resends SHUTDOWN-ACK")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		z.association.Shutdown();

		// A receives SHUTDOWN, sends SHUTDOWN-ACK. A is now in SHUTDOWN-ACK-SENT
		// state.
		DeliverFirstSentPacket(z, a);

		// Consume the SHUTDOWN-ACK sent by A (drop it).
		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::ShutdownAckChunk>(a.listener.ConsumeFirstSentPacket()) == true);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// Advance time to trigger timer expiry, which makes A resend SHUTDOWN-ACK.
		AdvanceTime(a, z, a.sctpOptions.initialRtoMs);

		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::ShutdownAckChunk>(a.listener.ConsumeFirstSentPacket()) == true);
	}

	SECTION("shutdown is ignored in SHUTDOWN-RECEIVED state")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		z.association.Shutdown();

		// A reads SHUTDOWN, produces SHUTDOWN-ACK.
		DeliverFirstSentPacket(z, a);
		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::ShutdownAckChunk>(a.listener.ConsumeFirstSentPacket()) == true);
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// Second shutdown while already shutting down, must be ignored.
		a.association.Shutdown();
		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);
	}

	SECTION("shutdown is ignored in SHUTDOWN-PENDING state")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		// Send a message that will remain outstanding (unacknowledged) which
		// transitions the association to SHUTDOWN-PENDING instead of SHUTDOWN-SENT.
		SendMessage(a, 1, 53, { 1, 2 });
		a.association.Shutdown();

		REQUIRE(PacketHasDataChunk(a.listener.ConsumeFirstSentPacket()) == true);
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// Second shutdown while already shutting down, must be ignored.
		a.association.Shutdown();
		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);
	}

	SECTION("establishes connection while sending data")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		SendMessage(a, 1, 53, { 1, 2 });

		// Z reads INIT, produces INIT-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		DeliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		DeliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);

		// Deliver the remaining packets so that the message reaches Z.
		ExchangeMessages(a, z);

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);
		REQUIRE(message->GetStreamId() == 1);
	}

	SECTION("sends a message after established")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		SendMessage(a, 1, 53, { 1, 2 });
		// Z reads the DATA chunk.
		DeliverFirstSentPacket(a, z);

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);
		REQUIRE(message->GetStreamId() == 1);
		REQUIRE(message->GetPayloadProtocolId() == 53);
	}

	SECTION("resends a packet when the T3-RTX timer expires")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		SendMessage(a, 1, 53, { 1, 2 });
		// Drop the first DATA packet.
		a.listener.ConsumeFirstSentPacket();

		// Advance time so that the T3-RTX timer expires and the DATA is resent.
		AdvanceTime(a, z, a.sctpOptions.initialRtoMs);

		// Z reads the retransmitted DATA chunk.
		DeliverFirstSentPacket(a, z);

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);
		REQUIRE(message->GetStreamId() == 1);
	}

	SECTION("sends a lot of bytes with a missed packet")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		std::vector<uint8_t> payload(a.sctpOptions.mtu * 20);

		SendMessage(a, 1, 53, payload);

		// Z reads the first DATA.
		DeliverFirstSentPacket(a, z);
		// Second DATA is lost.
		a.listener.ConsumeFirstSentPacket();

		// Retransmit and handle the rest.
		ExchangeMessages(a, z);

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);
		REQUIRE(message->GetStreamId() == 1);
		REQUIRE(message->GetPayloadLength() == payload.size());
	}

	SECTION("sends a heartbeat on idle connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);

		// Let the heartbeat interval timer expire.
		AdvanceTime(a, z, a.sctpOptions.heartbeatIntervalMs);

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(buffer) == true);

		// Feed it to Z and expect a HEARTBEAT-ACK that is propagated back to A.
		z.association.ReceiveSctpData(buffer.data(), buffer.size());
		DeliverFirstSentPacket(z, a);
	}

	SECTION("sends many messages")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		constexpr int Iterations{ 100 };

		std::vector<RTC::SCTP::Message> messages;

		messages.reserve(Iterations);

		for (int i = 0; i < Iterations; ++i)
		{
			messages.emplace_back(/*streamId*/ 1, /*ppid*/ 53, std::vector<uint8_t>{ 1, 2 });
		}

		const auto statuses = a.association.SendManyMessages(messages, /*sendMessageOptions*/ {});

		REQUIRE(statuses.size() == Iterations);

		for (const auto status : statuses)
		{
			REQUIRE(status == RTC::SCTP::Types::SendMessageStatus::SUCCESS);
		}

		ExchangeMessages(a, z);

		for (int i = 0; i < Iterations; ++i)
		{
			REQUIRE(z.listener.ConsumeFirstReceivedMessage().has_value() == true);
		}

		REQUIRE(z.listener.ConsumeFirstReceivedMessage().has_value() == false);
	}

	SECTION("initial metrics are unset")
	{
		AssociationUnderTest a;

		REQUIRE(a.association.MakeMetrics().has_value() == false);
	}

	SECTION("message interleaving metrics are set")
	{
		for (const bool aEnable : { false, true })
		{
			for (const bool zEnable : { false, true })
			{
				auto aSctpOptions                      = MakeSctpOptions();
				aSctpOptions.enableMessageInterleaving = aEnable;
				auto zSctpOptions                      = MakeSctpOptions();
				zSctpOptions.enableMessageInterleaving = zEnable;

				AssociationUnderTest a(aSctpOptions);
				AssociationUnderTest z(zSctpOptions);

				ConnectAssociations(a, z);

				REQUIRE(a.association.MakeMetrics()->usesMessageInterleaving == (aEnable && zEnable));
			}
		}
	}

	SECTION("rx and tx packet metrics increase")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		// A sent INIT and COOKIE-ECHO, received INIT-ACK and COOKIE-ACK.
		REQUIRE(a.association.MakeMetrics()->txPacketsCount == 2);
		REQUIRE(a.association.MakeMetrics()->rxPacketsCount == 2);
		REQUIRE(a.association.MakeMetrics()->txMessagesCount == 0);
		REQUIRE(
		  a.association.MakeMetrics()->cwndBytes == a.sctpOptions.initialCwndMtus * a.sctpOptions.mtu);
		REQUIRE(a.association.MakeMetrics()->unackDataCount == 0);

		REQUIRE(z.association.MakeMetrics()->rxPacketsCount == 2);
		REQUIRE(z.association.MakeMetrics()->rxMessagesCount == 0);

		SendMessage(a, 1, 53, { 1, 2 });

		REQUIRE(a.association.MakeMetrics()->unackDataCount == 1);

		// Z reads DATA.
		DeliverFirstSentPacket(a, z);
		// A reads SACK.
		DeliverFirstSentPacket(z, a);

		REQUIRE(a.association.MakeMetrics()->unackDataCount == 0);
		REQUIRE(z.listener.ConsumeFirstReceivedMessage().has_value() == true);

		REQUIRE(a.association.MakeMetrics()->txPacketsCount == 3);
		REQUIRE(a.association.MakeMetrics()->rxPacketsCount == 3);
		REQUIRE(a.association.MakeMetrics()->txMessagesCount == 1);

		REQUIRE(z.association.MakeMetrics()->rxPacketsCount == 3);
		REQUIRE(z.association.MakeMetrics()->rxMessagesCount == 1);
	}

	SECTION("streams have initial priority")
	{
		auto sctpOptions                  = MakeSctpOptions();
		sctpOptions.defaultStreamPriority = 42;

		AssociationUnderTest a(sctpOptions);

		REQUIRE(a.association.GetStreamPriority(1) == 42);

		SendMessage(a, 2, 53, { 1, 2 });

		REQUIRE(a.association.GetStreamPriority(2) == 42);
	}

	SECTION("can change stream priority")
	{
		auto sctpOptions                  = MakeSctpOptions();
		sctpOptions.defaultStreamPriority = 42;

		AssociationUnderTest a(sctpOptions);

		a.association.SetStreamPriority(1, 43);
		REQUIRE(a.association.GetStreamPriority(1) == 43);

		SendMessage(a, 2, 53, { 1, 2 });

		a.association.SetStreamPriority(2, 43);
		REQUIRE(a.association.GetStreamPriority(2) == 43);
	}

	SECTION("respects the per-stream queue limit")
	{
		auto sctpOptions                    = MakeSctpOptions();
		sctpOptions.maxSendBufferSize       = 4000;
		sctpOptions.perStreamSendQueueLimit = 1000;

		AssociationUnderTest a(sctpOptions);

		REQUIRE(
		  SendMessage(a, 1, 53, std::vector<uint8_t>(600)) == RTC::SCTP::Types::SendMessageStatus::SUCCESS);
		REQUIRE(
		  SendMessage(a, 1, 53, std::vector<uint8_t>(600)) == RTC::SCTP::Types::SendMessageStatus::SUCCESS);
		REQUIRE(
		  SendMessage(a, 1, 53, std::vector<uint8_t>(600)) ==
		  RTC::SCTP::Types::SendMessageStatus::ERROR_RESOURCE_EXHAUSTION);
		// The per-stream limit for stream 1 is reached, but not for stream 2.
		REQUIRE(
		  SendMessage(a, 2, 53, std::vector<uint8_t>(600)) == RTC::SCTP::Types::SendMessageStatus::SUCCESS);
		REQUIRE(
		  SendMessage(a, 2, 53, std::vector<uint8_t>(600)) == RTC::SCTP::Types::SendMessageStatus::SUCCESS);
		REQUIRE(
		  SendMessage(a, 2, 53, std::vector<uint8_t>(600)) ==
		  RTC::SCTP::Types::SendMessageStatus::ERROR_RESOURCE_EXHAUSTION);
	}

	SECTION("has reasonable buffered amount values")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		REQUIRE(a.association.GetStreamBufferedAmount(1) == 0);

		// Sending a small message will directly send it as a single packet, so
		// nothing is left in the queue.
		SendMessage(a, 1, 53, std::vector<uint8_t>(10));

		REQUIRE(a.association.GetStreamBufferedAmount(1) == 0);

		// Sending a large message will directly start sending a few packets, so the
		// buffered amount is not the full message size.
		const size_t largeSize = a.sctpOptions.mtu * 20;

		SendMessage(a, 1, 53, std::vector<uint8_t>(largeSize));

		REQUIRE(a.association.GetStreamBufferedAmount(1) > 0);
		REQUIRE(a.association.GetStreamBufferedAmount(1) < largeSize);
	}

	SECTION("has a default buffered amount low threshold of zero")
	{
		AssociationUnderTest a;

		REQUIRE(a.association.GetStreamBufferedAmountLowThreshold(1) == 0);
	}

	SECTION("triggers OnAssociationStreamBufferedAmountLow with default threshold zero")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		REQUIRE(a.listener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1) == false);

		ConnectAssociations(a, z);

		SendMessage(a, 1, 53, std::vector<uint8_t>(10));
		ExchangeMessages(a, z);

		REQUIRE(a.listener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1) == true);
	}

	SECTION("detects the peer implementation")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		// Both peers are mediasoup.
		REQUIRE(
		  a.association.MakeMetrics()->peerImplementation == RTC::SCTP::Types::SctpImplementation::MEDIASOUP);
		// As A initiated the connection establishment, the passive peer Z will not
		// receive enough information to know about A's implementation.
		REQUIRE(
		  z.association.MakeMetrics()->peerImplementation == RTC::SCTP::Types::SctpImplementation::UNKNOWN);
	}

	SECTION("both peers detect the peer implementation")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();
		z.association.Connect();

		ExchangeMessages(a, z);

		REQUIRE(
		  a.association.MakeMetrics()->peerImplementation == RTC::SCTP::Types::SctpImplementation::MEDIASOUP);
		REQUIRE(
		  z.association.MakeMetrics()->peerImplementation == RTC::SCTP::Types::SctpImplementation::MEDIASOUP);
	}

	SECTION("exposes the number of negotiated streams")
	{
		auto aSctpOptions                       = MakeSctpOptions();
		aSctpOptions.announcedMaxInboundStreams = 12;
		aSctpOptions.announcedMaxOutboundStreams = 45;

		auto zSctpOptions                       = MakeSctpOptions();
		zSctpOptions.announcedMaxInboundStreams = 23;
		zSctpOptions.announcedMaxOutboundStreams = 34;

		AssociationUnderTest a(aSctpOptions);
		AssociationUnderTest z(zSctpOptions);

		ConnectAssociations(a, z);

		const auto metricsA = a.association.MakeMetrics();

		REQUIRE(metricsA->negotiatedMaxInboundStreams == 12);
		REQUIRE(metricsA->negotiatedMaxOutboundStreams == 23);

		const auto metricsZ = z.association.MakeMetrics();

		REQUIRE(metricsZ->negotiatedMaxInboundStreams == 23);
		REQUIRE(metricsZ->negotiatedMaxOutboundStreams == 12);
	}

	SECTION("always sends INIT with non-zero checksum")
	{
		auto sctpOptions = MakeSctpOptions();
		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		AssociationUnderTest a(sctpOptions);

		a.association.Connect();

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::InitChunk>(buffer) == true);
		REQUIRE(ParsePacket(buffer)->GetChecksum() != 0);
	}

	SECTION("may send INIT-ACK with zero checksum")
	{
		auto sctpOptions = MakeSctpOptions();
		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		a.association.Connect();
		// Z reads INIT, produces INIT-ACK.
		DeliverFirstSentPacket(a, z);

		const auto buffer = z.listener.ConsumeFirstSentPacket();

		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::InitAckChunk>(buffer) == true);
		REQUIRE(ParsePacket(buffer)->GetChecksum() == 0);
	}

	SECTION("always sends COOKIE-ECHO with non-zero checksum")
	{
		auto sctpOptions = MakeSctpOptions();
		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		a.association.Connect();
		// Z reads INIT, produces INIT-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		DeliverFirstSentPacket(z, a);

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::CookieEchoChunk>(buffer) == true);
		REQUIRE(ParsePacket(buffer)->GetChecksum() != 0);
	}

	SECTION("sends COOKIE-ACK with zero checksum")
	{
		auto sctpOptions = MakeSctpOptions();
		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		a.association.Connect();
		// Z reads INIT, produces INIT-ACK.
		DeliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		DeliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		DeliverFirstSentPacket(a, z);

		const auto buffer = z.listener.ConsumeFirstSentPacket();

		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::CookieAckChunk>(buffer) == true);
		REQUIRE(ParsePacket(buffer)->GetChecksum() == 0);
	}

	SECTION("sends DATA with zero checksum")
	{
		auto sctpOptions = MakeSctpOptions();
		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		ConnectAssociations(a, z);

		SendMessage(a, 1, 53, std::vector<uint8_t>(a.sctpOptions.mtu - 100));

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(PacketHasDataChunk(buffer) == true);
		REQUIRE(ParsePacket(buffer)->GetChecksum() == 0);
	}

	SECTION("both sides send heartbeats")
	{
		// Make them have slightly different heartbeat intervals, to validate that
		// sending an ack by Z doesn't restart its heartbeat timer.
		auto aSctpOptions                = MakeSctpOptions();
		aSctpOptions.heartbeatIntervalMs = 1000;
		auto zSctpOptions                = MakeSctpOptions();
		zSctpOptions.heartbeatIntervalMs = 1100;

		AssociationUnderTest a(aSctpOptions);
		AssociationUnderTest z(zSctpOptions);

		ConnectAssociations(a, z);

		AdvanceTime(a, z, 1000);

		const auto bufferA = a.listener.ConsumeFirstSentPacket();

		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(bufferA) == true);
		// Z receives the heartbeat and sends an ACK that is propagated back to A.
		z.association.ReceiveSctpData(bufferA.data(), bufferA.size());
		DeliverFirstSentPacket(z, a);

		// A little while later, Z should send heartbeats to A.
		AdvanceTime(a, z, 100);

		const auto bufferZ = z.listener.ConsumeFirstSentPacket();

		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(bufferZ) == true);
		// A receives the heartbeat and sends an ACK that is propagated back to Z.
		a.association.ReceiveSctpData(bufferZ.data(), bufferZ.size());
		DeliverFirstSentPacket(a, z);
	}

	SECTION("closes connection after too many lost heartbeats")
	{
		AssociationUnderTest a;
		// Disable Z's heartbeats so that it doesn't interfere.
		auto zSctpOptions                = MakeSctpOptions();
		zSctpOptions.heartbeatIntervalMs = 0;
		AssociationUnderTest z(zSctpOptions);

		ConnectAssociations(a, z);

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);

		const auto maxRetransmissions = a.sctpOptions.maxRetransmissions.value();

		uint64_t timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs;

		for (size_t i = 0; i < maxRetransmissions; ++i)
		{
			// Let the heartbeat interval timer expire, sending a heartbeat.
			AdvanceTime(a, z, timeToNextHeartbeatMs);

			// Drop every heartbeat.
			REQUIRE(
			  PacketHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(
			    a.listener.ConsumeFirstSentPacket()) == true);

			// Let the heartbeat timeout expire.
			AdvanceTime(a, z, 1000);

			timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs - 1000;
		}

		// The last heartbeat.
		AdvanceTime(a, z, timeToNextHeartbeatMs);

		REQUIRE(a.listener.HasSentPackets() == true);
		a.listener.ConsumeFirstSentPacket();

		// Should suffice as exceeding RTO, which aborts the connection.
		AdvanceTime(a, z, 1000);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.IsClosed() == true);
		REQUIRE(a.listener.GetClosedErrorKind() == RTC::SCTP::Types::ErrorKind::TOO_MANY_RETRIES);
	}

	SECTION("recovers after a successful heartbeat ack")
	{
		AssociationUnderTest a;
		// Disable Z's heartbeats so that it doesn't interfere.
		auto zSctpOptions                = MakeSctpOptions();
		zSctpOptions.heartbeatIntervalMs = 0;
		AssociationUnderTest z(zSctpOptions);

		ConnectAssociations(a, z);

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);

		const auto maxRetransmissions = a.sctpOptions.maxRetransmissions.value();

		uint64_t timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs;

		for (size_t i = 0; i < maxRetransmissions; ++i)
		{
			AdvanceTime(a, z, timeToNextHeartbeatMs);

			// Drop every heartbeat.
			a.listener.ConsumeFirstSentPacket();

			// Let the heartbeat timeout expire.
			AdvanceTime(a, z, 1000);

			timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs - 1000;
		}

		// Get the last heartbeat and ack it (round trip through Z).
		AdvanceTime(a, z, timeToNextHeartbeatMs);

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(PacketHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(buffer) == true);

		z.association.ReceiveSctpData(buffer.data(), buffer.size());
		// A reads the HEARTBEAT-ACK, which clears the error counter.
		DeliverFirstSentPacket(z, a);

		// Should suffice as exceeding RTO, but the timer will not fire as it was
		// stopped by the ack.
		AdvanceTime(a, z, 1000);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(a.listener.IsClosed() == false);

		// Verify that we get new heartbeats again.
		AdvanceTime(a, z, timeToNextHeartbeatMs);

		REQUIRE(
		  PacketHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(
		    a.listener.ConsumeFirstSentPacket()) == true);
	}

	SECTION("resets a stream")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		ConnectAssociations(a, z);

		SendMessage(a, 1, 53, { 1, 2 });
		// Z reads the DATA chunk.
		DeliverFirstSentPacket(a, z);

		REQUIRE(z.listener.ConsumeFirstReceivedMessage().has_value() == true);

		// A reads the SACK.
		DeliverFirstSentPacket(z, a);

		// Reset the outgoing stream. This will directly send a RE-CONFIG.
		const std::vector<uint16_t> streamIds{ 1 };

		REQUIRE(a.association.ResetStreams(streamIds) == RTC::SCTP::Types::ResetStreamsStatus::PERFORMED);

		// Z reads the RE-CONFIG, which triggers OnAssociationInboundStreamsReset and
		// sends a RE-CONFIG response.
		DeliverFirstSentPacket(a, z);

		REQUIRE(z.listener.HasInboundStreamsResetForStreamId(1) == true);

		// A reads the response, which triggers OnAssociationStreamsResetPerformed.
		DeliverFirstSentPacket(z, a);

		REQUIRE(a.listener.HasStreamsResetPerformedForStreamId(1) == true);
	}
}
