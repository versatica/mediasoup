#include "include/catch.hpp"
#include "common.hpp"
#include "RTC/NackGenerator.hpp"
#include "RTC/RtpPacket.hpp"
#include <vector>

using namespace RTC;

struct Input
{
	Input() = default;
	Input(uint16_t seq, uint16_t firstNacked, size_t numNacked, bool keyFrameRequired = false)
		: seq(seq), firstNacked(firstNacked), numNacked(numNacked), keyFrameRequired(keyFrameRequired)
		{}

	uint16_t seq{ 0 };
	uint16_t firstNacked{ 0 };
	size_t numNacked{ 0 };
	bool keyFrameRequired{ false };
};

class TestNackGeneratorListener : public NackGenerator::Listener
{
	void OnNackGeneratorNackRequired(const std::vector<uint16_t>& seqNumbers) override
	{
		this->nackRequiredTriggered = true;

		auto it = seqNumbers.begin();
		auto firstNacked = *it;

		auto numNacked = seqNumbers.size();

		REQUIRE(this->currentInput.firstNacked == firstNacked);
		REQUIRE(this->currentInput.numNacked == numNacked);
	};

	void OnNackGeneratorKeyFrameRequired() override
	{
		this->keyFrameRequiredTriggered = true;

		REQUIRE(this->currentInput.keyFrameRequired);
	}

public:
	void Reset(Input input)
	{
		this->currentInput = input;
		this->nackRequiredTriggered = false;
		this->keyFrameRequiredTriggered = false;
	}

	void Check()
	{
		REQUIRE(this->nackRequiredTriggered == static_cast<bool>(this->currentInput.numNacked));
		REQUIRE(this->keyFrameRequiredTriggered == this->currentInput.keyFrameRequired);
	}

private:
	Input currentInput{};
	bool nackRequiredTriggered = false;
	bool keyFrameRequiredTriggered = false;
};

uint8_t rtpBuffer[] =
{
	0b10000000, 0b01111011, 0b01010010, 0b00001110,
	0b01011011, 0b01101011, 0b11001010, 0b10110101,
	0, 0, 0, 2
};

// [pt:123, seq:21006, timestamp:1533790901]
RtpPacket* packet = RtpPacket::Parse(rtpBuffer, sizeof(rtpBuffer));

void validate(std::vector<Input>& inputs)
{
	TestNackGeneratorListener listener;
	NackGenerator nackGenerator = NackGenerator(&listener);

	for (auto input : inputs)
	{
		listener.Reset(input);

		packet->SetSequenceNumber(input.seq);
		nackGenerator.ReceivePacket(packet);

		listener.Check();
	}
};


SCENARIO("NACK generator", "[rtp][rtcp]")
{
	SECTION("ignore too old packets")
	{
		std::vector<Input> inputs =
		{
			{ 2371, 0, 0 },
			{ 2372, 0, 0 },
			{ 2373, 0, 0 },
			{ 2374, 0, 0 },
			{ 2375, 0, 0 },
			{ 2376, 0, 0 },
			{ 2377, 0, 0 },
			{ 2378, 0, 0 },
			{ 2379, 0, 0 },
			{ 2380, 0, 0 },
			{ 2254, 0, 0 },
			{ 2250, 0, 0 },
		};

		validate(inputs);
	}

	SECTION("generate NACK for missing ordered packet")
	{
		std::vector<Input> inputs =
		{
			{ 2381, 0, 0 },
			{ 2383, 2382, 1 }
		};

		validate(inputs);
	}

	SECTION("sequence wrap generates no NACK")
	{
		std::vector<Input> inputs =
		{
			{ 65534, 0, 0 },
			{ 65535, 0, 0 },
			{     0, 0, 0 }
		};

		validate(inputs);
	}

	SECTION("generate NACK after sequence wrap")
	{
		std::vector<Input> inputs =
		{
			{ 65534, 0, 0 },
			{ 65535, 0, 0 },
			{     1, 0, 1 }
		};

		validate(inputs);
	}

	SECTION("generate NACK after sequence wrap, and yet another NACK")
	{
		std::vector<Input> inputs =
		{
			{ 65534, 0, 0 },
			{ 65535, 0, 0 },
			{     1, 0, 1 },
			{    11, 2, 9 }
		};

		validate(inputs);
	}

	SECTION("intercalated missing packets")
	{
		std::vector<Input> inputs =
		{
			{ 1, 0, 0 },
			{ 3, 2, 1 },
			{ 5, 4, 1 },
			{ 7, 6, 1 },
			{ 9, 8, 1 }
		};

		validate(inputs);
	}

	SECTION("non contiguous intercalated missing packets")
	{
		std::vector<Input> inputs =
		{
			{ 1, 0, 0 },
			{ 3, 2, 1 },
			{ 7, 4, 3 },
			{ 9, 8, 1 }
		};

		validate(inputs);
	}

	SECTION("big jump")
	{
		std::vector<Input> inputs =
		{
			{   1, 0,   0 },
			{ 300, 2, 298 },
			{   3, 0,   0 },
			{   4, 0,   0 },
			{   5, 0,   0 }
		};

		validate(inputs);
	}

	SECTION("Key Frame required. Nack list too large to be requested")
	{
		std::vector<Input> inputs =
		{
			{    1, 0, 0 },
			{ 3000, 0, 0, true}
		};

		validate(inputs);
	}
}
