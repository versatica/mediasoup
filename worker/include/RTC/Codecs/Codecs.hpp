#ifndef MS_RTC_CODECS_HPP
#define MS_RTC_CODECS_HPP

#include "common.hpp"
#include "RTC/Codecs/H264.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/Codecs/VP8.hpp"
#include "RTC/Codecs/VP9.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	namespace Codecs
	{
		bool CanBeKeyFrame(const RTC::RtpCodecMimeType& mimeType);
		void ProcessRtpPacket(RTC::RtpPacket* packet, const RTC::RtpCodecMimeType& mimeType);
		bool IsValidTypeForCodec(RTC::RtpParameters::Type type, const RTC::RtpCodecMimeType& mimeType);
		EncodingContext* GetEncodingContext(
		  const RTC::RtpCodecMimeType& mimeType, uint8_t spatialLayers, uint8_t temporalLayers);

		/* Inline namespace methods. */

		inline bool CanBeKeyFrame(const RTC::RtpCodecMimeType& mimeType)
		{
			switch (mimeType.type)
			{
				case RTC::RtpCodecMimeType::Type::VIDEO:
				{
					switch (mimeType.subtype)
					{
						case RTC::RtpCodecMimeType::Subtype::VP8:
						case RTC::RtpCodecMimeType::Subtype::VP9:
						case RTC::RtpCodecMimeType::Subtype::H264:
							return true;
						default:
							return false;
					}
				}

				default:
				{
					return false;
				}
			}
		}

		inline bool IsValidTypeForCodec(RTC::RtpParameters::Type type, const RTC::RtpCodecMimeType& mimeType)
		{
			switch (type)
			{
				case RTC::RtpParameters::Type::NONE:
				{
					return false;
				}

				case RTC::RtpParameters::Type::SIMPLE:
				{
					return true;
				}

				case RTC::RtpParameters::Type::SIMULCAST:
				{
					switch (mimeType.type)
					{
						case RTC::RtpCodecMimeType::Type::VIDEO:
						{
							switch (mimeType.subtype)
							{
								case RTC::RtpCodecMimeType::Subtype::VP8:
								case RTC::RtpCodecMimeType::Subtype::H264:
									return true;
								default:
									return false;
							}
						}

						default:
						{
							return false;
						}
					}
				}

				case RTC::RtpParameters::Type::SVC:
				{
					switch (mimeType.type)
					{
						case RTC::RtpCodecMimeType::Type::VIDEO:
						{
							switch (mimeType.subtype)
							{
								case RTC::RtpCodecMimeType::Subtype::VP9:
									return true;
								default:
									return false;
							}
						}

						default:
						{
							return false;
						}
					}
				}

				case RTC::RtpParameters::Type::PIPE:
				{
					return true;
				}

				default:
				{
					return false;
				}
			}
		}

		inline EncodingContext* GetEncodingContext(
		  const RTC::RtpCodecMimeType& mimeType, RTC::Codecs::EncodingContext::Params& params)
		{
			switch (mimeType.type)
			{
				case RTC::RtpCodecMimeType::Type::VIDEO:
				{
					switch (mimeType.subtype)
					{
						case RTC::RtpCodecMimeType::Subtype::VP8:
							return new RTC::Codecs::VP8::EncodingContext(params);
						case RTC::RtpCodecMimeType::Subtype::VP9:
							return new RTC::Codecs::VP9::EncodingContext(params);
						case RTC::RtpCodecMimeType::Subtype::H264:
							return new RTC::Codecs::H264::EncodingContext(params);
						default:
							return nullptr;
					}
				}

				default:
				{
					return nullptr;
				}
			}
		}
	} // namespace Codecs
} // namespace RTC

#endif
