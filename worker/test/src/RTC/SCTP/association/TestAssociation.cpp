#include "common.hpp"
#include "RTC/SCTP/association/Association.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/association/StateCookie.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieEchoChunk.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/chunks/OperationErrorChunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedChunkTypeErrorCause.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "mocks/include/MockShared.hpp"
#include "mocks/include/RTC/SCTP/association/MockAssociationListener.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <span>
#include <string_view>
#include <vector>

namespace
{
	// Initial value of the simulated clock (ms).
	constexpr uint64_t InitialNowMs{ 1000000 };

	// All backoff timer labels an SCTP association may create. Used by `runTimers()`
	// to fire whichever timers have expired after advancing time.
	constexpr std::array<std::string_view, 8> SctpTimerLabels{
		"sctp-t1-init",     "sctp-t1-cookie",          "sctp-t2-shutdown",       "sctp-t3-rtx",
		"sctp-delayed-ack", "sctp-heartbeat-interval", "sctp-heartbeat-timeout", "sctp-re-config",
	};

	/**
	 * Builds the default SctpOptions used in tests. Tweaked to make timers
	 * predictable.
	 */
	RTC::SCTP::SctpOptions makeSctpOptions()
	{
		RTC::SCTP::SctpOptions sctpOptions;

		// To make the heartbeat interval more predictable in tests.
		sctpOptions.heartbeatIntervalIncludeRtt = false;
		sctpOptions.maxBurst                    = 4;

		return sctpOptions;
	}

	/**
	 * An SCTP Association under test, together with its simulated clock and
	 * listener.
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
		  RTC::SCTP::SctpOptions sctpOptions = makeSctpOptions(),
		  bool mayConnectOnReceivedSctpData  = false)
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
	 * Parses a previously consumed sent packet buffer into a `SCTP::Packet`.
	 */
	std::unique_ptr<RTC::SCTP::Packet> parsePacket(const std::vector<uint8_t>& buffer)
	{
		return std::unique_ptr<RTC::SCTP::Packet>(RTC::SCTP::Packet::Parse(buffer.data(), buffer.size()));
	}

	/**
	 * Returns true if the packet contains a single chunk of the given type.
	 */
	template<typename ChunkType>
	bool packetHasSingleChunkOfType(const std::vector<uint8_t>& buffer)
	{
		const auto packet = parsePacket(buffer);

		return packet && packet->GetChunksCount() == 1 &&
		       packet->GetFirstChunkOfType<ChunkType>() != nullptr;
	}

	/**
	 * Returns true if the packet contains a DATA or I-DATA chunk.
	 */
	bool packetHasDataChunk(const std::vector<uint8_t>& buffer)
	{
		const auto packet = parsePacket(buffer);

		return packet && (packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>() != nullptr ||
		                  packet->GetFirstChunkOfType<RTC::SCTP::IDataChunk>() != nullptr);
	}

	/**
	 * Delivers a single sent packet from `from` to `to`. The packet must exist.
	 */
	void deliverFirstSentPacket(AssociationUnderTest& from, AssociationUnderTest& to)
	{
		const auto buffer = from.listener.ConsumeFirstSentPacket();

		REQUIRE(!buffer.empty());

		to.association.ReceiveSctpData(buffer.data(), buffer.size());
	}

	/**
	 * Delivers all currently queued packets between `a` and `z` back and forth
	 * until neither has anything more to send.
	 */
	void exchangeMessages(AssociationUnderTest& a, AssociationUnderTest& z)
	{
		bool deliveredPacket{ false };

		do
		{
			deliveredPacket = false;

			const auto bufferFromA = a.listener.ConsumeFirstSentPacket();

			if (!bufferFromA.empty())
			{
				deliveredPacket = true;
				z.association.ReceiveSctpData(bufferFromA.data(), bufferFromA.size());
			}

			const auto bufferFromZ = z.listener.ConsumeFirstSentPacket();

			if (!bufferFromZ.empty())
			{
				deliveredPacket = true;
				a.association.ReceiveSctpData(bufferFromZ.data(), bufferFromZ.size());
			}
		} while (deliveredPacket);
	}

