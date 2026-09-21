#include "common.hpp"
#include "RTC/BWE/ProbePacketGenerator.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTP/Packet.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <string>
#include <vector>

SCENARIO("BWE ProbePacketGenerator", "[bwe][probepacketgenerator]")
{
	// What a packet of a burst was when it was handed over, kept by value because
	// every one of them is the very same instance.
	struct HandedPacket
	{
		size_t length{ 0 };
		uint16_t sequenceNumber{ 0 };
		uint32_t timestamp{ 0 };
		uint32_t ssrc{ 0 };
		uint8_t payloadType{ 0 };
		std::string mid;
		bool hasAbsSendTime{ false };
		bool hasTransportWideCc01{ false };
		bool isPaddedTo4Bytes{ false };
	};

	class TestProbePacketGeneratorListener : public RTC::BWE::ProbePacketGenerator::Listener
	{
	public:
		bool OnProbePacketGeneratorSendRtpPacket(
		  RTC::BWE::ProbePacketGenerator* /*probePacketGenerator*/, RTC::RTP::Packet* packet) override
		{
			HandedPacket handedPacket;

			handedPacket.length         = packet->GetLength();
			handedPacket.sequenceNumber = packet->GetSequenceNumber();
			handedPacket.timestamp      = packet->GetTimestamp();
			handedPacket.ssrc           = packet->GetSsrc();
			handedPacket.payloadType    = packet->GetPayloadType();

			packet->ReadMid(handedPacket.mid);

			uint32_t absSendTime;
			uint16_t wideSeqNumber;

			handedPacket.hasAbsSendTime       = packet->ReadAbsSendTime(absSendTime);
			handedPacket.hasTransportWideCc01 = packet->ReadTransportWideCc01(wideSeqNumber);
			handedPacket.isPaddedTo4Bytes     = packet->IsPaddedTo4Bytes();

			this->handedPackets.push_back(handedPacket);

			return this->handedPackets.size() < this->stopAfter;
		}

	public:
		std::vector<HandedPacket> handedPackets;
		// Packets after which the burst is given up on.
		size_t stopAfter{ std::numeric_limits<size_t>::max() };
	};

	// How far the sequence number moved between two packets.
	//
	// NOTE: The subtraction is done back in 16 bits because the two operands are
	// promoted to int, and the sequence number starts at a random value, so one
	// run in every 65536 has it wrapping around right here.
	const auto sequenceNumberDelta = [](const HandedPacket& from, const HandedPacket& to) -> uint16_t
	{
		return static_cast<uint16_t>(to.sequenceNumber - from.sequenceNumber);
	};

	// Total of what the given packets carried.
	const auto totalLength = [](const std::vector<HandedPacket>& handedPackets) -> size_t
	{
		size_t total{ 0 };

		for (const auto& handedPacket : handedPackets)
		{
			total += handedPacket.length;
		}

		return total;
	};

	TestProbePacketGeneratorListener listener;
	RTC::BWE::ProbePacketGenerator probePacketGenerator(std::addressof(listener));

	SECTION("a packet carries what makes it reportable")
	{
		probePacketGenerator.GeneratePackets(1);

		REQUIRE(listener.handedPackets.size() == 1);

		const auto& handedPacket = listener.handedPackets.at(0);

		REQUIRE(handedPacket.ssrc == RTC::Consts::BweProbeRtpSsrc);
		REQUIRE(handedPacket.payloadType == RTC::Consts::BweProbeRtpPayloadType);
		REQUIRE(handedPacket.mid == RTC::Consts::BweProbeRtpMid);
		REQUIRE(handedPacket.hasAbsSendTime);
		REQUIRE(handedPacket.hasTransportWideCc01);
	}

	SECTION("and the room it leaves for them is one the sender can write into")
	{
		class WritingListener : public RTC::BWE::ProbePacketGenerator::Listener
		{
		public:
			bool OnProbePacketGeneratorSendRtpPacket(
			  RTC::BWE::ProbePacketGenerator* /*probePacketGenerator*/, RTC::RTP::Packet* packet) override
			{
				this->sentAtUs += 1000;
				this->wideSeqNumber++;

				REQUIRE(packet->UpdateAbsSendTime(this->sentAtUs));
				REQUIRE(packet->UpdateTransportWideCc01(this->wideSeqNumber));

				uint32_t readAbsSendTime;
				uint16_t readWideSeqNumber;

				REQUIRE(packet->ReadAbsSendTime(readAbsSendTime));
				REQUIRE(readAbsSendTime == Utils::Time::TimeUsToAbsSendTime(this->sentAtUs));

				REQUIRE(packet->ReadTransportWideCc01(readWideSeqNumber));
				REQUIRE(readWideSeqNumber == this->wideSeqNumber);

				return true;
			}

		public:
			int64_t sentAtUs{ 100000000 };
			uint16_t wideSeqNumber{ 0 };
		};

		WritingListener writingListener;
		RTC::BWE::ProbePacketGenerator writingProbePacketGenerator(std::addressof(writingListener));

		writingProbePacketGenerator.GeneratePackets(5000);

		REQUIRE(writingListener.wideSeqNumber > 1);
	}

	SECTION("and is padded to 4 bytes whatever its size")
	{
		probePacketGenerator.GeneratePackets(5000);

		REQUIRE(listener.handedPackets.size() > 1);

		for (const auto& handedPacket : listener.handedPackets)
		{
			REQUIRE(handedPacket.isPaddedTo4Bytes);
		}
	}

	SECTION("a burst that fits in one packet is exactly as long as it was asked for")
	{
		probePacketGenerator.GeneratePackets(1000);

		REQUIRE(listener.handedPackets.size() == 1);
		REQUIRE(listener.handedPackets.at(0).length == 1000);
	}

	SECTION("asking for less than a header still sends a whole one")
	{
		probePacketGenerator.GeneratePackets(1);

		REQUIRE(listener.handedPackets.size() == 1);
		REQUIRE(listener.handedPackets.at(0).length > 1);
	}

	SECTION("asking for nothing sends nothing")
	{
		probePacketGenerator.GeneratePackets(0);

		REQUIRE(listener.handedPackets.empty());
	}

	SECTION("what is asked for is split into packets that carry at least that much")
	{
		constexpr size_t Size{ 5000 };

		probePacketGenerator.GeneratePackets(Size);

		REQUIRE(listener.handedPackets.size() > 1);
		REQUIRE(totalLength(listener.handedPackets) >= Size);

		for (const auto& handedPacket : listener.handedPackets)
		{
			REQUIRE(handedPacket.length <= RTC::BWE::ProbePacketGenerator::MaxPacketLength);
		}
	}

	SECTION("and one that no burst could ever be is brought down to something sendable")
	{
		probePacketGenerator.GeneratePackets(std::numeric_limits<size_t>::max());

		REQUIRE(!listener.handedPackets.empty());
		REQUIRE(
		  totalLength(listener.handedPackets) <= RTC::BWE::ProbePacketGenerator::MaxGeneratePacketsSize +
		                                           RTC::BWE::ProbePacketGenerator::MaxPacketLength);
	}

	SECTION("and a burst of a single packet is not split")
	{
		probePacketGenerator.GeneratePackets(RTC::BWE::ProbePacketGenerator::MaxPacketLength);

		REQUIRE(listener.handedPackets.size() == 1);
		REQUIRE(listener.handedPackets.at(0).length == RTC::BWE::ProbePacketGenerator::MaxPacketLength);
	}

	SECTION("every packet moves the sequence number and the timestamp on")
	{
		probePacketGenerator.GeneratePackets(5000);

		REQUIRE(listener.handedPackets.size() > 1);

		for (size_t i{ 1 }; i < listener.handedPackets.size(); ++i)
		{
			const auto& previous = listener.handedPackets.at(i - 1);
			const auto& current  = listener.handedPackets.at(i);

			REQUIRE(sequenceNumberDelta(previous, current) == 1);
			REQUIRE(current.timestamp != previous.timestamp);
		}
	}

	SECTION("and the ones of a later burst carry on from the ones before")
	{
		probePacketGenerator.GeneratePackets(1);
		probePacketGenerator.GeneratePackets(1);

		REQUIRE(listener.handedPackets.size() == 2);

		const auto& first  = listener.handedPackets.at(0);
		const auto& second = listener.handedPackets.at(1);

		REQUIRE(sequenceNumberDelta(first, second) == 1);
	}

	SECTION("and two of them count their own packets rather than each other's")
	{
		TestProbePacketGeneratorListener otherListener;
		RTC::BWE::ProbePacketGenerator otherProbePacketGenerator(std::addressof(otherListener));

		probePacketGenerator.GeneratePackets(1);
		otherProbePacketGenerator.GeneratePackets(1);
		probePacketGenerator.GeneratePackets(1);
		otherProbePacketGenerator.GeneratePackets(1);

		REQUIRE(listener.handedPackets.size() == 2);
		REQUIRE(otherListener.handedPackets.size() == 2);

		REQUIRE(sequenceNumberDelta(listener.handedPackets.at(0), listener.handedPackets.at(1)) == 1);
		REQUIRE(
		  sequenceNumberDelta(otherListener.handedPackets.at(0), otherListener.handedPackets.at(1)) == 1);
	}

	SECTION("a packet that cannot be sent stops the ones behind it")
	{
		listener.stopAfter = 2;

		probePacketGenerator.GeneratePackets(50000);

		REQUIRE(listener.handedPackets.size() == 2);
	}
}
