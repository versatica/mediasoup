#define MS_CLASS "RTC::RTP::Packet"
// #define MS_LOG_DEV_LEVEL 3
// #define DUMP_PAYLOAD_DESCRIPTOR 1

#include "RTC/RTP/Packet.hpp"
#ifdef MS_RTC_LOGGER_RTP
#include "DepLibUV.hpp"
#endif
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring>  // std::memmove()
#include <iterator> // std::ostream_iterator
#include <sstream>  // std::ostringstream

namespace RTC
{
	namespace RTP
	{
		/* Class variables. */

		thread_local uint32_t Packet::nextMediasoupPacketId{ Utils::Crypto::GetRandomUInt(
			0u, std::numeric_limits<uint32_t>::max() / 2) };

		/* Class methods. */

		bool Packet::IsRtp(const uint8_t* buffer, size_t bufferLength)
		{
			const auto* header = const_cast<FixedHeader*>(reinterpret_cast<const FixedHeader*>(buffer));

			// clang-format off
			return (
				(bufferLength >= FixedHeaderMinSize) &&
				// @see https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
				(buffer[0] > 127 && buffer[0] < 192) &&
				// RTP Version must be 2.
				(header->version == 2)
			);
			// clang-format on
		}

		Packet* Packet::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (!Packet::IsRtp(buffer, bufferLength))
			{
				MS_WARN_TAG(rtp, "not a RTP Packet");

				return nullptr;
			}

			auto* packet = new Packet(const_cast<uint8_t*>(buffer), bufferLength);

			// Packet length must be the length of the given buffer.
			packet->SetLength(bufferLength);

			if (!packet->Validate())
			{
				delete packet;
				return nullptr;
			}

			// Mark the Packet as frozen since we are parsing the given buffer.
			packet->Freeze();

			return packet;
		}

