#include "common.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/tx/RoundRobinSendQueue.hpp"
#include "mocks/include/RTC/SCTP/association/MockAssociationListener.hpp"
#include <catch2/catch_test_macros.hpp>
#include <ranges>
#include <vector>

SCENARIO("SCTP RoundRobinSendQueue", "[sctp][roundrobinsendqueue]")
{
	constexpr size_t Mtu{ 1100 };
	constexpr uint64_t NowMs{ 0 };
	constexpr uint16_t StreamId{ 1 };
	constexpr uint32_t Ppid{ 53 };
	constexpr uint16_t DefaultPriority{ 10 };
	constexpr size_t BufferedAmountLowThreshold{ 500 };
	constexpr size_t OneFragmentPacketLength{ 100 };
	constexpr size_t TwoFragmentPacketLength{ 101 };

	SECTION("empty buffer")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		REQUIRE(q.IsEmpty());
		REQUIRE(q.Produce(NowMs, OneFragmentPacketLength).has_value() == false);
	}

	SECTION("add and get single chunk")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, { 1, 2, 4, 5, 6 }));

		REQUIRE(!q.IsEmpty());

		const auto dataToSend = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSend.has_value());
		REQUIRE(dataToSend->data.IsBeginning());
		REQUIRE(dataToSend->data.IsEnd());
	}

	SECTION("carve out beginning middle and end")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(60);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		const auto dataToSendBeg = q.Produce(NowMs, /*maxLength*/ 20);

		REQUIRE(dataToSendBeg.has_value());
		REQUIRE(dataToSendBeg->data.IsBeginning());
		REQUIRE(!dataToSendBeg->data.IsEnd());

		const auto dataToSendMid = q.Produce(NowMs, /*maxLength*/ 20);

		REQUIRE(dataToSendMid.has_value());
		REQUIRE(!dataToSendMid->data.IsBeginning());
		REQUIRE(!dataToSendMid->data.IsEnd());

		const auto dataToSendEnd = q.Produce(NowMs, /*maxLength*/ 20);

		REQUIRE(dataToSendEnd.has_value());
		REQUIRE(!dataToSendEnd->data.IsBeginning());
		REQUIRE(dataToSendEnd->data.IsEnd());

		REQUIRE(q.Produce(NowMs, OneFragmentPacketLength).has_value() == false);
	}

	SECTION("get chunks from two messages")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(60);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));
		q.AddMessage(NowMs, RTC::SCTP::Message(3, 54, payload));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == StreamId);
		REQUIRE(dataToSendOne->data.GetPayloadProtocolId() == Ppid);
		REQUIRE(dataToSendOne->data.IsBeginning());
		REQUIRE(dataToSendOne->data.IsEnd());

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamId() == 3);
		REQUIRE(dataToSendTwo->data.GetPayloadProtocolId() == 54);
		REQUIRE(dataToSendTwo->data.IsBeginning());
		REQUIRE(dataToSendTwo->data.IsEnd());
	}

	SECTION("buffer becomes full and emptied")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(600);

		REQUIRE(q.GetTotalBufferedAmount() < 1000);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		REQUIRE(q.GetTotalBufferedAmount() < 1000);

		q.AddMessage(NowMs, RTC::SCTP::Message(3, 54, payload));

		REQUIRE(q.GetTotalBufferedAmount() >= 1000);

		q.AddMessage(NowMs, RTC::SCTP::Message(5, 55, payload));

		REQUIRE(q.GetTotalBufferedAmount() >= 1000);

		auto dataToSendOne = q.Produce(NowMs, 1000);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == StreamId);
		REQUIRE(dataToSendOne->data.GetPayloadProtocolId() == Ppid);

		REQUIRE(q.GetTotalBufferedAmount() >= 1000);

		auto dataToSendTwo = q.Produce(NowMs, 1000);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamId() == 3);
		REQUIRE(dataToSendTwo->data.GetPayloadProtocolId() == 54);

		REQUIRE(q.GetTotalBufferedAmount() < 1000);
		REQUIRE(!q.IsEmpty());

		auto dataToSendThree = q.Produce(NowMs, 1000);

		REQUIRE(dataToSendThree.has_value());
		REQUIRE(dataToSendThree->data.GetStreamId() == 5);
		REQUIRE(dataToSendThree->data.GetPayloadProtocolId() == 55);

		REQUIRE(q.GetTotalBufferedAmount() < 1000);
		REQUIRE(q.IsEmpty());
	}

	SECTION("defaults to ordered send")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(20);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(!dataToSendOne->data.IsUnordered());

		RTC::SCTP::SendMessageOptions options;

		options.unordered = true;

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload), options);

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.IsUnordered());
	}

	SECTION("produce with lifetime expiry")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(20);

		uint64_t now = NowMs;

		q.AddMessage(now, RTC::SCTP::Message(StreamId, Ppid, payload));

		now += 1000000;

		REQUIRE(q.Produce(now, OneFragmentPacketLength).has_value());

		RTC::SCTP::SendMessageOptions expires2s;

		expires2s.lifetimeMs = 2000;

		q.AddMessage(now, RTC::SCTP::Message(StreamId, Ppid, payload), expires2s);

		now += 2000;

		REQUIRE(q.Produce(now, OneFragmentPacketLength).has_value());

		q.AddMessage(now, RTC::SCTP::Message(StreamId, Ppid, payload), expires2s);

		now += 2001;

		REQUIRE(!q.Produce(now, OneFragmentPacketLength).has_value());

		q.AddMessage(now, RTC::SCTP::Message(StreamId, Ppid, payload), expires2s);

		now += 1000000;

		REQUIRE(!q.Produce(now, OneFragmentPacketLength).has_value());

		RTC::SCTP::SendMessageOptions expires4s;

		expires4s.lifetimeMs = 4000;

		q.AddMessage(now, RTC::SCTP::Message(StreamId, Ppid, payload), expires2s);
		q.AddMessage(now, RTC::SCTP::Message(StreamId, Ppid, payload), expires4s);

		now += 2001;

		REQUIRE(q.Produce(now, OneFragmentPacketLength).has_value());
		REQUIRE(!q.Produce(now, OneFragmentPacketLength).has_value());
	}

	SECTION("discard partial packets")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(120);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));
		q.AddMessage(NowMs, RTC::SCTP::Message(2, 54, payload));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(!dataToSendOne->data.IsEnd());
		REQUIRE(dataToSendOne->data.GetStreamId() == StreamId);

		q.Discard(dataToSendOne->data.GetStreamId(), dataToSendOne->outgoingMessageId);

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(!dataToSendTwo->data.IsEnd());
		REQUIRE(dataToSendTwo->data.GetStreamId() == 2);

		const auto dataToSendThree = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendThree.has_value());
		REQUIRE(dataToSendThree->data.IsEnd());
		REQUIRE(dataToSendThree->data.GetStreamId() == 2);

		REQUIRE(!q.Produce(NowMs, OneFragmentPacketLength).has_value());

		q.Discard(dataToSendOne->data.GetStreamId(), dataToSendOne->outgoingMessageId);

		REQUIRE(!q.Produce(NowMs, OneFragmentPacketLength).has_value());
	}

	SECTION("prepare reset streams discards stream")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, { 1, 2, 3 }));
		q.AddMessage(NowMs, RTC::SCTP::Message(2, 54, { 1, 2, 3, 4, 5 }));

		REQUIRE(q.GetTotalBufferedAmount() == 8);

		q.PrepareResetStream(StreamId);

		REQUIRE(q.GetTotalBufferedAmount() == 5);

		const auto streamsReadyToBeReset = q.GetStreamsReadyToBeReset();

		REQUIRE(streamsReadyToBeReset.size() == 1);

		REQUIRE(std::ranges::find(streamsReadyToBeReset, StreamId) != streamsReadyToBeReset.end());

		q.CommitResetStreams();
		q.PrepareResetStream(2);

		REQUIRE(q.GetTotalBufferedAmount() == 0);
	}

	SECTION("prepare reset streams not partial packets")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(120);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));
		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		const auto dataToSendOne = q.Produce(NowMs, 50);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == StreamId);

		REQUIRE(q.GetTotalBufferedAmount() == (2 * payload.size()) - 50);

		q.PrepareResetStream(StreamId);

		REQUIRE(q.GetTotalBufferedAmount() == payload.size() - 50);
	}

	SECTION("enqueued items are paused during stream reset")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(50);

		q.PrepareResetStream(StreamId);

		REQUIRE(q.GetTotalBufferedAmount() == 0);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		REQUIRE(q.GetTotalBufferedAmount() == payload.size());

		REQUIRE(!q.Produce(NowMs, OneFragmentPacketLength).has_value());

		REQUIRE(q.HasStreamsReadyToBeReset());

		const auto streamsReadyToBeReset = q.GetStreamsReadyToBeReset();

		REQUIRE(streamsReadyToBeReset.size() == 1);
		REQUIRE(std::ranges::find(streamsReadyToBeReset, StreamId) != streamsReadyToBeReset.end());

		REQUIRE(!q.Produce(NowMs, OneFragmentPacketLength).has_value());

		q.CommitResetStreams();

		REQUIRE(q.GetTotalBufferedAmount() == payload.size());

		const auto dataToSendOne = q.Produce(NowMs, 50);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == StreamId);

		REQUIRE(q.GetTotalBufferedAmount() == 0);
	}

	SECTION("paused streams still send partial messages until end")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const size_t payloadLength  = 100;
		const size_t fragmentLength = 50;

		const std::vector<uint8_t> payload(payloadLength);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));
		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		const auto dataToSendOne = q.Produce(NowMs, fragmentLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == StreamId);

		REQUIRE(q.GetTotalBufferedAmount() == (2 * payloadLength) - fragmentLength);

		q.PrepareResetStream(StreamId);

		REQUIRE(q.GetTotalBufferedAmount() == payloadLength - fragmentLength);

		const auto dataToSendTwo = q.Produce(NowMs, fragmentLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamId() == StreamId);

		REQUIRE(q.GetTotalBufferedAmount() == 0);

		REQUIRE(!q.Produce(NowMs, fragmentLength).has_value());
	}

	SECTION("committing resets SSN")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(50);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));
		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamSequenceNumber() == 0);

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamSequenceNumber() == 1);

		q.PrepareResetStream(StreamId);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		REQUIRE(q.HasStreamsReadyToBeReset());

		const auto streamsReadyToBeReset = q.GetStreamsReadyToBeReset();

		REQUIRE(streamsReadyToBeReset.size() == 1);
		REQUIRE(std::ranges::find(streamsReadyToBeReset, StreamId) != streamsReadyToBeReset.end());

		q.CommitResetStreams();

		const auto dataToSendThree = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendThree.has_value());
		REQUIRE(dataToSendThree->data.GetStreamSequenceNumber() == 0);
	}

	SECTION("committing does not reset message id")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(50);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamSequenceNumber() == 0);
		REQUIRE(dataToSendOne->outgoingMessageId == 0);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamSequenceNumber() == 1);
		REQUIRE(dataToSendTwo->outgoingMessageId == 1);

		q.PrepareResetStream(StreamId);

		const auto streamsReadyToBeReset = q.GetStreamsReadyToBeReset();

		REQUIRE(streamsReadyToBeReset.size() == 1);
		REQUIRE(std::ranges::find(streamsReadyToBeReset, StreamId) != streamsReadyToBeReset.end());

		q.CommitResetStreams();

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		const auto dataToSendThree = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendThree.has_value());
		REQUIRE(dataToSendThree->data.GetStreamSequenceNumber() == 0);
		REQUIRE(dataToSendThree->outgoingMessageId == 2);
	}

	SECTION("committing resets SSN for paused streams only")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(50);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, payload));
		q.AddMessage(NowMs, RTC::SCTP::Message(3, Ppid, payload));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == 1);
		REQUIRE(dataToSendOne->data.GetStreamSequenceNumber() == 0);

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamId() == 3);
		REQUIRE(dataToSendTwo->data.GetStreamSequenceNumber() == 0);

		q.PrepareResetStream(3);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, payload));
		q.AddMessage(NowMs, RTC::SCTP::Message(3, Ppid, payload));

		const auto streamsReadyToBeReset = q.GetStreamsReadyToBeReset();

		REQUIRE(streamsReadyToBeReset.size() == 1);
		REQUIRE(std::ranges::find(streamsReadyToBeReset, 3) != streamsReadyToBeReset.end());

		q.CommitResetStreams();

		const auto dataToSendThree = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendThree.has_value());
		REQUIRE(dataToSendThree->data.GetStreamId() == 1);
		REQUIRE(dataToSendThree->data.GetStreamSequenceNumber() == 1);

		const auto dataToSendFour = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendFour.has_value());
		REQUIRE(dataToSendFour->data.GetStreamId() == 3);
		REQUIRE(dataToSendFour->data.GetStreamSequenceNumber() == 0);
	}

	SECTION("rollback resumes SSN")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(50);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));
		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamSequenceNumber() == 0);

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamSequenceNumber() == 1);

		q.PrepareResetStream(1);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		const auto streamsReadyToBeReset = q.GetStreamsReadyToBeReset();

		REQUIRE(streamsReadyToBeReset.size() == 1);
		REQUIRE(std::ranges::find(streamsReadyToBeReset, 1) != streamsReadyToBeReset.end());

		q.RollbackResetStreams();

		const auto dataToSendThree = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendThree.has_value());
		REQUIRE(dataToSendThree->data.GetStreamSequenceNumber() == 2);
	}

	SECTION("returns fragments for one message before moving to next")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(200);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, payload));
		q.AddMessage(NowMs, RTC::SCTP::Message(2, Ppid, payload));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == 1);

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamId() == 1);

		const auto dataToSendThree = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendThree.has_value());
		REQUIRE(dataToSendThree->data.GetStreamId() == 2);

		const auto dataToSendFour = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendFour.has_value());
		REQUIRE(dataToSendFour->data.GetStreamId() == 2);
	}

	SECTION("returns also small fragments before moving to next")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(TwoFragmentPacketLength);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, payload));
		q.AddMessage(NowMs, RTC::SCTP::Message(2, Ppid, payload));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == 1);
		REQUIRE(dataToSendOne->data.GetPayloadLength() == OneFragmentPacketLength);

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamId() == 1);
		REQUIRE(
		  dataToSendTwo->data.GetPayloadLength() == TwoFragmentPacketLength - OneFragmentPacketLength);

		const auto dataToSendThree = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendThree.has_value());
		REQUIRE(dataToSendThree->data.GetStreamId() == 2);
		REQUIRE(dataToSendThree->data.GetPayloadLength() == OneFragmentPacketLength);

		const auto dataToSendFour = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendFour.has_value());
		REQUIRE(dataToSendFour->data.GetStreamId() == 2);
		REQUIRE(
		  dataToSendFour->data.GetPayloadLength() == TwoFragmentPacketLength - OneFragmentPacketLength);
	}

	SECTION("will cycle in round robin fashion between streams")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, std::vector<uint8_t>(1)));
		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, std::vector<uint8_t>(2)));
		q.AddMessage(NowMs, RTC::SCTP::Message(2, Ppid, std::vector<uint8_t>(3)));
		q.AddMessage(NowMs, RTC::SCTP::Message(2, Ppid, std::vector<uint8_t>(4)));
		q.AddMessage(NowMs, RTC::SCTP::Message(3, Ppid, std::vector<uint8_t>(5)));
		q.AddMessage(NowMs, RTC::SCTP::Message(3, Ppid, std::vector<uint8_t>(6)));
		q.AddMessage(NowMs, RTC::SCTP::Message(4, Ppid, std::vector<uint8_t>(7)));
		q.AddMessage(NowMs, RTC::SCTP::Message(4, Ppid, std::vector<uint8_t>(8)));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == 1);
		REQUIRE(dataToSendOne->data.GetPayloadLength() == 1);

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamId() == 2);
		REQUIRE(dataToSendTwo->data.GetPayloadLength() == 3);

		const auto dataToSendThree = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendThree.has_value());
		REQUIRE(dataToSendThree->data.GetStreamId() == 3);
		REQUIRE(dataToSendThree->data.GetPayloadLength() == 5);

		const auto dataToSendFour = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendFour.has_value());
		REQUIRE(dataToSendFour->data.GetStreamId() == 4);
		REQUIRE(dataToSendFour->data.GetPayloadLength() == 7);

		const auto dataToSendFive = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendFive.has_value());
		REQUIRE(dataToSendFive->data.GetStreamId() == 1);
		REQUIRE(dataToSendFive->data.GetPayloadLength() == 2);

		const auto dataToSendSix = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendSix.has_value());
		REQUIRE(dataToSendSix->data.GetStreamId() == 2);
		REQUIRE(dataToSendSix->data.GetPayloadLength() == 4);

		const auto dataToSendSeven = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendSeven.has_value());
		REQUIRE(dataToSendSeven->data.GetStreamId() == 3);
		REQUIRE(dataToSendSeven->data.GetPayloadLength() == 6);

		const auto dataToSendEight = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendEight.has_value());
		REQUIRE(dataToSendEight->data.GetStreamId() == 4);
		REQUIRE(dataToSendEight->data.GetPayloadLength() == 8);
	}

	SECTION("doesn't trigger stream buffered amount low when set to zero")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.SetStreamBufferedAmountLowThreshold(1, 0);

		REQUIRE(!associationListener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1));
	}

	SECTION("triggers stream buffered amount low when sent")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, std::vector<uint8_t>(1)));

		REQUIRE(q.GetStreamBufferedAmount(1) == 1);

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(associationListener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1));
		REQUIRE(associationListener.CountOnStreamBufferedAmountLowCallsWithStreamId(1) == 1);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == 1);
		REQUIRE(dataToSendOne->data.GetPayloadLength() == 1);
		REQUIRE(q.GetStreamBufferedAmount(1) == 0);
	}

	SECTION("will retrigger stream buffered amount low if adding more")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, std::vector<uint8_t>(1)));

		REQUIRE(q.GetStreamBufferedAmount(1) == 1);

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(associationListener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1));
		REQUIRE(associationListener.CountOnStreamBufferedAmountLowCallsWithStreamId(1) == 1);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, std::vector<uint8_t>(1)));

		REQUIRE(q.GetStreamBufferedAmount(1) == 1);

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(associationListener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1));
		REQUIRE(associationListener.CountOnStreamBufferedAmountLowCallsWithStreamId(1) == 2);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == 1);
		REQUIRE(dataToSendOne->data.GetPayloadLength() == 1);
		REQUIRE(q.GetStreamBufferedAmount(1) == 0);
	}

	SECTION("only triggers stream buffered amount low when transitioning from above to below or equal")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.SetStreamBufferedAmountLowThreshold(1, 1000);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, std::vector<uint8_t>(10)));

		REQUIRE(q.GetStreamBufferedAmount(1) == 10);

		// Shouldn't trigger the event.
		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(!associationListener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1));

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == 1);
		REQUIRE(dataToSendOne->data.GetPayloadLength() == 10);
		REQUIRE(q.GetStreamBufferedAmount(1) == 0);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, std::vector<uint8_t>(20)));

		REQUIRE(q.GetStreamBufferedAmount(1) == 20);

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(!associationListener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1));

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamId() == 1);
		REQUIRE(dataToSendTwo->data.GetPayloadLength() == 20);
		REQUIRE(q.GetStreamBufferedAmount(1) == 0);
	}

	SECTION("will trigger stream buffered amount low set above zero")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.SetStreamBufferedAmountLowThreshold(1, 700);

		const std::vector<uint8_t> payload(1000);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, payload));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == 1);
		REQUIRE(dataToSendOne->data.GetPayloadLength() == OneFragmentPacketLength);
		REQUIRE(q.GetStreamBufferedAmount(1) == 900);

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetPayloadLength() == OneFragmentPacketLength);
		REQUIRE(q.GetStreamBufferedAmount(1) == 800);

		// It goes beyond 700 bytes, it should trigger the event.
		const auto dataToSendThree = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(associationListener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1));
		REQUIRE(associationListener.CountOnStreamBufferedAmountLowCallsWithStreamId(1) == 1);

		REQUIRE(dataToSendThree.has_value());
		REQUIRE(dataToSendThree->data.GetPayloadLength() == OneFragmentPacketLength);
		REQUIRE(q.GetStreamBufferedAmount(1) == 700);

		// Buffer decreases so it shouldn't emit the event.
		const auto dataToSendFour = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(associationListener.CountOnStreamBufferedAmountLowCallsWithStreamId(1) == 1);

		REQUIRE(dataToSendFour.has_value());
		REQUIRE(dataToSendFour->data.GetPayloadLength() == OneFragmentPacketLength);
		REQUIRE(q.GetStreamBufferedAmount(1) == 600);
	}

	SECTION("will retrigger stream buffered amount low set above zero")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.SetStreamBufferedAmountLowThreshold(1, 700);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, std::vector<uint8_t>(1000)));

		const auto dataToSendOne = q.Produce(NowMs, 400);

		REQUIRE(associationListener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(1));
		REQUIRE(associationListener.CountOnStreamBufferedAmountLowCallsWithStreamId(1) == 1);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == 1);
		REQUIRE(dataToSendOne->data.GetPayloadLength() == 400);
		REQUIRE(q.GetStreamBufferedAmount(1) == 600);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, std::vector<uint8_t>(200)));

		REQUIRE(q.GetStreamBufferedAmount(1) == 800);

		const auto dataToSendTwo = q.Produce(NowMs, 200);

		REQUIRE(associationListener.CountOnStreamBufferedAmountLowCallsWithStreamId(1) == 2);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamId() == 1);
		REQUIRE(dataToSendTwo->data.GetPayloadLength() == 200);
		REQUIRE(q.GetStreamBufferedAmount(1) == 600);
	}

	SECTION("triggers stream buffered amount low on threshold changed")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, std::vector<uint8_t>(100)));

		// Modifying the threshold, still under buffered_amount, should not trigger
		// event.
		q.SetStreamBufferedAmountLowThreshold(StreamId, 50);
		q.SetStreamBufferedAmountLowThreshold(StreamId, 99);

		REQUIRE(!associationListener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(StreamId));

		// When the threshold reaches buffered_amount, it will trigger event.
		q.SetStreamBufferedAmountLowThreshold(StreamId, 100);

		REQUIRE(associationListener.HasOnStreamBufferedAmountLowBeenCalledWithStreamId(StreamId));
		REQUIRE(associationListener.CountOnStreamBufferedAmountLowCallsWithStreamId(StreamId) == 1);

		// But not when it's set low again.
		q.SetStreamBufferedAmountLowThreshold(StreamId, 50);

		REQUIRE(associationListener.CountOnStreamBufferedAmountLowCallsWithStreamId(StreamId) == 1);

		// But it will trigger when it overshoots.
		q.SetStreamBufferedAmountLowThreshold(StreamId, 150);

		REQUIRE(associationListener.CountOnStreamBufferedAmountLowCallsWithStreamId(StreamId) == 2);

		// But not when it's set back to zero.
		q.SetStreamBufferedAmountLowThreshold(StreamId, 0);

		REQUIRE(associationListener.CountOnStreamBufferedAmountLowCallsWithStreamId(StreamId) == 2);
	}

	SECTION("total buffered amount low does not trigger on buffer filling up")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(BufferedAmountLowThreshold - 1);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		REQUIRE(q.GetTotalBufferedAmount() == payload.size());

		// Will not trigger if going above but never below.
		q.AddMessage(
		  NowMs, RTC::SCTP::Message(StreamId, Ppid, std::vector<uint8_t>(OneFragmentPacketLength)));

		REQUIRE(associationListener.CountOnTotalBufferedAmountLowCalls() == 0);
		REQUIRE(q.GetTotalBufferedAmount() > payload.size());
	}

	SECTION("triggers total buffered amount low when crossing")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(BufferedAmountLowThreshold);

		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		REQUIRE(q.GetTotalBufferedAmount() == payload.size());

		// Reaches it.
		q.AddMessage(NowMs, RTC::SCTP::Message(StreamId, Ppid, std::vector<uint8_t>(1)));

		// Drain it a bit, will trigger.
		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(associationListener.CountOnTotalBufferedAmountLowCalls() == 1);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(q.GetTotalBufferedAmount() < BufferedAmountLowThreshold);
	}

	SECTION("will stay in a stream as long as that message is sending")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		constexpr size_t OneFragmentPacketSize = OneFragmentPacketLength;

		q.AddMessage(NowMs, RTC::SCTP::Message(5, Ppid, std::vector<uint8_t>(1)));

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketSize);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == 5);
		REQUIRE(dataToSendOne->data.GetPayloadLength() == 1);

		// Next, it should pick a different stream.
		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, std::vector<uint8_t>(OneFragmentPacketSize * 2)));

		const auto dataToSendTwo = q.Produce(NowMs, OneFragmentPacketSize);

		REQUIRE(dataToSendTwo.has_value());
		REQUIRE(dataToSendTwo->data.GetStreamId() == 1);
		REQUIRE(dataToSendTwo->data.GetPayloadLength() == OneFragmentPacketSize);

		// It should still stay on the Stream1 now, even if might be tempted to switch
		// to this stream, as it's the stream following 5.
		q.AddMessage(NowMs, RTC::SCTP::Message(6, Ppid, std::vector<uint8_t>(1)));

		const auto dataToSendThree = q.Produce(NowMs, OneFragmentPacketSize);

		REQUIRE(dataToSendThree.has_value());
		REQUIRE(dataToSendThree->data.GetStreamId() == 1);
		REQUIRE(dataToSendThree->data.GetPayloadLength() == OneFragmentPacketSize);

		// After stream 1 message is complete, it should move to stream 6.
		const auto dataToSendFour = q.Produce(NowMs, OneFragmentPacketSize);

		REQUIRE(dataToSendFour.has_value());
		REQUIRE(dataToSendFour->data.GetStreamId() == 6);
		REQUIRE(dataToSendFour->data.GetPayloadLength() == 1);

		REQUIRE(q.Produce(NowMs, OneFragmentPacketSize).has_value() == false);
	}

	SECTION("streams have initial priority")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		REQUIRE(q.GetStreamPriority(1) == DefaultPriority);

		q.AddMessage(NowMs, RTC::SCTP::Message(2, Ppid, std::vector<uint8_t>(40)));

		REQUIRE(q.GetStreamPriority(2) == DefaultPriority);
	}

	SECTION("can change stream priority")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.SetStreamPriority(1, 42);

		REQUIRE(q.GetStreamPriority(1) == 42);

		q.AddMessage(NowMs, RTC::SCTP::Message(2, Ppid, std::vector<uint8_t>(40)));
		q.SetStreamPriority(2, 42);

		REQUIRE(q.GetStreamPriority(2) == 42);
	}

	SECTION("will send messages by priority")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.EnableMessageInterleaving(true);

		q.SetStreamPriority(1, 10);
		q.SetStreamPriority(2, 20);
		q.SetStreamPriority(3, 30);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, std::vector<uint8_t>(40)));
		q.AddMessage(NowMs, RTC::SCTP::Message(2, Ppid, std::vector<uint8_t>(20)));
		q.AddMessage(NowMs, RTC::SCTP::Message(3, Ppid, std::vector<uint8_t>(10)));

		const std::vector<uint16_t> expectedStreams = { 3, 2, 2, 1, 1, 1, 1 };

		for (const uint16_t streamId : expectedStreams)
		{
			const auto dataToSend = q.Produce(NowMs, 10);

			REQUIRE(dataToSend.has_value());
			REQUIRE(dataToSend->data.GetStreamId() == streamId);
		}

		REQUIRE(q.Produce(NowMs, 1).has_value() == false);
	}

	SECTION("will send lifecycle expire when expired in send queue")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(OneFragmentPacketLength);

		RTC::SCTP::SendMessageOptions options;

		options.lifetimeMs  = 1000;
		options.lifecycleId = 1;

		q.AddMessage(NowMs, RTC::SCTP::Message(2, Ppid, payload), options);

		REQUIRE(q.Produce(NowMs + 1001, OneFragmentPacketLength).has_value() == false);

		REQUIRE(associationListener.HasOnAssociationLifecycleMessageExpiredSentBeenCalledWithLifecycleId(
		  1, false));
		REQUIRE(associationListener.HasOnAssociationLifecycleMessageEndBeenCalledWithLifecycleId(1));
	}

	SECTION("will send lifecycle expire when discarding during pause")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(120);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, payload), { .lifecycleId = 1 });
		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, payload), { .lifecycleId = 2 });

		const auto dataToSendOne = q.Produce(NowMs, 50);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(dataToSendOne->data.GetStreamId() == 1);

		REQUIRE(q.GetTotalBufferedAmount() == (2 * payload.size()) - 50);

		q.PrepareResetStream(1);

		REQUIRE(associationListener.HasOnAssociationLifecycleMessageExpiredSentBeenCalledWithLifecycleId(
		  2, false));
		REQUIRE(associationListener.HasOnAssociationLifecycleMessageEndBeenCalledWithLifecycleId(2));

		REQUIRE(q.GetTotalBufferedAmount() == payload.size() - 50);
	}

	SECTION("will send lifecycle expire when discarding explicitly")
	{
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		const std::vector<uint8_t> payload(OneFragmentPacketLength + 20);

		q.AddMessage(NowMs, RTC::SCTP::Message(1, Ppid, payload), { .lifecycleId = 1 });

		const auto dataToSendOne = q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSendOne.has_value());
		REQUIRE(!dataToSendOne->data.IsEnd());
		REQUIRE(dataToSendOne->data.GetStreamId() == 1);

		q.Discard(dataToSendOne->data.GetStreamId(), dataToSendOne->outgoingMessageId);

		REQUIRE(associationListener.HasOnAssociationLifecycleMessageExpiredSentBeenCalledWithLifecycleId(
		  1, false));
		REQUIRE(associationListener.HasOnAssociationLifecycleMessageEndBeenCalledWithLifecycleId(1));
	}
}
