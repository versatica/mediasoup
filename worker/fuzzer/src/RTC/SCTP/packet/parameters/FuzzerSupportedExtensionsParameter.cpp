#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerSupportedExtensionsParameter.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

void FuzzerRtcSctpSupportedExtensionsParameter::Fuzz(
  const RTC::SCTP::SupportedExtensionsParameter* parameter)
{
	parameter->GetNumberOfChunkTypes();

	for (uint16_t idx{ 0 }; idx < parameter->GetNumberOfChunkTypes(); ++idx)
	{
		parameter->GetChunkTypeAt(idx);
	}

	parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::RE_CONFIG);
	parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::FORWARD_TSN);
	parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::I_DATA);
}
