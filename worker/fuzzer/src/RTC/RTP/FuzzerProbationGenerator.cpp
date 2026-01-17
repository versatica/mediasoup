#include "RTC/RTP/FuzzerProbationGenerator.hpp"
#include "RTC/RTP/ProbationGenerator.hpp"

void Fuzzer::RTC::RTP::ProbationGenerator::Fuzz(const uint8_t* /*data*/, size_t len)
{
	std::unique_ptr<::RTC::RTP::ProbationGenerator> probationGenerator{
		new ::RTC::RTP::ProbationGenerator()
	};

	probationGenerator->GetNextPacket(len);
}
