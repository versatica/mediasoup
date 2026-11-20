#include "common.hpp"
#include "RTC/SCTP/association/Association.hpp"
#include "RTC/SCTP/association/Capabilities.hpp"
#include "RTC/SCTP/association/StateCookie.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieEchoChunk.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/chunks/OperationErrorChunk.hpp"
#include "RTC/SCTP/packet/errorCauses/StaleCookieErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedChunkTypeErrorCause.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "RTC/SCTP/packet/parameters/StateCookieParameter.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "RTC/SCTP/rx/ReassemblyQueue.hpp"
#include "Utils.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include "test/include/testHelpers.hpp"
#include "mocks/include/MockShared.hpp"
#include "mocks/include/RTC/SCTP/association/MockAssociationListener.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <span>
#include <string_view>
#include <vector>

SCENARIO("SCTP Association", "[sctp][association]")
{
	// Initial value of the simulated clock (us). It's `static` because the local
	// class below uses it.
	static constexpr int64_t InitialNowUs{ 1000000 * 1000 };
	constexpr size_t SmallMessageSize{ 10 };

	// All backoff timer labels an SCTP association may create. Used by `runTimers()`
	// to fire whichever timers have expired after advancing time. It's `static`
	// because the lambdas below use it without capturing it.
	static constexpr std::array<std::string_view, 8> SctpTimerLabels{
		"sctp-t1-init",     "sctp-t1-cookie",          "sctp-t2-shutdown",       "sctp-t3-rtx",
		"sctp-delayed-ack", "sctp-heartbeat-interval", "sctp-heartbeat-timeout", "sctp-re-config"
	};

	/**
	 * Builds the default SctpOptions used in tests. Tweaked to make timers
	 * predictable. It's `static` because the local class below uses it.
	 */
	static const auto MakeSctpOptions = []() -> RTC::SCTP::SctpOptions
	{
		RTC::SCTP::SctpOptions sctpOptions;

		// To make the heartbeat interval more predictable in tests.
		sctpOptions.heartbeatIntervalIncludeRtt = false;
		sctpOptions.maxBurst                    = 4;

		return sctpOptions;
	};

	/**
	 * An SCTP Association under test, together with its simulated clock and
	 * listener.
	 */
	class AssociationUnderTest
	{
	public:
		AssociationUnderTest() : AssociationUnderTest(MakeSctpOptions())
		{
		}

		/**
		 * `mayConnectOnReceivedSctpData` defaults to false so that, just like
		 * dcsctp, the connection is only initiated by explicitly calling `Connect()`.
		 * In production mediasoup associations auto-initiate the connection upon
		 * receiving data (both peers are active), but for tests we want to mimic
		 * dcsctp's asymmetric handshake (one active peer, one passive peer) so that
		 * packet sequences and counts match.
		 */
		explicit AssociationUnderTest(
		  RTC::SCTP::SctpOptions sctpOptions, bool mayConnectOnReceivedSctpData = false)
		  // NOTE: The order in which these members are initialized is **critical**.
		  : sctpOptions(sctpOptions),
		    shared(/*getTimeUs*/
				       [this]() -> int64_t
				       {
			           return this->nowUs;
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
		 *
		 * @remarks
		 * - The increment is given in milliseconds since it comes from the SCTP
		 *   options and the timers, which work in milliseconds.
		 */
		void AdvanceNowMs(int64_t incrementMs)
		{
			this->nowUs += incrementMs * 1000;
		}

	public:
		int64_t nowUs{ InitialNowUs };
		const RTC::SCTP::SctpOptions sctpOptions;
		mocks::RTC::SCTP::MockAssociationListener listener;
		mocks::MockShared shared;
		RTC::SCTP::Association association;
	};

	/**
	 * Parses a previously consumed sent packet buffer into a `SCTP::Packet`.
	 */
	const auto parsePacket = [](const std::vector<uint8_t>& buffer) -> std::unique_ptr<RTC::SCTP::Packet>
	{
		return std::unique_ptr<RTC::SCTP::Packet>(RTC::SCTP::Packet::Parse(buffer.data(), buffer.size()));
	};

	/**
	 * Returns true if the packet contains a single chunk of the given type.
	 */
	const auto packetHasSingleChunkOfType =
	  [&parsePacket](RTC::SCTP::Chunk::ChunkType chunkType, const std::vector<uint8_t>& buffer) -> bool
	{
		const auto packet = parsePacket(buffer);

		return packet && packet->GetChunksCount() == 1 && packet->GetChunkAt(0)->GetType() == chunkType;
	};

	/**
	 * Returns true if the packet contains a DATA or I-DATA chunk.
	 */
	const auto packetHasDataChunk = [&parsePacket](const std::vector<uint8_t>& buffer) -> bool
	{
		const auto packet = parsePacket(buffer);

		return packet && (packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>() != nullptr ||
		                  packet->GetFirstChunkOfType<RTC::SCTP::IDataChunk>() != nullptr);
	};

	/**
	 * Delivers a single sent packet from `from` to `to`. The packet must exist.
	 */
	const auto deliverFirstSentPacket = [](AssociationUnderTest& from, AssociationUnderTest& to) -> void
	{
		const auto buffer = from.listener.ConsumeFirstSentPacket();

		REQUIRE(!buffer.empty());

		to.association.ReceiveSctpData(buffer.data(), buffer.size(), to.shared.GetTimeUs());
	};

	/**
	 * Delivers all currently queued packets between `a` and `z` back and forth
	 * until neither has anything more to send.
	 */
	const auto exchangeMessages = [](AssociationUnderTest& a, AssociationUnderTest& z) -> void
	{
		bool deliveredPacket{ false };

		do
		{
			deliveredPacket = false;

			const auto bufferFromA = a.listener.ConsumeFirstSentPacket();

			if (!bufferFromA.empty())
			{
				deliveredPacket = true;
				z.association.ReceiveSctpData(bufferFromA.data(), bufferFromA.size(), z.shared.GetTimeUs());
			}

			const auto bufferFromZ = z.listener.ConsumeFirstSentPacket();

			if (!bufferFromZ.empty())
			{
				deliveredPacket = true;
				a.association.ReceiveSctpData(bufferFromZ.data(), bufferFromZ.size(), a.shared.GetTimeUs());
			}
		} while (deliveredPacket);
	};

	/**
	 * Fires every Association backoff timer that has expired, looping until none
	 * remain expired.
	 */
	const auto runTimers = [](const AssociationUnderTest& s) -> void
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
	};

	/**
	 * Advances the simulated clock of both associations by `durationMs` and fires
	 * any timer that has expired.
	 */
	const auto advanceNowMs =
	  [&runTimers](AssociationUnderTest& a, AssociationUnderTest& z, int64_t incrementMs) -> void
	{
		a.AdvanceNowMs(incrementMs);
		z.AdvanceNowMs(incrementMs);

		runTimers(a);
		runTimers(z);
	};

	/**
	 * Returns the milliseconds left until the next running backoff timer of `s`
	 * expires, or `std::nullopt` if `s` has no running timer.
	 */
	const auto getTimeToNextTimeoutMs = [](AssociationUnderTest& s) -> std::optional<int64_t>
	{
		std::optional<int64_t> timeToNextTimeoutMs;

		for (const auto label : SctpTimerLabels)
		{
			const auto* timer = s.shared.GetBackoffTimer(label);

			if (!timer || !timer->IsRunning())
			{
				continue;
			}

			const int64_t timeToTimeoutMs = timer->GetExpiresAtMs() - s.shared.GetTimeMs();

			// A timer whose expiry is already in the past is not a pending timeout.
			if (timeToTimeoutMs < 0)
			{
				continue;
			}

			if (!timeToNextTimeoutMs.has_value() || timeToTimeoutMs < timeToNextTimeoutMs.value())
			{
				timeToNextTimeoutMs = timeToTimeoutMs;
			}
		}

		return timeToNextTimeoutMs;
	};

	/**
	 * Delivers all queued packets between `a` and `z`, then advances the simulated
	 * clock to the next timer expiry, and repeats. Returns once neither has a
	 * running timer or once `maxTimeoutMs` of simulated time has elapsed.
	 */
	const auto exchangeMessagesAndAdvanceTime =
	  [&exchangeMessages, &getTimeToNextTimeoutMs, &advanceNowMs](
	    AssociationUnderTest& a, AssociationUnderTest& z, int64_t maxTimeoutMs = 10000) -> void
	{
		const int64_t startedAtMs = a.shared.GetTimeMs();

		while (a.shared.GetTimeMs() - startedAtMs < maxTimeoutMs)
		{
			exchangeMessages(a, z);

			const auto timeToNextTimeoutMsA = getTimeToNextTimeoutMs(a);
			const auto timeToNextTimeoutMsZ = getTimeToNextTimeoutMs(z);

			if (!timeToNextTimeoutMsA.has_value() && !timeToNextTimeoutMsZ.has_value())
			{
				// No more pending timer.
				return;
			}

			int64_t timeToNextTimeoutMs{ 0 };

			if (!timeToNextTimeoutMsA.has_value())
			{
				timeToNextTimeoutMs = timeToNextTimeoutMsZ.value();
			}
			else if (!timeToNextTimeoutMsZ.has_value())
			{
				timeToNextTimeoutMs = timeToNextTimeoutMsA.value();
			}
			else
			{
				timeToNextTimeoutMs = std::min(timeToNextTimeoutMsA.value(), timeToNextTimeoutMsZ.value());
			}

			advanceNowMs(a, z, timeToNextTimeoutMs);
		}
	};

	/**
	 * Calls `Connect()` on `a` (the active peer) and drives the handshake to
	 * completion against `z` (the passive peer).
	 */
	const auto connectAssociations = [&exchangeMessages](
	                                   AssociationUnderTest& a, AssociationUnderTest& z) -> void
	{
		a.association.Connect();
		exchangeMessages(a, z);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(a.listener.CountOnAssociationConnectedCalls() == 1);
		REQUIRE(z.listener.CountOnAssociationConnectedCalls() == 1);
	};

	/**
	 * Connects `a` (active) against `z` (passive) and returns `a`'s local
	 * verification tag, i.e. the tag that the peer must put in packets sent to
	 * `a`. It's captured from the COOKIE-ACK that `z` sends to `a`, and is needed
	 * to craft raw packets to be injected into `a`.
	 */
	const auto connectAndGetVerificationTag = [&deliverFirstSentPacket, &parsePacket](
	                                            AssociationUnderTest& a,
	                                            AssociationUnderTest& z) -> uint32_t
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
		a.association.ReceiveSctpData(buffer.data(), buffer.size(), a.shared.GetTimeUs());

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);

		return verificationTag;
	};

	/**
	 * Creates an empty SCTP packet addressed to a peer, carrying the given
	 * verification tag. `buffer` must outlive the returned packet, as the packet
	 * is built in place on it.
	 */
	const auto buildPacket =
	  [](std::vector<uint8_t>& buffer, uint32_t verificationTag) -> std::unique_ptr<RTC::SCTP::Packet>
	{
		std::unique_ptr<RTC::SCTP::Packet> packet(
		  RTC::SCTP::Packet::Factory(buffer.data(), buffer.size()));

		packet->SetSourcePort(5000);
		packet->SetDestinationPort(5000);
		packet->SetVerificationTag(verificationTag);

		return packet;
	};

	/**
	 * Writes the packet checksum and injects it into the given association.
	 */
	const auto injectPacket = [](AssociationUnderTest& target, RTC::SCTP::Packet* packet) -> void
	{
		packet->WriteCRC32cChecksum();

		target.association.ReceiveSctpData(
		  packet->GetBuffer(), packet->GetLength(), target.shared.GetTimeUs());
	};

	/**
	 * Enqueues a message to be sent on the given association.
	 */
	const auto sendMessage =
	  [](
	    AssociationUnderTest& s,
	    uint16_t streamId,
	    uint32_t ppid,
	    std::vector<uint8_t> payload,
	    const RTC::SCTP::SendMessageOptions& sendMessageOptions = {}) -> RTC::SCTP::Types::SendMessageStatus
	{
		return s.association.SendMessage(
		  RTC::SCTP::Message(streamId, ppid, std::move(payload)), sendMessageOptions);
	};

	/**
	 * Consumes every received message on the given association and returns their
	 * payload protocol identifiers, in the order they were received.
	 */
	const auto getReceivedMessagePpids = [](AssociationUnderTest& s) -> std::vector<uint32_t>
	{
		std::vector<uint32_t> ppids;

		for (;;)
		{
			const auto message = s.listener.ConsumeFirstReceivedMessage();

			if (!message.has_value())
			{
				break;
			}

			ppids.push_back(message.value().GetPayloadProtocolId());
		}

		return ppids;
	};

	// Has A connect and replies to its INIT with an INIT-ACK carrying a cookie of
	// given length. Returns the verification tag of A, which packets sent to it
	// must carry.
	const auto replyToInit = [&parsePacket, &buildPacket, &injectPacket](
	                           AssociationUnderTest& a, uint16_t cookieLength) -> uint32_t
	{
		a.association.Connect();

		const auto initBuffer = a.listener.ConsumeFirstSentPacket();
		const auto initPacket = parsePacket(initBuffer);

		REQUIRE(initPacket);

		const auto* initChunk = initPacket->GetFirstChunkOfType<RTC::SCTP::InitChunk>();

		REQUIRE(initChunk);

		const uint32_t verificationTag = initChunk->GetInitiateTag();

		std::vector<uint8_t> buffer(2048);

		const auto packet        = buildPacket(buffer, verificationTag);
		auto* const initAckChunk = packet->BuildChunkInPlace<RTC::SCTP::InitAckChunk>();
		const std::vector<uint8_t> cookie(cookieLength, 0xAA);

		initAckChunk->SetInitiateTag(12345678);
		initAckChunk->SetAdvertisedReceiverWindowCredit(1000000);
		initAckChunk->SetNumberOfOutboundStreams(1);
		initAckChunk->SetNumberOfInboundStreams(1);
		initAckChunk->SetInitialTsn(1);

		auto* const stateCookieParameter =
		  initAckChunk->BuildParameterInPlace<RTC::SCTP::StateCookieParameter>();

		stateCookieParameter->SetCookie(cookie.data(), cookieLength);
		stateCookieParameter->Consolidate();
		initAckChunk->Consolidate();

		injectPacket(a, packet.get());

		return verificationTag;
	};

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
		REQUIRE(a.listener.CountOnAssociationConnectedCalls() == 1);
		REQUIRE(z.listener.CountOnAssociationConnectedCalls() == 1);
		REQUIRE(a.listener.HasRestarted() == false);
		REQUIRE(z.listener.HasRestarted() == false);
	}

	SECTION("notifies connecting only once when establishing connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		REQUIRE(a.listener.CountOnAssociationConnectingCalls() == 1);

		// Z reads INIT, produces INIT-ACK.
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTING);
		REQUIRE(a.listener.CountOnAssociationConnectingCalls() == 1);

		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		deliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(a.listener.CountOnAssociationConnectingCalls() == 1);
		REQUIRE(z.listener.CountOnAssociationConnectingCalls() == 0);
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
		REQUIRE(a.listener.CountOnAssociationConnectedCalls() == 1);
		REQUIRE(z.listener.CountOnAssociationConnectedCalls() == 1);
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
		REQUIRE(a.listener.CountOnAssociationConnectedCalls() == 1);
		REQUIRE(z.listener.CountOnAssociationConnectedCalls() == 1);
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
		advanceNowMs(a, z, a.sctpOptions.t1CookieTimeoutMs);

		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);
		// A reads COOKIE-ACK.
		deliverFirstSentPacket(z, a);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(a.listener.CountOnAssociationConnectedCalls() == 1);
		REQUIRE(z.listener.CountOnAssociationConnectedCalls() == 1);
	}

	SECTION("resends INIT and establishes connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		a.association.Connect();

		// INIT is never received by Z.
		REQUIRE(
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::INIT, a.listener.ConsumeFirstSentPacket()) == true);

		advanceNowMs(a, z, a.sctpOptions.t1InitTimeoutMs);

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
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::INIT, a.listener.ConsumeFirstSentPacket()) == true);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto maxInitRetransmissions = a.sctpOptions.maxInitRetransmissions.value();

		for (size_t i = 0; i < maxInitRetransmissions; ++i)
		{
			advanceNowMs(a, z, a.sctpOptions.t1InitTimeoutMs * (1 << i));

			// INIT is resent.
			REQUIRE(
			  packetHasSingleChunkOfType(
			    RTC::SCTP::Chunk::ChunkType::INIT, a.listener.ConsumeFirstSentPacket()) == true);
		}

		// Another timeout, after the max init retransmits.
		advanceNowMs(a, z, a.sctpOptions.t1InitTimeoutMs * (1 << maxInitRetransmissions));

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
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO, a.listener.ConsumeFirstSentPacket()) == true);

		advanceNowMs(a, z, a.sctpOptions.t1CookieTimeoutMs);

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
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO, a.listener.ConsumeFirstSentPacket()) == true);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto maxInitRetransmissions = a.sctpOptions.maxInitRetransmissions.value();

		for (size_t i = 0; i < maxInitRetransmissions; ++i)
		{
			advanceNowMs(a, z, a.sctpOptions.t1CookieTimeoutMs * (1 << i));

			// COOKIE-ECHO is resent.
			REQUIRE(
			  packetHasSingleChunkOfType(
			    RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO, a.listener.ConsumeFirstSentPacket()) == true);
		}

		// Another timeout, after the max init retransmits.
		advanceNowMs(a, z, a.sctpOptions.t1CookieTimeoutMs * (1 << maxInitRetransmissions));

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
		REQUIRE(a.listener.CountOnAssociationConnectedCalls() == 0);
		REQUIRE(z.listener.CountOnAssociationConnectedCalls() == 1);

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
		REQUIRE(a.listener.CountOnAssociationConnectedCalls() == 0);
		REQUIRE(z.listener.CountOnAssociationConnectedCalls() == 1);
		REQUIRE(a.listener.CountOnAssociationClosedCalls() == 1);
		REQUIRE(z.listener.CountOnAssociationClosedCalls() == 1);
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
		REQUIRE(z.listener.CountOnAssociationClosedCalls() == 1);
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

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto maxRetransmissions = a.sctpOptions.maxRetransmissions.value();

		for (size_t i = 0; i < maxRetransmissions; ++i)
		{
			advanceNowMs(a, z, a.sctpOptions.initialRtoMs * (1 << i));

			// SHUTDOWN is resent (and dropped).
			REQUIRE(
			  packetHasSingleChunkOfType(
			    RTC::SCTP::Chunk::ChunkType::SHUTDOWN, a.listener.ConsumeFirstSentPacket()) == true);
			REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);
		}

		// The last expiry makes it abort the connection.
		advanceNowMs(a, z, a.sctpOptions.initialRtoMs * (1 << maxRetransmissions));

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.IsClosed() == true);
		REQUIRE(a.listener.GetClosedErrorKind() == RTC::SCTP::Types::ErrorKind::TOO_MANY_RETRIES);
		// An ABORT chunk is sent.
		REQUIRE(
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::ABORT, a.listener.ConsumeFirstSentPacket()) == true);
		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);
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
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::SHUTDOWN_ACK, a.listener.ConsumeFirstSentPacket()) == true);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);
		REQUIRE(z.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const uint16_t maxRetransmissions = a.sctpOptions.maxRetransmissions.value();

		for (uint16_t i{ 0 }; i < maxRetransmissions; ++i)
		{
			// Advance time to trigger timer expiry, which makes A resend SHUTDOWN-ACK.
			advanceNowMs(a, z, a.sctpOptions.initialRtoMs * (1 << i));

			REQUIRE(
			  packetHasSingleChunkOfType(
			    RTC::SCTP::Chunk::ChunkType::SHUTDOWN_ACK, a.listener.ConsumeFirstSentPacket()) == true);
			REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);
		}

		// The last expiry makes it abort the association.
		advanceNowMs(a, z, a.sctpOptions.initialRtoMs * (1 << maxRetransmissions));

		REQUIRE(a.listener.IsClosed() == true);
		REQUIRE(a.listener.GetClosedErrorKind() == RTC::SCTP::Types::ErrorKind::TOO_MANY_RETRIES);
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);

		REQUIRE(
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::ABORT, a.listener.ConsumeFirstSentPacket()) == true);
		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);
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
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::SHUTDOWN_ACK, a.listener.ConsumeFirstSentPacket()) == true);
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

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
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

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& messageValue = message.value();

		REQUIRE(messageValue.GetStreamId() == 1);
		REQUIRE(messageValue.GetPayloadProtocolId() == 53);
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
		advanceNowMs(a, z, a.sctpOptions.initialRtoMs);

		// Z reads the retransmitted DATA chunk.
		deliverFirstSentPacket(a, z);

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(message->GetStreamId() == 1);
	}

	SECTION("sends a lot of bytes with a missed packet")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		sctpCommon::ResetBuffers();

		const std::vector<uint8_t> payload(
		  sctpCommon::DataBuffer, sctpCommon::DataBuffer + (a.sctpOptions.mtu * 20));

		sendMessage(a, 1, 53, payload);

		// Z reads the first DATA.
		deliverFirstSentPacket(a, z);
		// Second DATA is lost.
		a.listener.ConsumeFirstSentPacket();

		// Retransmit and handle the rest.
		exchangeMessages(a, z);

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& messageValue = message.value();

		REQUIRE(messageValue.GetStreamId() == 1);
		REQUIRE(
		  helpers::AreBuffersEqual(
		    messageValue.GetPayload().data(),
		    messageValue.GetPayloadLength(),
		    payload.data(),
		    payload.size()) == true);
	}

	SECTION("retransmission metrics are set for fast retransmit")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		// Enough to trigger fast retransmit of the missing second packet.
		const std::vector<uint8_t> payload(a.sctpOptions.mtu * 5);

		sendMessage(a, 1, 53, payload);

		// Z reads the first DATA.
		deliverFirstSentPacket(a, z);
		// Second DATA is lost.
		a.listener.ConsumeFirstSentPacket();

		// Receive and retransmit the remaining.
		exchangeMessages(a, z);

		const auto metrics = a.association.MakeMetrics();

		REQUIRE(metrics.has_value() == true);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& metricsValue = metrics.value();

		REQUIRE(metricsValue.rtxPacketsCount == 1);
		REQUIRE(
		  metricsValue.rtxBytesCount ==
		  Utils::Byte::PadDownTo4Bytes(a.sctpOptions.mtu - RTC::SCTP::Packet::CommonHeaderLength));
	}

	SECTION("retransmission metrics are set for normal retransmit")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		const std::vector<uint8_t> payload(SmallMessageSize);

		sendMessage(a, 1, 53, payload);

		// Drop the first DATA packet.
		a.listener.ConsumeFirstSentPacket();

		// Advance time so that the T3-RTX timer expires and the DATA is resent.
		advanceNowMs(a, z, a.sctpOptions.initialRtoMs);

		exchangeMessages(a, z);

		const auto metrics = a.association.MakeMetrics();

		REQUIRE(metrics.has_value() == true);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& metricsValue = metrics.value();

		REQUIRE(metricsValue.rtxPacketsCount == 1);

		// NOTE: `SctpOptions::enableMessageInterleaving` defaults to true, so the
		// association negotiates I-DATA and the retransmitted chunk carries an
		// I-DATA header.
		REQUIRE(
		  metricsValue.rtxBytesCount ==
		  Utils::Byte::PadTo4Bytes(SmallMessageSize + RTC::SCTP::IDataChunk::IDataChunkHeaderLength));
	}

	SECTION("retransmits a lost packet only once in a large burst")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		// A message large enough that many packets are still in flight when the
		// second packet is detected as lost and fast retransmitted. The SACKs for
		// those in-flight packets reach A before the retransmission reaches Z, and
		// they must not cause the lost packet to be retransmitted again.
		const std::vector<uint8_t> payload(a.sctpOptions.mtu * 20);

		sendMessage(a, 1, 53, payload);

		// Z reads the first DATA.
		deliverFirstSentPacket(a, z);
		// Second DATA is lost.
		a.listener.ConsumeFirstSentPacket();

		// Retransmit and handle the rest.
		exchangeMessagesAndAdvanceTime(a, z);

		const auto message = z.listener.ConsumeFirstReceivedMessage();

		REQUIRE(message.has_value() == true);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(message->GetPayloadLength() == payload.size());

		const auto metrics = a.association.MakeMetrics();

		REQUIRE(metrics.has_value() == true);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& metricsValue = metrics.value();

		REQUIRE(metricsValue.rtxPacketsCount == 1);
		REQUIRE(
		  metricsValue.rtxBytesCount ==
		  Utils::Byte::PadDownTo4Bytes(a.sctpOptions.mtu - RTC::SCTP::Packet::CommonHeaderLength));
	}

	SECTION("only rate limits duplicate FORWARD-TSN")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		const std::vector<uint8_t> payload(SmallMessageSize);

		// Sends an unreliable message and returns the packet carrying it.
		const auto sendUnreliable = [&sendMessage, &a, &payload](uint32_t ppid) -> std::vector<uint8_t>
		{
			sendMessage(a, 1, ppid, payload, { .maxRetransmissions = 0 });

			return a.listener.ConsumeFirstSentPacket();
		};

		// Delivers `buffer` to `z`, and the resulting SACK back to `a`.
		const auto deliverAndSack = [&a, &z](const std::vector<uint8_t>& buffer) -> void
		{
			z.association.ReceiveSctpData(buffer.data(), buffer.size(), z.shared.GetTimeUs());

			const auto sackBuffer = z.listener.ConsumeFirstSentPacket();

			REQUIRE(!sackBuffer.empty());

			a.association.ReceiveSctpData(sackBuffer.data(), sackBuffer.size(), a.shared.GetTimeUs());
		};

		// Returns the TSN of the I-DATA chunk carried by `buffer`.
		const auto getIDataChunkTsn = [&parsePacket](const std::vector<uint8_t>& buffer) -> uint32_t
		{
			const auto packet = parsePacket(buffer);

			REQUIRE(packet);

			const auto* iDataChunk = packet->GetFirstChunkOfType<RTC::SCTP::IDataChunk>();

			REQUIRE(iDataChunk);

			return iDataChunk->GetTsn();
		};

		// Returns the new cumulative TSN of the I-FORWARD-TSN chunk carried by
		// `buffer`, or `std::nullopt` if it carries no such chunk.
		const auto getIForwardTsn =
		  [&parsePacket](const std::vector<uint8_t>& buffer) -> std::optional<uint32_t>
		{
			const auto packet = parsePacket(buffer);

			if (!packet)
			{
				return std::nullopt;
			}

			const auto* iForwardTsnChunk = packet->GetFirstChunkOfType<RTC::SCTP::IForwardTsnChunk>();

			if (!iForwardTsnChunk)
			{
				return std::nullopt;
			}

			return iForwardTsnChunk->GetNewCumulativeTsn();
		};

		// The first message is lost. Three following messages are delivered, and
		// the SACKs generated for them will mark the first one as lost and
		// abandoned.
		const uint32_t lostTsn1 = getIDataChunkTsn(sendUnreliable(51));
		const auto packet52     = sendUnreliable(52);
		const auto packet53     = sendUnreliable(53);
		const auto packet54     = sendUnreliable(54);

		// Let some time pass, so that a non-zero RTT is measured.
		advanceNowMs(a, z, 50);

		deliverAndSack(packet52);
		deliverAndSack(packet53);
		deliverAndSack(packet54);

		const auto forwardTsn1 = a.listener.ConsumeFirstSentPacket();

		REQUIRE(
		  packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::I_FORWARD_TSN, forwardTsn1) == true);
		REQUIRE(getIForwardTsn(forwardTsn1) == lostTsn1);

		z.association.ReceiveSctpData(forwardTsn1.data(), forwardTsn1.size(), z.shared.GetTimeUs());
		exchangeMessages(a, z);

		// Without advancing time (i.e. within the FORWARD-TSN rate limiting
		// interval), lose another message. The FORWARD-TSN for it advances the
		// cumulative TSN, so it must be sent immediately.
		const uint32_t lostTsn2 = getIDataChunkTsn(sendUnreliable(55));

		deliverAndSack(sendUnreliable(56));
		deliverAndSack(sendUnreliable(57));
		deliverAndSack(sendUnreliable(58));

		const auto forwardTsn2 = a.listener.ConsumeFirstSentPacket();

		REQUIRE(
		  packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::I_FORWARD_TSN, forwardTsn2) == true);
		REQUIRE(getIForwardTsn(forwardTsn2) == lostTsn2);

		// That FORWARD-TSN was lost. A duplicate is not sent until the rate
		// limiting interval has passed.
		deliverAndSack(sendUnreliable(59));

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);

		advanceNowMs(a, z, 200);

		sendMessage(a, 1, 60, payload, { .maxRetransmissions = 0 });

		// The duplicate I-FORWARD-TSN goes bundled with the I-DATA of the new
		// message.
		const auto lastBuffer = a.listener.ConsumeFirstSentPacket();
		const auto lastPacket = parsePacket(lastBuffer);

		REQUIRE(lastPacket);
		REQUIRE(lastPacket->GetChunksCount() == 2);
		REQUIRE(lastPacket->GetChunkAt(0)->GetType() == RTC::SCTP::Chunk::ChunkType::I_FORWARD_TSN);
		REQUIRE(lastPacket->GetChunkAt(1)->GetType() == RTC::SCTP::Chunk::ChunkType::I_DATA);

		const auto* lastIForwardTsnChunk =
		  static_cast<const RTC::SCTP::IForwardTsnChunk*>(lastPacket->GetChunkAt(0));
		const auto* lastIDataChunk = static_cast<const RTC::SCTP::IDataChunk*>(lastPacket->GetChunkAt(1));

		REQUIRE(lastIForwardTsnChunk->GetNewCumulativeTsn() == lostTsn2);
		REQUIRE(lastIDataChunk->GetPayloadProtocolId() == 60);
	}

	SECTION("sends a heartbeat on idle connection")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);

		// Let the heartbeat interval timer expire.
		advanceNowMs(a, z, a.sctpOptions.heartbeatIntervalMs);

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(
		  packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::HEARTBEAT_REQUEST, buffer) == true);

		const auto packet = parsePacket(buffer);
		const auto* heartbeatRequestChunk =
		  packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>();
		const auto* heartbeatInfoParameter =
		  heartbeatRequestChunk->GetFirstParameterOfType<RTC::SCTP::HeartbeatInfoParameter>();

		REQUIRE(heartbeatInfoParameter);
		// NOTE: 8 is HeartbeatInfoLength in HeartbeatHandler.cpp.
		REQUIRE(heartbeatInfoParameter->GetInfoLength() == 8);

		// Whatever Z sent before receiving the HEARTBEAT-REQUEST cannot be the
		// reply to it, so drop it.
		while (z.listener.HasSentPackets())
		{
			z.listener.ConsumeFirstSentPacket();
		}

		// Feed it to Z and expect a HEARTBEAT-ACK that is propagated back to A.
		z.association.ReceiveSctpData(buffer.data(), buffer.size(), z.shared.GetTimeUs());

		const auto ackBuffer = z.listener.ConsumeFirstSentPacket();
		const auto ackPacket = parsePacket(ackBuffer);

		REQUIRE(ackPacket);
		REQUIRE(ackPacket->GetFirstChunkOfType<RTC::SCTP::HeartbeatAckChunk>() != nullptr);

		a.association.ReceiveSctpData(ackBuffer.data(), ackBuffer.size(), a.shared.GetTimeUs());
	}

	SECTION("sends many messages")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		constexpr size_t Iterations{ 100 };

		std::vector<RTC::SCTP::Message> messages;

		messages.reserve(Iterations);

		for (size_t i = 0; i < Iterations; ++i)
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

		for (size_t i = 0; i < Iterations; ++i)
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
				auto aSctpOptions = MakeSctpOptions();

				aSctpOptions.enableMessageInterleaving = aEnable;

				auto zSctpOptions = MakeSctpOptions();

				zSctpOptions.enableMessageInterleaving = zEnable;

				AssociationUnderTest a(aSctpOptions);
				AssociationUnderTest z(zSctpOptions);

				connectAssociations(a, z);

				// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				REQUIRE(a.association.MakeMetrics()->usesMessageInterleaving == (aEnable && zEnable));
			}
		}
	}

	SECTION("rx and tx packet metrics increase")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		const auto initialARwnd = static_cast<size_t>(
		  static_cast<double>(a.sctpOptions.maxReceiverWindowBufferSize) *
		  RTC::SCTP::ReassemblyQueue::HighWatermarkLimit);

		// A sent INIT and COOKIE-ECHO, received INIT-ACK and COOKIE-ACK.
		//
		// NOLINTBEGIN(bugprone-unchecked-optional-access)
		REQUIRE(a.association.MakeMetrics()->txPacketsCount == 2);
		REQUIRE(a.association.MakeMetrics()->rxPacketsCount == 2);
		REQUIRE(a.association.MakeMetrics()->txMessagesCount == 0);
		// NOLINTEND(bugprone-unchecked-optional-access)
		REQUIRE(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  a.association.MakeMetrics()->cwndBytes == a.sctpOptions.initialCwndMtus * a.sctpOptions.mtu);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(a.association.MakeMetrics()->unackDataCount == 0);

		// NOLINTBEGIN(bugprone-unchecked-optional-access)
		REQUIRE(z.association.MakeMetrics()->rxPacketsCount == 2);
		REQUIRE(z.association.MakeMetrics()->rxMessagesCount == 0);
		// NOLINTEND(bugprone-unchecked-optional-access)

		sendMessage(a, 1, 53, { 1, 2 });

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(a.association.MakeMetrics()->unackDataCount == 1);

		// Z reads DATA.
		deliverFirstSentPacket(a, z);
		// A reads SACK.
		deliverFirstSentPacket(z, a);

		// NOLINTBEGIN(bugprone-unchecked-optional-access)
		REQUIRE(a.association.MakeMetrics()->peerRwndBytes == initialARwnd);
		REQUIRE(a.association.MakeMetrics()->unackDataCount == 0);
		// NOLINTEND(bugprone-unchecked-optional-access)
		REQUIRE(z.listener.ConsumeFirstReceivedMessage().has_value() == true);

		// NOLINTBEGIN(bugprone-unchecked-optional-access)
		REQUIRE(a.association.MakeMetrics()->txPacketsCount == 3);
		REQUIRE(a.association.MakeMetrics()->rxPacketsCount == 3);
		REQUIRE(a.association.MakeMetrics()->txMessagesCount == 1);
		// NOLINTEND(bugprone-unchecked-optional-access)

		// NOLINTBEGIN(bugprone-unchecked-optional-access)
		REQUIRE(z.association.MakeMetrics()->rxPacketsCount == 3);
		REQUIRE(z.association.MakeMetrics()->rxMessagesCount == 1);
		// NOLINTEND(bugprone-unchecked-optional-access)

		// Send one more (large, fragmented) and receive the delayed SACK.
		sendMessage(a, 1, 53, std::vector<uint8_t>((a.sctpOptions.mtu * 2) + 1));

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(a.association.MakeMetrics()->unackDataCount == 3);

		// Z reads DATA.
		deliverFirstSentPacket(a, z);
		// Z reads DATA.
		deliverFirstSentPacket(a, z);
		// A reads SACK.
		deliverFirstSentPacket(z, a);

		// NOLINTBEGIN(bugprone-unchecked-optional-access)
		REQUIRE(a.association.MakeMetrics()->unackDataCount == 1);
		REQUIRE(a.association.MakeMetrics()->peerRwndBytes > 0);
		REQUIRE(a.association.MakeMetrics()->peerRwndBytes < initialARwnd);
		// NOLINTEND(bugprone-unchecked-optional-access)

		// Z reads DATA.
		deliverFirstSentPacket(a, z);

		REQUIRE(z.listener.ConsumeFirstReceivedMessage().has_value() == true);

		// NOLINTBEGIN(bugprone-unchecked-optional-access)
		REQUIRE(a.association.MakeMetrics()->txPacketsCount == 6);
		REQUIRE(a.association.MakeMetrics()->rxPacketsCount == 4);
		REQUIRE(a.association.MakeMetrics()->txMessagesCount == 2);
		// NOLINTEND(bugprone-unchecked-optional-access)

		// NOLINTBEGIN(bugprone-unchecked-optional-access)
		REQUIRE(z.association.MakeMetrics()->rxPacketsCount == 6);
		REQUIRE(z.association.MakeMetrics()->rxMessagesCount == 2);
		// NOLINTEND(bugprone-unchecked-optional-access)

		// Delayed SACK.
		advanceNowMs(a, z, a.sctpOptions.delayedAckMaxTimeoutMs);

		// A reads SACK.
		deliverFirstSentPacket(z, a);

		// NOLINTBEGIN(bugprone-unchecked-optional-access)
		REQUIRE(a.association.MakeMetrics()->unackDataCount == 0);
		REQUIRE(a.association.MakeMetrics()->rxPacketsCount == 5);
		REQUIRE(a.association.MakeMetrics()->peerRwndBytes == initialARwnd);
		// NOLINTEND(bugprone-unchecked-optional-access)
	}

	SECTION("streams have initial priority")
	{
		auto sctpOptions = MakeSctpOptions();

		sctpOptions.defaultStreamPriority = 42;

		AssociationUnderTest a(sctpOptions);

		REQUIRE(a.association.GetStreamPriority(1) == 42);

		sendMessage(a, 2, 53, { 1, 2 });

		REQUIRE(a.association.GetStreamPriority(2) == 42);
	}

	SECTION("can change stream priority")
	{
		auto sctpOptions = MakeSctpOptions();

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
		auto sctpOptions = MakeSctpOptions();

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

	SECTION("applies the configured default buffered amount low threshold to new streams")
	{
		auto sctpOptions = MakeSctpOptions();

		sctpOptions.defaultStreamBufferedAmountLowThreshold = 1024;

		AssociationUnderTest a(sctpOptions);

		// Sending a message creates the outgoing stream, which must inherit the
		// configured default buffered amount low threshold.
		sendMessage(a, 1, 53, std::vector<uint8_t>(10));

		REQUIRE(a.association.GetStreamBufferedAmountLowThreshold(1) == 1024);
	}

	SECTION("does not trigger buffered amount low until crossing a non-zero default threshold")
	{
		auto sctpOptions = MakeSctpOptions();

		sctpOptions.defaultStreamBufferedAmountLowThreshold = 1024;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z;

		connectAssociations(a, z);

		// A small message never takes the buffered amount above the threshold, so
		// draining it must not trigger the event.
		sendMessage(a, 1, 53, std::vector<uint8_t>(10));
		exchangeMessages(a, z);

		REQUIRE(a.listener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1) == false);
	}

	SECTION("detects the peer implementation")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		// Both peers are mediasoup.
		REQUIRE(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  a.association.MakeMetrics()->peerImplementation ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);
		// As A initiated the connection establishment, the passive peer Z will not
		// receive enough information to know about A's implementation.
		REQUIRE(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
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

		REQUIRE(a.listener.CountOnAssociationConnectedCalls() == 1);
		REQUIRE(z.listener.CountOnAssociationConnectedCalls() == 1);
		REQUIRE(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  a.association.MakeMetrics()->peerImplementation ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);
		REQUIRE(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  z.association.MakeMetrics()->peerImplementation ==
		  RTC::SCTP::Types::SctpImplementation::MEDIASOUP);
	}

	SECTION("exposes the number of negotiated streams")
	{
		auto aSctpOptions = MakeSctpOptions();

		aSctpOptions.announcedMaxInboundStreams  = 12;
		aSctpOptions.announcedMaxOutboundStreams = 45;

		auto zSctpOptions = MakeSctpOptions();

		zSctpOptions.announcedMaxInboundStreams  = 23;
		zSctpOptions.announcedMaxOutboundStreams = 34;

		AssociationUnderTest a(aSctpOptions);
		AssociationUnderTest z(zSctpOptions);

		connectAssociations(a, z);

		const auto metricsA = a.association.MakeMetrics();

		REQUIRE(metricsA.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& metricsAValue = metricsA.value();

		REQUIRE(metricsAValue.negotiatedMaxInboundStreams == 12);
		REQUIRE(metricsAValue.negotiatedMaxOutboundStreams == 23);

		const auto metricsZ = z.association.MakeMetrics();

		REQUIRE(metricsZ.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& metricsZValue = metricsZ.value();

		REQUIRE(metricsZValue.negotiatedMaxInboundStreams == 23);
		REQUIRE(metricsZValue.negotiatedMaxOutboundStreams == 12);
	}

	SECTION("always sends INIT with non-zero checksum")
	{
		auto sctpOptions = MakeSctpOptions();

		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

		AssociationUnderTest a(sctpOptions);

		a.association.Connect();

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::INIT, buffer) == true);
		REQUIRE(parsePacket(buffer)->GetChecksum() != 0);
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
		deliverFirstSentPacket(a, z);

		const auto buffer = z.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::INIT_ACK, buffer) == true);
		REQUIRE(parsePacket(buffer)->GetChecksum() == 0);
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
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO, buffer) == true);
		REQUIRE(parsePacket(buffer)->GetChecksum() != 0);
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
		deliverFirstSentPacket(a, z);
		// A reads INIT-ACK, produces COOKIE-ECHO.
		deliverFirstSentPacket(z, a);
		// Z reads COOKIE-ECHO, produces COOKIE-ACK.
		deliverFirstSentPacket(a, z);

		const auto buffer = z.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::COOKIE_ACK, buffer) == true);
		REQUIRE(parsePacket(buffer)->GetChecksum() == 0);
	}

	SECTION("sends DATA with zero checksum")
	{
		auto sctpOptions = MakeSctpOptions();

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

	SECTION("both sides send DATA with zero checksum with TRUSTED_NETWORK")
	{
		auto sctpOptions = MakeSctpOptions();

		sctpOptions.zeroChecksumAlternateErrorDetectionMethod =
		  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::TRUSTED_NETWORK;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z(sctpOptions);

		connectAssociations(a, z);

		const auto metricsA = a.association.MakeMetrics();
		const auto metricsZ = z.association.MakeMetrics();

		REQUIRE(metricsA.has_value());
		REQUIRE(metricsZ.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& metricsAValue = metricsA.value();
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& metricsZValue = metricsZ.value();

		REQUIRE(metricsAValue.usesZeroChecksum == true);
		// Z negotiated it from the State Cookie it generated.
		REQUIRE(metricsZValue.usesZeroChecksum == true);

		sendMessage(a, 1, 53, std::vector<uint8_t>(a.sctpOptions.mtu - 100));

		const auto bufferA = a.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasDataChunk(bufferA) == true);
		REQUIRE(parsePacket(bufferA)->GetChecksum() == 0);

		sendMessage(z, 1, 53, std::vector<uint8_t>(z.sctpOptions.mtu - 100));

		const auto bufferZ = z.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasDataChunk(bufferZ) == true);
		REQUIRE(parsePacket(bufferZ)->GetChecksum() == 0);
	}

	SECTION("both sides send heartbeats")
	{
		// Make them have slightly different heartbeat intervals, to validate that
		// sending an ack by Z doesn't restart its heartbeat timer.
		auto aSctpOptions = MakeSctpOptions();

		aSctpOptions.heartbeatIntervalMs = 1000;

		auto zSctpOptions = MakeSctpOptions();

		zSctpOptions.heartbeatIntervalMs = 1100;

		AssociationUnderTest a(aSctpOptions);
		AssociationUnderTest z(zSctpOptions);

		connectAssociations(a, z);

		advanceNowMs(a, z, 1000);

		const auto bufferA = a.listener.ConsumeFirstSentPacket();

		REQUIRE(
		  packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::HEARTBEAT_REQUEST, bufferA) == true);

		// Z receives the heartbeat and sends an ACK that is propagated back to A.
		z.association.ReceiveSctpData(bufferA.data(), bufferA.size(), z.shared.GetTimeUs());
		deliverFirstSentPacket(z, a);

		// A little while later, Z should send heartbeats to A.
		advanceNowMs(a, z, 100);

		const auto bufferZ = z.listener.ConsumeFirstSentPacket();

		REQUIRE(
		  packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::HEARTBEAT_REQUEST, bufferZ) == true);

		// A receives the heartbeat and sends an ACK that is propagated back to Z.
		a.association.ReceiveSctpData(bufferZ.data(), bufferZ.size(), a.shared.GetTimeUs());
		deliverFirstSentPacket(a, z);
	}

	SECTION("closes connection after too many lost heartbeats")
	{
		AssociationUnderTest a;
		// Disable Z's heartbeats so that it doesn't interfere.
		auto zSctpOptions = MakeSctpOptions();

		zSctpOptions.heartbeatIntervalMs = 0;

		AssociationUnderTest z(zSctpOptions);

		connectAssociations(a, z);

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto maxRetransmissions = a.sctpOptions.maxRetransmissions.value();

		int64_t timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs;

		for (size_t i = 0; i < maxRetransmissions; ++i)
		{
			// Let the heartbeat interval timer expire, sending a heartbeat.
			advanceNowMs(a, z, timeToNextHeartbeatMs);

			// Drop every heartbeat.
			REQUIRE(
			  packetHasSingleChunkOfType(
			    RTC::SCTP::Chunk::ChunkType::HEARTBEAT_REQUEST, a.listener.ConsumeFirstSentPacket()) ==
			  true);

			// Let the heartbeat timeout expire.
			advanceNowMs(a, z, 1000);

			timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs - 1000;
		}

		// The last heartbeat.
		advanceNowMs(a, z, timeToNextHeartbeatMs);

		REQUIRE(a.listener.HasSentPackets() == true);

		a.listener.ConsumeFirstSentPacket();

		// Should suffice as exceeding RTO, which aborts the connection.
		advanceNowMs(a, z, 1000);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.IsClosed() == true);
		REQUIRE(a.listener.CountOnAssociationClosedCalls() == 1);
		REQUIRE(a.listener.GetClosedErrorKind() == RTC::SCTP::Types::ErrorKind::TOO_MANY_RETRIES);
	}

	SECTION("recovers after a successful heartbeat ack")
	{
		AssociationUnderTest a;
		// Disable Z's heartbeats so that it doesn't interfere.
		auto zSctpOptions = MakeSctpOptions();

		zSctpOptions.heartbeatIntervalMs = 0;

		AssociationUnderTest z(zSctpOptions);

		connectAssociations(a, z);

		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto maxRetransmissions = a.sctpOptions.maxRetransmissions.value();

		int64_t timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs;

		for (size_t i = 0; i < maxRetransmissions; ++i)
		{
			advanceNowMs(a, z, timeToNextHeartbeatMs);

			// Drop every heartbeat.
			a.listener.ConsumeFirstSentPacket();

			// Let the heartbeat timeout expire.
			advanceNowMs(a, z, 1000);

			timeToNextHeartbeatMs = a.sctpOptions.heartbeatIntervalMs - 1000;
		}

		// Get the last heartbeat and ack it (round trip through Z).
		advanceNowMs(a, z, timeToNextHeartbeatMs);

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(
		  packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::HEARTBEAT_REQUEST, buffer) == true);

		z.association.ReceiveSctpData(buffer.data(), buffer.size(), z.shared.GetTimeUs());
		// A reads the HEARTBEAT-ACK, which clears the error counter.
		deliverFirstSentPacket(z, a);

		// Should suffice as exceeding RTO, but the timer will not fire as it was
		// stopped by the ack.
		advanceNowMs(a, z, 1000);

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(a.listener.IsClosed() == false);
		REQUIRE(a.listener.CountOnAssociationClosedCalls() == 0);
		REQUIRE(a.listener.ConsumeFirstSentPacket().empty() == true);

		// Verify that we get new heartbeats again.
		advanceNowMs(a, z, timeToNextHeartbeatMs);

		REQUIRE(
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::HEARTBEAT_REQUEST, a.listener.ConsumeFirstSentPacket()) == true);
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
		advanceNowMs(a, z, a.sctpOptions.delayedAckMaxTimeoutMs);
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

		advanceNowMs(a, z, 200);

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
		const auto* cookieAckChunk = packet->BuildChunkInPlace<RTC::SCTP::CookieAckChunk>();

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

	SECTION("doesn't report an unknown chunk too large to be echoed")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		const uint32_t verificationTag = connectAndGetVerificationTag(a, z);

		// Injects into A a chunk of unknown type (0x49), whose top two bits (01)
		// request the receiver to skip it and report an error. It is built as a
		// HEARTBEAT-REQUEST chunk carrying a Heartbeat Info parameter with given
		// info length, and then its type is patched.
		const auto injectUnknownChunk =
		  [&buildPacket, &injectPacket, &a, verificationTag](uint16_t infoLength) -> void
		{
			std::vector<uint8_t> buffer(a.sctpOptions.mtu);

			const auto packet = buildPacket(buffer, verificationTag);
			auto* const heartbeatRequestChunk =
			  packet->BuildChunkInPlace<RTC::SCTP::HeartbeatRequestChunk>();
			auto* const heartbeatInfoParameter =
			  heartbeatRequestChunk->BuildParameterInPlace<RTC::SCTP::HeartbeatInfoParameter>();
			const std::vector<uint8_t> info(infoLength, 0xAA);

			heartbeatInfoParameter->SetInfo(info.data(), infoLength);
			heartbeatInfoParameter->Consolidate();
			heartbeatRequestChunk->Consolidate();

			const_cast<uint8_t*>(packet->GetBuffer())[12] = 0x49;

			injectPacket(a, packet.get());
		};

		// The OPERATION-ERROR reply takes 12 bytes of common header, 4 of chunk
		// header, 4 of error cause header and the unknown chunk, which is 8 bytes
		// of headers plus the info padded to 4 bytes. With 1160 bytes of info that
		// is 1188 bytes, the most A sends (its 1191 bytes MTU padded down to 4
		// bytes).
		injectUnknownChunk(1160);

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::OPERATION_ERROR, buffer) == true);
		REQUIRE(buffer.size() == 1188);

		// With 1161 bytes of info the reply would take 1192 bytes, so it is not
		// sent.
		injectUnknownChunk(1161);

		REQUIRE(a.listener.HasSentPackets() == false);
	}

	SECTION("processes the chunks after an unknown chunk whose type says to skip it")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		const uint32_t verificationTag = connectAndGetVerificationTag(a, z);

		std::vector<uint8_t> buffer(a.sctpOptions.mtu);

		const auto packet = buildPacket(buffer, verificationTag);

		// Build a 4-byte chunk and patch its type to an unknown value (0x80), whose
		// top two bits (10) request the receiver to skip it and continue
		// processing the packet, without reporting it.
		const auto* const unknownChunk = packet->BuildChunkInPlace<RTC::SCTP::CookieAckChunk>();

		unknownChunk->Consolidate();

		auto* const heartbeatRequestChunk = packet->BuildChunkInPlace<RTC::SCTP::HeartbeatRequestChunk>();
		auto* const heartbeatInfoParameter =
		  heartbeatRequestChunk->BuildParameterInPlace<RTC::SCTP::HeartbeatInfoParameter>();
		const std::vector<uint8_t> info{ 1, 2, 3, 4 };

		heartbeatInfoParameter->SetInfo(info.data(), info.size());
		heartbeatInfoParameter->Consolidate();
		heartbeatRequestChunk->Consolidate();

		const_cast<uint8_t*>(packet->GetBuffer())[12] = 0x80;

		injectPacket(a, packet.get());

		// The HEARTBEAT-REQUEST chunk after the unknown chunk is processed, so A
		// replies with a HEARTBEAT-ACK chunk.
		REQUIRE(packetHasSingleChunkOfType(
		  RTC::SCTP::Chunk::ChunkType::HEARTBEAT_ACK, a.listener.ConsumeFirstSentPacket()));
	}

	SECTION("stops processing the packet at an unknown chunk whose type says to stop")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		const uint32_t verificationTag = connectAndGetVerificationTag(a, z);

		std::vector<uint8_t> buffer(a.sctpOptions.mtu);

		const auto packet = buildPacket(buffer, verificationTag);

		// Build a 4-byte chunk and patch its type to an unknown value (0x00 is
		// DATA, so 0x3F is used), whose top two bits (00) request the receiver to
		// stop processing the packet, without reporting it.
		const auto* const unknownChunk = packet->BuildChunkInPlace<RTC::SCTP::CookieAckChunk>();

		unknownChunk->Consolidate();

		auto* const heartbeatRequestChunk = packet->BuildChunkInPlace<RTC::SCTP::HeartbeatRequestChunk>();
		auto* const heartbeatInfoParameter =
		  heartbeatRequestChunk->BuildParameterInPlace<RTC::SCTP::HeartbeatInfoParameter>();
		const std::vector<uint8_t> info{ 1, 2, 3, 4 };

		heartbeatInfoParameter->SetInfo(info.data(), info.size());
		heartbeatInfoParameter->Consolidate();
		heartbeatRequestChunk->Consolidate();

		const_cast<uint8_t*>(packet->GetBuffer())[12] = 0x3F;

		injectPacket(a, packet.get());

		// The HEARTBEAT-REQUEST chunk after the unknown chunk is not processed, so
		// A sends nothing.
		REQUIRE(a.listener.HasSentPackets() == false);
	}

	SECTION("aborts on an INIT-ACK whose cookie is too large to be echoed")
	{
		// The COOKIE-ECHO takes 12 bytes of common header, 4 of chunk header and
		// the cookie padded to 4 bytes. With a 1172 bytes cookie that is 1188
		// bytes, the most A sends (its 1191 bytes MTU padded down to 4 bytes).
		{
			AssociationUnderTest a;

			replyToInit(a, 1172);

			const auto buffer = a.listener.ConsumeFirstSentPacket();

			REQUIRE(packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO, buffer) == true);
			REQUIRE(buffer.size() == 1188);
			REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTING);
		}

		// With a 1173 bytes cookie the COOKIE-ECHO would take 1192 bytes, so A
		// aborts.
		{
			AssociationUnderTest a;

			replyToInit(a, 1173);

			const auto buffer = a.listener.ConsumeFirstSentPacket();

			REQUIRE(packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::ABORT, buffer) == true);
			REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
			REQUIRE(a.listener.HasFailed() == true);
			REQUIRE(a.listener.GetFailedErrorKind() == RTC::SCTP::Types::ErrorKind::PROTOCOL_VIOLATION);
		}
	}

	SECTION("doesn't bundle a SACK that does not fit with a resent COOKIE-ECHO")
	{
		AssociationUnderTest a;

		// A 1172 bytes cookie fills the COOKIE-ECHO packet, leaving no room for
		// anything else.
		const uint32_t verificationTag = replyToInit(a, 1172);

		REQUIRE(
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO, a.listener.ConsumeFirstSentPacket()) == true);

		// Injects into A a DATA chunk with given TSN.
		const auto injectDataChunk = [&buildPacket, &injectPacket, &a, verificationTag](uint32_t tsn) -> void
		{
			std::vector<uint8_t> buffer(a.sctpOptions.mtu);

			const auto packet     = buildPacket(buffer, verificationTag);
			auto* const dataChunk = packet->BuildChunkInPlace<RTC::SCTP::DataChunk>();
			const uint8_t payload[]{ 0x01 };

			dataChunk->SetTsn(tsn);
			dataChunk->SetStreamId(0);
			dataChunk->SetPayloadProtocolId(53);
			dataChunk->SetB(true);
			dataChunk->SetE(true);
			dataChunk->SetUserDataPayload(payload, sizeof(payload));
			dataChunk->Consolidate();

			injectPacket(a, packet.get());
		};

		// The first DATA chunk is acked right away, alone.
		injectDataChunk(1);

		REQUIRE(
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::SACK, a.listener.ConsumeFirstSentPacket()) == true);

		// The second one leaves a delayed SACK pending.
		injectDataChunk(2);

		REQUIRE(a.listener.HasSentPackets() == false);

		// When the T1-cookie timer expires the COOKIE-ECHO is resent, and the
		// pending SACK, which does not fit, is not bundled with it.
		a.AdvanceNowMs(a.sctpOptions.t1CookieTimeoutMs);

		auto* t1CookieTimer = a.shared.GetBackoffTimer("sctp-t1-cookie");

		REQUIRE(t1CookieTimer);
		REQUIRE(t1CookieTimer->EvaluateHasExpired() == true);

		const auto buffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO, buffer) == true);
		REQUIRE(buffer.size() == 1188);

		// The SACK is still pending, so it is sent once the delayed ack timer
		// expires.
		auto* delayedAckTimer = a.shared.GetBackoffTimer("sctp-delayed-ack");

		REQUIRE(delayedAckTimer);
		REQUIRE(delayedAckTimer->EvaluateHasExpired() == true);
		REQUIRE(
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::SACK, a.listener.ConsumeFirstSentPacket()) == true);
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
		REQUIRE(
		  helpers::AreBuffersEqual(
		    replyInfoParameter->GetInfo(), replyInfoParameter->GetInfoLength(), info, sizeof(info)) ==
		  true);
	}

	SECTION("sends a message with limited retransmissions")
	{
		// Disable message interleaving so that ordered DATA chunks (with SSN) are
		// used.
		auto sctpOptions = MakeSctpOptions();

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
		advanceNowMs(a, z, a.sctpOptions.delayedAckMaxTimeoutMs);
		exchangeMessages(a, z);
		advanceNowMs(a, z, a.sctpOptions.initialRtoMs);
		exchangeMessages(a, z);
		advanceNowMs(a, z, a.sctpOptions.delayedAckMaxTimeoutMs);
		exchangeMessages(a, z);

		// The first and third messages are delivered; the second one is abandoned.
		REQUIRE(getReceivedMessagePpids(z) == std::vector<uint32_t>{ 51, 53 });
	}

	SECTION("lifecycle events for fail on max retransmissions")
	{
		auto sctpOptions = MakeSctpOptions();

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
		advanceNowMs(a, z, a.sctpOptions.delayedAckMaxTimeoutMs);
		exchangeMessages(a, z);

		// The chunk is now nacked. Let the RTO expire to discard the message.
		advanceNowMs(a, z, a.sctpOptions.initialRtoMs);
		exchangeMessages(a, z);

		// Handle the delayed SACK.
		advanceNowMs(a, z, a.sctpOptions.delayedAckMaxTimeoutMs);
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
		auto sctpOptions = MakeSctpOptions();

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
		auto sctpOptions = MakeSctpOptions();

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
		const RTC::SCTP::Capabilities remoteCapabilities;

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
		  remoteCapabilities);

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
			auto sctpOptions = MakeSctpOptions();

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
		auto sctpOptions = MakeSctpOptions();

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
		z.AdvanceNowMs((RTC::SCTP::StateCookie::ValidCookieLifeUs / 1000) + 1000);

		// Z reads the now stale COOKIE-ECHO.
		deliverFirstSentPacket(a, z);

		REQUIRE(z.association.GetAssociationState() != RTC::SCTP::Types::AssociationState::CONNECTED);
		// Z responds with an ERROR chunk (carrying a Stale Cookie error cause).
		const auto errorBuffer = z.listener.ConsumeFirstSentPacket();

		REQUIRE(
		  packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::OPERATION_ERROR, errorBuffer) == true);

		const auto errorPacket = parsePacket(errorBuffer);
		const auto* operationErrorChunk =
		  errorPacket->GetFirstChunkOfType<RTC::SCTP::OperationErrorChunk>();

		REQUIRE(operationErrorChunk->GetErrorCausesCount() >= 1);
		REQUIRE(
		  operationErrorChunk->GetErrorCauseAt(0)->GetCode() ==
		  RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE);

		const auto* staleCookieErrorCause =
		  static_cast<const RTC::SCTP::StaleCookieErrorCause*>(operationErrorChunk->GetErrorCauseAt(0));

		REQUIRE(staleCookieErrorCause->GetMeasureOfStaleness() > 0);
	}

	SECTION("delivers the messages received after a too large one")
	{
		auto sctpOptions = MakeSctpOptions();

		sctpOptions.maxReceiveMessageSize = 100;

		AssociationUnderTest a(sctpOptions);
		AssociationUnderTest z;

		connectAssociations(a, z);

		// Each message is sent in its own packet.
		sendMessage(z, /*streamId*/ 1, /*ppid*/ 51, std::vector<uint8_t>(200));
		sendMessage(z, /*streamId*/ 1, /*ppid*/ 52, std::vector<uint8_t>(10));

		// Drop the packet with the too large message.
		z.listener.ConsumeFirstSentPacket();

		// A reads the packet with the second message, which is held since the
		// stream is ordered and the first message is missing.
		deliverFirstSentPacket(z, a);

		REQUIRE(getReceivedMessagePpids(a).empty());

		// Z retransmits the too large message once its T3-rtx timer expires, so A
		// completes both messages while processing that packet.
		exchangeMessagesAndAdvanceTime(a, z);

		// The too large message is dropped, but the next one must be delivered.
		REQUIRE(getReceivedMessagePpids(a) == std::vector<uint32_t>{ 52 });
	}

	SECTION("COOKIE-ECHO matching both tags while shutting down doesn't establish the association")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		const uint32_t localVerificationTag = connectAndGetVerificationTag(a, z);

		z.association.Shutdown();
		// A reads SHUTDOWN, produces SHUTDOWN-ACK and enters SHUTDOWN_ACK_SENT
		// state with the T2-shutdown timer running.
		deliverFirstSentPacket(z, a);

		const auto shutdownAckBuffer = a.listener.ConsumeFirstSentPacket();

		REQUIRE(
		  packetHasSingleChunkOfType(RTC::SCTP::Chunk::ChunkType::SHUTDOWN_ACK, shutdownAckBuffer) ==
		  true);
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// The SHUTDOWN-ACK sent to Z carries Z's verification tag, which is A's
		// remote verification tag.
		const uint32_t remoteVerificationTag = parsePacket(shutdownAckBuffer)->GetVerificationTag();

		// A (not authenticated) State Cookie whose tags match the ones of A's TCB,
		// so the COOKIE-ECHO is handled as a duplicated one (RFC 9260 section
		// 5.2.4 action D).
		const RTC::SCTP::Capabilities remoteCapabilities;

		std::vector<uint8_t> cookieBuffer(RTC::SCTP::StateCookie::StateCookieLength);

		RTC::SCTP::StateCookie::Write(
		  cookieBuffer.data(),
		  cookieBuffer.size(),
		  /*localVerificationTag*/ localVerificationTag,
		  /*remoteVerificationTag*/ remoteVerificationTag,
		  /*localInitialTsn*/ 1000,
		  /*remoteInitialTsn*/ 2000,
		  /*remoteAdvertisedReceiverWindowCredit*/ 65535,
		  /*tieTag*/ 0,
		  remoteCapabilities);

		std::vector<uint8_t> packetBuffer(a.sctpOptions.mtu);

		const auto packet           = buildPacket(packetBuffer, localVerificationTag);
		auto* const cookieEchoChunk = packet->BuildChunkInPlace<RTC::SCTP::CookieEchoChunk>();

		cookieEchoChunk->SetCookie(cookieBuffer.data(), cookieBuffer.size());
		cookieEchoChunk->Consolidate();

		injectPacket(a, packet.get());

		// A must only enter ESTABLISHED from COOKIE_ECHOED state, so it keeps
		// shutting down.
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);
	}

	SECTION("COOKIE-ECHO from a restarted peer in SHUTDOWN_SENT state doesn't establish the association")
	{
		constexpr uint32_t PeerNewVerificationTag{ 0x11223344 };
		constexpr uint32_t LocalNewVerificationTag{ 0x55667788 };

		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAndGetVerificationTag(a, z);

		// A restarted peer sends an INIT, and A replies with an INIT-ACK whose
		// State Cookie carries the Tie-Tag of its TCB.
		std::vector<uint8_t> initBuffer(a.sctpOptions.mtu);

		const auto initPacket = buildPacket(initBuffer, 0);
		auto* const initChunk = initPacket->BuildChunkInPlace<RTC::SCTP::InitChunk>();

		initChunk->SetInitiateTag(PeerNewVerificationTag);
		initChunk->SetAdvertisedReceiverWindowCredit(65535);
		initChunk->SetNumberOfOutboundStreams(10);
		initChunk->SetNumberOfInboundStreams(10);
		initChunk->SetInitialTsn(5000);
		initChunk->Consolidate();

		injectPacket(a, initPacket.get());

		const auto initAckBuffer = a.listener.ConsumeFirstSentPacket();
		const auto initAckPacket = parsePacket(initAckBuffer);

		REQUIRE(initAckPacket);

		const auto* const initAckChunk = initAckPacket->GetFirstChunkOfType<RTC::SCTP::InitAckChunk>();

		REQUIRE(initAckChunk);

		const auto* const stateCookieParameter =
		  initAckChunk->GetFirstParameterOfType<RTC::SCTP::StateCookieParameter>();

		REQUIRE(stateCookieParameter);

		const std::unique_ptr<RTC::SCTP::StateCookie> initAckCookie{ RTC::SCTP::StateCookie::Parse(
			stateCookieParameter->GetCookie(), stateCookieParameter->GetCookieLength()) };

		REQUIRE(initAckCookie);

		const uint64_t tieTag = initAckCookie->GetTieTag();

		REQUIRE(tieTag != 0);

		// A has no outstanding data, so it sends SHUTDOWN and enters
		// SHUTDOWN_SENT state with the T2-shutdown timer running.
		a.association.Shutdown();

		REQUIRE(a.listener.HasSentPackets() == true);

		a.listener.ConsumeFirstSentPacket();

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// The restarted peer sends a COOKIE-ECHO with new tags and the Tie-Tag of
		// A's TCB (RFC 9260 section 5.2.4 action A).
		const RTC::SCTP::Capabilities remoteCapabilities;

		std::vector<uint8_t> cookieBuffer(RTC::SCTP::StateCookie::StateCookieLength);

		RTC::SCTP::StateCookie::Write(
		  cookieBuffer.data(),
		  cookieBuffer.size(),
		  /*localVerificationTag*/ LocalNewVerificationTag,
		  /*remoteVerificationTag*/ PeerNewVerificationTag,
		  /*localInitialTsn*/ 1000,
		  /*remoteInitialTsn*/ 5000,
		  /*remoteAdvertisedReceiverWindowCredit*/ 65535,
		  tieTag,
		  remoteCapabilities);

		std::vector<uint8_t> cookieEchoBuffer(a.sctpOptions.mtu);

		const auto cookieEchoPacket = buildPacket(cookieEchoBuffer, LocalNewVerificationTag);
		auto* const cookieEchoChunk = cookieEchoPacket->BuildChunkInPlace<RTC::SCTP::CookieEchoChunk>();

		cookieEchoChunk->SetCookie(cookieBuffer.data(), cookieBuffer.size());
		cookieEchoChunk->Consolidate();

		injectPacket(a, cookieEchoPacket.get());

		// A keeps shutting down, as the application requested.
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// And A replies with an ERROR chunk with a Cookie Received While Shutting
		// Down error cause.
		const auto replyBuffer = a.listener.ConsumeFirstSentPacket();
		const auto replyPacket = parsePacket(replyBuffer);

		REQUIRE(replyPacket);

		const auto* const operationErrorChunk =
		  replyPacket->GetFirstChunkOfType<RTC::SCTP::OperationErrorChunk>();

		REQUIRE(operationErrorChunk);

		const auto* const errorCause = operationErrorChunk->GetErrorCauseAt(0);

		REQUIRE(errorCause);
		REQUIRE(
		  errorCause->GetCode() ==
		  RTC::SCTP::ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN);
	}

	SECTION(
	  "COOKIE-ECHO of simultaneous associations in SHUTDOWN_ACK_SENT state doesn't establish the association")
	{
		constexpr uint32_t PeerNewVerificationTag{ 0x99AABBCC };

		AssociationUnderTest a;
		AssociationUnderTest z;

		const uint32_t localVerificationTag = connectAndGetVerificationTag(a, z);

		z.association.Shutdown();
		// A reads SHUTDOWN, produces SHUTDOWN-ACK and enters SHUTDOWN_ACK_SENT
		// state with the T2-shutdown timer running.
		deliverFirstSentPacket(z, a);

		REQUIRE(
		  packetHasSingleChunkOfType(
		    RTC::SCTP::Chunk::ChunkType::SHUTDOWN_ACK, a.listener.ConsumeFirstSentPacket()) == true);
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// A COOKIE-ECHO whose packet carries A's local verification tag but whose
		// State Cookie carries a remote verification tag other than the one of
		// A's TCB (RFC 9260 section 5.2.4 action B).
		const RTC::SCTP::Capabilities remoteCapabilities;

		std::vector<uint8_t> cookieBuffer(RTC::SCTP::StateCookie::StateCookieLength);

		RTC::SCTP::StateCookie::Write(
		  cookieBuffer.data(),
		  cookieBuffer.size(),
		  /*localVerificationTag*/ localVerificationTag,
		  /*remoteVerificationTag*/ PeerNewVerificationTag,
		  /*localInitialTsn*/ 1000,
		  /*remoteInitialTsn*/ 5000,
		  /*remoteAdvertisedReceiverWindowCredit*/ 65535,
		  /*tieTag*/ 0,
		  remoteCapabilities);

		std::vector<uint8_t> cookieEchoBuffer(a.sctpOptions.mtu);

		const auto cookieEchoPacket = buildPacket(cookieEchoBuffer, localVerificationTag);
		auto* const cookieEchoChunk = cookieEchoPacket->BuildChunkInPlace<RTC::SCTP::CookieEchoChunk>();

		cookieEchoChunk->SetCookie(cookieBuffer.data(), cookieBuffer.size());
		cookieEchoChunk->Consolidate();

		injectPacket(a, cookieEchoPacket.get());

		// A keeps shutting down.
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// And A replies with an ERROR chunk with a Cookie Received While Shutting
		// Down error cause.
		const auto replyBuffer = a.listener.ConsumeFirstSentPacket();
		const auto replyPacket = parsePacket(replyBuffer);

		REQUIRE(replyPacket);

		const auto* const operationErrorChunk =
		  replyPacket->GetFirstChunkOfType<RTC::SCTP::OperationErrorChunk>();

		REQUIRE(operationErrorChunk);

		const auto* const errorCause = operationErrorChunk->GetErrorCauseAt(0);

		REQUIRE(errorCause);
		REQUIRE(
		  errorCause->GetCode() ==
		  RTC::SCTP::ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN);
	}

	SECTION(
	  "COOKIE-ECHO from a restarted peer in SHUTDOWN_PENDING state doesn't establish the association")
	{
		constexpr uint32_t PeerNewVerificationTag{ 0x11223344 };
		constexpr uint32_t LocalNewVerificationTag{ 0x55667788 };

		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAndGetVerificationTag(a, z);

		// A restarted peer sends an INIT, and A replies with an INIT-ACK whose
		// State Cookie carries the Tie-Tag of its TCB.
		std::vector<uint8_t> initBuffer(a.sctpOptions.mtu);

		const auto initPacket = buildPacket(initBuffer, 0);
		auto* const initChunk = initPacket->BuildChunkInPlace<RTC::SCTP::InitChunk>();

		initChunk->SetInitiateTag(PeerNewVerificationTag);
		initChunk->SetAdvertisedReceiverWindowCredit(65535);
		initChunk->SetNumberOfOutboundStreams(10);
		initChunk->SetNumberOfInboundStreams(10);
		initChunk->SetInitialTsn(5000);
		initChunk->Consolidate();

		injectPacket(a, initPacket.get());

		const auto initAckBuffer = a.listener.ConsumeFirstSentPacket();
		const auto initAckPacket = parsePacket(initAckBuffer);

		REQUIRE(initAckPacket);

		const auto* const initAckChunk = initAckPacket->GetFirstChunkOfType<RTC::SCTP::InitAckChunk>();

		REQUIRE(initAckChunk);

		const auto* const stateCookieParameter =
		  initAckChunk->GetFirstParameterOfType<RTC::SCTP::StateCookieParameter>();

		REQUIRE(stateCookieParameter);

		const std::unique_ptr<RTC::SCTP::StateCookie> initAckCookie{ RTC::SCTP::StateCookie::Parse(
			stateCookieParameter->GetCookie(), stateCookieParameter->GetCookieLength()) };

		REQUIRE(initAckCookie);

		const uint64_t tieTag = initAckCookie->GetTieTag();

		REQUIRE(tieTag != 0);

		// A sends a message that Z never receives, so it is outstanding.
		sendMessage(a, /*streamId*/ 1, /*ppid*/ 51, { 1, 2, 3, 4 });

		a.listener.ConsumeFirstSentPacket();

		// A has outstanding data, so it enters SHUTDOWN_PENDING state.
		a.association.Shutdown();

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// The restarted peer sends a COOKIE-ECHO with new tags and the Tie-Tag of
		// A's TCB (RFC 9260 section 5.2.4 action A).
		const RTC::SCTP::Capabilities remoteCapabilities;

		std::vector<uint8_t> cookieBuffer(RTC::SCTP::StateCookie::StateCookieLength);

		RTC::SCTP::StateCookie::Write(
		  cookieBuffer.data(),
		  cookieBuffer.size(),
		  /*localVerificationTag*/ LocalNewVerificationTag,
		  /*remoteVerificationTag*/ PeerNewVerificationTag,
		  /*localInitialTsn*/ 1000,
		  /*remoteInitialTsn*/ 5000,
		  /*remoteAdvertisedReceiverWindowCredit*/ 65535,
		  tieTag,
		  remoteCapabilities);

		std::vector<uint8_t> cookieEchoBuffer(a.sctpOptions.mtu);

		const auto cookieEchoPacket = buildPacket(cookieEchoBuffer, LocalNewVerificationTag);
		auto* const cookieEchoChunk = cookieEchoPacket->BuildChunkInPlace<RTC::SCTP::CookieEchoChunk>();

		cookieEchoChunk->SetCookie(cookieBuffer.data(), cookieBuffer.size());
		cookieEchoChunk->Consolidate();

		injectPacket(a, cookieEchoPacket.get());

		// A keeps shutting down, as the application requested.
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::SHUTTING_DOWN);

		// And A replies with an ERROR chunk with a Cookie Received While Shutting
		// Down error cause.
		const auto replyBuffer = a.listener.ConsumeFirstSentPacket();
		const auto replyPacket = parsePacket(replyBuffer);

		REQUIRE(replyPacket);

		const auto* const operationErrorChunk =
		  replyPacket->GetFirstChunkOfType<RTC::SCTP::OperationErrorChunk>();

		REQUIRE(operationErrorChunk);

		const auto* const errorCause = operationErrorChunk->GetErrorCauseAt(0);

		REQUIRE(errorCause);
		REQUIRE(
		  errorCause->GetCode() ==
		  RTC::SCTP::ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN);
	}

	SECTION("COOKIE-ECHO doesn't bring a closed association back to life")
	{
		constexpr uint32_t ForgedVerificationTag{ 0xDEADBEEF };

		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		a.association.Close();

		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.IsClosed() == true);

		// Drop the ABORT sent to Z.
		a.listener.ConsumeFirstSentPacket();

		// A (not authenticated) State Cookie whose local verification tag matches
		// the one of the packet carrying it.
		const RTC::SCTP::Capabilities remoteCapabilities;

		std::vector<uint8_t> cookieBuffer(RTC::SCTP::StateCookie::StateCookieLength);

		RTC::SCTP::StateCookie::Write(
		  cookieBuffer.data(),
		  cookieBuffer.size(),
		  /*localVerificationTag*/ ForgedVerificationTag,
		  /*remoteVerificationTag*/ 0xCAFEBABE,
		  /*localInitialTsn*/ 1000,
		  /*remoteInitialTsn*/ 2000,
		  /*remoteAdvertisedReceiverWindowCredit*/ 65535,
		  /*tieTag*/ 0,
		  remoteCapabilities);

		std::vector<uint8_t> cookieEchoBuffer(a.sctpOptions.mtu);

		const auto cookieEchoPacket = buildPacket(cookieEchoBuffer, ForgedVerificationTag);
		auto* const cookieEchoChunk = cookieEchoPacket->BuildChunkInPlace<RTC::SCTP::CookieEchoChunk>();

		cookieEchoChunk->SetCookie(cookieBuffer.data(), cookieBuffer.size());
		cookieEchoChunk->Consolidate();

		injectPacket(a, cookieEchoPacket.get());

		// A stays closed and doesn't reply.
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.HasSentPackets() == false);
	}

	SECTION("chunks bundled after an invalid COOKIE-ECHO are not processed")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		// A packet with a wrong verification tag carrying a COOKIE-ECHO chunk with
		// an invalid State Cookie and an ABORT chunk after it.
		std::vector<uint8_t> buffer(a.sctpOptions.mtu);

		const auto packet           = buildPacket(buffer, 0x12345678);
		auto* const cookieEchoChunk = packet->BuildChunkInPlace<RTC::SCTP::CookieEchoChunk>();
		const std::vector<uint8_t> cookie(16, 0xAB);

		cookieEchoChunk->SetCookie(cookie.data(), cookie.size());
		cookieEchoChunk->Consolidate();

		const auto* const abortAssociationChunk =
		  packet->BuildChunkInPlace<RTC::SCTP::AbortAssociationChunk>();

		abortAssociationChunk->Consolidate();

		injectPacket(a, packet.get());

		// The ABORT chunk is not processed, so A is still connected.
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
	}

	SECTION("COOKIE-ECHO not matching any case of RFC 9260 section 5.2.4 is discarded")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		// Get A's remote verification tag from a packet sent by A.
		sendMessage(a, /*streamId*/ 1, /*ppid*/ 51, { 1, 2, 3, 4 });

		const uint32_t remoteVerificationTag =
		  parsePacket(a.listener.ConsumeFirstSentPacket())->GetVerificationTag();

		// A (not authenticated) State Cookie with A's remote verification tag and a
		// non zero Tie-Tag other than the one of A's TCB, carried in a packet with
		// a verification tag other than A's local one. It matches none of the
		// cases of RFC 9260 section 5.2.4.
		const RTC::SCTP::Capabilities remoteCapabilities;

		std::vector<uint8_t> cookieBuffer(RTC::SCTP::StateCookie::StateCookieLength);

		RTC::SCTP::StateCookie::Write(
		  cookieBuffer.data(),
		  cookieBuffer.size(),
		  /*localVerificationTag*/ 0x12345678,
		  /*remoteVerificationTag*/ remoteVerificationTag,
		  /*localInitialTsn*/ 1000,
		  /*remoteInitialTsn*/ 2000,
		  /*remoteAdvertisedReceiverWindowCredit*/ 65535,
		  /*tieTag*/ 1234,
		  remoteCapabilities);

		std::vector<uint8_t> buffer(a.sctpOptions.mtu);

		const auto packet           = buildPacket(buffer, 0x12345678);
		auto* const cookieEchoChunk = packet->BuildChunkInPlace<RTC::SCTP::CookieEchoChunk>();

		cookieEchoChunk->SetCookie(cookieBuffer.data(), cookieBuffer.size());
		cookieEchoChunk->Consolidate();

		const auto* const abortAssociationChunk =
		  packet->BuildChunkInPlace<RTC::SCTP::AbortAssociationChunk>();

		abortAssociationChunk->Consolidate();

		injectPacket(a, packet.get());

		// The COOKIE-ECHO is silently discarded, so A doesn't reply with a
		// COOKIE-ACK, and the ABORT chunk after it is not processed.
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);
		REQUIRE(a.listener.HasSentPackets() == false);
	}

	SECTION("INIT with zero outbound streams doesn't close an established association")
	{
		constexpr uint32_t InitiateTag{ 0x11223344 };

		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		std::vector<uint8_t> buffer(a.sctpOptions.mtu);

		const auto packet     = buildPacket(buffer, 0);
		auto* const initChunk = packet->BuildChunkInPlace<RTC::SCTP::InitChunk>();

		initChunk->SetInitiateTag(InitiateTag);
		initChunk->SetAdvertisedReceiverWindowCredit(65535);
		initChunk->SetNumberOfOutboundStreams(0);
		initChunk->SetNumberOfInboundStreams(10);
		initChunk->SetInitialTsn(5000);
		initChunk->Consolidate();

		injectPacket(a, packet.get());

		// The packet is discarded, so A is still connected.
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CONNECTED);

		// And A replies with an ABORT chunk using the Initiate Tag as the
		// Verification Tag (RFC 9260 section 3.3.2).
		const auto replyBuffer = a.listener.ConsumeFirstSentPacket();
		const auto replyPacket = parsePacket(replyBuffer);

		REQUIRE(replyPacket);
		REQUIRE(replyPacket->GetVerificationTag() == InitiateTag);

		const auto* const abortAssociationChunk =
		  replyPacket->GetFirstChunkOfType<RTC::SCTP::AbortAssociationChunk>();

		REQUIRE(abortAssociationChunk);
		REQUIRE(abortAssociationChunk->GetT() == false);
	}

	SECTION("INIT with zero outbound streams in CLOSED state doesn't notify the closure again")
	{
		AssociationUnderTest a;
		AssociationUnderTest z;

		connectAssociations(a, z);

		a.association.Close();

		REQUIRE(a.listener.IsClosed() == true);
		REQUIRE(a.listener.CountOnAssociationClosedCalls() == 1);
		REQUIRE(a.listener.GetClosedErrorKind() == RTC::SCTP::Types::ErrorKind::SUCCESS);

		std::vector<uint8_t> buffer(a.sctpOptions.mtu);

		const auto packet     = buildPacket(buffer, 0);
		auto* const initChunk = packet->BuildChunkInPlace<RTC::SCTP::InitChunk>();

		initChunk->SetInitiateTag(0x11223344);
		initChunk->SetAdvertisedReceiverWindowCredit(65535);
		initChunk->SetNumberOfOutboundStreams(0);
		initChunk->SetNumberOfInboundStreams(10);
		initChunk->SetInitialTsn(5000);
		initChunk->Consolidate();

		injectPacket(a, packet.get());

		// The closure is not notified again.
		REQUIRE(a.association.GetAssociationState() == RTC::SCTP::Types::AssociationState::CLOSED);
		REQUIRE(a.listener.CountOnAssociationClosedCalls() == 1);
		REQUIRE(a.listener.HasFailed() == false);
		REQUIRE(a.listener.GetClosedErrorKind() == RTC::SCTP::Types::ErrorKind::SUCCESS);
	}
}
