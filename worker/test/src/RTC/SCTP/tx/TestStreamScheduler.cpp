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
	constexpr size_t PayloadLength{ 4 };
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
	  uint32_t outgoingMessageId, uint16_t streamId, uint32_t mid, size_t payloadLength = PayloadLength)
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
		 * Equivalent to EXPECT_CALL(producer, bytes_to_send_in_next_message)
		 * .WillOnce(Return(n)) in dcsctp.
		 */
		void PushBytesToSend(size_t bytes)
		{
			this->bytesQueue.push_back(bytes);
		}

		std::optional<RTC::SCTP::SendQueueInterface::DataToSend> Produce(uint64_t nowMs, size_t maxLength) override
		{
			REQUIRE(!this->produceQueue.empty());

			const auto fn = std::move(this->produceQueue.front());

			this->produceQueue.pop_front();

			return fn(nowMs, maxLength);
		}

		size_t GetBytesToSendInNextMessage() const override
		{
			REQUIRE(!this->bytesQueue.empty());

			const size_t bytes = this->bytesQueue.front();

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
		  size_t packetLength = PayloadLength)
		{
			this->producer.PushBytesToSend(packetLength); // MayMakeActive().

			// Equivalent to WillRepeatedly() in dcsctp.
			for (int i{ 0 }; i < 100; ++i)
			{
				this->producer.PushProduce(createChunk(i, streamId, i, packetLength));
				this->producer.PushBytesToSend(packetLength);
			}

			this->stream = scheduler.CreateStream(std::addressof(producer), streamId, priority);
			this->stream->MayMakeActive();
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
	// A scheduler without active streams doesn't produce data.
	SECTION("has no active streams")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	// Stream properties can be set and retrieved.
	SECTION("can set and get stream properties")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);
		MockStreamProducer producer;
		auto stream = scheduler.CreateStream(std::addressof(producer), 1, 2);

		REQUIRE(stream->GetStreamId() == 1);
		REQUIRE(stream->GetPriority() == 2);

		stream->SetPriority(0);

		REQUIRE(stream->GetPriority() == 0);
	}

	SECTION("can produce from a single stream")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);
		MockStreamProducer producer;

		producer.PushBytesToSend(PayloadLength);
		producer.PushProduce(createChunk(0, 1, 0));
		producer.PushBytesToSend(0);

		auto stream = scheduler.CreateStream(std::addressof(producer), 1, 2);

		stream->MayMakeActive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 0));
		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	// A scheduler with a single stream produced packets from it.
	SECTION("will round-robin between streams")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);
		MockStreamProducer producer1;

		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(0, 1, 100));
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(1, 1, 101));
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(2, 1, 102));
		producer1.PushBytesToSend(0);

		auto stream1 = scheduler.CreateStream(std::addressof(producer1), 1, 2);

		stream1->MayMakeActive();

		MockStreamProducer producer2;

		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(3, 2, 200));
		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(4, 2, 201));
		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(5, 2, 202));
		producer2.PushBytesToSend(0);

		auto stream2 = scheduler.CreateStream(std::addressof(producer2), 2, 2);

		stream2->MayMakeActive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 100));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 200));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 101));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 201));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 102));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 202));
		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	// Switches between two streams after every packet.
	SECTION("will round-robin between streams")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);
		MockStreamProducer producer1;

		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(0, 1, 100));
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(1, 1, 101));
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(2, 1, 102));
		producer1.PushBytesToSend(0);

		auto stream1 = scheduler.CreateStream(std::addressof(producer1), 1, 2);

		stream1->MayMakeActive();

		MockStreamProducer producer2;

		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(3, 2, 200));
		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(4, 2, 201));
		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(5, 2, 202));
		producer2.PushBytesToSend(0);

		auto stream2 = scheduler.CreateStream(std::addressof(producer2), 2, 2);

		stream2->MayMakeActive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 100));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 200));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 101));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 201));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 102));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 202));
		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	// Switches between two streams after every packet, but keeps producing from
	// the same stream when a packet contains of multiple fragments.
	SECTION("will round-robin only when finished producing chunk")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);
		MockStreamProducer producer1;

		producer1.PushBytesToSend(PayloadLength); // MayMakeActive
		producer1.PushProduce(createChunk(0, 1, 100));
		producer1.PushBytesToSend(PayloadLength);
		// MID(101) fragmented in 3 chunks:
		// 1. beginning:true, end:false
		// 2. beginning:false, end:false
		// 3. beginning:false, end:true
		producer1.PushProduce(
		  [](uint64_t, size_t)
		  {
			  return RTC::SCTP::SendQueueInterface::DataToSend(
			    1, RTC::SCTP::UserData(1, 0, 101, 0, 42, std::vector<uint8_t>(4), true, false, true));
		  });
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(
		  [](uint64_t, size_t)
		  {
			  return RTC::SCTP::SendQueueInterface::DataToSend(
			    1, RTC::SCTP::UserData(1, 0, 101, 0, 42, std::vector<uint8_t>(4), false, false, true));
		  });
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(
		  [](uint64_t, size_t)
		  {
			  return RTC::SCTP::SendQueueInterface::DataToSend(
			    1, RTC::SCTP::UserData(1, 0, 101, 0, 42, std::vector<uint8_t>(4), false, true, true));
		  });
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(2, 1, 102));
		producer1.PushBytesToSend(0);

		auto stream1 = scheduler.CreateStream(std::addressof(producer1), 1, 2);

		stream1->MayMakeActive();

		MockStreamProducer producer2;

		producer2.PushBytesToSend(PayloadLength); // MayMakeActive().
		producer2.PushProduce(createChunk(3, 2, 200));
		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(4, 2, 201));
		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(5, 2, 202));
		producer2.PushBytesToSend(0);

		auto stream2 = scheduler.CreateStream(std::addressof(producer2), 2, 2);

		stream2->MayMakeActive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 100));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 200));
		// MID(101) is fully produced before giving up on stream2.
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 101));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 101));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 101));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 201));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 102));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 202));
		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	// Resumes a paused stream - makes a stream active after inactivating it.
	SECTION("single stream can be resumed")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);
		MockStreamProducer producer1;

		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(0, 1, 100));
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(1, 1, 101));
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(2, 1, 102));
		producer1.PushBytesToSend(PayloadLength); // When making active again.
		producer1.PushBytesToSend(0);

		auto stream1 = scheduler.CreateStream(std::addressof(producer1), 1, 2);

		stream1->MayMakeActive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 100));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 101));

		stream1->MakeInactive();

		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());

		stream1->MayMakeActive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 102));
		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	// Iterates between streams, where one is suddenly paused and later resumed.
	SECTION("will round-robin with paused stream")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);
		MockStreamProducer producer1;

		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(0, 1, 100));
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(1, 1, 101));
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(2, 1, 102));
		producer1.PushBytesToSend(PayloadLength); // When making active again.
		producer1.PushBytesToSend(0);

		auto stream1 = scheduler.CreateStream(std::addressof(producer1), 1, 2);

		stream1->MayMakeActive();

		MockStreamProducer producer2;

		producer2.PushBytesToSend(PayloadLength); // MayMakeActive().
		producer2.PushProduce(createChunk(3, 2, 200));
		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(4, 2, 201));
		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(5, 2, 202));
		producer2.PushBytesToSend(0);

		auto stream2 = scheduler.CreateStream(std::addressof(producer2), 2, 2);

		stream2->MayMakeActive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 100));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 200));

		stream1->MakeInactive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 201));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 202));

		stream1->MayMakeActive();
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 101));
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 102));
		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	// Verifies that packet counts are evenly distributed in round robin
	// scheduling.
	SECTION("will distribute round-robin packets evenly between two streams")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		const TestStream stream1(scheduler, 1, 1);
		const TestStream stream2(scheduler, 2, 1);

		const auto packetCounts = getPacketCounts(scheduler, 10);

		REQUIRE(packetCounts.at(1) == 5);
		REQUIRE(packetCounts.at(2) == 5);
	}

	// Verifies that packet counts are evenly distributed among active streams,
	// where a stream is suddenly made inactive, two are added, and then the
	// paused stream is resumed.
	SECTION("will distribute evenly with paused and added streams")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		const TestStream stream1(scheduler, 1, 1);
		TestStream stream2(scheduler, 2, 1);

		const auto counts1 = getPacketCounts(scheduler, 10);

		REQUIRE(counts1.at(1) == 5);
		REQUIRE(counts1.at(2) == 5);

		stream2.GetStream().MakeInactive();

		const TestStream stream3(scheduler, 3, 1);
		const TestStream stream4(scheduler, 4, 1);

		const auto counts2 = getPacketCounts(scheduler, 15);

		REQUIRE(counts2.at(1) == 5);
		// stream2 is inative, it is not in the map.
		REQUIRE(!counts2.contains(2));
		REQUIRE(counts2.at(3) == 5);
		REQUIRE(counts2.at(4) == 5);

		stream2.GetStream().MayMakeActive();

		const auto counts3 = getPacketCounts(scheduler, 20);

		REQUIRE(counts3.at(1) == 5);
		REQUIRE(counts3.at(2) == 5);
		REQUIRE(counts3.at(3) == 5);
		REQUIRE(counts3.at(4) == 5);
	}

	// Degrades to fair queuing with streams having identical priority.
	SECTION("will do fair queuing with same priority")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		scheduler.EnableMessageInterleaving(true);

		constexpr size_t SmallPacket{ 30 };
		constexpr size_t LargePacket{ 70 };

		MockStreamProducer producer1;

		producer1.PushBytesToSend(SmallPacket); // MayMakeActive().
		producer1.PushProduce(createChunk(0, 1, 100, SmallPacket));
		producer1.PushBytesToSend(SmallPacket);
		producer1.PushProduce(createChunk(1, 1, 101, SmallPacket));
		producer1.PushBytesToSend(SmallPacket);
		producer1.PushProduce(createChunk(2, 1, 102, SmallPacket));
		producer1.PushBytesToSend(0);

		auto stream1 = scheduler.CreateStream(std::addressof(producer1), 1, 2);

		stream1->MayMakeActive();

		MockStreamProducer producer2;

		producer2.PushBytesToSend(LargePacket); // MayMakeActive().
		producer2.PushProduce(createChunk(3, 2, 200, LargePacket));
		producer2.PushBytesToSend(LargePacket);
		producer2.PushProduce(createChunk(4, 2, 201, LargePacket));
		producer2.PushBytesToSend(LargePacket);
		producer2.PushProduce(createChunk(5, 2, 202, LargePacket));
		producer2.PushBytesToSend(0);

		auto stream2 = scheduler.CreateStream(std::addressof(producer2), 2, 2);

		stream2->MayMakeActive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 100)); // t = 30
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 101)); // t = 60
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 200)); // t = 70
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 102)); // t = 90
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 201)); // t = 140
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 202)); // t = 210
		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	// Will do weighted fair queuing with three streams having different priority.
	SECTION("will do weighted fair queuing with same size and different priority")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		scheduler.EnableMessageInterleaving(true);

		MockStreamProducer producer1;

		// Priority 125 -> allowed to produce every 1000/125 ~= 80 time units.
		producer1.PushBytesToSend(PayloadLength); // MayMakeActive().
		producer1.PushProduce(createChunk(0, 1, 100));
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(1, 1, 101));
		producer1.PushBytesToSend(PayloadLength);
		producer1.PushProduce(createChunk(2, 1, 102));
		producer1.PushBytesToSend(0);

		auto stream1 = scheduler.CreateStream(std::addressof(producer1), 1, 125);

		stream1->MayMakeActive();

		MockStreamProducer producer2;

		// Priority 200 -> allowed to produce every 1000/200 ~= 50 time units.
		producer2.PushBytesToSend(PayloadLength); // MayMakeActive().
		producer2.PushProduce(createChunk(3, 2, 200));
		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(4, 2, 201));
		producer2.PushBytesToSend(PayloadLength);
		producer2.PushProduce(createChunk(5, 2, 202));
		producer2.PushBytesToSend(0);

		auto stream2 = scheduler.CreateStream(std::addressof(producer2), 2, 200);

		stream2->MayMakeActive();

		MockStreamProducer producer3;

		// Priority 500 -> allowed to produce every 1000/500 ~= 20 time units.
		producer3.PushBytesToSend(PayloadLength); // MayMakeActive().
		producer3.PushProduce(createChunk(6, 3, 300));
		producer3.PushBytesToSend(PayloadLength);
		producer3.PushProduce(createChunk(7, 3, 301));
		producer3.PushBytesToSend(PayloadLength);
		producer3.PushProduce(createChunk(8, 3, 302));
		producer3.PushBytesToSend(0);

		auto stream3 = scheduler.CreateStream(std::addressof(producer3), 3, 500);

		stream3->MayMakeActive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 300)); // t ~= 20
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 301)); // t ~= 40
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 200)); // t ~= 50
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 302)); // t ~= 60
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 100)); // t ~= 80
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 201)); // t ~= 100
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 202)); // t ~= 150
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 101)); // t ~= 160
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 102)); // t ~= 240
		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	// Will do weighted fair queuing with three streams having different priority
	// and sending different payload sizes.
	SECTION("will do weighted fair queuing with different size and priority")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		scheduler.EnableMessageInterleaving(true);

		constexpr size_t SmallPacket{ 20 };
		constexpr size_t MediumPacket{ 50 };
		constexpr size_t LargePacket{ 70 };

		MockStreamProducer producer1;

		// Stream with priority = 125 -> inverse weight ~= 80.
		producer1.PushBytesToSend(MediumPacket); // MayMakeActive(); vft ~ 0 + 50*80 = 4000
		producer1.PushProduce(createChunk(0, 1, 100, MediumPacket));
		producer1.PushBytesToSend(SmallPacket); // vft ~ 4000 + 20*80 = 5600
		producer1.PushProduce(createChunk(1, 1, 101, SmallPacket));
		producer1.PushBytesToSend(LargePacket); // vft ~ 5600 + 70*80 = 11200
		producer1.PushProduce(createChunk(2, 1, 102, LargePacket));
		producer1.PushBytesToSend(0);

		auto stream1 = scheduler.CreateStream(std::addressof(producer1), 1, 125);

		stream1->MayMakeActive();

		MockStreamProducer producer2;

		// Stream with priority = 200 -> inverse weight ~= 50.
		producer2.PushBytesToSend(MediumPacket); // MayMakeActive(); vft ~ 0 + 50*50 = 2500
		producer2.PushProduce(createChunk(3, 2, 200, MediumPacket));
		producer2.PushBytesToSend(LargePacket); // vft ~ 2500 + 70*50 = 6000
		producer2.PushProduce(createChunk(4, 2, 201, LargePacket));
		producer2.PushBytesToSend(SmallPacket); // vft ~ 6000 + 20*50 = 7000
		producer2.PushProduce(createChunk(5, 2, 202, SmallPacket));
		producer2.PushBytesToSend(0);

		auto stream2 = scheduler.CreateStream(std::addressof(producer2), 2, 200);

		stream2->MayMakeActive();

		MockStreamProducer producer3;

		// Stream with priority = 500 -> inverse weight ~= 20
		producer3.PushBytesToSend(SmallPacket); // MayMakeActive; vft ~ 0 + 20*20 = 400
		producer3.PushProduce(createChunk(6, 3, 300, SmallPacket));
		producer3.PushBytesToSend(MediumPacket); // vft ~ 400 + 50*20 = 1400
		producer3.PushProduce(createChunk(7, 3, 301, MediumPacket));
		producer3.PushBytesToSend(LargePacket); // vft ~ 1400 + 70*20 = 2800
		producer3.PushProduce(createChunk(8, 3, 302, LargePacket));
		producer3.PushBytesToSend(0);

		auto stream3 = scheduler.CreateStream(std::addressof(producer3), 3, 500);

		stream3->MayMakeActive();

		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 300)); // t ~= 400
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 301)); // t ~= 1400
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 200)); // t ~= 2500
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 302)); // t ~= 2800
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 100)); // t ~= 4000
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 101)); // t ~= 5600
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 201)); // t ~= 6000
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 202)); // t ~= 7000
		REQUIRE(checkDataToSendHasMid(scheduler.Produce(NowMs, Mtu), 102)); // t ~= 11200
		REQUIRE(!scheduler.Produce(NowMs, Mtu).has_value());
	}

	// Two streams of different priority, identical packet size: ratio of packets
	// must match ratio of priorities.
	SECTION("will distribute WFQ packets in two streams by priority")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		scheduler.EnableMessageInterleaving(true);

		const TestStream stream1(scheduler, 1, 100);
		const TestStream stream2(scheduler, 2, 200);

		const auto packetCounts = getPacketCounts(scheduler, 15);

		REQUIRE(packetCounts.at(1) == 5);
		REQUIRE(packetCounts.at(2) == 10);
	}

	SECTION("will distribute WFQ packets in four streams by priority")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		scheduler.EnableMessageInterleaving(true);

		const TestStream stream1(scheduler, 1, 100);
		const TestStream stream2(scheduler, 2, 200);
		const TestStream stream3(scheduler, 3, 300);
		const TestStream stream4(scheduler, 4, 400);

		const auto packetCounts = getPacketCounts(scheduler, 50);

		REQUIRE(packetCounts.at(1) == 5);
		REQUIRE(packetCounts.at(2) == 10);
		REQUIRE(packetCounts.at(3) == 15);
		REQUIRE(packetCounts.at(4) == 20);
	}

	// A simple test with two streams of different priority, but sending packets
	// of different size. Verifies that the ratio of total packet payload
	// represents their priority.
	//
	// In this example,
	// - stream1 has priority 100 and sends packets of size 8.
	// -stream2 has priority 400 and sends packets of size 4.
	//
	// With round robin, stream1 would get twice as many payload bytes on the wire
	// as stream2, but with WFQ and a 4x priority increase, stream2 should 4x as
	// many payload bytes on the wire. That translates to stream2 getting 8x as
	// many packets on the wire as they are half as large.
	SECTION("will distribute from two streams fairly")
	{
		RTC::SCTP::StreamScheduler scheduler(Mtu);

		scheduler.EnableMessageInterleaving(true);

		const TestStream stream1(scheduler, 1, 100, /*packetLength*/ 8);
		const TestStream stream2(scheduler, 2, 400, /*packetLength*/ 4);

		const auto packetCounts = getPacketCounts(scheduler, 90);

		REQUIRE(packetCounts.at(1) == 10);
		REQUIRE(packetCounts.at(2) == 80);
	}
}
