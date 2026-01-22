#ifndef MS_RTC_RTP_PACKET_HPP
#define MS_RTC_RTP_PACKET_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "FBS/rtpPacket.h"
#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpHeaderExtensionIds.hpp"
#include "RTC/Serializable.hpp"
#ifdef MS_RTC_LOGGER_RTP
#include "RTC/RtcLogger.hpp"
#endif
#include <flatbuffers/flatbuffers.h>
#include <array>
#include <map>
#include <string>
#include <vector>

namespace RTC
{
	namespace RTP
	{
		/**
		 * RTP Packet.
		 *
		 * @see RFC 3550.
		 */
		class Packet : public Serializable, public Codecs::DependencyDescriptor::Listener
		{
		public:
			/**
			 * RTP Fixed Header.
			 *
			 * @see RFC 3550.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |V=2|P|X|  CC   |M|     PT      |       sequence number         |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |                           timestamp                           |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |           synchronization source (SSRC) identifier            |
			 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
			 * |            contributing source (CSRC) identifiers             |
			 * |                             ....                              |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 */
			struct FixedHeader
			{
#if defined(MS_LITTLE_ENDIAN)
				uint8_t csrcCount : 4;
				uint8_t extension : 1;
				uint8_t padding : 1;
				uint8_t version : 2;
				uint8_t payloadType : 7;
				uint8_t marker : 1;
#elif defined(MS_BIG_ENDIAN)
				uint8_t version : 2;
				uint8_t padding : 1;
				uint8_t extension : 1;
				uint8_t csrcCount : 4;
				uint8_t marker : 1;
				uint8_t payloadType : 7;
#endif
				uint16_t sequenceNumber;
				uint32_t timestamp;
				uint32_t ssrc;
			};

		private:
			/**
			 * RTP Header Extension.
			 *
			 * @see RFC 3350.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |      defined by profile       |           length              |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |                        header extension                       |
			 * |                             ....                              |
			 */
			struct HeaderExtension
			{
				/**
				 * Defined by profile.
				 */
				uint16_t id;
				/**
				 * Number of 32-bit words in the extension, excluding the id & length
				 * four-octet.
				 */
				uint16_t len;
				uint8_t value[];
			};

		public:
			/**
			 * One-Byte and Two-Bytes Extension types.
			 *
			 * @see RFC 8285.
			 */
			enum class ExtensionsType : uint8_t
			{
				/**
				 * Auto means that One-Byte or Two-Bytes is choosen based on given
				 * Extensions.
				 */
				Auto     = 0,
				OneByte  = 1,
				TwoBytes = 2
			};

		private:
			struct OneByteExtension
			{
#if defined(MS_LITTLE_ENDIAN)
				uint8_t len : 4;
				uint8_t id : 4;
#elif defined(MS_BIG_ENDIAN)
				uint8_t id : 4;
				uint8_t len : 4;
#endif
				uint8_t value[];
			};

		private:
			struct TwoBytesExtension
			{
				uint8_t id;
				uint8_t len;
				uint8_t value[];
			};

		public:
			/**
			 * Struct for setting and replacing Extensions.
			 */
			struct Extension
			{
				Extension(RTC::RtpHeaderExtensionUri::Type type, uint8_t id, uint8_t len, uint8_t* value)
				  : type(type), id(id), len(len), value(value) {};

				RTC::RtpHeaderExtensionUri::Type type;
				uint8_t id;
				uint8_t len;
				uint8_t* value;
			};

		public:
			/**
			 * Minimum size (in bytes) of the RTP Fixed Header (without CSRC fields).
			 */
			static const size_t FixedHeaderMinLength{ 12 };

