#ifndef MS_RTC_RTP_CODECS_TOOLS_HPP
#define MS_RTC_RTP_CODECS_TOOLS_HPP

#include "common.hpp"
#include "RTC/RTP/Codecs/AV1.hpp"
#include "RTC/RTP/Codecs/H264.hpp"
#include "RTC/RTP/Codecs/Opus.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/Codecs/VP8.hpp"
#include "RTC/RTP/Codecs/VP9.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			class Tools
			{
			public:
				static bool CanBeKeyFrame(const RTC::RtpCodecMimeType& mimeType)
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
								{
									return true;
								}

								default:
								{
									return false;
								}
							}
						}

						default:
						{
							return false;
						}
					}
				}

				static void ProcessRtpPacket(
				  RTP::Packet* packet,
				  const RTC::RtpCodecMimeType& mimeType,
				  std::unique_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure>&
				    templateDependencyStructure)
				{
					switch (mimeType.type)
					{
						case RTC::RtpCodecMimeType::Type::VIDEO:
						{
							switch (mimeType.subtype)
							{
								case RTC::RtpCodecMimeType::Subtype::VP8:
								{
									Codecs::VP8::ProcessRtpPacket(packet);

									break;
								}

								case RTC::RtpCodecMimeType::Subtype::VP9:
								{
									Codecs::VP9::ProcessRtpPacket(packet);

									break;
								}

								case RTC::RtpCodecMimeType::Subtype::H264:
								{
									Codecs::H264::ProcessRtpPacket(packet, templateDependencyStructure);

									break;
								}

								case RTC::RtpCodecMimeType::Subtype::AV1:
								{
									Codecs::AV1::ProcessRtpPacket(packet, templateDependencyStructure);

									break;
								}

								default:;
							}
						}

						case RTC::RtpCodecMimeType::Type::AUDIO:
						{
							switch (mimeType.subtype)
							{
								case RTC::RtpCodecMimeType::Subtype::OPUS:
								case RTC::RtpCodecMimeType::Subtype::MULTIOPUS:
								{
									Codecs::Opus::ProcessRtpPacket(packet);

									break;
								}

								default:;
							}
						}

						default:;
					}
				}

				static bool IsValidTypeForCodec(
				  RTC::RtpParameters::Type type, const RTC::RtpCodecMimeType& mimeType)
				{
					switch (type)
					{
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
										{
											return true;
										}

										default:
										{
											return false;
										}
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
										case RTC::RtpCodecMimeType::Subtype::AV1:
										{
											return true;
										}

										default:
										{
											return false;
										}
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

							NO_DEFAULT_GCC();
					}
				}

				static EncodingContext* GetEncodingContext(
				  const RTC::RtpCodecMimeType& mimeType, Codecs::EncodingContext::Params& params)
				{
					switch (mimeType.type)
					{
						case RTC::RtpCodecMimeType::Type::VIDEO:
						{
							switch (mimeType.subtype)
							{
								case RTC::RtpCodecMimeType::Subtype::VP8:
								{
									return new Codecs::VP8::EncodingContext(params);
								}

								case RTC::RtpCodecMimeType::Subtype::VP9:
								{
									return new Codecs::VP9::EncodingContext(params);
								}

								case RTC::RtpCodecMimeType::Subtype::AV1:
								{
									return new Codecs::AV1::EncodingContext(params);
								}

								case RTC::RtpCodecMimeType::Subtype::H264:
								{
									return new Codecs::H264::EncodingContext(params);
								}

								default:
								{
									return nullptr;
								}
							}
						}

						case RTC::RtpCodecMimeType::Type::AUDIO:
						{
							switch (mimeType.subtype)
							{
								case RTC::RtpCodecMimeType::Subtype::OPUS:
								case RTC::RtpCodecMimeType::Subtype::MULTIOPUS:
								{
									return new Codecs::Opus::EncodingContext(params);
								}

								default:
								{
									return nullptr;
								}
							}
						}

						default:
						{
							return nullptr;
						}
					}
				}
			};
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC

#endif
