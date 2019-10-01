#include "RTC/FuzzerStunPacket.hpp"
#include "RTC/StunPacket.hpp"

void Fuzzer::RTC::StunPacket::Fuzz(const uint8_t* data, size_t len)
{
	if (!::RTC::StunPacket::IsStun(data, len))
		return;

	::RTC::StunPacket* packet = ::RTC::StunPacket::Parse(data, len);

	if (!packet)
		return;

	// packet->Dump();
	packet->GetClass();
	packet->GetMethod();
	packet->GetData();
	packet->GetSize();
	packet->SetUsername("foo", 3);
	packet->SetPriority(123);
	packet->SetIceControlling(123);
	packet->SetIceControlled(123);
	packet->SetUseCandidate();
	// TODO: packet->SetXorMappedAddress();
	packet->SetErrorCode(666);
	// TODO: packet->SetMessageIntegrity();
	packet->SetFingerprint();
	packet->GetUsername();
	packet->GetPriority();
	packet->GetIceControlling();
	packet->GetIceControlled();
	packet->HasUseCandidate();
	packet->GetErrorCode();
	packet->HasMessageIntegrity();
	packet->HasFingerprint();
	packet->CheckAuthentication("foo", "bar");
	// TODO: packet->CreateSuccessResponse(); // This cannot be easily tested.
	// TODO: packet->CreateErrorResponse(); // This cannot be easily tested.
	packet->Authenticate("lalala");
	// TODO: Cannot test Serialize() because we don't know the exact required
	// buffer size (setters above may change the total size).
	// TODO: packet->Serialize();

	delete packet;
}
