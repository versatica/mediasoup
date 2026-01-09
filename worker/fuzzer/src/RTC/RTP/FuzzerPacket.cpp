#include "RTC/RTP/FuzzerPacket.hpp"
#include "RTC/RTP/Packet.hpp"
#include <string>
#include <vector>

void Fuzzer::RTC::RTP::Packet::Fuzz(const uint8_t* data, size_t len)
{
	if (!::RTC::RTP::Packet::IsRtp(data, len))
	{
		return;
	}

	std::unique_ptr<::RTC::RTP::Packet> packet{ ::RTC::RTP::Packet::Parse(data, len) };

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

	std::vector<::RTC::RTP::Packet::AddedExtension> extensions;
	uint8_t extenLen;
	// bool voice;
	// uint8_t volume;
	// bool camera;
	// bool flip;
	// uint16_t rotation;
	// uint32_t absSendTime;
	// uint16_t playoutDelayMinDelay;
	// uint16_t playoutDelayMaxDelay;
	// uint16_t wideSeqNumber;
	// std::string mid;
	// std::string rid;

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

	// packet->SetMidExtensionId(5);
	packet->HasExtension(5);
	packet->GetExtension(5, extenLen);
	// packet->ReadMid(mid);
	// packet->UpdateMid(mid);

	// packet->SetRidExtensionId(6);
	packet->HasExtension(6);
	packet->GetExtension(6, extenLen);
	// packet->ReadRid(rid);

	// packet->SetAbsSendTimeExtensionId(3);
	packet->HasExtension(3);
	packet->GetExtension(3, extenLen);
	// packet->ReadAbsSendTime(absSendTime);
	// packet->UpdateAbsSendTime(12345678u);

	// packet->SetTransportWideCc01ExtensionId(4);
	packet->HasExtension(4);
	packet->GetExtension(4, extenLen);
	// packet->ReadTransportWideCc01(wideSeqNumber);
	// packet->UpdateTransportWideCc01(12345u);

	// packet->SetSsrcAudioLevelExtensionId(1);
	packet->HasExtension(1);
	packet->GetExtension(1, extenLen);
	// packet->ReadSsrcAudioLevel(volume, voice);

	// packet->SetVideoOrientationExtensionId(2);
	packet->HasExtension(2);
	packet->GetExtension(2, extenLen);
	// packet->ReadVideoOrientation(camera, flip, rotation);

	// packet->SetPlayoutDelayExtensionId(8);
	packet->HasExtension(8);
	packet->GetExtension(8, extenLen);
	// packet->ReadPlayoutDelay(playoutDelayMinDelay, playoutDelayMaxDelay);

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
	  1,     // id
	  4,     // len
	  value1 // value
	);

	uint8_t value2[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11 };

	extensions.emplace_back(
	  2,     // id
	  11,    // len
	  value2 // value
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
	  14,    // id
	  24,    // len
	  value3 // value
	);

	extensions.emplace_back(
	  15,    // id
	  24,    // len
	  value3 // value
	);

	extensions.emplace_back(
	  22,    // id
	  24,    // len
	  value3 // value
	);

	extensions.emplace_back(
	  100,   // id
	  24,    // len
	  value3 // value
	);

	// NOTE: Cannot use One-Byte Extensions because we are using big ids and
	// lengths.
	// packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::OneByte, extensions);
	packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::TwoBytes, extensions);
	packet->SetExtensions(::RTC::RTP::Packet::ExtensionsType::Auto, extensions);

	// packet->SetAbsSendTimeExtensionId(13);
	packet->HasExtension(13);
	packet->GetExtension(13, extenLen);
	// packet->ReadAbsSendTime(absSendTime);
	// packet->UpdateAbsSendTime(12345678u);

	// packet->SetTransportWideCc01ExtensionId(14);
	packet->HasExtension(14);
	packet->GetExtension(14, extenLen);
	// packet->ReadTransportWideCc01(wideSeqNumber);
	// packet->UpdateTransportWideCc01(12345u);
	// packet->SetExtensionLength(14, 2);

	// packet->SetSsrcAudioLevelExtensionId(11);
	packet->HasExtension(11);
	packet->GetExtension(11, extenLen);
	// packet->ReadSsrcAudioLevel(volume, voice);

	// packet->SetVideoOrientationExtensionId(12);
	packet->HasExtension(12);
	packet->GetExtension(12, extenLen);
	// packet->ReadVideoOrientation(camera, flip, rotation);

	// packet->SetPlayoutDelayExtensionId(15);
	packet->HasExtension(15);
	packet->GetExtension(15, extenLen);
	// packet->ReadPlayoutDelay(playoutDelayMinDelay, playoutDelayMaxDelay);

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
	packet->RemovePayload();
}
