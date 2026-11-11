#include "fuzzer/include/RTC/ICE/FuzzerStunPacket.hpp"
#include "RTC/ICE/StunPacket.hpp"
#include <string_view>

namespace
{
	constexpr size_t ResponseFactoryBufferLength{ 65536 };
	alignas(4) thread_local uint8_t ResponseFactoryBuffer[ResponseFactoryBufferLength];
	constexpr size_t SerializeBufferLength{ 65536 };
	alignas(4) thread_local uint8_t SerializeBuffer[SerializeBufferLength];
	constexpr size_t CloneBufferLength{ 65536 };
	alignas(4) thread_local uint8_t CloneBuffer[CloneBufferLength];
} // namespace

void FuzzerRtcIceStunPacket::Fuzz(const uint8_t* data, size_t len)
{
	if (!RTC::ICE::StunPacket::IsStun(data, len))
	{
		return;
	}

	const std::unique_ptr<RTC::ICE::StunPacket> packet{ RTC::ICE::StunPacket::Parse(data, len) };

	if (!packet)
	{
		return;
	}

	sockaddr_storage xorMappedAddressStorage{};
	std::string_view errorReasonPhrase;

	// packet->Dump();
	packet->GetClass();
	packet->GetMethod();
	packet->GetTransactionId();
	packet->HasAttribute(RTC::ICE::StunPacket::AttributeType::USERNAME);
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
	catch (...) // NOLINT(bugprone-empty-catch)
	{
	}

	try
	{
		packet->AddPriority(123456);
	}
	catch (...) // NOLINT(bugprone-empty-catch)
	{
	}

	try
	{
		packet->AddIceControlling(98989232);
	}
	catch (...) // NOLINT(bugprone-empty-catch)
	{
	}

	try
	{
		packet->AddIceControlled(87823823);
	}
	catch (...) // NOLINT(bugprone-empty-catch)
	{
	}

	try
	{
		packet->AddUseCandidate();
	}
	catch (...) // NOLINT(bugprone-empty-catch)
	{
	}

	try
	{
		packet->AddNomination(7623547);
	}
	catch (...) // NOLINT(bugprone-empty-catch)
	{
	}

	try
	{
		packet->AddXorMappedAddress(reinterpret_cast<sockaddr*>(std::addressof(xorMappedAddressStorage)));
	}
	catch (...) // NOLINT(bugprone-empty-catch)
	{
	}

	try
	{
		packet->AddErrorCode(555, "ERROR å∫∂æ®€å∂ƒ∫");
	}
	catch (...) // NOLINT(bugprone-empty-catch)
	{
	}

	try
	{
		packet->Protect("askjhdakjsd");
	}
	catch (...) // NOLINT(bugprone-empty-catch)
	{
	}

	try
	{
		packet->Protect();
	}
	catch (...) // NOLINT(bugprone-empty-catch)
	{
	}

	if (packet->GetClass() == RTC::ICE::StunPacket::Class::REQUEST)
	{
		const std::unique_ptr<RTC::ICE::StunPacket> successResponse{ packet->CreateSuccessResponse(
			ResponseFactoryBuffer, sizeof(ResponseFactoryBuffer)) };
		const std::unique_ptr<RTC::ICE::StunPacket> errorResponse{ packet->CreateErrorResponse(
			ResponseFactoryBuffer,
			sizeof(ResponseFactoryBuffer),
			444,
			"ERROR aljsh œœ∫∂å∫∂ zhx   å∫∂å∫∂ !!!") };
	}

	packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

	const std::unique_ptr<const RTC::ICE::StunPacket> clonedPacket{ packet->Clone(
		CloneBuffer, sizeof(CloneBuffer)) };
}
