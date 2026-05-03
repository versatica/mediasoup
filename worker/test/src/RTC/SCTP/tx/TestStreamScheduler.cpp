#include "common.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/tx/SendQueueInterface.hpp"
#include "RTC/SCTP/tx/StreamScheduler.hpp"
#include <catch2/catch_test_macros.hpp>
#include <deque>
#include <map>
#include <vector>

namespace
{
	constexpr uint64_t Mtu{ 1000 };
	constexpr size_t PayloadSize{ 4 };
	constexpr uint64_t NowMs{ 0 };

	bool checkDataToSendHasMid(
	  std::optional<RTC::SCTP::SendQueueInterface::DataToSend> dataToSend, uint32_t mid)
	{
		if (!dataToSend.has_value())
		{
			return false;
		}

		if (dataToSend->data.GetMessageId() != mid)
		{
			return false;
		}

		return true;
	}

	std::function<std::optional<RTC::SCTP::SendQueueInterface::DataToSend>(uint64_t, size_t)> createChunk(
	  uint32_t outgoingMessageId, uint16_t streamId, uint32_t mid, size_t payloadLength = PayloadSize)
	{
		return
		  [streamId, mid, payloadLength, outgoingMessageId](uint64_t /*nowMs*/, size_t /*maxLength*/)
		{
			return RTC::SCTP::SendQueueInterface::DataToSend(
			  outgoingMessageId,
			  RTC::SCTP::UserData(
			    streamId,
			    /*ssn*/ 0,
			    mid,
			    /*fsn*/ 0,
			    /*ppid*/ 42,
			    std::vector<uint8_t>(payloadLength),
			    /*isBeginning*/ true,
			    /*isEnd*/ true,
			    /*unoreded*/ true));
		};
	}

	std::map<uint16_t, size_t> getPacketCounts(
	  RTC::SCTP::StreamScheduler& scheduler, size_t packetsToGenerate)
	{
		std::map</*streamId*/ uint16_t, size_t> packetCounts;

		for (size_t i{ 0 }; i < packetsToGenerate; ++i)
		{
			const std::optional<RTC::SCTP::SendQueueInterface::DataToSend> dataToSend =
			  scheduler.Produce(NowMs, Mtu);

			if (dataToSend.has_value())
			{
				++packetCounts[dataToSend->data.GetStreamId()];
			}
		}

		return packetCounts;
	}

	class MockStreamProducer : public RTC::SCTP::StreamScheduler::StreamProducer
	{
	public:
		/**
		 * Equivalent to EXPECT_CALL(producer, Produce).WillOnce(...).WillOnce(...)
		 * in dcsctp.
		 */
		void PushProduce(
		  std::function<std::optional<RTC::SCTP::SendQueueInterface::DataToSend>(uint64_t, size_t)> fn)
		{
			this->produceQueue.push_back(std::move(fn));
		}

		/**
		 * Equivalent to EXPECT_CALL(producer, GetBytesToSendInNextMessage).WillOnce(Return(n))
		 * in dcsctp.
		 */
		void PushBytesToSend(size_t bytes)
		{
			this->bytesQueue.push_back(bytes);
		}

		std::optional<RTC::SCTP::SendQueueInterface::DataToSend> Produce(uint64_t nowMs, size_t maxLength) override
		{
			REQUIRE(!produceQueue.empty());

			const auto fn = std::move(produceQueue.front());

			this->produceQueue.pop_front();

			return fn(nowMs, maxLength);
		}

		size_t GetBytesToSendInNextMessage() const override
		{
			REQUIRE(!bytesQueue.empty());

			const size_t bytes = bytesQueue.front();

			this->bytesQueue.pop_front();

			return bytes;
		}

	private:
		std::deque<std::function<std::optional<RTC::SCTP::SendQueueInterface::DataToSend>(uint64_t, size_t)>>
		  produceQueue;
		mutable std::deque<size_t> bytesQueue;
	};

	class TestStream
	{
	public:
		TestStream(
		  RTC::SCTP::StreamScheduler& scheduler,
		  uint16_t streamId,
		  uint16_t priority,
		  size_t packetSize = PayloadSize)
		{
			// Equivalent to WillRepeatedly() in dcsctp.
			for (int i{ 0 }; i < 100; ++i)
			{
				producer.PushProduce(createChunk(i, streamId, i, packetSize));
				producer.PushBytesToSend(packetSize);
			}

			// End signal.
			producer.PushBytesToSend(0);

			stream = scheduler.CreateStream(&producer, streamId, priority);
			stream->MayMakeActive();
		}

		RTC::SCTP::StreamScheduler::Stream& GetStream()
		{
			return *stream;
		}

	private:
		MockStreamProducer producer;
		std::unique_ptr<RTC::SCTP::StreamScheduler::Stream> stream;
	};
} // namespace

SCENARIO("SCTP StreamScheduler", "[sctp][streamscheduler]")
{
	SECTION("has no active streams")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	SECTION("can set and get stream properties")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		MockStreamProducer producer;
		auto stream = scheduler.CreateStream(&producer, 1, 2);

		REQUIRE(stream->GetStreamId() == 1);
		REQUIRE(stream->GetPriority() == 2);

		stream->SetPriority(0);

		REQUIRE(stream->GetPriority() == 0);
	}

	// TODO: SCTP: More tests.

	SECTION("will round-robin between streams")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		MockStreamProducer producer1;

		producer1.PushBytesToSend(PayloadSize);
		producer1.PushProduce(createChunk(0, 1, 100));
		producer1.PushBytesToSend(PayloadSize);
		producer1.PushProduce(createChunk(1, 1, 101));
		producer1.PushBytesToSend(PayloadSize);
		producer1.PushProduce(createChunk(2, 1, 102));
		producer1.PushBytesToSend(0);

		auto stream1 = scheduler.CreateStream(&producer1, 1, 2);

		stream1->MayMakeActive();

		MockStreamProducer producer2;

		producer2.PushBytesToSend(PayloadSize);
		producer2.PushProduce(createChunk(3, 2, 200));
		producer2.PushBytesToSend(PayloadSize);
		producer2.PushProduce(createChunk(4, 2, 201));
		producer2.PushBytesToSend(PayloadSize);
		producer2.PushProduce(createChunk(5, 2, 202));
		producer2.PushBytesToSend(0);

		auto stream2 = scheduler.CreateStream(&producer2, 2, 2);

		stream2->MayMakeActive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 100));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 200));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 101));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 201));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 102));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 202));
		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	// TODO: SCTP: More tests.
}
