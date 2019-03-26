#include "RTC/FuzzerRtpPacket.hpp"
#include "RTC/RtpPacket.hpp"
#include <cstring> // std::memory()
#include <map>
#include <string>

void Fuzzer::RTC::RtpPacket::Fuzz(const uint8_t* data, size_t len)
{
	if (!::RTC::RtpPacket::IsRtp(data, len))
		return;

	// We need to clone the given data into a separate buffer because setters
	// below will try to write into packet memory.
	//
	// NOTE: Let's make the buffer bigger to test API that increases packet size.
	uint8_t data2[len + 16];
	uint8_t extenLen;
	bool voice;
	uint8_t volume;
	bool camera;
	bool flip;
	uint16_t rotation;
	uint32_t absSendTime;
	std::string mid;
	std::string rid;
	std::map<uint8_t, uint8_t> idMapping;

	std::memcpy(data2, data, len);

	::RTC::RtpPacket* packet = ::RTC::RtpPacket::Parse(data2, len);

	if (!packet)
		return;

	packet->Dump();
	packet->GetData();
	packet->GetSize();
	packet->GetPayloadType();
	packet->SetPayloadType(100);
	packet->HasMarker();
	packet->SetMarker(true);
	packet->SetPayloadPaddingFlag(true);
	packet->GetSequenceNumber();
	packet->SetSequenceNumber(12345);
	packet->GetTimestamp();
	packet->SetTimestamp(8888);
	packet->GetSsrc();
	packet->SetSsrc(666);
	packet->HasHeaderExtension();
	packet->GetHeaderExtensionId();
	packet->GetHeaderExtensionLength();
	packet->GetHeaderExtensionValue();
	packet->HasOneByteExtensions();
	packet->HasTwoBytesExtensions();

	packet->SetAudioLevelExtensionId(1);
	packet->GetExtension(1, extenLen);
	packet->ReadAudioLevel(volume, voice);

	packet->SetVideoOrientationExtensionId(2);
	packet->GetExtension(2, extenLen);
	packet->ReadVideoOrientation(camera, flip, rotation);

	packet->SetAbsSendTimeExtensionId(3);
	packet->GetExtension(3, extenLen);
	packet->ReadAbsSendTime(absSendTime);

	packet->SetMidExtensionId(5);
	packet->GetExtension(5, extenLen);
	packet->ReadMid(mid);

	packet->SetRidExtensionId(6);
	packet->GetExtension(6, extenLen);
	packet->ReadRid(rid);

	idMapping[1] = 11;
	idMapping[2] = 12;
	idMapping[3] = 13;

	packet->MangleHeaderExtensionIds(idMapping);

	packet->SetAudioLevelExtensionId(11);
	packet->GetExtension(11, extenLen);
	packet->ReadAudioLevel(volume, voice);

	packet->SetVideoOrientationExtensionId(12);
	packet->GetExtension(12, extenLen);
	packet->ReadVideoOrientation(camera, flip, rotation);

	packet->SetAbsSendTimeExtensionId(13);
	packet->GetExtension(13, extenLen);
	packet->ReadAbsSendTime(absSendTime);

	packet->GetPayload();
	packet->GetPayloadLength();
	packet->GetPayloadPadding();
	packet->IsKeyFrame();

	packet->SetOneByteHeaderExtension();
	packet->SetTwoBytesHeaderExtension();

	uint8_t buffer[len + 16];
	auto* clonedPacket = packet->Clone(buffer);

	delete clonedPacket;

	// TODO: packet->RtxEncode(); // This cannot be tested this way.
	// TODO: packet->RtxDecode(); // This cannot be tested this way.
	// TODO: packet->SetPayloadDescriptorHandler();
	// TODO: packet->EncodePayload();
	// TODO: packet->EncodePayload();
	// TODO: packet->ShiftPayload();

	delete packet;
}
