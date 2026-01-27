#include "RTC/ICE/FuzzerStunPacket.hpp"
#include "RTC/ICE/StunPacket.hpp"
#include <string_view>

static constexpr size_t ResponseFactoryBufferLength{ 65536 };
thread_local static uint8_t ResponseFactoryBuffer[ResponseFactoryBufferLength];
static constexpr size_t SerializeBufferLength{ 65536 };
thread_local static uint8_t SerializeBuffer[SerializeBufferLength];
static constexpr size_t CloneBufferLength{ 65536 };
thread_local static uint8_t CloneBuffer[CloneBufferLength];

void Fuzzer::RTC::ICE::StunPacket::Fuzz(const uint8_t* data, size_t len)
{
	if (!::RTC::ICE::StunPacket::IsStun(data, len))
	{
		return;
	}

	auto* packet = ::RTC::ICE::StunPacket::Parse(data, len);

	if (!packet)
	{
		return;
	}

	struct sockaddr_storage xorMappedAddressStorage{};
	std::string_view errorReasonPhrase;

	// packet->Dump();
	packet->GetClass();
	packet->GetMethod();
	packet->GetTransactionId();
	packet->HasAttribute(::RTC::ICE::StunPacket::AttributeType::USERNAME);
	packet->GetUsername();
	packet->GetPriority();
	packet->GetIceControlling();
	packet->GetIceControlled();
	packet->GetNomination();
	packet->GetSoftware();
	packet->GetXorMappedAddress(std::addressof(xorMappedAddressStorage));
	packet->GetErrorCode(errorReasonPhrase);
	packet->CheckAuthentication("1234:1234", "aksjd");
	packet->CheckAuthentication("foo");

	try
	{
		packet->AddUsername("foo");
	}
	catch (...)
	{
	}

	try
	{
		packet->AddPriority(123456);
	}
	catch (...)
	{
	}

	try
	{
		packet->AddIceControlling(98989232u);
	}
	catch (...)
	{
	}

	try
	{
		packet->AddIceControlled(87823823u);
	}
	catch (...)
	{
	}

	try
	{
		packet->AddUseCandidate();
	}
	catch (...)
	{
	}

	try
	{
		packet->AddNomination(7623547u);
	}
	catch (...)
	{
	}

	try
	{
		packet->AddXorMappedAddress(
		  reinterpret_cast<struct sockaddr*>(std::addressof(xorMappedAddressStorage)));
	}
	catch (...)
	{
	}

	try
	{
		packet->AddErrorCode(555, "ERROR å∫∂æ®€å∂ƒ∫");
	}
	catch (...)
	{
	}

	try
	{
		packet->Protect("askjhdakjsd");
	}
	catch (...)
	{
	}

	try
	{
		packet->Protect();
	}
	catch (...)
	{
	}

	if (packet->GetClass() == ::RTC::ICE::StunPacket::Class::REQUEST)
	{
		auto* successResponse =
		  packet->CreateSuccessResponse(ResponseFactoryBuffer, sizeof(ResponseFactoryBuffer));
		auto* errorResponse = packet->CreateErrorResponse(
		  ResponseFactoryBuffer, sizeof(ResponseFactoryBuffer), 444, "ERROR aljsh œœ∫∂å∫∂ zhx   å∫∂å∫∂ !!!");

		delete successResponse;
		delete errorResponse;
	}

	packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

	const auto* clonedPacket = packet->Clone(CloneBuffer, sizeof(CloneBuffer));

	delete packet;
	delete clonedPacket;
}
