#include "common.hpp"
#include "DepLibUV.hpp"
#include "RTC/NackGenerator.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/rtpCommon.hpp"
#include "RTC/Serializable.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

SCENARIO("NACK generator", "[rtp][rtcp][nack]")
{
	constexpr unsigned int SendNackDelay{ 0u }; // In ms.

	struct TestNackGeneratorInput
	{
		TestNackGeneratorInput() = default;
		TestNackGeneratorInput(
		  uint16_t seq,
		  bool isKeyFrame,
		  uint16_t firstNacked,
		  size_t numNacked,
		  bool keyFrameRequired = false,
		  size_t nackListSize   = 0)
		  : seq(seq),
		    isKeyFrame(isKeyFrame),
		    firstNacked(firstNacked),
		    numNacked(numNacked),
		    keyFrameRequired(keyFrameRequired),
		    nackListSize(nackListSize)
		{
		}

		uint16_t seq{ 0 };
		bool isKeyFrame{ false };
		uint16_t firstNacked{ 0 };
		size_t numNacked{ 0 };
		bool keyFrameRequired{ false };
		size_t nackListSize{ 0 };
	};

	class TestPayloadDescriptorHandler : public RTC::RTP::Codecs::PayloadDescriptorHandler
	{
	public:
		explicit TestPayloadDescriptorHandler(bool isKeyFrame) : isKeyFrame(isKeyFrame) {};
		~TestPayloadDescriptorHandler() override = default;
		void Dump(int indentation = 0) const override
		{
		}
		bool Process(
		  RTC::RTP::Codecs::EncodingContext* /*context*/,
		  RTC::RTP::Packet* /*packet*/,
		  bool& /*marker*/) override
		{
			return true;
		}
		void RtpPacketChanged(RTC::RTP::Packet* packet) override
		{
		}
		std::unique_ptr<RTC::RTP::Codecs::PayloadDescriptor::Encoder> GetEncoder() const override
		{
			return nullptr;
		}
		void Encode(
		  RTC::RTP::Packet* /*packet*/, RTC::RTP::Codecs::PayloadDescriptor::Encoder* /*encoder*/) override
		{
		}
		void Restore(RTC::RTP::Packet* /*packet*/) override
		{
		}
		uint8_t GetSpatialLayer() const override
		{
			return 0;
		}
		uint8_t GetTemporalLayer() const override
		{
			return 0;
		}
		bool IsKeyFrame() const override
		{
			return this->isKeyFrame;
		}

	private:
		bool isKeyFrame{ false };
	};

	class TestNackGeneratorListener : public RTC::NackGenerator::Listener
	{
		void OnNackGeneratorNackRequired(const std::vector<uint16_t>& seqNumbers) override
		{
			this->nackRequiredTriggered = true;

			auto it          = seqNumbers.begin();
			auto firstNacked = *it;
			auto numNacked   = seqNumbers.size();

			REQUIRE(this->currentInput.firstNacked == firstNacked);
			REQUIRE(this->currentInput.numNacked == numNacked);
		};

		void OnNackGeneratorKeyFrameRequired() override
		{
			this->keyFrameRequiredTriggered = true;

			REQUIRE(this->currentInput.keyFrameRequired);
		}

	public:
		void Reset(TestNackGeneratorInput& input)
		{
			this->currentInput              = input;
			this->nackRequiredTriggered     = false;
			this->keyFrameRequiredTriggered = false;
		}

		void Check(RTC::NackGenerator& nackGenerator)
		{
			REQUIRE(this->nackRequiredTriggered == static_cast<bool>(this->currentInput.numNacked));
			REQUIRE(this->keyFrameRequiredTriggered == this->currentInput.keyFrameRequired);
		}

	private:
		TestNackGeneratorInput currentInput{};
		bool nackRequiredTriggered{ false };
		bool keyFrameRequiredTriggered{ false };
	};

	auto validate =
	  [](std::unique_ptr<RTC::RTP::Packet>& packet, std::vector<TestNackGeneratorInput>& inputs)
	{
		TestNackGeneratorListener listener;
		auto nackGenerator = RTC::NackGenerator(&listener, SendNackDelay);

		for (auto input : inputs)
		{
			listener.Reset(input);

			auto* tpdh = new TestPayloadDescriptorHandler(input.isKeyFrame);

			packet->SetPayloadDescriptorHandler(tpdh);
			packet->SetSequenceNumber(input.seq);
			nackGenerator.ReceivePacket(packet.get(), /*isRecovered*/ false);

			listener.Check(nackGenerator);
		}
	};

	// clang-format off
	alignas(4) uint8_t rtpBuffer[] =
	{
		0x80, 0x7b, 0x52, 0x0e,
		0x5b, 0x6b, 0xca, 0xb5,
		0x00, 0x00, 0x00, 0x02
	};
	// clang-format on

	// [pt:123, seq:21006, timestamp:1533790901]
	std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Parse(rtpBuffer, sizeof(rtpBuffer)) };

	packet->Serialize(rtpCommon::SerializeBuffer, sizeof(rtpCommon::SerializeBuffer));

	SECTION("no NACKs required")
	{
		// clang-format off
		std::vector<TestNackGeneratorInput> inputs =
		{
			{ 2371, false, 0, 0, false, 0 },
			{ 2372, false, 0, 0, false, 0 },
			{ 2373, false, 0, 0, false, 0 },
			{ 2374, false, 0, 0, false, 0 },
			{ 2375, false, 0, 0, false, 0 },
			{ 2376, false, 0, 0, false, 0 },
			{ 2377, false, 0, 0, false, 0 },
			{ 2378, false, 0, 0, false, 0 },
			{ 2379, false, 0, 0, false, 0 },
			{ 2380, false, 0, 0, false, 0 },
			{ 2254, false, 0, 0, false, 0 },
			{ 2250, false, 0, 0, false, 0 },
		};
		// clang-format on

		validate(packet, inputs);
	}

	SECTION("generate NACK for missing ordered packet")
	{
		// clang-format off
		std::vector<TestNackGeneratorInput> inputs =
		{
			{ 2381, false,    0, 0, false, 0 },
			{ 2383, false, 2382, 1, false, 1 }
		};
		// clang-format on

		validate(packet, inputs);
	}

	SECTION("sequence wrap generates no NACK")
	{
		// clang-format off
		std::vector<TestNackGeneratorInput> inputs =
		{
			{ 65534, false, 0, 0, false, 0 },
			{ 65535, false, 0, 0, false, 0 },
			{     0, false, 0, 0, false, 0 }
		};
		// clang-format on

		validate(packet, inputs);
	}

	SECTION("generate NACK after sequence wrap")
	{
		// clang-format off
		std::vector<TestNackGeneratorInput> inputs =
		{
			{ 65534, false, 0, 0, false, 0 },
			{ 65535, false, 0, 0, false, 0 },
			{     1, false, 0, 1, false, 1 }
		};
		// clang-format on

		validate(packet, inputs);
	}

	SECTION("generate NACK after sequence wrap, and yet another NACK")
	{
		// clang-format off
		std::vector<TestNackGeneratorInput> inputs =
		{
			{ 65534, false, 0, 0, false,  0 },
			{ 65535, false, 0, 0, false,  0 },
			{     1, false, 0, 1, false,  1 },
			{    11, false, 2, 9, false, 10 },
			{    12,  true, 0, 0, false, 10 },
			{    13,  true, 0, 0, false,  0 }
		};
		// clang-format on

		validate(packet, inputs);
	}

	SECTION("intercalated missing packets")
	{
		// clang-format off
		std::vector<TestNackGeneratorInput> inputs =
		{
			{ 1, false, 0, 0, false, 0 },
			{ 3, false, 2, 1, false, 1 },
			{ 5, false, 4, 1, false, 2 },
			{ 7, false, 6, 1, false, 3 },
			{ 9, false, 8, 1, false, 4 }
		};
		// clang-format on

		validate(packet, inputs);
	}

	SECTION("non contiguous intercalated missing packets")
	{
		// clang-format off
		std::vector<TestNackGeneratorInput> inputs =
		{
			{ 1, false, 0, 0, false, 0 },
			{ 3, false, 2, 1, false, 1 },
			{ 7, false, 4, 3, false, 4 },
			{ 9, false, 8, 1, false, 5 }
		};
		// clang-format on

		validate(packet, inputs);
	}

	SECTION("big jump")
	{
		// clang-format off
		std::vector<TestNackGeneratorInput> inputs =
		{
			{   1, false, 0,   0, false,   0 },
			{ 300, false, 2, 298, false, 298 },
			{   3, false, 0,   0, false, 297 },
			{   4, false, 0,   0, false, 296 },
			{   5, false, 0,   0, false, 295 }
		};
		// clang-format on

		validate(packet, inputs);
	}

	SECTION("Key Frame required. Nack list too large to be requested")
	{
		// clang-format off
		std::vector<TestNackGeneratorInput> inputs =
		{
			{    1, false, 0, 0, false, 0 },
			{ 3000, false, 0, 0,  true, 0 }
		};
		// clang-format on

		validate(packet, inputs);
	}

	// Must run the loop to wait for UV timers and close them.
	DepLibUV::RunLoop();
}
