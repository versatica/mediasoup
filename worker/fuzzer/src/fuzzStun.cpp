#include "fuzzers.hpp"
#include "RTC/StunMessage.hpp"

using namespace RTC;

namespace fuzzers
{
	void fuzzStun(const uint8_t* data, size_t len)
	{
		if (!StunMessage::IsStun(data, len))
			return;

		StunMessage* msg = StunMessage::Parse(data, len);

		if (!msg)
			return;

		msg->Dump();
		msg->GetClass();
		msg->GetMethod();
		msg->GetData();
		msg->GetSize();
		msg->SetUsername("foo", 3);
		msg->SetPriority(123);
		msg->SetIceControlling(123);
		msg->SetIceControlled(123);
		msg->SetUseCandidate();
		// TODO: msg->SetXorMappedAddress();
		msg->SetErrorCode(666);
		// TODO: msg->SetMessageIntegrity();
		msg->SetFingerprint();
		msg->GetUsername();
		msg->GetPriority();
		msg->GetIceControlling();
		msg->GetIceControlled();
		msg->HasUseCandidate();
		msg->GetErrorCode();
		msg->HasMessageIntegrity();
		msg->HasFingerprint();
		msg->CheckAuthentication("foo", "bar");
		// TODO: msg->CreateSuccessResponse(); // This cannot be easily tested.
		// TODO: msg->CreateErrorResponse(); // This cannot be easily tested.
		msg->Authenticate("lalala");
		// TODO: Cannot test Serialize() because we don't know the exact required
		// buffer size (setters above may change the total size).
		// TODO: msg->Serialize();

		delete msg;
	}
}
