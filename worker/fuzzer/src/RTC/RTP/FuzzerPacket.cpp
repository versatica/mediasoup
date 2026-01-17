#include "RTC/RTP/FuzzerPacket.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpHeaderExtensionIds.hpp"
#include <string>
#include <vector>

void Fuzzer::RTC::RTP::Packet::Fuzz(const uint8_t* data, size_t len)
{
	if (!::RTC::RTP::Packet::IsRtp(data, len))
	{
		return;
	}

	std::unique_ptr<::RTC::RTP::Packet> packet{ ::RTC::RTP::Packet::Parse(data, len, len) };

	if (!packet)
	{
		return;
	}

	// We need to serialize the Packet into a separate buffer because setters
	// below will try to write into packet memory.
	//
	// NOTE: Let's make the buffer bigger to test API that increases packet size.
	std::unique_ptr<uint8_t[]> buffer(new uint8_t[len + 512]);

	packet->Serialize(buffer.get(), len + 512);

	std::vector<::RTC::RTP::Packet::Extension> extensions;
	uint8_t extenLen;
	bool voice;
	uint8_t volume;
	bool camera;
	bool flip;
	uint16_t rotation;
	uint32_t absSendTime;
	uint16_t playoutDelayMinDelay;
	uint16_t playoutDelayMaxDelay;
	uint16_t wideSeqNumber;
	std::string mid;
	std::string rid;

	packet->GetBuffer();
	packet->GetBufferLength();
	packet->GetLength();
	// packet->Dump();
	packet->GetVersion();
	packet->GetPayloadType();
	packet->SetPayloadType(100);
	packet->HasMarker();
	packet->SetMarker(true);
	packet->SetMarker(false);
	packet->GetSequenceNumber();
	packet->SetSequenceNumber(12345);
	packet->GetTimestamp();
	packet->SetTimestamp(8888);
	packet->GetSsrc();
	packet->SetSsrc(666);
	packet->HasCsrcs();

	packet->HasHeaderExtension();
	packet->GetHeaderExtensionId();
	packet->GetHeaderExtensionValue();
	packet->GetHeaderExtensionValueLength();
	packet->HasExtensions();
	packet->HasOneByteExtensions();
	packet->HasTwoBytesExtensions();

	::RTC::RtpHeaderExtensionIds headerExtensionIds{};

	headerExtensionIds.mid               = 5;
	headerExtensionIds.rid               = 6;
	headerExtensionIds.absSendTime       = 3;
	headerExtensionIds.transportWideCc01 = 4;
	headerExtensionIds.ssrcAudioLevel    = 1;
	headerExtensionIds.videoOrientation  = 2;
	headerExtensionIds.playoutDelay      = 8;

	packet->AssignExtensionIds(headerExtensionIds);

	packet->HasExtension(5);
	packet->GetExtensionValue(5, extenLen);
	packet->ReadMid(mid);
	packet->UpdateMid(mid);

	packet->HasExtension(6);
	packet->GetExtensionValue(6, extenLen);
	packet->ReadRid(rid);

	packet->HasExtension(3);
	packet->GetExtensionValue(3, extenLen);
	packet->ReadAbsSendTime(absSendTime);
	packet->UpdateAbsSendTime(12345678u);

	packet->HasExtension(4);
	packet->GetExtensionValue(4, extenLen);
	packet->ReadTransportWideCc01(wideSeqNumber);
	packet->UpdateTransportWideCc01(12345u);

	packet->HasExtension(1);
	packet->GetExtensionValue(1, extenLen);
	packet->ReadSsrcAudioLevel(volume, voice);

	packet->HasExtension(2);
	packet->GetExtensionValue(2, extenLen);
	packet->ReadVideoOrientation(camera, flip, rotation);

	packet->HasExtension(8);
	packet->GetExtensionValue(8, extenLen);
	packet->ReadPlayoutDelay(playoutDelayMinDelay, playoutDelayMaxDelay);

	packet->HasExtension(6);
	packet->HasExtension(7);
	packet->HasExtension(8);
	packet->HasExtension(9);
	packet->HasExtension(10);
	packet->HasExtension(11);
	packet->HasExtension(12);
	packet->HasExtension(13);
	packet->HasExtension(14);
	packet->HasExtension(15);

	uint8_t value1[] = { 0x01, 0x02, 0x03, 0x04 };

	extensions.emplace_back(
	  ::RTC::RtpHeaderExtensionUri::Type::MID, // type
	  1,                                       // id
	  4,                                       // len
	  value1                                   // value
	);

	uint8_t value2[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11 };

	extensions.emplace_back(
	  ::RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID, // type
	  2,                                                 // id
	  11,                                                // len
	  value2                                             // value
	);

	packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::OneByte, extensions);
	packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::TwoBytes, extensions);
	packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::Auto, extensions);

	extensions.clear();

	packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::TwoBytes, extensions);
	packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::OneByte, extensions);
	packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::Auto, extensions);

	uint8_t value3[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
		                   0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18 };

	extensions.emplace_back(
	  ::RTC::RtpHeaderExtensionUri::Type::MID, // type
	  14,                                      // id
	  24,                                      // len
	  value3                                   // value
	);

	extensions.emplace_back(
	  ::RTC::RtpHeaderExtensionUri::Type::RTP_STREAM_ID, // type
	  15,                                                // id
	  24,                                                // len
	  value3                                             // value
	);

	extensions.emplace_back(
	  ::RTC::RtpHeaderExtensionUri::Type::REPAIRED_RTP_STREAM_ID, // type
	  22,                                                         // id
	  24,                                                         // len
	  value3                                                      // value
	);

	extensions.emplace_back(
	  ::RTC::RtpHeaderExtensionUri::Type::DEPENDENCY_DESCRIPTOR, // type
	  100,                                                       // id
	  24,                                                        // len
	  value3                                                     // value
	);

	// NOTE: Cannot use One-Byte Extensions because we are using big ids and
	// lengths.
	// packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::OneByte, extensions);
	packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::TwoBytes, extensions);
	packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::Auto, extensions);

	packet->HasExtension(13);
	packet->GetExtensionValue(13, extenLen);
	packet->ReadAbsSendTime(absSendTime);
	packet->UpdateAbsSendTime(12345678);

	packet->HasExtension(14);
	packet->GetExtensionValue(14, extenLen);
	packet->ReadTransportWideCc01(wideSeqNumber);
	packet->UpdateTransportWideCc01(12345);

	packet->HasExtension(11);
	packet->GetExtensionValue(11, extenLen);
	packet->ReadSsrcAudioLevel(volume, voice);

	packet->HasExtension(12);
	packet->GetExtensionValue(12, extenLen);
	packet->ReadVideoOrientation(camera, flip, rotation);

	packet->HasExtension(15);
	packet->GetExtensionValue(15, extenLen);
	packet->ReadPlayoutDelay(playoutDelayMinDelay, playoutDelayMaxDelay);

	packet->HasPayload();
	packet->GetPayload();
	packet->GetPayloadLength();
	packet->HasPadding();
	packet->IsPaddedTo4Bytes();
	packet->GetPaddingLength();
	packet->SetPaddingLength(1);
	packet->SetPaddingLength(6);
	packet->SetPaddingLength(0);
	packet->IsPaddedTo4Bytes();

	// clang-format off
	uint8_t payload[] =
	{
		0x11, 0x22, 0x33, 0x44,
		0x55, 0x66, 0x77, 0x88,
		0x99, 0xAA
	};
	// clang-format on

	packet->SetPayload(payload, sizeof(payload));
	packet->RtxEncode(1, 2, 3);
	packet->RtxDecode(4, 5);
	packet->PadTo4Bytes();
	packet->ShiftPayload(4, 2);
	packet->ShiftPayload(4, -2);
	packet->ShiftPayload(3, 4);
	packet->ShiftPayload(3, -4);

	// These cannot be tested this way.
	// packet->SetPayloadDescriptorHandler();
	// packet->ProcessPayload();
	// packet->GetPayloadEncoder();
	// packet->EncodePayload();
	// packet->RestorePayload();
	// packet->IsKeyFrame();
	// packet->GetSpatialLayer();
	// packet->GetTemporalLayer();

	std::unique_ptr<uint8_t[]> buffer2(new uint8_t[len + 512]);

	packet.reset(packet->Clone(buffer2.get(), len + 512));

	packet->RemoveHeaderExtension();
	packet->SetPayloadLength(sizeof(payload) - 2);
	packet->RemovePayload();
}
