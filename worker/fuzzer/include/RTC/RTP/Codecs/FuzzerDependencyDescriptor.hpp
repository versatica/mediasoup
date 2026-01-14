#ifndef MS_FUZZER_RTC_RTP_CODECS_DEPENDENCY_DESCRIPTOR_HPP
#define MS_FUZZER_RTC_RTP_CODECS_DEPENDENCY_DESCRIPTOR_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTP
		{
			namespace Codecs
			{
				namespace DependencyDescriptor
				{
					void Fuzz(const uint8_t* data, size_t len);
				} // namespace DependencyDescriptor
			} // namespace Codecs
		} // namespace RTP
	} // namespace RTC
} // namespace Fuzzer

#endif