			/**
			 * Whether given buffer could be a valid RTP Packet.
			 *
			 * @remarks
			 * - Before calling this static method, the caller should verify whether
			 *   the given buffer is a RTCP packet.
			 */
			static bool IsRtp(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Parse a RTP Packet.
			 *
			 * @remarks
			 * - `packetLength` must be the exact length of the Packet.
			 * - `bufferLength` must be >= `packetLength`.
			 *
			 * @throw MediaSoupTypeError - If `bufferLength` is lower than
			 *   `packetLength`.
			 */
			static Packet* Parse(const uint8_t* buffer, size_t packetLength, size_t bufferLength);

			/**
			 * Parse a RTP Packet.
			 *
			 * @remarks
			 * - `bufferLength` must be the exact length of the Packet.
			 */
			static Packet* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a RTP Packet.
			 */
			static Packet* Factory(uint8_t* buffer, size_t bufferLength);

			/**
			 * Generate value for "urn:mediasoup:params:rtp-hdrext:packet-id"
			 * mediasoup custom Extension.
			 */
			static uint32_t GetNextMediasoupPacketId();

		private:
			thread_local static uint32_t nextMediasoupPacketId;

		private:
			/**
			 * Constructor is private because we only want to create Packet instances
			 * via Parse() and Factory().
			 */
			Packet(uint8_t* buffer, size_t bufferLength);

		public:
			~Packet() override;

			void Dump(int indentation = 0) const final;

			Packet* Clone(uint8_t* buffer, size_t bufferLength) const final;

			flatbuffers::Offset<FBS::RtpPacket::Dump> FillBuffer(flatbuffers::FlatBufferBuilder& builder) const;

			uint8_t GetVersion() const
			{
				return GetFixedHeaderPointer()->version;
			}

			uint8_t GetPayloadType() const
			{
				return GetFixedHeaderPointer()->payloadType;
			}

			void SetPayloadType(uint8_t payloadType);

			bool HasMarker() const
			{
				return GetFixedHeaderPointer()->marker;
			}

			void SetMarker(bool marker);

			uint16_t GetSequenceNumber() const
			{
				return ntohs(GetFixedHeaderPointer()->sequenceNumber);
			}

			void SetSequenceNumber(uint16_t seq);

			uint32_t GetTimestamp() const
			{
				return ntohl(GetFixedHeaderPointer()->timestamp);
			}

			void SetTimestamp(uint32_t timestamp);

			uint32_t GetSsrc() const
			{
				return ntohl(GetFixedHeaderPointer()->ssrc);
			}

			void SetSsrc(uint32_t ssrc);

			bool HasCsrcs() const
			{
				return (GetFixedHeaderPointer()->csrcCount != 0);
			}

			bool HasHeaderExtension() const
			{
				return (GetFixedHeaderPointer()->extension);
			}

			/**
			 * Get the Extension Header id or 0 if there isn't.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced Header Extension.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			uint16_t GetHeaderExtensionId() const
			{
				if (!HasHeaderExtension())
				{
					return 0;
				}

				return ntohs(GetHeaderExtensionPointer()->id);
			}

			/**
			 * Pointer to the Header Extension value or `nullptr` if there is no
			 * Header Extension or its has no value.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced Header Extension.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			uint8_t* GetHeaderExtensionValue() const
			{
				auto headerExtensionValueLength = GetHeaderExtensionValueLength();

				if (headerExtensionValueLength == 0)
				{
					return nullptr;
				}

				return GetHeaderExtensionPointer()->value;
			}

			/**
			 * Length of the Header Extension value (excluding the id & length
			 * four-octet).
			 */
			size_t GetHeaderExtensionValueLength() const
			{
				if (!HasHeaderExtension())
				{
					return 0;
				}

				return static_cast<size_t>(ntohs(GetHeaderExtensionPointer()->len) * 4);
			}

			/**
			 * Remove the Header Extension.
			 */
			void RemoveHeaderExtension();

			/**
			 * Whether the Packet has One-Byte or Two-Bytes Extensions.
			 *
			 * @see RFC 8285.
			 */
			bool HasExtensions() const
			{
				return HasOneByteExtensions() || HasTwoBytesExtensions();
			}

			/**
			 * Whether the Packet has One-Byte Extensions.
			 *
			 * @see RFC 8285.
			 */
			bool HasOneByteExtensions() const
			{
				return GetHeaderExtensionId() == 0xBEDE;
			}

			/**
			 * Whether the Packet has Two-Bytes Extensions.
			 *
			 * @see RFC 8285.
			 */
			bool HasTwoBytesExtensions() const
			{
				return (GetHeaderExtensionId() & 0b1111111111110000) == 0b0001000000000000;
			}

			/**
			 * Whether the One-Byte or Two-Bytes Extension with given `id` exists in
			 * the Packet.
			 *
			 * @see RFC 8285.
			 *
			 * @remarks
			 * - If the length of the Extension value is 0 this method returns `true`.
			 */
			bool HasExtension(uint8_t id) const
			{
				if (id == 0)
				{
					return false;
				}
				else if (HasOneByteExtensions())
				{
					if (id > 14)
					{
						return false;
					}

					// `-1` because we have 14 elements total 0..13 and `id` is in the
					// range 1..14.
					auto offset = this->oneByteExtensions[id - 1];

					return offset != -1;
				}
				else if (HasTwoBytesExtensions())
				{
					return this->twoBytesExtensions.find(id) != this->twoBytesExtensions.end();
				}
				else
				{
					return false;
				}
			}

			/**
			 * Get a pointer to the value of the the One-Byte or Two-Bytes Extension
			 * with given `id` and set its value length into given `len`.
			 *
			 * @see RFC 8285.
			 */
			uint8_t* GetExtensionValue(uint8_t id, uint8_t& len) const
			{
				len = 0;

				if (id == 0)
				{
					len = 0;

					return nullptr;
				}
				else if (HasOneByteExtensions())
				{
					if (id > 14)
					{
						len = 0;

						return nullptr;
					}

					// `-1` because we have 14 elements total 0..13 and `id` is in the
					// range 1..14.
					const auto offset = this->oneByteExtensions[id - 1];

					if (offset == -1)
					{
						len = 0;

						return nullptr;
					}

					auto* extension = reinterpret_cast<OneByteExtension*>(GetHeaderExtensionValue() + offset);

					// In One-Byte Extensions value length 0 means 1.
					len = extension->len + 1;

					return extension->value;
				}
				else if (HasTwoBytesExtensions())
				{
					const auto it = this->twoBytesExtensions.find(id);

					if (it == this->twoBytesExtensions.end())
					{
						len = 0;

						return nullptr;
					}

					const auto offset = it->second;

					auto* extension = reinterpret_cast<TwoBytesExtension*>(GetHeaderExtensionValue() + offset);

					len = extension->len;

					return extension->value;
				}
				else
				{
					len = 0;

					return nullptr;
				}
			}

			/**
			 * Add or replace Extensions.
			 *
			 * @see RFC 8285.
			 *
			 * @throw MediaSoupTypeError - If there is no space available for given
			 *   Extensions or if given Extensions are invalid/wrong.
			 */
			void SetExtensions(ExtensionsType type, const std::vector<Extension>& extensions);

			/**
			 * Assign Extension ids.
			 *
			 * @see RFC 8285.
			 */
			void AssignExtensionIds(RTC::RtpHeaderExtensionIds& headerExtensionIds);

			bool ReadMid(std::string& mid) const;

			bool UpdateMid(const std::string& mid);

			bool ReadRid(std::string& rid) const;

			/**
			 * @remarks
			 * - `absSendTime` is set with the raw 3 bytes unsigned integer stored
			 *   in the Extension value.
			 */
			bool ReadAbsSendTime(uint32_t& absSendtime) const;

			/**
			 * @remarks
			 * - Contrary to `ReadAbsSendTime()` method, given `ms` is internally
			 *   converted to ABS Send Time.
			 */
			bool UpdateAbsSendTime(uint64_t ms) const;

			bool ReadTransportWideCc01(uint16_t& wideSeqNumber) const;

			bool UpdateTransportWideCc01(uint16_t wideSeqNumber) const;

			bool ReadSsrcAudioLevel(uint8_t& volume, bool& voice) const;

			bool ReadDependencyDescriptor(
			  std::unique_ptr<Codecs::DependencyDescriptor>& dependencyDescriptor,
			  std::unique_ptr<Codecs::DependencyDescriptor::TemplateDependencyStructure>&
			    templateDependencyStructure) const;

			bool UpdateDependencyDescriptor(const uint8_t* data, size_t len);

			bool ReadVideoOrientation(bool& camera, bool& flip, uint16_t& rotation) const;

			bool ReadAbsCaptureTime(uint64_t& absCaptureTimestamp, int64_t& estimatedCaptureClockOffset) const;

			bool ReadPlayoutDelay(uint16_t& minDelay, uint16_t& maxDelay) const;

			bool ReadMediasoupPacketId(uint32_t& mediasoupPacketId) const;

			/**
			 * Whether this Packet has payload.
			 *
			 * @remarks
			 * - This method doesn't validate whether the padding length announced in
			 *   the last byte of the Packet is valid.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			bool HasPayload() const
			{
				return GetPayloadLength() > 0;
			}

			/**
			 * Pointer to the beginning of the payload (if any). Payload length is
			 * set into given `length`.
			 *
			 * @remarks
			 * - This method doesn't validate whether the padding length announced in
			 *   the last byte of the Packet is valid.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 * - This method doens't take into account padding, so in a padding-only
			 *   Packet this method returns `nullptr` with `len` 0.
			 */
			uint8_t* GetPayload(size_t& len) const
			{
				auto* payloadPointer = GetPayloadPointer();
				const size_t availablePayloadAndPaddingLength = GetLength() - (payloadPointer - GetBuffer());
				const auto paddingLength = GetPaddingLength();

				// If there is announced padding, compute effective payload length
				// without padding.
				if (availablePayloadAndPaddingLength > paddingLength)
				{
					len = availablePayloadAndPaddingLength - paddingLength;

					return payloadPointer;
				}
				// If there are more announced padding bytes than the available length
				// for payload and padding, assume payload length 0.
				else
				{
					len = 0;

					return nullptr;
				}
			}

			/**
			 * Pointer to the beginning of the payload (if any).
			 *
			 * @remarks
			 * - This method doesn't validate whether the padding length announced in
			 *   the last byte of the Packet is valid.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 * - This method doens't take into account padding, so in a padding-only
			 *   Packet this method returns `nullptr`.
			 */
			uint8_t* GetPayload() const
			{
				thread_local size_t len;

				return GetPayload(len);
			}

			/**
			 * Length of the payload excluding padding bytes.
			 *
			 * @remarks
			 * - This method doesn't validate whether the padding length announced in
			 *   the last byte of the Packet is valid.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			size_t GetPayloadLength() const
			{
				const size_t availablePayloadAndPaddingLength =
				  GetLength() - (GetPayloadPointer() - GetBuffer());
				const auto paddingLength = GetPaddingLength();

				// If there is announced padding, compute effective payload length
				// without padding.
				if (availablePayloadAndPaddingLength >= paddingLength)
				{
					return availablePayloadAndPaddingLength - paddingLength;
				}
				// If there are more announced padding bytes than the available length
				// for payload and padding, return 0.
				else
				{
					return 0;
				}
			}

			/**
			 * Set the payload. It copies the given `payload` into the Packet.
			 *
			 * @remarks
			 * - This method removes existing padding (if any).
			 *
			 * @throw MediaSoupTypeError - If given `payloadLength` is higher than
			 *   available length for the payload of if `nullptr` is given as
			 *   `payload` with `payloadLength` higher than 0.
			 */
			void SetPayload(const uint8_t* payload, size_t payloadLength);

			/**
			 * Set the payload length.
			 *
			 * @remarks
			 * - This method removes existing padding (if any).
			 *
			 * @throw MediaSoupTypeError - If given `payloadLength` is higher than
			 *   available length for the payload.
			 */
			void SetPayloadLength(size_t payloadLength);

			/**
			 * Remove the payload.
			 *
			 * @remarks
			 * - This method removes existing padding (if any).
			 */
			void RemovePayload()
			{
				SetPayload(nullptr, 0);
			}

			/**
			 * Shift or unshift the content of the payload `delta` bytes after
			 * `payloadOffset`.
			 *
			 * @remarks
			 * - This method removes existing padding (if any).
			 *
			 * @throw MediaSoupTypeError - If wrong values are given.
			 */
			void ShiftPayload(size_t payloadOffset, int32_t delta);

			/**
			 * Whether this Packet has padding.
			 */
			bool HasPadding() const
			{
				return (GetFixedHeaderPointer()->padding);
			}

			/**
			 * Length of the padding.
			 *
			 * @remarks
			 * - This method doesn't validate whether the padding length announced in
			 *   the last byte of the Packet is valid.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			uint8_t GetPaddingLength() const
			{
				if (!HasPadding())
				{
					return 0;
				}

				return Utils::Byte::Get1Byte(GetBuffer(), GetLength() - 1);
			}

			/**
			 * Set padding length.
			 *
			 * @remarks
			 * - This method removes existing padding (if any).
			 *
			 * @throw MediaSoupTypeError - If given `paddingLength` is higher than
			 *   available length for the padding.
			 */
			void SetPaddingLength(uint8_t paddingLength);

			/**
			 * Whether Packet length is padded to 4 bytes.
			 */
			bool IsPaddedTo4Bytes() const
			{
				return Utils::Byte::IsPaddedTo4Bytes(GetLength());
			}

			/**
			 * Pad Packet length to 4 bytes by modifying padding bytes.
			 *
			 * @remarks
			 * - This method removes existing padding and adds up to 3 bytes of
			 *   padding bytes if needed.
			 *
			 * @throw MediaSoupTypeError - If needed padding length is higher than
			 *   available length for the padding.
			 */
			void PadTo4Bytes();

			/**
			 * Encode the Packet into a RTX Packet.
			 *
			 * @remarks
			 * - This method removes existing padding (if any).
			 *
			 * @throw MediaSoupTypeError - If there is no space for the new computed
			 *   payload.
			 */
			void RtxEncode(uint8_t payloadType, uint32_t ssrc, uint16_t seq);

			/**
			 * Decode the RTX Packet into a regular RTP Packet.
			 *
			 * @return `true` if RTX decoding iwas done.
			 *
			 * @remarks
			 * - This method removes existing padding (if any).
			 */
			bool RtxDecode(uint8_t payloadType, uint32_t ssrc);

			/**
			 * Set payload descritor handler.
			 */
			void SetPayloadDescriptorHandler(Codecs::PayloadDescriptorHandler* payloadDescriptorHandler);

			/**
			 * Process the payload.
			 */
			bool ProcessPayload(Codecs::EncodingContext* context, bool& marker);

			/**
			 * Get the payload encoder.
			 */
			std::unique_ptr<Codecs::PayloadDescriptor::Encoder> GetPayloadEncoder() const;

			/**
			 * Encode the payload.
			 */
			void EncodePayload(Codecs::PayloadDescriptor::Encoder* encoder);

			/**
			 * Restore the payload.
			 */
			void RestorePayload();

			/**
			 * Whether the payload contains a video keyframe.
			 */
			bool IsKeyFrame() const
			{
				if (!this->payloadDescriptorHandler)
				{
					return false;
				}

				return this->payloadDescriptorHandler->IsKeyFrame();
			}

			/**
			 * Spatial layer of the video codec.
			 */
			uint8_t GetSpatialLayer() const
			{
				if (!this->payloadDescriptorHandler)
				{
					return 0u;
				}

				return this->payloadDescriptorHandler->GetSpatialLayer();
			}

			/**
			 * Temporal layer of the video codec.
			 */
			uint8_t GetTemporalLayer() const
			{
				if (!this->payloadDescriptorHandler)
				{
					return 0u;
				}

				return this->payloadDescriptorHandler->GetTemporalLayer();
			}

		private:
			/**
			 * @remarks
			 * - Returns FixedHeader* instead of const FixedHeader* since we may want
			 *   to modify its fields.
			 */
			FixedHeader* GetFixedHeaderPointer() const
			{
				return reinterpret_cast<FixedHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			/**
			 * Pointer to the location where the CSRC list is supposed to begin.
			 */
			uint8_t* GetCsrcsPointer() const
			{
				return const_cast<uint8_t*>(GetBuffer()) + Packet::FixedHeaderMinLength;
			}

			/**
			 * Number of entries in the CSRC list.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced CSRC list.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			size_t GetCsrcCount() const
			{
				return GetFixedHeaderPointer()->csrcCount * sizeof(GetFixedHeaderPointer()->ssrc);
			}

			/**
			 * Pointer to the location where Extension Header is supposed to begin.
			 */
			HeaderExtension* GetHeaderExtensionPointer() const
			{
				return reinterpret_cast<HeaderExtension*>(GetCsrcsPointer() + GetCsrcCount());
			}

			/**
			 * Length of the Header Extension including the id & length four-octet.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced Header Extension.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			size_t GetHeaderExtensionLength() const
			{
				if (!HasHeaderExtension())
				{
					return 0;
				}

				return 4 + static_cast<size_t>(ntohs(GetHeaderExtensionPointer()->len) * 4);
			}

			/**
			 * Pointer to the location where the payload is supposed to begin.
			 */
			uint8_t* GetPayloadPointer() const
			{
				return reinterpret_cast<uint8_t*>(GetHeaderExtensionPointer()) + GetHeaderExtensionLength();
			}

			/**
			 * Validates whether the Packet is valid. It also stores internal
			 * containers holding Extensions if `storeExtensions` is `true`.
			 */
#ifdef MS_TEST
		public:
#endif
			bool Validate(bool storeExtensions);
#ifdef MS_TEST
		private:
#endif

			/**
			 * Parses Extensions. Returns `true` if they are valid. It also stores
			 * internal containers holding Extensions if `storeExtensions` is `true`.
			 *
			 * @see RFC 8285.
			 */
			bool ParseExtensions(bool storeExtensions);

			/**
			 * Set the value length of the Extension with given `id`.
			 *
			 * @remarks
			 * - The caller is responsible of not setting a length higher than the
			 *   available one (taking into account existing padding bytes).
			 * - If the Extension with `id` doesn't exist this methods terminates
			 *   the process.
			 */
			void SetExtensionLength(uint8_t id, uint8_t len);

			/* Pure virtual methods inherited from Codecs::DependencyDescriptor::Listener. */
		public:
			void OnDependencyDescriptorUpdated(const uint8_t* data, size_t len) override;

#ifdef MS_RTC_LOGGER_RTP
		public:
			RtcLogger::RtpPacket logger;
#endif

		private:
			// Array of One Byte Extensions. Index is the id - 1 of the Extension,
			// each entry is the offset (in bytes) from the beginning of the Header
			// Extension value to the beginning of the Extension.
			std::array<ssize_t, 14> oneByteExtensions{ -1, -1, -1, -1, -1, -1, -1,
				                                         -1, -1, -1, -1, -1, -1, -1 };
			// Ordered map of Two Bytes Extensions. Key is the id 1 of the Extension,
			// each entry is the offset (in bytes) from the beginning of the Header
			// Extension value to the beginning of the Extension.
			std::map<uint8_t, ssize_t> twoBytesExtensions;
			// Extension ids.
			RTC::RtpHeaderExtensionIds headerExtensionIds{};
			// Codec related.
			std::shared_ptr<Codecs::PayloadDescriptorHandler> payloadDescriptorHandler;
		};
	} // namespace RTP
} // namespace RTC

#endif