		Packet* Packet::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < FixedHeaderMinSize)
			{
				MS_THROW_TYPE_ERROR("no space for fixed header");
			}

			auto* packet      = new Packet(buffer, bufferLength);
			auto* fixedHeader = packet->GetFixedHeaderPointer();

			fixedHeader->version        = 2;
			fixedHeader->padding        = 0;
			fixedHeader->extension      = 0;
			fixedHeader->csrcCount      = 0;
			fixedHeader->marker         = 0;
			fixedHeader->payloadType    = 0;
			fixedHeader->sequenceNumber = 0;
			fixedHeader->timestamp      = 0;
			fixedHeader->ssrc           = 0;

			// No need to invoke SetLength() since constructor invoked it with
			// minimum Packet length.

			return packet;
		}

		uint32_t Packet::GetNextMediasoupPacketId()
		{
			MS_TRACE();

			return Packet::nextMediasoupPacketId++;
		}

		/* Instance methods. */

		Packet::Packet(uint8_t* buffer, size_t bufferLength) : Serializable(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(FixedHeaderMinSize);

#ifdef MS_RTC_LOGGER_RTP
			// Initialize logger.
			this->logger.timestamp        = DepLibUV::GetTimeMs();
			this->logger.recvRtpTimestamp = GetTimestamp();
			this->logger.recvSeqNumber    = GetSequenceNumber();
#endif
		}

		Packet::~Packet()
		{
			MS_TRACE();
		}

		void Packet::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTP::Packet>");
			MS_DUMP_CLEAN(indentation, "  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP_CLEAN(indentation, "  frozen: %s", IsFrozen() ? "yes" : "no");

			MS_DUMP_CLEAN(indentation, "  sequence number: %" PRIu16, GetSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  timestamp: %" PRIu32, GetTimestamp());
			MS_DUMP_CLEAN(indentation, "  marker: %s", HasMarker() ? "true" : "false");
			MS_DUMP_CLEAN(indentation, "  payload type: %" PRIu8, GetPayloadType());
			MS_DUMP_CLEAN(indentation, "  ssrc: %" PRIu32, GetSsrc());
			MS_DUMP_CLEAN(indentation, "  csrcs: %s", HasCsrcs() ? "true" : "false");

			if (HasHeaderExtension())
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  header extension: id:%" PRIu16 ", value length:%zu",
				  GetHeaderExtensionId(),
				  GetHeaderExtensionValueLength());
			}

			if (HasExtensions())
			{
				std::vector<std::string> extIds;
				std::ostringstream extIdsStream;

				if (HasOneByteExtensions())
				{
					for (const auto offset : this->oneByteExtensions)
					{
						if (offset == -1)
						{
							continue;
						}

						auto* extension = reinterpret_cast<OneByteExtension*>(GetHeaderExtensionValue() + offset);

						extIds.push_back(
						  "{id:" + std::to_string(extension->id) + ", len:" + std::to_string(extension->len) +
						  "}");
					}
				}
				else
				{
					extIds.reserve(this->twoBytesExtensions.size());

					for (const auto& kv : this->twoBytesExtensions)
					{
						const auto offset = kv.second;

						if (offset == -1)
						{
							continue;
						}

						auto* extension =
						  reinterpret_cast<TwoBytesExtension*>(GetHeaderExtensionValue() + offset);

						extIds.push_back(
						  "{id:" + std::to_string(extension->id) + ", len:" + std::to_string(extension->len) +
						  "}");
					}
				}

				if (!extIds.empty())
				{
					std::copy(
					  extIds.begin(), extIds.end() - 1, std::ostream_iterator<std::string>(extIdsStream, ", "));
					extIdsStream << extIds.back();

					MS_DUMP_CLEAN(
					  indentation,
					  "  RFC5285 extensions (%s): %s",
					  HasOneByteExtensions() ? "One-Byte" : "Two-Bytes",
					  extIdsStream.str().c_str());
				}
			}

			// TODO: Specific Extensions.

			MS_DUMP_CLEAN(indentation, "  payload length: %zu", GetPayloadLength());
			MS_DUMP_CLEAN(indentation, "  padding length: %" PRIu8, GetPaddingLength());

			if (this->payloadDescriptorHandler)
			{
				MS_DUMP_CLEAN(indentation, "  key frame: %s", IsKeyFrame() ? "true" : "false");
				MS_DUMP_CLEAN(indentation, "  spatial layer: %" PRIu8, GetSpatialLayer());
				MS_DUMP_CLEAN(indentation, "  temporal layer: %" PRIu8, GetTemporalLayer());
#ifdef DUMP_PAYLOAD_DESCRIPTOR
				this->payloadDescriptorHandler->Dump(indentation + 1);
#endif
			}

			MS_DUMP_CLEAN(indentation, "</RTP::Packet>");
		}

		Packet* Packet::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedPacket = new Packet(buffer, bufferLength);

			Serializable::CloneInto(clonedPacket);

			// Clone Extension containers.
			clonedPacket->oneByteExtensions  = this->oneByteExtensions;
			clonedPacket->twoBytesExtensions = this->twoBytesExtensions;
			// Assign the payload descriptor handler.
			clonedPacket->payloadDescriptorHandler = this->payloadDescriptorHandler;

			if (this->payloadDescriptorHandler)
			{
				clonedPacket->payloadDescriptorHandler->RtpPacketChanged(clonedPacket);
			}

			return clonedPacket;
		}

		void Packet::SetPayloadType(uint8_t payloadType)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetFixedHeaderPointer()->payloadType = payloadType;
		}

		void Packet::SetMarker(bool marker)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetFixedHeaderPointer()->marker = marker;
		}

		void Packet::SetSequenceNumber(uint16_t seq)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetFixedHeaderPointer()->sequenceNumber = htons(seq);
		}

		void Packet::SetTimestamp(uint32_t timestamp)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetFixedHeaderPointer()->timestamp = htonl(timestamp);
		}

		void Packet::SetSsrc(uint32_t ssrc)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetFixedHeaderPointer()->ssrc = htonl(ssrc);
		}

		void Packet::RemoveHeaderExtension()
		{
			MS_TRACE();

			AssertNotFrozen();

			if (!HasHeaderExtension())
			{
				return;
			}

			// Clear One-Byte and Two-Bytes Extensions.
			std::fill(std::begin(this->oneByteExtensions), std::end(this->oneByteExtensions), -1);
			this->twoBytesExtensions.clear();

			const auto headerExtensionLength = GetHeaderExtensionLength();

			auto* payload            = GetPayloadPointer();
			const auto payloadLength = GetPayloadLength();
			const auto paddingLength = GetPaddingLength();

			// Update Packet length.
			// NOTE: This throws if given length is higher than buffer length.
			SetLength(GetLength() - headerExtensionLength);

			// Unset the Header Extension flag.
			GetFixedHeaderPointer()->extension = 0;

			// Shift the payload.
			std::memmove(payload - headerExtensionLength, payload, payloadLength + paddingLength);
		}

		void Packet::SetExtensions(ExtensionsType type, const std::vector<AddedExtension>& extensions)
		{
			MS_TRACE();

			AssertNotFrozen();

			// Clear One-Byte and Two-Bytes Extensions.
			std::fill(std::begin(this->oneByteExtensions), std::end(this->oneByteExtensions), -1);
			this->twoBytesExtensions.clear();

			const auto hadHeaderExtension                 = HasHeaderExtension();
			const auto previousHeaderExtensionValueLength = GetHeaderExtensionValueLength();

			// If no explicit ExtensionType is given then select the best one based
			// on given Extensions.
			if (type == ExtensionsType::Auto)
			{
				uint8_t highestId{ 0u };
				uint8_t highestLen{ 0u };

				for (const auto& extension : extensions)
				{
					highestId  = std::max(extension.id, highestId);
					highestLen = std::max(extension.len, highestLen);
				}

				type = highestId <= 14 && highestLen > 0 && highestLen <= 16 ? ExtensionsType::OneByte
				                                                             : ExtensionsType::TwoBytes;

				MS_DEBUG_DEV(
				  "using %" PRIu8 " byte(s) extensions [highestId:%" PRIu8 ", highestLen:%" PRIu8 "]",
				  type,
				  highestId,
				  highestLen);
			}

			// If One-Byte is requested and the Packet already has One-Byte Extensions,
			// keep the Header Extension id.
			if (type == ExtensionsType::OneByte && HasOneByteExtensions())
			{
				// Nothing to do.
			}
			// If Two-Bytes is requested and the Packet already has Two-Bytes Extensions,
			// keep the Header Extension id.
			else if (type == ExtensionsType::TwoBytes && HasTwoBytesExtensions())
			{
				// Nothing to do.
			}
			// Otherwise, if there is Header Extension of non matching type, modify its id.
			else if (hadHeaderExtension)
			{
				if (type == ExtensionsType::OneByte)
				{
					GetHeaderExtensionPointer()->id = htons(0xBEDE);
				}
				else if (type == ExtensionsType::TwoBytes)
				{
					GetHeaderExtensionPointer()->id = htons(0b0001000000000000);
				}
			}

			// Calculate total length required for all Extensions (with padding if needed).
			size_t extensionsLength{ 0 };

			if (type == ExtensionsType::OneByte)
			{
				for (const auto& extension : extensions)
				{
					if (extension.id == 0)
					{
						MS_THROW_TYPE_ERROR("invalid Extension with id 0");
					}
					else if (extension.id > 14)
					{
						MS_THROW_TYPE_ERROR(
						  "invalid Extension with id %" PRIu8 " > 14 when using One-Byte Extensions",
						  extension.id);
					}
					else if (extension.len == 0)
					{
						MS_THROW_TYPE_ERROR(
						  "invalid Extension with id %" PRIu8 " and length 0 when using One-Byte Extensions",
						  extension.id);
					}
					else if (extension.len > 16)
					{
						MS_THROW_TYPE_ERROR(
						  "invalid Extension with id %" PRIu8 " and length %" PRIu8
						  " when using One-Byte Extensions",
						  extension.id,
						  extension.len);
					}

					extensionsLength += (1 + extension.len);
				}
			}
			else if (type == ExtensionsType::TwoBytes)
			{
				for (const auto& extension : extensions)
				{
					if (extension.id == 0)
					{
						MS_THROW_TYPE_ERROR("invalid Extension with id 0");
					}

					extensionsLength += (2 + extension.len);
				}
			}

			auto paddedExtensionsLength          = Utils::Byte::PadTo4Bytes(extensionsLength);
			const size_t extensionsPaddingLength = paddedExtensionsLength - extensionsLength;

			// Calculate the number of bytes to shift (may be negative if the Packet
			// already had Header Extension).
			int16_t shift{ 0 };

			if (hadHeaderExtension)
			{
				shift = static_cast<int16_t>(paddedExtensionsLength - previousHeaderExtensionValueLength);
			}
			else
			{
				shift = 4 + static_cast<int16_t>(paddedExtensionsLength);
			}

			auto* payload            = GetPayloadPointer();
			const auto payloadLength = GetPayloadLength();
			const auto paddingLength = GetPaddingLength();

			if (hadHeaderExtension && shift != 0)
			{
				// Update Packet length.
				// NOTE: This throws if given length is higher than buffer length.
				SetLength(GetLength() + shift);

				// Update the Header Extension length.
				GetHeaderExtensionPointer()->len = htons(paddedExtensionsLength / 4);

				// Shift the payload.
				std::memmove(payload + shift, payload, payloadLength + paddingLength);
			}
			else if (!hadHeaderExtension)
			{
				// Update Packet length.
				// NOTE: This throws if given length is higher than buffer length.
				SetLength(GetLength() + shift);

				// Set the Header Extension flag.
				GetFixedHeaderPointer()->extension = 1;

				auto* headerExtension = GetHeaderExtensionPointer();

				// Shift the payload.
				// NOTE: We need to move payload before code below, otherwise we would
				// override written bytes later.
				std::memmove(payload + shift, payload, payloadLength + paddingLength);

				// Set the Header Extension id.
				if (type == ExtensionsType::OneByte)
				{
					headerExtension->id = htons(0xBEDE);
				}
				else if (type == ExtensionsType::TwoBytes)
				{
					headerExtension->id = htons(0b0001000000000000);
				}

				// Set the Header Extension length.
				headerExtension->len = htons(paddedExtensionsLength / 4);
			}

			const uint8_t* extensionsStart = GetHeaderExtensionValue();
			uint8_t* ptr                   = const_cast<uint8_t*>(extensionsStart);

			if (type == ExtensionsType::OneByte)
			{
				for (const auto& extension : extensions)
				{
					// Store the One-Byte Extension offset in the array.
					// `-1` because we have 14 elements total 0..13 and `id` is in the
					// range 1..14.
					this->oneByteExtensions[extension.id - 1] = ptr - extensionsStart;

					*ptr = (extension.id << 4) | ((extension.len - 1) & 0x0F);
					++ptr;
					std::memmove(ptr, extension.value, extension.len);
					ptr += extension.len;
				}
			}
			else if (type == ExtensionsType::TwoBytes)
			{
				for (const auto& extension : extensions)
				{
					// Store the Two-Bytes Extension offset in the map.
					this->twoBytesExtensions[extension.id] = ptr - extensionsStart;

					*ptr = extension.id;
					++ptr;
					*ptr = extension.len;
					++ptr;
					std::memmove(ptr, extension.value, extension.len);
					ptr += extension.len;
				}
			}

			for (size_t i = 0; i < extensionsPaddingLength; ++i)
			{
				*ptr = 0u;
				++ptr;
			}
		}

		void Packet::SetPayload(const uint8_t* payload, size_t payloadLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			if (!payload && payloadLength > 0)
			{
				MS_THROW_TYPE_ERROR("invalid payloadLength %zu without payload", payloadLength);
			}

			auto previousLength        = GetLength();
			auto previousPayloadLength = GetPayloadLength();
			auto previousPaddingLength = GetPaddingLength();
			auto newLength = previousLength - previousPayloadLength - previousPaddingLength + payloadLength;

			// Set the new Packet total length.
			// NOTE: This throws if given length is higher than buffer length.
			SetLength(newLength);

			// Unset padding flag.
			GetFixedHeaderPointer()->padding = 0;

			std::memmove(GetPayloadPointer(), payload, payloadLength);
		}

		void Packet::ShiftPayload(size_t payloadOffset, int32_t delta)
		{
			MS_TRACE();

			AssertNotFrozen();

			if (delta == 0)
			{
				return;
			}

			auto* payload            = GetPayloadPointer();
			const auto payloadLength = GetPayloadLength();
			const auto absDelta =
			  delta < 0 ? static_cast<uint32_t>(-(int64_t)delta) : static_cast<uint32_t>(delta);

			if (payloadOffset >= payloadLength)
			{
				MS_THROW_TYPE_ERROR(
				  "payloadOffset (%zu) is bigger than payload length (%zu)", payloadOffset, payloadLength);
			}
			else if (delta < 0 && absDelta > (payloadLength - payloadOffset))
			{
				MS_THROW_TYPE_ERROR("negative delta (%" PRIi32 ") too big", delta);
			}

			// Remove padding (if any).
			if (HasPadding())
			{
				SetPaddingLength(0);
			}

			if (delta > 0)
			{
				// Update Packet length.
				// NOTE: This throws if given length is higher than buffer length.
				SetLength(GetLength() + delta);

				std::memmove(
				  payload + payloadOffset + delta, payload + payloadOffset, payloadLength - payloadOffset);
			}
			else
			{
				// Update Packet length.
				// NOTE: This throws if given length is higher than buffer length.
				SetLength(GetLength() - absDelta);

				std::memmove(
				  payload + payloadOffset,
				  payload + payloadOffset + absDelta,
				  payloadLength - payloadOffset - absDelta);
			}
		}

		void Packet::SetPaddingLength(uint8_t paddingLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousLength        = GetLength();
			auto previousPaddingLength = GetPaddingLength();
			auto newLength             = previousLength - previousPaddingLength + paddingLength;

			// Set the new Packet total length.
			// NOTE: This throws if given length is higher than buffer length.
			SetLength(newLength);

			if (paddingLength > 0)
			{
				GetFixedHeaderPointer()->padding = 1;

				Utils::Byte::Set1Byte(const_cast<uint8_t*>(GetBuffer()), GetLength() - 1, paddingLength);
			}
			else
			{
				GetFixedHeaderPointer()->padding = 0;
			}
		}

		void Packet::PadTo4Bytes()
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousLength        = GetLength();
			auto previousPaddingLength = GetPaddingLength();
			auto newNotPaddedLength    = previousLength - previousPaddingLength;
			auto newPaddedLength       = Utils::Byte::PadTo4Bytes(newNotPaddedLength);

			if (newPaddedLength == previousLength)
			{
				return;
			}

			// Set the new Packet total length.
			// NOTE: This throws if given length is higher than buffer length.
			SetLength(newPaddedLength);

			auto newPaddingLength = newPaddedLength - newNotPaddedLength;

			if (newPaddingLength > 0)
			{
				GetFixedHeaderPointer()->padding = 1;

				Utils::Byte::Set1Byte(const_cast<uint8_t*>(GetBuffer()), GetLength() - 1, newPaddingLength);
			}
			else
			{
				GetFixedHeaderPointer()->padding = 0;
			}
		}

		void Packet::RtxEncode(uint8_t payloadType, uint32_t ssrc, uint16_t seq)
		{
			MS_TRACE();

			AssertNotFrozen();

			// Remove padding (if any).
			if (HasPadding())
			{
				// NOTE: This must be called before SetLength() method below.
				SetPaddingLength(0);
			}

			// Update Packet length.
			// NOTE: This throws if given length is higher than buffer length.
			SetLength(GetLength() + 2);

			// Rewrite the payload type.
			SetPayloadType(payloadType);

			// Rewrite the SSRC.
			SetSsrc(ssrc);

			auto* payload            = GetPayloadPointer();
			const auto payloadLength = GetPayloadLength();

			// Write the original sequence number at the begining of the payload.
			std::memmove(payload + 2, payload, payloadLength);
			Utils::Byte::Set2Bytes(payload, 0, GetSequenceNumber());

			// Rewrite the sequence number.
			SetSequenceNumber(seq);
		}

		bool Packet::RtxDecode(uint8_t payloadType, uint32_t ssrc)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto* payload            = GetPayloadPointer();
			const auto payloadLength = GetPayloadLength();

			// NOTE: libwebrtc sends some RTX packets with no payload when the stream
			// is started. Just ignore them.
			if (payloadLength < 2)
			{
				return false;
			}

			// Rewrite the payload type.
			SetPayloadType(payloadType);

			// Rewrite the sequence number.
			SetSequenceNumber(Utils::Byte::Get2Bytes(payload, 0));

			// Rewrite the SSRC.
			SetSsrc(ssrc);

			// Shift the payload to its original place.
			std::memmove(payload, payload + 2, payloadLength - 2);

			// Remove padding (if any).
			if (HasPadding())
			{
				// NOTE: This must be called before SetLength() method below.
				SetPaddingLength(0);
			}

			// Update Packet length.
			SetLength(GetLength() - 2);

			return true;
		}

		void Packet::SetPayloadDescriptorHandler(
		  RTC::Codecs::PayloadDescriptorHandler* payloadDescriptorHandler)
		{
			MS_TRACE();

			AssertNotFrozen();

			this->payloadDescriptorHandler.reset(payloadDescriptorHandler);
		}

		bool Packet::ProcessPayload(RTC::Codecs::EncodingContext* context, bool& marker)
		{
			MS_TRACE();

			AssertNotFrozen();

			if (!this->payloadDescriptorHandler)
			{
				return true;
			}

			return this->payloadDescriptorHandler->Process(context, this, marker);
		}

		std::unique_ptr<RTC::Codecs::PayloadDescriptor::Encoder> Packet::GetPayloadEncoder() const
		{
			MS_TRACE();

			if (!this->payloadDescriptorHandler)
			{
				return nullptr;
			}

			return this->payloadDescriptorHandler->GetEncoder();
		}

		void Packet::EncodePayload(RTC::Codecs::PayloadDescriptor::Encoder* encoder)
		{
			MS_TRACE();

			AssertNotFrozen();

			if (!this->payloadDescriptorHandler)
			{
				return;
			}

			this->payloadDescriptorHandler->Encode(this, encoder);
		}

		void Packet::RestorePayload()
		{
			MS_TRACE();

			AssertNotFrozen();

			if (!this->payloadDescriptorHandler)
			{
				return;
			}

			this->payloadDescriptorHandler->Restore(this);
		}

		bool Packet::Validate()
		{
			MS_TRACE();

			// Here we are at the beginning of the Packet.
			const auto* ptr = const_cast<uint8_t*>(GetBuffer());

			if (GetVersion() != 2)
			{
				MS_WARN_TAG(rtp, "invalid Packet, version must be 2");

				return false;
			}

			ptr += FixedHeaderMinSize;

			// Here we are at the beginning of the optional CCRS list.
			if (HasCsrcs())
			{
				auto csrcsLength = GetCsrcCount();

				if (GetLength() < (ptr - GetBuffer()) + csrcsLength)
				{
					MS_WARN_TAG(rtp, "invalid Packet, not enough space for the announced CSRC list");

					return false;
				}

				ptr += csrcsLength;
			}

			// Here we are at the beginning of the optional Header Extension.
			if (HasHeaderExtension())
			{
				// The Header Extension is at least 4 bytes.
				if (GetLength() < (ptr - GetBuffer()) + 4)
				{
					MS_WARN_TAG(rtp, "invalid Packet, not enough space for the announced Header Extension");

					return false;
				}

				const auto headerExtensionLength = GetHeaderExtensionLength();

				if (GetLength() < (ptr - GetBuffer()) + headerExtensionLength)
				{
					MS_WARN_TAG(
					  rtp, "invalid Packet, not enough space for the announced Header Extension value");

					return false;
				}

				if (!ParseExtensions())
				{
					MS_WARN_TAG(rtp, "invalid Packet, invalid Extensions");

					return false;
				}

				ptr += headerExtensionLength;
			}

			// Here we are at the beginning of the optional payload.
			const auto payloadLength = GetPayloadLength();
			const auto paddingLength = GetPaddingLength();
			const auto availablePayloadAndPaddingLength = GetLength() - (GetPayloadPointer() - GetBuffer());

			if (payloadLength + paddingLength != availablePayloadAndPaddingLength)
			{
				MS_WARN_TAG(rtp, "invalid Packet, not enough space for announced padding");

				return false;
			}

			if (HasPadding() && paddingLength == 0)
			{
				MS_WARN_TAG(rtp, "invalid Packet, padding byte cannot be 0");

				return false;
			}

			ptr += availablePayloadAndPaddingLength;

			// Here we are at the end of the Packet.
			MS_ASSERT(
			  ptr - GetBuffer() == GetLength(),
			  "Packet computed length does not match its assigned length");

			return true;
		}

		bool Packet::ParseExtensions()
		{
			MS_TRACE();

			if (HasOneByteExtensions())
			{
				const uint8_t* extensionsStart = GetHeaderExtensionValue();
				const uint8_t* extensionsEnd   = extensionsStart + GetHeaderExtensionValueLength();
				uint8_t* ptr                   = const_cast<uint8_t*>(extensionsStart);

				// One-Byte Extensions cannot have length 0.
				while (ptr < extensionsEnd)
				{
					const auto* extension = reinterpret_cast<OneByteExtension*>(ptr);
					const uint8_t id      = extension->id;
					// NOTE: In Ont-Byte Extensions, announced value must be incremented
					// by 1.
					const size_t len = extension->len + 1;

					// id=0 means alignment.
					if (id == 0)
					{
						++ptr;
					}
					// id=15 in One-Byte extensions means "stop parsing here".
					else if (id == 15)
					{
						break;
					}
					// Valid Extension id.
					else
					{
						if (ptr + 1 + len > extensionsEnd)
						{
							MS_WARN_TAG(
							  rtp,
							  "not enough space for the announced value of the One-Byte Extension with id %" PRIu8,
							  id);

							return false;
						}

						// Store the One-Byte Extension offset in the array.
						// `-1` because we have 14 elements total 0..13 and `id` is in the
						// range 1..14.
						this->oneByteExtensions[id - 1] = ptr - extensionsStart;

						ptr += (1 + len);
					}

					// Counting padding bytes.
					while (ptr < extensionsEnd && *ptr == 0)
					{
						++ptr;
					}
				}

				return true;
			}
			else if (HasTwoBytesExtensions())
			{
				const uint8_t* extensionsStart = GetHeaderExtensionValue();
				const uint8_t* extensionsEnd   = extensionsStart + GetHeaderExtensionValueLength();
				// ptr points to the Extension id field (1 byte).
				// ptr+1 points to the length field (1 byte, can have value 0).
				uint8_t* ptr = const_cast<uint8_t*>(extensionsStart);

				// Two-Byte Extensions can have length 0.
				while (ptr + 1 < extensionsEnd)
				{
					const auto* extension = reinterpret_cast<TwoBytesExtension*>(ptr);
					const uint8_t id      = extension->id;
					const size_t len      = extension->len;

					// id=0 means alignment.
					if (id == 0)
					{
						++ptr;
					}
					// Valid Extension id.
					else
					{
						if (ptr + 2 + len > extensionsEnd)
						{
							MS_WARN_TAG(
							  rtp,
							  "not enough space for the announced value of the Two-Bytes Extension with id %" PRIu8,
							  id);

							return false;
						}

						// Store the Two-Bytes Extension offset in the map.
						this->twoBytesExtensions[id] = ptr - extensionsStart;

						ptr += (2 + len);
					}

					// Counting padding bytes.
					while (ptr < extensionsEnd && *ptr == 0)
					{
						++ptr;
					}
				}

				return true;
			}
			// If there is no Header Extension of if there is but it doesn't conform
			// to RFC 8285 Extensions, then this is ok.
			else
			{
				return true;
			}
		}

		void Packet::OnDependencyDescriptorUpdated(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			AssertNotFrozen();

			MS_DUMP("TODO");
			// UpdateDependencyDescriptor(data, size_t len);
		}
	} // namespace RTP
} // namespace RTC