	/**
	 * Fires every Association backoff timer that has expired, looping until none
	 * remain expired.
	 */
	void runTimers(AssociationUnderTest& s)
	{
		bool fired{ false };

		do
		{
			fired = false;

			for (const auto label : SctpTimerLabels)
			{
				auto* timer = s.shared.GetBackoffTimer(label);

				// NOTE: We must check `IsRunning()` because MockBackoffTimerHandle's
				// `EvaluateHasExpired()` only compares times. A stopped timer whose
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
	 * any timer that has expired.
	 */
	void advanceTimeMs(AssociationUnderTest& a, AssociationUnderTest& z, uint64_t durationMs)
	{
		a.AdvanceTimeMs(durationMs);
		z.AdvanceTimeMs(durationMs);

		runTimers(a);
		runTimers(z);
	}

	/**
	 * Calls `Connect()` on `a` (the active peer) and drives the handshake to
	 * completion against `z` (the passive peer).
	 */
	void connectAssociations(AssociationUnderTest& a, AssociationUnderTest& z)
	{
		a.association.Connect();
		exchangeMessages(a, z);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
	}

	/**
	 * Connects `a` (active) against `z` (passive) and returns `a`'s local
	 * verification tag, i.e. the tag that the peer must put in packets sent to
	 * `a`. It's captured from the COOKIE-ACK that `z` sends to `a`, and is needed
	 * to craft raw packets to be injected into `a`.
	 */
	uint32_t connectAndGetVerificationTag(AssociationUnderTest& a, AssociationUnderTest& z)
	{
		a.association.Connect();
		// Z reads INIT, produces INIT-ACK.
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);

		// Z now has a pending COOKIE-ACK destined to A; its verification tag is A's
		// local verification tag.
		const auto buffer = z.listener.ConsumeFirstSentPacket();

		REQUIRE(!buffer.empty());

		const uint32_t verificationTag = parsePacket(buffer)->GetVerificationTag();

		// Deliver the COOKIE-ACK so the connection is fully established on A.
		a.association.ReceiveSctpData(buffer.data(), buffer.size());

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);

		return verificationTag;
	}

	/**
	 * Creates an empty SCTP packet addressed to a peer, carrying the given
	 * verification tag. `buffer` must outlive the returned packet, as the packet
	 * is built in place on it.
	 */
	std::unique_ptr<RTC::SCTP::Packet> buildPacket(std::vector<uint8_t>& buffer, uint32_t verificationTag)
	{
		std::unique_ptr<RTC::SCTP::Packet> packet(
		  RTC::SCTP::Packet::Factory(buffer.data(), buffer.size()));

		packet->SetSourcePort(5000);
		packet->SetDestinationPort(5000);
		packet->SetVerificationTag(verificationTag);

		return packet;
	}

	/**
	 * Writes the packet checksum and injects it into the given association.
	 */
	void injectPacket(AssociationUnderTest& target, RTC::SCTP::Packet* packet)
	{
		packet->WriteCRC32cChecksum();

		target.association.ReceiveSctpData(packet->GetBuffer(), packet->GetLength());
	}

	/**
	 * Enqueues a message to be sent on the given association.
	 */
	RTC::SCTP::Types::SendMessageStatus sendMessage(
	  AssociationUnderTest& s,
	  uint16_t streamId,
	  uint32_t ppid,
	  std::vector<uint8_t> payload,
	  const RTC::SCTP::SendMessageOptions& sendMessageOptions = {})
	{
		return s.association.SendMessage(
		  RTC::SCTP::Message(streamId, ppid, std::move(payload)), sendMessageOptions);
	}

	/**
	 * Consumes every received message on the given association and returns their
	 * payload protocol identifiers, in the order they were received.
	 */
	std::vector<uint32_t> getReceivedMessagePpids(AssociationUnderTest& s)
	{
		std::vector<uint32_t> ppids;

		for (;;)
		{
			const auto message = s.listener.ConsumeFirstReceivedMessage();

			if (!message.has_value())
			{
				break;
			}

			ppids.push_back(message->GetPayloadProtocolId());
		}

		return ppids;
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
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		deliverFirstSentPacket(z, a);

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

		exchangeMessages(a, z);

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
		deliverFirstSentPacket(z, a);
		// Z reads INIT-ACK, sends COOKIE-ECHO.
		deliverFirstSentPacket(a, z);
		// A reads COOKIE-ECHO, establishes connection.
		deliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);

		// Proceed with the remaining packets.
		exchangeMessages(a, z);

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
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);
		// COOKIE-ACK is lost.
		z.listener.ConsumeFirstSentPacket();

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTING);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);

		// This will make A re-send the COOKIE-ECHO.
		advanceTimeMs(a, z, a.sctpOptions.t1CookieTimeoutMs);

		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		deliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
	}

	SECTION("resends INIT and establishes connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		// INIT is never received by Z.
		REQUIRE(
		  packetHasSingleChunkOfType<RTC::SCTP::InitChunk>(a.listener.ConsumeFirstSentPacket()) == true);

		advanceTimeMs(a, z, a.sctpOptions.t1InitTimeoutMs);

		// Z reads INIT, produces INIT-ACK.
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		deliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
	}

	SECTION("resending INIT too many times aborts")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		// INIT is never received by Z.
		REQUIRE(
		  packetHasSingleChunkOfType<RTC::SCTP::InitChunk>(a.listener.ConsumeFirstSentPacket()) == true);

		const auto maxInitRetransmissions = a.sctpOptions.maxInitRetransmissions.value();

		for (size_t i = 0; i < maxInitRetransmissions; ++i)
		{
			advanceTimeMs(a, z, a.sctpOptions.t1InitTimeoutMs * (1u << i));

			// INIT is resent.
			REQUIRE(
			  packetHasSingleChunkOfType<RTC::SCTP::InitChunk>(a.listener.ConsumeFirstSentPacket()) == true);
		}

		// Another timeout, after the max init retransmits.
		advanceTimeMs(a, z, a.sctpOptions.t1InitTimeoutMs * (1u << maxInitRetransmissions));

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
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);

		// COOKIE-ECHO is never received by Z.
		REQUIRE(
		  packetHasSingleChunkOfType<RTC::SCTP::CookieEchoChunk>(a.listener.ConsumeFirstSentPacket()) ==
		  true);

		advanceTimeMs(a, z, a.sctpOptions.t1CookieTimeoutMs);

		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		deliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
	}

	SECTION("resending COOKIE-ECHO too many times aborts")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		// Z reads INIT, produces INIT-ACK.
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);

		// COOKIE-ECHO is never received by Z.
		REQUIRE(
		  packetHasSingleChunkOfType<RTC::SCTP::CookieEchoChunk>(a.listener.ConsumeFirstSentPacket()) ==
		  true);

		const auto maxInitRetransmissions = a.sctpOptions.maxInitRetransmissions.value();

		for (size_t i = 0; i < maxInitRetransmissions; ++i)
		{
			advanceTimeMs(a, z, a.sctpOptions.t1CookieTimeoutMs * (1u << i));

			// COOKIE-ECHO is resent.
			REQUIRE(
			  packetHasSingleChunkOfType<RTC::SCTP::CookieEchoChunk>(a.listener.ConsumeFirstSentPacket()) ==
			  true);
		}

		// Another timeout, after the max init retransmits.
		advanceTimeMs(a, z, a.sctpOptions.t1CookieTimeoutMs * (1u << maxInitRetransmissions));

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
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);
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
		deliverFirstSentPacket(a, z);
		// A reads SHUTDOWN-ACK, produces SHUTDOWN-COMPLETE.
		deliverFirstSentPacket(z, a);
		// Z reads SHUTDOWN-COMPLETE.
		deliverFirstSentPacket(a, z);

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);
		REQUIRE(z.listener.ConsumeFirstSentPacket().empty() == true);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
	}

	SECTION("shuts down connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		a.association.Shutdown();
		// Z reads SHUTDOWN, produces SHUTDOWN-ACK.
		deliverFirstSentPacket(a, z);
		// A reads SHUTDOWN-ACK, produces SHUTDOWN-COMPLETE.
		deliverFirstSentPacket(z, a);
		// Z reads SHUTDOWN-COMPLETE.
		deliverFirstSentPacket(a, z);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.IsClosed() == true);
		REQUIRE(z.listener.IsClosed() == true);
	}

	SECTION("shutdown timer expiring too many times closes connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		a.association.Shutdown();
		// Drop the first SHUTDOWN packet.
		a.listener.ConsumeFirstSentPacket();

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		const auto maxRetransmissions = a.sctpOptions.maxRetransmissions.value();

		for (size_t i = 0; i < maxRetransmissions; ++i)
		{
			advanceTimeMs(a, z, a.sctpOptions.initialRtoMs * (1u << i));

			// SHUTDOWN is resent (and dropped).
			REQUIRE(
			  packetHasSingleChunkOfType<RTC::SCTP::ShutdownChunk>(a.listener.ConsumeFirstSentPacket()) ==
			  true);
		}

		// The last expiry makes it abort the connection.
		advanceTimeMs(a, z, a.sctpOptions.initialRtoMs * (1u << maxRetransmissions));

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.IsClosed() == true);
		REQUIRE(a.listener.GetClosedErrorKind() == RTC::SCTP::Types::ErrorKind::TOO_MANY_RETRIES);
		// An ABORT chunk is sent.
		REQUIRE(
		  packetHasSingleChunkOfType<RTC::SCTP::AbortAssociationChunk>(
		    a.listener.ConsumeFirstSentPacket()) == true);
	}

	SECTION("T2-shutdown timer expiry resends SHUTDOWN-ACK")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		z.association.Shutdown();

		// A receives SHUTDOWN, sends SHUTDOWN-ACK. A is now in SHUTDOWN-ACK-SENT
		// state.
		deliverFirstSentPacket(z, a);

		// Consume the SHUTDOWN-ACK sent by A (drop it).
		REQUIRE(
		  packetHasSingleChunkOfType<RTC::SCTP::ShutdownAckChunk>(a.listener.ConsumeFirstSentPacket()) ==
		  true);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// Advance time to trigger timer expiry, which makes A resend SHUTDOWN-ACK.
		advanceTimeMs(a, z, a.sctpOptions.initialRtoMs);

		REQUIRE(
		  packetHasSingleChunkOfType<RTC::SCTP::ShutdownAckChunk>(a.listener.ConsumeFirstSentPacket()) ==
		  true);
	}

	SECTION("shutdown is ignored in SHUTDOWN-RECEIVED state")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		z.association.Shutdown();

		// A reads SHUTDOWN, produces SHUTDOWN-ACK.
		deliverFirstSentPacket(z, a);

		REQUIRE(
		  packetHasSingleChunkOfType<RTC::SCTP::ShutdownAckChunk>(a.listener.ConsumeFirstSentPacket()) ==
		  true);
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// Second shutdown while already shutting down, must be ignored.
		a.association.Shutdown();
		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);
	}

	SECTION("shutdown is ignored in SHUTDOWN-PENDING state")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		// Send a message that will remain outstanding (unacknowledged) which
		// transitions the association to SHUTDOWN-PENDING instead of SHUTDOWN-SENT.
		sendMessage(a, 1, 53, { 1, 2 });
		a.association.Shutdown();

		REQUIRE(packetHasDataChunk(a.listener.ConsumeFirstSentPacket()) == true);
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

		sendMessage(a, 1, 53, { 1, 2 });

		// Z reads INIT, produces INIT-ACK.
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		deliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);

		// Deliver the remaining packets so that the message reaches Z.
		exchangeMessages(a, z);

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);
		REQUIRE(message->GetStreamId() == 1);
	}

	SECTION("sends a message after established")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		sendMessage(a, 1, 53, { 1, 2 });
		// Z reads the DATA chunk.
		deliverFirstSentPacket(a, z);

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);
		REQUIRE(message->GetStreamId() == 1);
		REQUIRE(message->GetPayloadProtocolId() == 53);
	}

	SECTION("resends a packet when the T3-RTX timer expires")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		sendMessage(a, 1, 53, { 1, 2 });
		// Drop the first DATA packet.
		a.listener.ConsumeFirstSentPacket();

		// Advance time so that the T3-RTX timer expires and the DATA is resent.
		advanceTimeMs(a, z, a.sctpOptions.initialRtoMs);

		// Z reads the retransmitted DATA chunk.
		deliverFirstSentPacket(a, z);

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);
		REQUIRE(message->GetStreamId() == 1);
	}

	SECTION("sends a lot of bytes with a missed packet")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		const std::vector<uint8_t> payload(a.sctpOptions.mtu * 20);

		sendMessage(a, 1, 53, payload);

		// Z reads the first DATA.
		deliverFirstSentPacket(a, z);
		// Second DATA is lost.
		a.listener.ConsumeFirstSentPacket();

		// Retransmit and handle the rest.
		exchangeMessages(a, z);

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);
		REQUIRE(message->GetStreamId() == 1);
		REQUIRE(message->GetPayloadLength() == payload.size());
	}

	SECTION("sends a heartbeat on idle connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);

		// Let the heartbeat interval timer expire.
		advanceTimeMs(a, z, a.sctpOptions.heartbeatIntervalMs);

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(buffer) == true);

		// Feed it to Z and expect a HEARTBEAT-ACK that is propagated back to A.
		z.association.ReceiveSctpData(buffer.data(), buffer.size());
		deliverFirstSentPacket(z, a);
	}

	SECTION("sends many messages")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

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

		exchangeMessages(a, z);

		for (int i = 0; i < Iterations; ++i)
		{
			REQUIRE(z.listener.ConsumeFirstReceivedMessage().has_value() == true);
		}

		REQUIRE(z.listener.ConsumeFirstReceivedMessage().has_value() == false);
	}

	SECTION("initial metrics are unset")
	{
		const AssociationUnderTest a;

		REQUIRE(a.association.MakeMetrics().has_value() == false);
	}

	SECTION("message interleaving metrics are set")
	{
		for (const bool aEnable : { false, true })
		{
			for (const bool zEnable : { false, true })
			{
				auto aSctpOptions = makeSctpOptions();

				aSctpOptions.enableMessageInterleaving = aEnable;

				auto zSctpOptions = makeSctpOptions();

				zSctpOptions.enableMessageInterleaving = zEnable;

				AssociationUnderTest a(aSctpOptions);
				AssociationUnderTest z(zSctpOptions);

				connectAssociations(a, z);

				REQUIRE(a.association.MakeMetrics()->usesMessageInterleaving == (aEnable && zEnable));
			}
		}
	}

	SECTION("rx and tx packet metrics increase")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		// A sent INIT and COOKIE-ECHO, received INIT-ACK and COOKIE-ACK.
		REQUIRE(a.association.MakeMetrics()->txPacketsCount == 2);
		REQUIRE(a.association.MakeMetrics()->rxPacketsCount == 2);
		REQUIRE(a.association.MakeMetrics()->txMessagesCount == 0);
		REQUIRE(
		  a.association.MakeMetrics()->cwndBytes == a.sctpOptions.initialCwndMtus * a.sctpOptions.mtu);
		REQUIRE(a.association.MakeMetrics()->unackDataCount == 0);

		REQUIRE(z.association.MakeMetrics()->rxPacketsCount == 2);
		REQUIRE(z.association.MakeMetrics()->rxMessagesCount == 0);

		sendMessage(a, 1, 53, { 1, 2 });

		REQUIRE(a.association.MakeMetrics()->unackDataCount == 1);

		// Z reads DATA.
		deliverFirstSentPacket(a, z);
		// A reads SACK.
		deliverFirstSentPacket(z, a);

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
		auto sctpOptions = makeSctpOptions();

		sctpOptions.defaultStreamPriority = 42;

		AssociationUnderTest a(sctpOptions);

		REQUIRE(a.association.GetStreamPriority(1) == 42);

		sendMessage(a, 2, 53, { 1, 2 });

		REQUIRE(a.association.GetStreamPriority(2) == 42);
	}

	SECTION("can change stream priority")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.defaultStreamPriority = 42;

		AssociationUnderTest a(sctpOptions);

		a.association.SetStreamPriority(1, 43);

		REQUIRE(a.association.GetStreamPriority(1) == 43);

		sendMessage(a, 2, 53, { 1, 2 });

		a.association.SetStreamPriority(2, 43);

		REQUIRE(a.association.GetStreamPriority(2) == 43);
	}

	SECTION("respects the per-stream queue limit")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.maxSendBufferSize       = 4000;
		sctpOptions.perStreamSendQueueLimit = 1000;

		AssociationUnderTest a(sctpOptions);

		REQUIRE(
		  sendMessage(a, 1, 53, std::vector<uint8_t>(600)) ==
		  RTC::SCTP::Types::SendMessageStatus::SUCCESS);
		REQUIRE(
		  sendMessage(a, 1, 53, std::vector<uint8_t>(600)) ==
		  RTC::SCTP::Types::SendMessageStatus::SUCCESS);
		REQUIRE(
		  sendMessage(a, 1, 53, std::vector<uint8_t>(600)) ==
		  RTC::SCTP::Types::SendMessageStatus::ERROR_RESOURCE_EXHAUSTION);
		// The per-stream limit for stream 1 is reached, but not for stream 2.
		REQUIRE(
		  sendMessage(a, 2, 53, std::vector<uint8_t>(600)) ==
		  RTC::SCTP::Types::SendMessageStatus::SUCCESS);
		REQUIRE(
		  sendMessage(a, 2, 53, std::vector<uint8_t>(600)) ==
		  RTC::SCTP::Types::SendMessageStatus::SUCCESS);
		REQUIRE(
		  sendMessage(a, 2, 53, std::vector<uint8_t>(600)) ==
		  RTC::SCTP::Types::SendMessageStatus::ERROR_RESOURCE_EXHAUSTION);
	}

	SECTION("has reasonable buffered amount values")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		REQUIRE(a.association.GetStreamBufferedAmount(1) == 0);

		// Sending a small message will directly send it as a single packet, so
		// nothing is left in the queue.
		sendMessage(a, 1, 53, std::vector<uint8_t>(10));

		REQUIRE(a.association.GetStreamBufferedAmount(1) == 0);

		// Sending a large message will directly start sending a few packets, so the
		// buffered amount is not the full message size.
		const size_t largeSize = a.sctpOptions.mtu * 20;

		sendMessage(a, 1, 53, std::vector<uint8_t>(largeSize));

		REQUIRE(a.association.GetStreamBufferedAmount(1) > 0);
		REQUIRE(a.association.GetStreamBufferedAmount(1) < largeSize);
	}

	SECTION("has a default buffered amount low threshold of zero")
	{
		const AssociationUnderTest a;

		REQUIRE(a.association.GetStreamBufferedAmountLowThreshold(1) == 0);
	}

	SECTION("triggers OnAssociationStreamBufferedAmountLow with default threshold zero")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		REQUIRE(a.listener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1) == false);

		connectAssociations(a, z);

		sendMessage(a, 1, 53, std::vector<uint8_t>(10));
		exchangeMessages(a, z);

		REQUIRE(a.listener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1) == true);
	}

	SECTION("detects the peer implementation")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		// Both peers are mediasoup.
		REQUIRE(
		  a.association.MakeMetrics()->peerImplementation ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);
		// As A initiated the connection establishment, the passive peer Z will not
		// receive enough information to know about A's implementation.
		REQUIRE(
		  z.association.MakeMetrics()->peerImplementation ==
		  RTC::SCTP::Types::SctpImplementation::UNKNOWN);
	}

	SECTION("both peers detect the peer implementation")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();
		z.association.Connect();

		exchangeMessages(a, z);

		REQUIRE(
		  a.association.MakeMetrics()->peerImplementation ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);
		REQUIRE(
		  z.association.MakeMetrics()->peerImplementation ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);
	}

	SECTION("exposes the number of negotiated streams")
	{
		auto aSctpOptions = makeSctpOptions();

		aSctpOptions.announcedMaxInboundStreams  = 12;
		aSctpOptions.announcedMaxOutboundStreams = 45;

		auto zSctpOptions = makeSctpOptions();

		zSctpOptions.announcedMaxInboundStreams  = 23;
		zSctpOptions.announcedMaxOutboundStreams = 34;

		AssociationUnderTest a(aSctpOptions);
		AssociationUnderTest z(zSctpOptions);

		connectAssociations(a, z);

		const auto metricsA = a.association.MakeMetrics();

		REQUIRE(metricsA->negotiatedMaxInboundStreams == 12);
		REQUIRE(metricsA->negotiatedMaxOutboundStreams == 23);

		const auto metricsZ = z.association.MakeMetrics();

		REQUIRE(metricsZ->negotiatedMaxInboundStreams == 23);
		REQUIRE(metricsZ->negotiatedMaxOutboundStreams == 12);
	}

	SECTION("always sends INIT with non-zero checksum")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		AssociationUnderTest a(sctpOptions);

		a.association.Connect();

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType<RTC::SCTP::InitChunk>(buffer) == true);
		REQUIRE(parsePacket(buffer)->GetChecksum() != 0);
	}

	SECTION("may send INIT-ACK with zero checksum")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		a.association.Connect();
		// Z reads INIT, produces INIT-ACK.
		deliverFirstSentPacket(a, z);

		const auto buffer = z.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType<RTC::SCTP::InitAckChunk>(buffer) == true);
		REQUIRE(parsePacket(buffer)->GetChecksum() == 0);
	}

	SECTION("always sends COOKIE-ECHO with non-zero checksum")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		a.association.Connect();
		// Z reads INIT, produces INIT-ACK.
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType<RTC::SCTP::CookieEchoChunk>(buffer) == true);
		REQUIRE(parsePacket(buffer)->GetChecksum() != 0);
	}

	SECTION("sends COOKIE-ACK with zero checksum")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		a.association.Connect();
		// Z reads INIT, produces INIT-ACK.
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);

		const auto buffer = z.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType<RTC::SCTP::CookieAckChunk>(buffer) == true);
		REQUIRE(parsePacket(buffer)->GetChecksum() == 0);
	}

	SECTION("sends DATA with zero checksum")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		connectAssociations(a, z);

		sendMessage(a, 1, 53, std::vector<uint8_t>(a.sctpOptions.mtu - 100));

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasDataChunk(buffer) == true);
		REQUIRE(parsePacket(buffer)->GetChecksum() == 0);
	}

	SECTION("both sides send heartbeats")
	{
		// Make them have slightly different heartbeat intervals, to validate that
		// sending an ack by Z doesn't restart its heartbeat timer.
		auto aSctpOptions = makeSctpOptions();

		aSctpOptions.heartbeatIntervalMs = 1000;

		auto zSctpOptions = makeSctpOptions();

		zSctpOptions.heartbeatIntervalMs = 1100;

		AssociationUnderTest a(aSctpOptions);
		AssociationUnderTest z(zSctpOptions);

		connectAssociations(a, z);

		advanceTimeMs(a, z, 1000);

		const auto bufferA = a.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(bufferA) == true);

		// Z receives the heartbeat and sends an ACK that is propagated back to A.
		z.association.ReceiveSctpData(bufferA.data(), bufferA.size());
		deliverFirstSentPacket(z, a);

		// A little while later, Z should send heartbeats to A.
		advanceTimeMs(a, z, 100);

		const auto bufferZ = z.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(bufferZ) == true);

		// A receives the heartbeat and sends an ACK that is propagated back to Z.
		a.association.ReceiveSctpData(bufferZ.data(), bufferZ.size());
		deliverFirstSentPacket(a, z);
	}

	SECTION("closes connection after too many lost heartbeats")
	{
		AssociationUnderTest a;
		// Disable Z's heartbeats so that it doesn't interfere.
		auto zSctpOptions = makeSctpOptions();

		zSctpOptions.heartbeatIntervalMs = 0;

		AssociationUnderTest z(zSctpOptions);

		connectAssociations(a, z);

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);

		const auto maxRetransmissions = a.sctpOptions.maxRetransmissions.value();

		uint64_t timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs;

		for (size_t i = 0; i < maxRetransmissions; ++i)
		{
			// Let the heartbeat interval timer expire, sending a heartbeat.
			advanceTimeMs(a, z, timeToNextHeartbeatMs);

			// Drop every heartbeat.
			REQUIRE(
			  packetHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(
			    a.listener.ConsumeFirstSentPacket()) == true);

			// Let the heartbeat timeout expire.
			advanceTimeMs(a, z, 1000);

			timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs - 1000;
		}

		// The last heartbeat.
		advanceTimeMs(a, z, timeToNextHeartbeatMs);

		REQUIRE(a.listener.HasSentPackets() == true);

		a.listener.ConsumeFirstSentPacket();

		// Should suffice as exceeding RTO, which aborts the connection.
		advanceTimeMs(a, z, 1000);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.IsClosed() == true);
		REQUIRE(a.listener.GetClosedErrorKind() == RTC::SCTP::Types::ErrorKind::TOO_MANY_RETRIES);
	}

	SECTION("recovers after a successful heartbeat ack")
	{
		AssociationUnderTest a;
		// Disable Z's heartbeats so that it doesn't interfere.
		auto zSctpOptions = makeSctpOptions();

		zSctpOptions.heartbeatIntervalMs = 0;

		AssociationUnderTest z(zSctpOptions);

		connectAssociations(a, z);

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);

		const auto maxRetransmissions = a.sctpOptions.maxRetransmissions.value();

		uint64_t timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs;

		for (size_t i = 0; i < maxRetransmissions; ++i)
		{
			advanceTimeMs(a, z, timeToNextHeartbeatMs);

			// Drop every heartbeat.
			a.listener.ConsumeFirstSentPacket();

			// Let the heartbeat timeout expire.
			advanceTimeMs(a, z, 1000);

			timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs - 1000;
		}

		// Get the last heartbeat and ack it (round trip through Z).
		advanceTimeMs(a, z, timeToNextHeartbeatMs);

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(buffer) == true);

		z.association.ReceiveSctpData(buffer.data(), buffer.size());
		// A reads the HEARTBEAT-ACK, which clears the error counter.
		deliverFirstSentPacket(z, a);

		// Should suffice as exceeding RTO, but the timer will not fire as it was
		// stopped by the ack.
		advanceTimeMs(a, z, 1000);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(a.listener.IsClosed() == false);

		// Verify that we get new heartbeats again.
		advanceTimeMs(a, z, timeToNextHeartbeatMs);

		REQUIRE(
		  packetHasSingleChunkOfType<RTC::SCTP::HeartbeatRequestChunk>(
		    a.listener.ConsumeFirstSentPacket()) == true);
	}

	SECTION("resets a stream")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		sendMessage(a, 1, 53, { 1, 2 });
		// Z reads the DATA chunk.
		deliverFirstSentPacket(a, z);

		REQUIRE(z.listener.ConsumeFirstReceivedMessage().has_value() == true);

		// A reads the SACK.
		deliverFirstSentPacket(z, a);

		// Reset the outgoing stream. This will directly send a RE-CONFIG.
		const std::vector<uint16_t> streamIds{ 1 };

		REQUIRE(a.association.ResetStreams(streamIds) == RTC::SCTP::Types::ResetStreamsStatus::PERFORMED);

		// Z reads the RE-CONFIG, which triggers OnAssociationInboundStreamsReset and
		// sends a RE-CONFIG response.
		deliverFirstSentPacket(a, z);

		REQUIRE(z.listener.HasInboundStreamsResetForStreamId(1) == true);

		// A reads the response, which triggers OnAssociationStreamsResetPerformed.
		deliverFirstSentPacket(z, a);

		REQUIRE(a.listener.HasStreamsResetPerformedForStreamId(1) == true);
	}

	SECTION("small messages with priority arrive in a specific order")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.SetStreamPriority(1, 700);
		a.association.SetStreamPriority(2, 200);
		a.association.SetStreamPriority(3, 100);

		// Enqueue messages before connecting, to ensure they aren't sent as soon as
		// sendMessage() is called.
		sendMessage(a, 3, 301, std::vector<uint8_t>(10));
		sendMessage(a, 1, 101, std::vector<uint8_t>(10));
		sendMessage(a, 2, 201, std::vector<uint8_t>(10));
		sendMessage(a, 1, 102, std::vector<uint8_t>(10));
		sendMessage(a, 1, 103, std::vector<uint8_t>(10));

		connectAssociations(a, z);
		exchangeMessages(a, z);

		REQUIRE(getReceivedMessagePpids(z) == std::vector<uint32_t>{ 101, 102, 103, 201, 301 });
	}

	SECTION("large messages with priority arrive in a specific order")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		const size_t largeSize = a.sctpOptions.mtu * 20;

		a.association.SetStreamPriority(1, 700);
		a.association.SetStreamPriority(2, 200);
		a.association.SetStreamPriority(3, 100);

		// Enqueue messages before connecting, to ensure they aren't sent as soon as
		// sendMessage() is called.
		sendMessage(a, 3, 301, std::vector<uint8_t>(largeSize));
		sendMessage(a, 1, 101, std::vector<uint8_t>(largeSize));
		sendMessage(a, 2, 201, std::vector<uint8_t>(largeSize));
		sendMessage(a, 1, 102, std::vector<uint8_t>(largeSize));

		connectAssociations(a, z);
		exchangeMessages(a, z);

		REQUIRE(getReceivedMessagePpids(z) == std::vector<uint32_t>{ 101, 102, 201, 301 });
	}

	SECTION("a higher priority message interrupts a lower priority message")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		const size_t largeSize = a.sctpOptions.mtu * 20;

		a.association.SetStreamPriority(2, 128);
		sendMessage(a, 2, 201, std::vector<uint8_t>(largeSize));

		// Due to a non-zero initial congestion window, the message will already
		// start to send, but will not be sent completely. Now enqueue two messages
		// on a higher priority stream: one small and one large.
		a.association.SetStreamPriority(1, 512);
		sendMessage(a, 1, 101, std::vector<uint8_t>(10));
		sendMessage(a, 1, 102, std::vector<uint8_t>(largeSize));

		exchangeMessages(a, z);

		REQUIRE(getReceivedMessagePpids(z) == std::vector<uint32_t>{ 101, 102, 201 });
	}

	SECTION("lifecycle events are generated for acked messages")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		const size_t largeSize = a.sctpOptions.mtu * 20;

		sendMessage(a, 2, 101, std::vector<uint8_t>(largeSize), { .lifecycleId = 41 });
		sendMessage(a, 2, 102, std::vector<uint8_t>(largeSize));
		sendMessage(a, 2, 103, std::vector<uint8_t>(largeSize), { .lifecycleId = 42 });

		exchangeMessages(a, z);
		// In case of delayed ack.
		advanceTimeMs(a, z, a.sctpOptions.delayedAckMaxTimeoutMs);
		exchangeMessages(a, z);

		REQUIRE(a.listener.HasOnAssociationLifecycleMessageDeliveredBeenCalledWithLifecycleId(41) == true);
		REQUIRE(a.listener.HasOnAssociationLifecycleMessageEndBeenCalledWithLifecycleId(41) == true);
		REQUIRE(a.listener.HasOnAssociationLifecycleMessageDeliveredBeenCalledWithLifecycleId(42) == true);
		REQUIRE(a.listener.HasOnAssociationLifecycleMessageEndBeenCalledWithLifecycleId(42) == true);

		REQUIRE(getReceivedMessagePpids(z) == std::vector<uint32_t>{ 101, 102, 103 });
	}

	SECTION("lifecycle events for an expired message with a lifetime limit")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		// Send it before the association is connected, to prevent it from being
		// sent too quickly. The idea is that it should be expired before even
		// attempting to send it in full.
		sendMessage(a, 1, 51, std::vector<uint8_t>(10), { .lifetimeMs = 100, .lifecycleId = 1 });

		advanceTimeMs(a, z, 200);

		connectAssociations(a, z);
		exchangeMessages(a, z);

		REQUIRE(
		  a.listener.HasOnAssociationLifecycleMessageExpiredSentBeenCalledWithLifecycleId(
		    1, /*maybeDelivered*/ false) == true);
		REQUIRE(a.listener.HasOnAssociationLifecycleMessageEndBeenCalledWithLifecycleId(1) == true);

		REQUIRE(getReceivedMessagePpids(z).empty() == true);
	}

	SECTION("responds with an error to an unknown chunk")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		const uint32_t verificationTag = connectAndGetVerificationTag(a, z);

		std::vector<uint8_t> buffer(a.sctpOptions.mtu);

		const auto packet = buildPacket(buffer, verificationTag);

		// Build a 4-byte chunk and patch its type to an unknown value (0x49), whose
		// top two bits (01) request the receiver to skip it and report an error.
		auto* cookieAckChunk = packet->BuildChunkInPlace<RTC::SCTP::CookieAckChunk>();

		cookieAckChunk->Consolidate();

		const_cast<uint8_t*>(packet->GetBuffer())[12] = 0x49;

		injectPacket(a, packet.get());

		// A replies with an OPERATION-ERROR chunk carrying an Unrecognized Chunk
		// Type error cause.
		const auto buffer2     = a.listener.ConsumeFirstSentPacket();
		const auto replyPacket = parsePacket(buffer2);

		REQUIRE(replyPacket);

		const auto* operationErrorChunk =
		  replyPacket->GetFirstChunkOfType<RTC::SCTP::OperationErrorChunk>();

		REQUIRE(operationErrorChunk);

		const auto* errorCause = operationErrorChunk->GetErrorCauseAt(0);

		REQUIRE(errorCause);
		REQUIRE(errorCause->GetCode() == RTC::SCTP::ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE);
	}

	SECTION("reports a received error chunk as a callback")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		const uint32_t verificationTag = connectAndGetVerificationTag(a, z);

		std::vector<uint8_t> buffer(a.sctpOptions.mtu);

		const auto packet = buildPacket(buffer, verificationTag);

		auto* operationErrorChunk = packet->BuildChunkInPlace<RTC::SCTP::OperationErrorChunk>();
		auto* unrecognizedChunkTypeErrorCause =
		  operationErrorChunk->BuildErrorCauseInPlace<RTC::SCTP::UnrecognizedChunkTypeErrorCause>();

		const uint8_t unknownChunk[]{ 0x49, 0x00, 0x00, 0x04 };

		unrecognizedChunkTypeErrorCause->SetUnrecognizedChunk(unknownChunk, sizeof(unknownChunk));
		unrecognizedChunkTypeErrorCause->Consolidate();
		operationErrorChunk->Consolidate();

		injectPacket(a, packet.get());

		REQUIRE(a.listener.HasErrored() == true);
		REQUIRE(a.listener.GetErroredErrorKind() == RTC::SCTP::Types::ErrorKind::PEER_REPORTED);
	}

	SECTION("answers a heartbeat request with an ack")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		const uint32_t verificationTag = connectAndGetVerificationTag(a, z);

		std::vector<uint8_t> buffer(a.sctpOptions.mtu);

		const auto packet = buildPacket(buffer, verificationTag);

		auto* heartbeatRequestChunk = packet->BuildChunkInPlace<RTC::SCTP::HeartbeatRequestChunk>();
		auto* heartbeatInfoParameter =
		  heartbeatRequestChunk->BuildParameterInPlace<RTC::SCTP::HeartbeatInfoParameter>();

		const uint8_t info[]{ 1, 2, 3, 4 };

		heartbeatInfoParameter->SetInfo(info, sizeof(info));
		heartbeatInfoParameter->Consolidate();
		heartbeatRequestChunk->Consolidate();

		injectPacket(a, packet.get());

		// A replies with a HEARTBEAT-ACK chunk echoing the Heartbeat Info.
		const auto buffer2     = a.listener.ConsumeFirstSentPacket();
		const auto replyPacket = parsePacket(buffer2);

		REQUIRE(replyPacket);

		const auto* heartbeatAckChunk = replyPacket->GetFirstChunkOfType<RTC::SCTP::HeartbeatAckChunk>();

		REQUIRE(heartbeatAckChunk);

		const auto* replyInfoParameter =
		  heartbeatAckChunk->GetFirstParameterOfType<RTC::SCTP::HeartbeatInfoParameter>();

		REQUIRE(replyInfoParameter);
		REQUIRE(replyInfoParameter->HasInfo() == true);
	}

	SECTION("sends a message with limited retransmissions")
	{
		// Disable message interleaving so that ordered DATA chunks (with SSN) are
		// used.
		auto sctpOptions = makeSctpOptions();

		sctpOptions.enableMessageInterleaving = false;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		connectAssociations(a, z);

		RTC::SCTP::SendMessageOptions sendMessageOptions;

		sendMessageOptions.maxRetransmissions = 0;

		const std::vector<uint8_t> payload(a.sctpOptions.mtu - 100);

		sendMessage(a, 1, 51, payload, sendMessageOptions);
		sendMessage(a, 1, 52, payload, sendMessageOptions);
		sendMessage(a, 1, 53, payload, sendMessageOptions);

		// Z reads the first DATA.
		deliverFirstSentPacket(a, z);
		// Second DATA is lost.
		a.listener.ConsumeFirstSentPacket();
		// Z reads the third DATA.
		deliverFirstSentPacket(a, z);

		// Let everything settle: the missing chunk will be nacked, and only after
		// the t3-rtx timer expires will it be marked for retransmission and then,
		// since it has no retransmissions left, abandoned (triggering a FORWARD-TSN).
		// We exchange packets and advance timers (delayed ack and RTO) until there's
		// nothing more to do, instead of asserting the exact intermediate sequence.
		exchangeMessages(a, z);
		advanceTimeMs(a, z, a.sctpOptions.delayedAckMaxTimeoutMs);
		exchangeMessages(a, z);
		advanceTimeMs(a, z, a.sctpOptions.initialRtoMs);
		exchangeMessages(a, z);
		advanceTimeMs(a, z, a.sctpOptions.delayedAckMaxTimeoutMs);
		exchangeMessages(a, z);

		// The first and third messages are delivered; the second one is abandoned.
		REQUIRE(getReceivedMessagePpids(z) == std::vector<uint32_t>{ 51, 53 });
	}

	SECTION("lifecycle events for fail on max retransmissions")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.enableMessageInterleaving = false;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		connectAssociations(a, z);

		const std::vector<uint8_t> payload(a.sctpOptions.mtu - 100);

		sendMessage(a, 1, 51, payload, { .maxRetransmissions = 0, .lifecycleId = 1 });
		sendMessage(a, 1, 52, payload, { .maxRetransmissions = 0, .lifecycleId = 2 });
		sendMessage(a, 1, 53, payload, { .maxRetransmissions = 0, .lifecycleId = 3 });

		// Z reads the first DATA.
		deliverFirstSentPacket(a, z);
		// Second DATA is lost.
		a.listener.ConsumeFirstSentPacket();

		exchangeMessages(a, z);

		// Handle the delayed SACK.
		advanceTimeMs(a, z, a.sctpOptions.delayedAckMaxTimeoutMs);
		exchangeMessages(a, z);

		// The chunk is now nacked. Let the RTO expire to discard the message.
		advanceTimeMs(a, z, a.sctpOptions.initialRtoMs);
		exchangeMessages(a, z);

		// Handle the delayed SACK.
		advanceTimeMs(a, z, a.sctpOptions.delayedAckMaxTimeoutMs);
		exchangeMessages(a, z);

		REQUIRE(a.listener.HasOnAssociationLifecycleMessageDeliveredBeenCalledWithLifecycleId(1) == true);
		REQUIRE(a.listener.HasOnAssociationLifecycleMessageEndBeenCalledWithLifecycleId(1) == true);
		REQUIRE(
		  a.listener.HasOnAssociationLifecycleMessageExpiredSentBeenCalledWithLifecycleId(
		    2, /*maybeDelivered*/ true) == true);
		REQUIRE(a.listener.HasOnAssociationLifecycleMessageEndBeenCalledWithLifecycleId(2) == true);
		REQUIRE(a.listener.HasOnAssociationLifecycleMessageDeliveredBeenCalledWithLifecycleId(3) == true);
		REQUIRE(a.listener.HasOnAssociationLifecycleMessageEndBeenCalledWithLifecycleId(3) == true);

		REQUIRE(getReceivedMessagePpids(z) == std::vector<uint32_t>{ 51, 53 });
	}

	SECTION("lifecycle events for an expired message with a retransmit limit")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.enableMessageInterleaving = false;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		connectAssociations(a, z);

		// Will not be able to send it in full within the congestion window, so it
		// will need SACKs to be received for more fragments to be sent.
		const std::vector<uint8_t> payload(a.sctpOptions.mtu * 20);

		sendMessage(a, 1, 51, payload, { .maxRetransmissions = 0, .lifecycleId = 1 });

		// Z reads the first DATA.
		deliverFirstSentPacket(a, z);
		// Second DATA is lost.
		a.listener.ConsumeFirstSentPacket();

		exchangeMessages(a, z);

		REQUIRE(
		  a.listener.HasOnAssociationLifecycleMessageExpiredSentBeenCalledWithLifecycleId(
		    1, /*maybeDelivered*/ false) == true);
		REQUIRE(a.listener.HasOnAssociationLifecycleMessageEndBeenCalledWithLifecycleId(1) == true);

		REQUIRE(getReceivedMessagePpids(z).empty() == true);
	}

	SECTION("establishes connection with State Cookie authentication enabled")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.requireAuthenticatedCookie = true;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		// The full handshake must succeed: Z generates an authenticated cookie that
		// A echoes back and Z verifies against its own secret.
		connectAssociations(a, z);
	}

	SECTION("forged COOKIE-ECHO is accepted without authentication but rejected with it")
	{
		// Forge a plain (unauthenticated) State Cookie with attacker-chosen values,
		// as in the published PoC. It passes the magic checks but carries no MAC.
		const uint32_t forgedTag = 0xDEADBEEF;
		const RTC::SCTP::NegotiatedCapabilities negotiatedCapabilities;

		std::vector<uint8_t> cookieBuffer(RTC::SCTP::StateCookie::StateCookieLength);

		RTC::SCTP::StateCookie::Write(
		  cookieBuffer.data(),
		  cookieBuffer.size(),
		  /*localVerificationTag*/ forgedTag,
		  /*remoteVerificationTag*/ 0xCAFEBABE,
		  /*localInitialTsn*/ 1000,
		  /*remoteInitialTsn*/ 2000,
		  /*remoteAdvertisedReceiverWindowCredit*/ 65535,
		  /*tieTag*/ 0,
		  negotiatedCapabilities);

		// Builds a COOKIE-ECHO packet carrying the forged cookie. `buffers` must
		// outlive the returned packet.
		const auto buildForgedCookieEcho =
		  [&](std::vector<uint8_t>& packetBuffer) -> std::unique_ptr<RTC::SCTP::Packet>
		{
			auto packet           = buildPacket(packetBuffer, forgedTag);
			auto* cookieEchoChunk = packet->BuildChunkInPlace<RTC::SCTP::CookieEchoChunk>();

			cookieEchoChunk->SetCookie(cookieBuffer.data(), cookieBuffer.size());
			cookieEchoChunk->Consolidate();

			return packet;
		};

		// Without authentication, the forged COOKIE-ECHO is accepted and an
		// association is wrongly established (this is the vulnerability).
		{
			AssociationUnderTest z;

			std::vector<uint8_t> packetBuffer(1500);

			const auto packet = buildForgedCookieEcho(packetBuffer);

			injectPacket(z, packet.get());

			REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		}

		// With authentication required, the forged COOKIE-ECHO fails MAC
		// verification and is silently discarded.
		{
			auto sctpOptions = makeSctpOptions();

			sctpOptions.requireAuthenticatedCookie = true;

			AssociationUnderTest z(sctpOptions);

			std::vector<uint8_t> packetBuffer(1500);

			const auto packet = buildForgedCookieEcho(packetBuffer);

			injectPacket(z, packet.get());

			REQUIRE(z.association.GetAssociationState() != RTC::SCTP::Types::AssociationState::CONNECTED);
			// No COOKIE-ACK (nor any other packet) is sent in response.
			REQUIRE(z.listener.ConsumeFirstSentPacket().empty() == true);
		}
	}

	SECTION("rejects a stale COOKIE-ECHO when authentication is required")
	{
		auto sctpOptions = makeSctpOptions();

		sctpOptions.requireAuthenticatedCookie = true;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		a.association.Connect();
		// Z reads INIT, produces INIT-ACK with an authenticated cookie timestamped
		// with Z's current time.
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO (echoing Z's cookie).
		deliverFirstSentPacket(z, a);

		// Make the cookie stale on Z's side by advancing its clock beyond the cookie
		// lifespan before the COOKIE-ECHO is delivered.
		z.AdvanceTimeMs(RTC::SCTP::StateCookie::ValidCookieLifeMs + 1000);

		// Z reads the now stale COOKIE-ECHO.
		deliverFirstSentPacket(a, z);

		REQUIRE(z.association.GetAssociationState() != RTC::SCTP::Types::AssociationState::CONNECTED);
		// Z responds with an ERROR chunk (carrying a Stale Cookie error cause).
		REQUIRE(
		  packetHasSingleChunkOfType<RTC::SCTP::OperationErrorChunk>(
		    z.listener.ConsumeFirstSentPacket()) == true);
	}
}
