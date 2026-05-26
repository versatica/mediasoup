#include "common.hpp"
#include "DepLibUV.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/association/PacketSender.hpp"
#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "test/include/catch2Macros.hpp"
#include "mocks/include/MockShared.hpp"
#include "mocks/include/RTC/SCTP/association/MockAssociationListener.hpp"
#include "mocks/include/RTC/SCTP/tx/MockSendQueue.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("SCTP TransmissionControlBlock", "[sctp][transmissioncontrolblock]")
{
	constexpr uint32_t LocalVerificationTag{ 123 };
	constexpr uint32_t RemoteVerificationTag{ 456 };
	constexpr uint32_t LocalInitialTsn{ 10 };
	constexpr uint32_t RemoteInitialTsn{ 1000 };
	constexpr uint32_t Arwnd{ 65536 };
	constexpr uint64_t TieTag{ 12345678 };

	class MockPacketSenderListener : public RTC::SCTP::PacketSender::Listener
	{
	public:
		void OnPacketSenderPacketSent(
		  RTC::SCTP::PacketSender* /*packetSender*/, const RTC::SCTP::Packet* /*packet*/, bool /*sent*/) override
		{
		}
	};

	const RTC::SCTP::SctpOptions sctpOptions;

	mocks::RTC::SCTP::MockAssociationListener associationListener;
	mocks::MockShared shared(/*getTimeMs*/
	                         []()
	                         {
		                         return DepLibUV::GetTimeMs();
	                         });
	mocks::RTC::SCTP::MockSendQueue sendQueue;
	RTC::SCTP::NegotiatedCapabilities negotiatedCapabilities;
	MockPacketSenderListener packetSenderListener;
	RTC::SCTP::PacketSender packetSender(std::addressof(packetSenderListener), associationListener);

	auto isAssociationEstablished = []()
	{
		return true;
	};

	SECTION("without message interleaving")
	{
		negotiatedCapabilities.messageInterleaving = false;

		sendQueue.ExpectEnableMessageInterleavingCalledWith(false);

		const RTC::SCTP::TransmissionControlBlock tcb(
		  associationListener,
		  sctpOptions,
		  std::addressof(shared),
		  sendQueue,
		  packetSender,
		  LocalVerificationTag,
		  RemoteVerificationTag,
		  LocalInitialTsn,
		  RemoteInitialTsn,
		  Arwnd,
		  TieTag,
		  negotiatedCapabilities,
		  /*maxPacketLength*/ 1200,
		  isAssociationEstablished);

		REQUIRE_VERIFICATION_RESULT(sendQueue.VerifyExpectations());
	}

	SECTION("with message interleaving")
	{
		negotiatedCapabilities.messageInterleaving = true;

		sendQueue.ExpectEnableMessageInterleavingCalledWith(true);

		const RTC::SCTP::TransmissionControlBlock tcb(
		  associationListener,
		  sctpOptions,
		  std::addressof(shared),
		  sendQueue,
		  packetSender,
		  LocalVerificationTag,
		  RemoteVerificationTag,
		  LocalInitialTsn,
		  RemoteInitialTsn,
		  Arwnd,
		  TieTag,
		  negotiatedCapabilities,
		  /*maxPacketLength*/ 1200,
		  isAssociationEstablished);

		REQUIRE_VERIFICATION_RESULT(sendQueue.VerifyExpectations());
	}
}
