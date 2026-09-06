#include "common.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/TransportCongestionControlServer.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>
#include <deque>
#include <vector>

SCENARIO("TransportCongestionControlServer", "[rtp]")
{
	struct TestTransportCongestionControlServerInput
	{
		uint16_t wideSeqNumber;
		int64_t nowUs;
	};

	struct TestTransportCongestionControlServerResult
	{
		uint16_t wideSeqNumber;
		bool received;
		int64_t timestampUs;
	};

	using TestResults = std::deque<std::vector<TestTransportCongestionControlServerResult>>;

	class TestTransportCongestionControlServerListener
	  : public RTC::TransportCongestionControlServer::Listener
	{
	public:
		virtual void OnTransportCongestionControlServerSendRtcpPacket(
		  RTC::TransportCongestionControlServer* tccServer, RTC::RTCP::Packet* packet) override
		{
			auto* tccPacket = dynamic_cast<RTC::RTCP::FeedbackRtpTransportPacket*>(packet);

			if (!tccPacket)
			{
				return;
			}

			auto packetStatuses = tccPacket->GetPacketStatuses();

			REQUIRE(!this->results.empty());

			auto testResults = this->results.front();
			this->results.pop_front();

			REQUIRE(testResults.size() == packetStatuses.size());

			auto packetStatusIt = packetStatuses.begin();
			auto testResultIt   = testResults.begin();

			for (; packetStatusIt != packetStatuses.end() && testResultIt != testResults.end();
			     ++packetStatusIt, ++testResultIt)
			{
				REQUIRE(packetStatusIt->sequenceNumber == testResultIt->wideSeqNumber);
				REQUIRE(packetStatusIt->received == testResultIt->received);

				if (packetStatusIt->received)
				{
					// The reconstructed times sit in the frame of the reference time,
					// which is shifted by a whole wrap period so that it's positive
					// regardless of the sign of the reference time on the wire.
					REQUIRE(
					  packetStatusIt->receivedAtUs ==
					  RTC::RTCP::FeedbackRtpTransportPacket::TimeWrapPeriodUs + testResultIt->timestampUs);
				}
			}
		}

	public:
		void SetResults(TestResults& results)
		{
			this->results = results;
		}

		void Check()
		{
			REQUIRE(this->results.empty());
		}

	private:
		TestResults results;
	};

	mocks::MockShared shared(/*getTimeMs*/
	                         []()
	                         {
		                         return 1000;
	                         });

	// clang-format off
	alignas(4) uint8_t buffer[] =
	{
		0x90, 0x01, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0x05,
		0xbe, 0xde, 0x00, 0x01,	// Header extensions
		0x51, 0x60, 0xee, 0x00  // TCC feedback
	};
	// clang-format on

	auto validate =
	  [&buffer,
		 &shared](std::vector<TestTransportCongestionControlServerInput>& inputs, TestResults& results)
	{
		TestTransportCongestionControlServerListener listener;
		auto tccServer = RTC::TransportCongestionControlServer(
		  std::addressof(listener),
		  std::addressof(shared),
		  RTC::BweType::TRANSPORT_CC,
		  RTC::Consts::MtuSize);

		tccServer.SetMaxIncomingBitrate(150000);
		tccServer.TransportConnected();

		std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Parse(buffer, sizeof(buffer)) };

		RTC::RTP::HeaderExtensionIds headerExtensionIds{};

		headerExtensionIds.transportWideCc01 = 5;

		packet->AssignExtensionIds(headerExtensionIds);
		packet->SetSequenceNumber(1);

		// Save results.
		listener.SetResults(results);

		static constexpr int64_t TransportCcFeedbackSendIntervalUs{ 100 * 1000 };

		int64_t startTsUs = inputs[0].nowUs;

		for (auto input : inputs)
		{
			// Periodic sending TCC packets.
			const int64_t diffTsUs = input.nowUs - startTsUs;

			if (diffTsUs >= TransportCcFeedbackSendIntervalUs)
			{
				tccServer.FillAndSendTransportCcFeedback();
				startTsUs = input.nowUs;
			}

			packet->UpdateTransportWideCc01(input.wideSeqNumber);
			tccServer.IncomingPacket(input.nowUs, packet.get());
		}

		tccServer.FillAndSendTransportCcFeedback();
		listener.Check();
	};

	SECTION("normal time and sequence")
	{
		// clang-format off
		std::vector<TestTransportCongestionControlServerInput> inputs
		{
			{ 1u, 1000000 },
			{ 2u, 1050000 },
			{ 3u, 1100000 },
			{ 4u, 1150000 },
			{ 5u, 1200000 },
		};

		TestResults results
		{
			{
				{ 1u, true, 1000000 },
				{ 2u, true, 1050000 },
			},
			{
				{ 3u, true, 1100000 },
				{ 4u, true, 1150000 },
			},
			{
				{ 5u, true, 1200000 },
			},
		};
		// clang-format on

		validate(inputs, results);
	}

	SECTION("lost packets")
	{
		// clang-format off
		std::vector<TestTransportCongestionControlServerInput> inputs
		{
			{  1u, 1000000 },
			{  3u, 1050000 },
			{  5u, 1100000 },
			{  6u, 1150000 },
		};

		TestResults results
		{
			{
				{ 1u,  true, 1000000 },
				{ 2u, false,       0 },
				{ 3u,  true, 1050000 },
			},
			{
				{ 4u, false,       0 },
				{ 5u,  true, 1100000 },
				{ 6u,  true, 1150000 },
			},
		};
		// clang-format on

		validate(inputs, results);
	}

	SECTION("duplicate packets")
	{
		// clang-format off
		std::vector<TestTransportCongestionControlServerInput> inputs
		{
			{  1u, 1000000 },
			{  1u, 1050000 },
			{  2u, 1100000 },
			{  3u, 1150000 },
			{  3u, 1200000 },
			{  4u, 1250000 },
		};

		TestResults results
		{
			{
				{ 1u,  true, 1000000 },
			},
			{
				{ 2u,  true, 1100000 },
				{ 3u,  true, 1150000 },
			},
			{
				{ 4u,  true, 1250000 },
			},
		};
		// clang-format on

		validate(inputs, results);
	}

	SECTION("packets arrive out of order")
	{
		// clang-format off
		std::vector<TestTransportCongestionControlServerInput> inputs
		{
			{ 1u, 1000000 },
			{ 2u, 1050000 },
			{ 4u, 1100000 },
			{ 5u, 1150000 },
			{ 3u, 1200000 }, // Out of order
			{ 6u, 1250000 },
		};

		TestResults results
		{
			{
				{ 1u, true, 1000000 },
				{ 2u, true, 1050000 },
			},
			{
				{ 3u, false,       0 },
				{ 4u,  true, 1100000 },
				{ 5u,  true, 1150000 },
			},
			{
				{ 3u, true, 1200000 },
				{ 4u, true, 1100000 },
				{ 5u, true, 1150000 },
				{ 6u, true, 1250000 },
			},
		};
		// clang-format on

		validate(inputs, results);
	}
}
