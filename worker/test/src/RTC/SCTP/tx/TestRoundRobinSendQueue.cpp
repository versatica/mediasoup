#include "common.hpp"
#include "mocks/include/RTC/SCTP/public/MockAssociationListener.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "RTC/SCTP/tx/RoundRobinSendQueue.hpp"
#include "RTC/SCTP/tx/SendQueueInterface.hpp"
#include <catch2/catch_test_macros.hpp>
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
		RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		REQUIRE(q.IsEmpty());
		REQUIRE(q.Produce(NowMs, OneFragmentPacketLength).has_value() == false);
	}

	SECTION("add and get single chunk")
	{
		RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);

		q.Add(NowMs, RTC::SCTP::Message(StreamId, Ppid, { 1, 2, 4, 5, 6 }));

		REQUIRE(!q.IsEmpty());

		std::optional<RTC::SCTP::SendQueueInterface::DataToSend> dataToSend =
		  q.Produce(NowMs, OneFragmentPacketLength);

		REQUIRE(dataToSend.has_value());
		REQUIRE(dataToSend->data.IsBeginning());
		REQUIRE(dataToSend->data.IsEnd());
	}

	SECTION("carve out beginning middle and end")
	{
		RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::RoundRobinSendQueue q(
		  associationListener, Mtu, DefaultPriority, BufferedAmountLowThreshold);
		std::vector<uint8_t> payload(60);

		q.Add(NowMs, RTC::SCTP::Message(StreamId, Ppid, payload));

		std::optional<RTC::SCTP::SendQueueInterface::DataToSend> dataToSendBeg =
		  q.Produce(NowMs, /*maxLength*/ 20);

		REQUIRE(dataToSendBeg.has_value());
		REQUIRE(dataToSendBeg->data.IsBeginning());
		REQUIRE(!dataToSendBeg->data.IsEnd());

		// TODO: SCTP: more.
	}

	// TODO: SCTP: more tests.
}
