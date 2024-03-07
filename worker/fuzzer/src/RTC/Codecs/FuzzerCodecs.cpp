#include "RTC/Codecs/FuzzerCodecs.hpp"
#include "RTC/Codecs/Opus.hpp"
#include "RTC/Codecs/VP8.hpp"
#include "RTC/Codecs/VP9.hpp"
#include "RTC/Codecs/H264.hpp"
#include "RTC/Codecs/H264_SVC.hpp"

void Fuzzer::RTC::Codecs::Fuzz(const uint8_t* data, size_t len)
{
	::RTC::Codecs::Opus::Parse(data, len);
	::RTC::Codecs::VP8::Parse(data, len);
	::RTC::Codecs::VP9::Parse(data, len);
	::RTC::Codecs::H264::Parse(data, len);
	::RTC::Codecs::H264_SVC::Parse(data, len);
}
