#ifndef MS_RTC_RTP_PACKET_HPP
#define MS_RTC_RTP_PACKET_HPP

#include "common.hpp"
#include "FBS/rtpPacket.h"
#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"
#include "RTC/RTP/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTP/HeaderExtensionIds.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/Serializable.hpp"
#include "Utils.hpp"
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
			 *
			 * @remarks
			 * - This struct is guaranteed to be aligned to 4 bytes.
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

#ifdef MS_TEST
		public:
#else
		private:
#endif
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
			 *
			 * @remarks
			 * - This struct is guaranteed to be aligned to 2 bytes.
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
				 * extensions.
				 */
				Auto     = 0,
				OneByte  = 1,
				TwoBytes = 2
			};

#ifdef MS_TEST
		public:
#else
		private:
#endif
			/**
			 * @remarks
			 * - This struct is guaranteed to be aligned to 1 byte.
			 */
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

#ifdef MS_TEST
		public:
#else
		private:
#endif
			/**
			 * @remarks
			 * - This struct is guaranteed to be aligned to 1 byte.
			 */
			struct TwoBytesExtension
			{
				uint8_t id;
				uint8_t len;
				uint8_t value[];
			};

		public:
			/**
			 * Struct for setting and replacing Extensions.
			 *
			 * @remarks
			 * - This struct is NOT guaranteed to be aligned to any fixed number of
			 *   bytes because it contains a pointer, which is 4 or 8 bytes depending
			 *   on the architecture. Anyway we never cast any buffer to this struct.
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
			 * Whether given buffer could be a valid RTP packet.
			 *
			 * @remarks
			 * - Before calling this static method, the caller should verify whether
			 *   the given buffer is a RTCP packet.
			 */
			static bool IsRtp(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Parse a RTP packet.
			 *
			 * @remarks
			 * - `packetLength` must be the exact length of the packet.
			 * - `bufferLength` must be >= `packetLength`.
			 *
			 * @throw MediaSoupTypeError - If `bufferLength` is lower than
			 *   `packetLength`.
			 */
			static Packet* Parse(const uint8_t* buffer, size_t packetLength, size_t bufferLength);

			/**
			 * Parse a RTP packet.
			 *
			 * @remarks
			 * - `bufferLength` must be the exact length of the packet.
			 */
			static Packet* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a RTP packet.
			 */
			static Packet* Factory(uint8_t* buffer, size_t bufferLength);

			/**
			 * Generate value for "urn:mediasoup:params:rtp-hdrext:packet-id"
			 * mediasoup custom extension.
			 */
			static uint32_t GetNextMediasoupPacketId();

		private:
			static thread_local uint32_t nextMediasoupPacketId;

		private:
			/**
			 * Constructor is private because we only want to create packet instances
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
			 * Get the extension header id or 0 if there isn't.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced header extension.
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
			 * Pointer to the header extension value or `nullptr` if there is no
			 * header extension or its has no value.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced header extension.
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
			 * Length of the header extension value (excluding the id & length
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
			 * Remove the header extension.
			 */
			void RemoveHeaderExtension();

			/**
			 * Whether the packet has One-Byte or Two-Bytes extensions.
			 *
			 * @see RFC 8285.
			 */
			bool HasExtensions() const
			{
				return HasOneByteExtensions() || HasTwoBytesExtensions();
			}

			/**
			 * Whether the packet has One-Byte extensions.
			 *
			 * @see RFC 8285.
			 */
			bool HasOneByteExtensions() const
			{
				return GetHeaderExtensionId() == 0xBEDE;
			}

			/**
			 * Whether the packet has Two-Bytes extensions.
			 *
			 * @see RFC 8285.
			 */
			bool HasTwoBytesExtensions() const
			{
				return (GetHeaderExtensionId() & 0b1111111111110000) == 0b0001000000000000;
			}

			/**
			 * Whether the One-Byte or Two-Bytes extension with given `id` exists in
			 * the packet.
			 *
			 * @see RFC 8285.
			 *
			 * @remarks
			 * - If the length of the extension value is 0 this method returns `true`.
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
					const auto offset = this->oneByteExtensions[id - 1];

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
			 * Get a pointer to the value of the the One-Byte or Two-Bytes extension
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

					// In One-Byte extensions value length 0 means 1.
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
			 * Add or replace extensions.
			 *
			 * @see RFC 8285.
			 *
			 * @throw MediaSoupTypeError - If there is no space available for given
			 *   extensions or if given extensions are invalid/wrong.
			 */
			void SetExtensions(ExtensionsType type, const std::vector<Extension>& extensions);

			/**
			 * Assign extension ids.
			 *
			 * @see RFC 8285.
			 */
			void AssignExtensionIds(RTP::HeaderExtensionIds& headerExtensionIds);

			bool ReadMid(std::string& mid) const;

			bool UpdateMid(const std::string& mid);

			bool ReadRid(std::string& rid) const;

			/**
			 * @remarks
			 * - `absSendTime` is set with the raw 3 bytes unsigned integer stored
			 *   in the extension value.
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
			 * Whether this packet has payload.
			 *
			 * @remarks
			 * - This method doesn't validate whether the padding length announced in
			 *   the last byte of the packet is valid.
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
			 *   the last byte of the packet is valid.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 * - This method doens't take into account padding, so in a padding-only
			 *   packet this method returns `nullptr` with `len` 0.
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
			 *   the last byte of the packet is valid.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 * - This method doens't take into account padding, so in a padding-only
			 *   packet this method returns `nullptr`.
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
			 *   the last byte of the packet is valid.
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
			 * Set the payload. It copies the given `payload` into the packet.
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
			 * Whether this packet has padding.
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
			 *   the last byte of the packet is valid.
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
			 * Whether packet length is padded to 4 bytes.
			 */
			bool IsPaddedTo4Bytes() const
			{
				return Utils::Byte::IsPaddedTo4Bytes(GetLength());
			}

			/**
			 * Pad packet length to 4 bytes by modifying padding bytes.
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
			 * Encode the packet into a RTX packet.
			 *
			 * @remarks
			 * - This method removes existing padding (if any).
			 *
			 * @throw MediaSoupTypeError - If there is no space for the new computed
			 *   payload.
			 */
			void RtxEncode(uint8_t payloadType, uint32_t ssrc, uint16_t seq);

			/**
			 * Decode the RTX packet into a regular RTP packet.
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
			 * Pointer to the location where extension header is supposed to begin.
			 */
			HeaderExtension* GetHeaderExtensionPointer() const
			{
				return reinterpret_cast<HeaderExtension*>(GetCsrcsPointer() + GetCsrcCount());
			}

			/**
			 * Length of the header extension including the id & length four-octet.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced header extension.
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
			 * Validates whether the packet is valid. It also stores internal
			 * containers holding extensions if `storeExtensions` is `true`.
			 */
#ifdef MS_TEST
		public:
#endif
			bool Validate(bool storeExtensions);
#ifdef MS_TEST
		private:
#endif

			/**
			 * Parses extensions. Returns `true` if they are valid. It also stores
			 * internal containers holding extensions if `storeExtensions` is `true`.
			 *
			 * @see RFC 8285.
			 */
			bool ParseExtensions(bool storeExtensions);

			/**
			 * Set the value length of the extension with given `id`.
			 *
			 * @remarks
			 * - The caller is responsible of not setting a length higher than the
			 *   available one (taking into account existing padding bytes).
			 * - If the extension with `id` doesn't exist this methods terminates
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
			// Array of One Byte Extensions. Index is the id - 1 of the extension,
			// each entry is the offset (in bytes) from the beginning of the header
			// extension value to the beginning of the extension.
			std::array<ssize_t, 14> oneByteExtensions{ -1, -1, -1, -1, -1, -1, -1,
				                                         -1, -1, -1, -1, -1, -1, -1 };
			// Ordered map of Two Bytes Extensions. Key is the id 1 of the extension,
			// each entry is the offset (in bytes) from the beginning of the header
			// extension value to the beginning of the extension.
			std::map<uint8_t, ssize_t> twoBytesExtensions;
			// Extension ids.
			RTP::HeaderExtensionIds headerExtensionIds{};
			// Codec related.
			std::shared_ptr<Codecs::PayloadDescriptorHandler> payloadDescriptorHandler;
		};
	} // namespace RTP
} // namespace RTC

#endif
