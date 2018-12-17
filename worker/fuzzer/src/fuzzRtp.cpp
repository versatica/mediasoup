#include "fuzzers.hpp"
#include "RTC/RtpPacket.hpp"

using namespace RTC;

namespace fuzzers
{
	void fuzzRtcp(const uint8_t* data, size_t len)
	{
		if (!RtpPacket::IsRtp(data, len))
			return;

		RtpPacket* packet = RtpPacket::Parse(data, len);

		if (!packet)
			return;

		// packet->Dump(); // NOTE: This prints to stdout.
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
		packet->HasExtensionHeader();
		packet->GetExtensionHeaderId();
		packet->GetExtensionHeaderLength();
		packet->GetExtensionHeaderValue();
		// TODO: packet->MangleExtensionHeaderIds();
		packet->HasOneByteExtensions();
		packet->HasTwoBytesExtensions();
		// TODO: packet->AddExtensionMapping();
		// TODO: packet->GetExtension();
		// TODO: packet->ReadAudioLevel();
		// TODO: packet->ReadAbsSendTime();
		// TODO: packet->ReadMid();
		// TODO: packet->ReadRid();
		packet->GetPayload();
		packet->GetPayloadLength();
		packet->GetPayloadPadding();
		packet->IsKeyFrame();

		{
			uint8_t buffer[len];
			auto* clonedPacket = packet->Clone(buffer);

			delete clonedPacket;
		}

		// TODO: packet->RtxEncode(); // This cannot be tested this way.
		// TODO: packet->RtxDecode(); // This cannot be tested this way.
		// TODO: packet->SetPayloadDescriptorHandler();
		// TODO: packet->EncodePayload();
		// TODO: packet->EncodePayload();
		// TODO: packet->ShiftPayload();

		delete packet;
	}
}
