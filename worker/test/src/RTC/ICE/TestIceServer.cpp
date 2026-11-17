#include "common.hpp"
#include "RTC/ICE/IceServer.hpp"
#include "RTC/ICE/StunPacket.hpp"
#include "RTC/TransportTuple.hpp"
#include "test/include/RTC/ICE/iceCommon.hpp"
#include "mocks/include/MockShared.hpp"
#include <uv.h>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>
#include <vector>

SCENARIO("ICE IceServer", "[ice][iceserver]")
{
	iceCommon::ResetBuffers();

	constexpr std::string_view ConsentCheckTimerLabel{ "ice-server-consent-check" };
	constexpr uint8_t ConsentTimeoutSec{ 30 };

	// One more than the maximum number of tuples the IceServer stores.
	constexpr size_t TooManyTuples{ 9 };

	class TestIceServerListener : public RTC::ICE::IceServer::Listener
	{
	public:
		void OnIceServerSendStunPacket(
		  RTC::ICE::IceServer* /*iceServer*/,
		  const RTC::ICE::StunPacket* packet,
		  const RTC::TransportTuple* /*tuple*/) override
		{
			std::string_view reasonPhrase;

			this->sentPacketClass     = packet->GetClass();
			this->sentPacketErrorCode = packet->GetErrorCode(reasonPhrase);

			// The packet only lives during this call, so its bytes are copied.
			this->sentPacket.assign(packet->GetBuffer(), packet->GetBuffer() + packet->GetLength());
		}

		void OnIceServerLocalUsernameFragmentAdded(
		  RTC::ICE::IceServer* /*iceServer*/, const std::string& usernameFragment) override
		{
			this->addedUsernameFragments.push_back(usernameFragment);
		}

		void OnIceServerLocalUsernameFragmentRemoved(
		  RTC::ICE::IceServer* /*iceServer*/, const std::string& usernameFragment) override
		{
			this->removedUsernameFragments.push_back(usernameFragment);
		}

		void OnIceServerTupleAdded(
		  RTC::ICE::IceServer* /*iceServer*/, const RTC::TransportTuple* /*tuple*/) override
		{
			++this->tupleAddedCount;
		}

		void OnIceServerTupleRemoved(RTC::ICE::IceServer* /*iceServer*/, RTC::TransportTuple* /*tuple*/) override
		{
			++this->tupleRemovedCount;
		}

		void OnIceServerSelectedTuple(
		  RTC::ICE::IceServer* /*iceServer*/, const RTC::TransportTuple* /*tuple*/) override
		{
			++this->selectedTupleCount;
		}

		void OnIceServerConnected(RTC::ICE::IceServer* /*iceServer*/) override
		{
			++this->connectedCount;
		}

		void OnIceServerCompleted(RTC::ICE::IceServer* /*iceServer*/) override
		{
			++this->completedCount;
		}

		void OnIceServerDisconnected(RTC::ICE::IceServer* /*iceServer*/) override
		{
			++this->disconnectedCount;
		}

	public:
		RTC::ICE::StunPacket::Class sentPacketClass{ RTC::ICE::StunPacket::Class::UNSET };
		uint16_t sentPacketErrorCode{ 0 };
		std::vector<uint8_t> sentPacket;
		std::vector<std::string> addedUsernameFragments;
		std::vector<std::string> removedUsernameFragments;
		size_t tupleAddedCount{ 0 };
		size_t tupleRemovedCount{ 0 };
		size_t selectedTupleCount{ 0 };
		size_t connectedCount{ 0 };
		size_t completedCount{ 0 };
		size_t disconnectedCount{ 0 };
	};

	const std::string usernameFragment{ "serverufrag" };
	const std::string password{ "serverpassword" };

	int64_t nowUs{ 1000 * 1000 };
	TestIceServerListener listener;
	mocks::MockShared shared(/*getTimeUs*/
	                         [&nowUs]() -> int64_t
	                         {
		                         return nowUs;
	                         });

	// Remote IPv4 address with the given port. It must outlive the tuple built
	// on it.
	const auto makeRemoteAddress = [](uint16_t port) -> sockaddr_storage
	{
		sockaddr_storage storage{};

		REQUIRE(
		  uv_ip4_addr("1.2.3.4", port, reinterpret_cast<sockaddr_in*>(std::addressof(storage))) == 0);

		return storage;
	};

	// UDP tuple with the given remote address. No UDP socket is given since the
	// IceServer never uses it.
	const auto makeTuple = [](const sockaddr_storage& remoteAddress) -> RTC::TransportTuple
	{
		return { nullptr, reinterpret_cast<const sockaddr*>(std::addressof(remoteAddress)) };
	};

	// Create a Binding request from an ICE controlling peer, protect it with the
	// given credentials and let the IceServer process it.
	const auto processRequest = [](
	                              RTC::ICE::IceServer& iceServer,
	                              RTC::TransportTuple& tuple,
	                              const std::string& requestUsernameFragment,
	                              const std::string& requestPassword,
	                              bool useCandidate,
	                              std::optional<uint32_t> nomination) -> void
	{
		const std::unique_ptr<RTC::ICE::StunPacket> request{ RTC::ICE::StunPacket::Factory(
			iceCommon::FactoryBuffer,
			sizeof(iceCommon::FactoryBuffer),
			RTC::ICE::StunPacket::Class::REQUEST,
			RTC::ICE::StunPacket::Method::BINDING) };

		request->AddUsername(requestUsernameFragment + ":clientufrag");
		request->AddPriority(1000);
		request->AddIceControlling(1234);

		if (useCandidate)
		{
			request->AddUseCandidate();
		}

		if (nomination.has_value())
		{
			request->AddNomination(nomination.value());
		}

		request->Protect(requestPassword);

		iceServer.ProcessStunPacket(request.get(), std::addressof(tuple));
	};

	// Create a Binding request with the given credentials but without the given
	// attribute (PRIORITY or FINGERPRINT) or with ICE-CONTROLLED instead of
	// ICE-CONTROLLING, and let the IceServer process it.
	const auto processInvalidRequest = [&usernameFragment, &password](
	                                     RTC::ICE::IceServer& iceServer,
	                                     RTC::TransportTuple& tuple,
	                                     bool addPriority,
	                                     bool addFingerprint,
	                                     bool iceControlled) -> void
	{
		const std::unique_ptr<RTC::ICE::StunPacket> request{ RTC::ICE::StunPacket::Factory(
			iceCommon::FactoryBuffer,
			sizeof(iceCommon::FactoryBuffer),
			RTC::ICE::StunPacket::Class::REQUEST,
			RTC::ICE::StunPacket::Method::BINDING) };

		request->AddUsername(usernameFragment + ":clientufrag");

		if (addPriority)
		{
			request->AddPriority(1000);
		}

		if (iceControlled)
		{
			// A tie-breaker of 0 must also be detected as ICE-CONTROLLED.
			request->AddIceControlled(0);
		}
		else
		{
			request->AddIceControlling(1234);
		}

		if (addFingerprint)
		{
			request->Protect(password);
		}

		iceServer.ProcessStunPacket(request.get(), std::addressof(tuple));
	};

	const auto isSelectedTuple =
	  [](const RTC::ICE::IceServer& iceServer, const RTC::TransportTuple& tuple) -> bool
	{
		const auto* const selectedTuple = iceServer.GetSelectedTuple();

		return selectedTuple && selectedTuple->Compare(std::addressof(tuple));
	};

	// Whether the last STUN packet sent by the IceServer is protected with the
	// given password.
	const auto isSentPacketProtectedWith = [&listener](const std::string& expectedPassword) -> bool
	{
		const std::unique_ptr<RTC::ICE::StunPacket> packet{ RTC::ICE::StunPacket::Parse(
			listener.sentPacket.data(), listener.sentPacket.size()) };

		return packet && packet->CheckAuthentication(expectedPassword) ==
		                   RTC::ICE::StunPacket::AuthenticationResult::OK;
	};

	SECTION("constructor and destructor notify the listener")
	{
		const auto remoteAddress = makeRemoteAddress(10001);

		auto tuple = makeTuple(remoteAddress);

		auto iceServer = std::make_unique<RTC::ICE::IceServer>(
		  std::addressof(listener), std::addressof(shared), usernameFragment, password, ConsentTimeoutSec);

		REQUIRE(iceServer->GetState() == RTC::ICE::IceServer::IceState::NEW);
		REQUIRE(listener.addedUsernameFragments == std::vector<std::string>{ usernameFragment });

		processRequest(
		  *iceServer,
		  tuple,
		  usernameFragment,
		  password,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		REQUIRE(listener.tupleAddedCount == 1);

		iceServer.reset();

		REQUIRE(listener.removedUsernameFragments == std::vector<std::string>{ usernameFragment });
		REQUIRE(listener.tupleRemovedCount == 1);
	}

	SECTION("Binding request without USE-CANDIDATE moves to 'connected'")
	{
		const auto remoteAddress = makeRemoteAddress(10001);

		auto tuple = makeTuple(remoteAddress);

		RTC::ICE::IceServer iceServer(
		  std::addressof(listener), std::addressof(shared), usernameFragment, password, ConsentTimeoutSec);

		processRequest(
		  iceServer,
		  tuple,
		  usernameFragment,
		  password,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		REQUIRE(listener.sentPacketClass == RTC::ICE::StunPacket::Class::SUCCESS_RESPONSE);
		REQUIRE(isSentPacketProtectedWith(password));
		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::CONNECTED);
		REQUIRE(listener.connectedCount == 1);
		REQUIRE(listener.completedCount == 0);
		REQUIRE(listener.tupleAddedCount == 1);
		REQUIRE(listener.selectedTupleCount == 1);
		REQUIRE(iceServer.IsValidTuple(std::addressof(tuple)));
		REQUIRE(isSelectedTuple(iceServer, tuple));

		const auto* const timer = shared.GetTimer(ConsentCheckTimerLabel);

		REQUIRE(timer != nullptr);
		REQUIRE(timer->IsActive());
		REQUIRE(timer->GetTimeoutMs() == ConsentTimeoutSec * 1000);
	}

	SECTION("Binding request with USE-CANDIDATE moves to 'completed'")
	{
		const auto remoteAddress = makeRemoteAddress(10001);

		auto tuple = makeTuple(remoteAddress);

		RTC::ICE::IceServer iceServer(
		  std::addressof(listener), std::addressof(shared), usernameFragment, password, ConsentTimeoutSec);

		processRequest(
		  iceServer,
		  tuple,
		  usernameFragment,
		  password,
		  /*useCandidate*/ true,
		  /*nomination*/ std::nullopt);

		REQUIRE(listener.sentPacketClass == RTC::ICE::StunPacket::Class::SUCCESS_RESPONSE);
		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::COMPLETED);
		REQUIRE(listener.connectedCount == 0);
		REQUIRE(listener.completedCount == 1);
		REQUIRE(isSelectedTuple(iceServer, tuple));
	}

	SECTION("invalid Binding request is replied with an error and changes nothing")
	{
		const auto remoteAddress = makeRemoteAddress(10001);

		auto tuple = makeTuple(remoteAddress);

		RTC::ICE::IceServer iceServer(
		  std::addressof(listener), std::addressof(shared), usernameFragment, password, ConsentTimeoutSec);

		const auto checkErrorResponse = [&iceServer, &listener, &tuple](uint16_t errorCode) -> void
		{
			REQUIRE(listener.sentPacketClass == RTC::ICE::StunPacket::Class::ERROR_RESPONSE);
			REQUIRE(listener.sentPacketErrorCode == errorCode);
			REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::NEW);
			REQUIRE(!iceServer.IsValidTuple(std::addressof(tuple)));
			REQUIRE(listener.tupleAddedCount == 0);
		};

		// Wrong password.
		processRequest(
		  iceServer,
		  tuple,
		  usernameFragment,
		  "wrongpassword",
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		checkErrorResponse(401);

		// Wrong username fragment.
		processRequest(
		  iceServer,
		  tuple,
		  "wrongufrag",
		  password,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		checkErrorResponse(401);

		// No PRIORITY.
		processInvalidRequest(
		  iceServer, tuple, /*addPriority*/ false, /*addFingerprint*/ true, /*iceControlled*/ false);

		checkErrorResponse(400);

		// No FINGERPRINT (nor MESSAGE-INTEGRITY).
		processInvalidRequest(
		  iceServer, tuple, /*addPriority*/ true, /*addFingerprint*/ false, /*iceControlled*/ false);

		checkErrorResponse(400);

		// ICE-CONTROLLED, so a role conflict.
		processInvalidRequest(
		  iceServer, tuple, /*addPriority*/ true, /*addFingerprint*/ true, /*iceControlled*/ true);

		checkErrorResponse(487);

		// Method other than Binding (TURN Allocate).
		const std::unique_ptr<RTC::ICE::StunPacket> request{ RTC::ICE::StunPacket::Factory(
			iceCommon::FactoryBuffer,
			sizeof(iceCommon::FactoryBuffer),
			RTC::ICE::StunPacket::Class::REQUEST,
			// NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
			static_cast<RTC::ICE::StunPacket::Method>(0x003)) };

		request->Protect(password);

		iceServer.ProcessStunPacket(request.get(), std::addressof(tuple));

		checkErrorResponse(400);
	}

	SECTION("renomination in 'completed' state only accepts a higher nomination")
	{
		const auto remoteAddress1 = makeRemoteAddress(10001);
		const auto remoteAddress2 = makeRemoteAddress(10002);

		auto tuple1 = makeTuple(remoteAddress1);
		auto tuple2 = makeTuple(remoteAddress2);

		RTC::ICE::IceServer iceServer(
		  std::addressof(listener), std::addressof(shared), usernameFragment, password, ConsentTimeoutSec);

		processRequest(
		  iceServer, tuple1, usernameFragment, password, /*useCandidate*/ true, /*nomination*/ 1);

		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::COMPLETED);
		REQUIRE(isSelectedTuple(iceServer, tuple1));

		// Higher nomination on another tuple.
		processRequest(
		  iceServer, tuple2, usernameFragment, password, /*useCandidate*/ true, /*nomination*/ 2);

		REQUIRE(isSelectedTuple(iceServer, tuple2));

		// Equal nomination is ignored, even with USE-CANDIDATE.
		processRequest(
		  iceServer, tuple1, usernameFragment, password, /*useCandidate*/ true, /*nomination*/ 2);

		REQUIRE(listener.sentPacketClass == RTC::ICE::StunPacket::Class::SUCCESS_RESPONSE);
		REQUIRE(isSelectedTuple(iceServer, tuple2));

		// Lower nomination is ignored, even with USE-CANDIDATE.
		processRequest(
		  iceServer, tuple1, usernameFragment, password, /*useCandidate*/ true, /*nomination*/ 1);

		REQUIRE(listener.sentPacketClass == RTC::ICE::StunPacket::Class::SUCCESS_RESPONSE);
		REQUIRE(isSelectedTuple(iceServer, tuple2));

		// USE-CANDIDATE without nomination is a regular nomination.
		processRequest(
		  iceServer,
		  tuple1,
		  usernameFragment,
		  password,
		  /*useCandidate*/ true,
		  /*nomination*/ std::nullopt);

		REQUIRE(isSelectedTuple(iceServer, tuple1));
		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::COMPLETED);
		REQUIRE(listener.completedCount == 1);
	}

	SECTION("nomination not higher than the current one does not move from 'connected' to 'completed'")
	{
		const auto remoteAddress1 = makeRemoteAddress(10001);
		const auto remoteAddress2 = makeRemoteAddress(10002);

		auto tuple1 = makeTuple(remoteAddress1);
		auto tuple2 = makeTuple(remoteAddress2);

		RTC::ICE::IceServer iceServer(
		  std::addressof(listener), std::addressof(shared), usernameFragment, password, ConsentTimeoutSec);

		processRequest(
		  iceServer,
		  tuple1,
		  usernameFragment,
		  password,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::CONNECTED);

		// Nomination 0 is not higher than the initial one.
		processRequest(
		  iceServer, tuple2, usernameFragment, password, /*useCandidate*/ true, /*nomination*/ 0);

		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::CONNECTED);
		REQUIRE(isSelectedTuple(iceServer, tuple1));
		REQUIRE(iceServer.IsValidTuple(std::addressof(tuple2)));
		REQUIRE(listener.completedCount == 0);

		processRequest(
		  iceServer, tuple2, usernameFragment, password, /*useCandidate*/ true, /*nomination*/ 1);

		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::COMPLETED);
		REQUIRE(isSelectedTuple(iceServer, tuple2));
		REQUIRE(listener.completedCount == 1);
	}

	SECTION("removing the selected tuple selects another one or moves to 'disconnected'")
	{
		const auto remoteAddress1 = makeRemoteAddress(10001);
		const auto remoteAddress2 = makeRemoteAddress(10002);

		auto tuple1 = makeTuple(remoteAddress1);
		auto tuple2 = makeTuple(remoteAddress2);

		RTC::ICE::IceServer iceServer(
		  std::addressof(listener), std::addressof(shared), usernameFragment, password, ConsentTimeoutSec);

		processRequest(
		  iceServer,
		  tuple1,
		  usernameFragment,
		  password,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);
		processRequest(
		  iceServer,
		  tuple2,
		  usernameFragment,
		  password,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		const auto* const timer = shared.GetTimer(ConsentCheckTimerLabel);

		REQUIRE(timer != nullptr);
		REQUIRE(isSelectedTuple(iceServer, tuple1));

		iceServer.RemoveTuple(std::addressof(tuple1));

		REQUIRE(listener.tupleRemovedCount == 1);
		REQUIRE(!iceServer.IsValidTuple(std::addressof(tuple1)));
		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::CONNECTED);
		REQUIRE(isSelectedTuple(iceServer, tuple2));
		REQUIRE(timer->IsActive());

		iceServer.RemoveTuple(std::addressof(tuple2));

		REQUIRE(listener.tupleRemovedCount == 2);
		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::DISCONNECTED);
		REQUIRE(listener.disconnectedCount == 1);
		REQUIRE(iceServer.GetSelectedTuple() == nullptr);
		REQUIRE(!timer->IsActive());
	}

	SECTION("too many tuples removes the oldest non selected one")
	{
		std::vector<sockaddr_storage> remoteAddresses;
		std::vector<RTC::TransportTuple> tuples;

		// Reserve so that the tuples do not point to moved addresses.
		remoteAddresses.reserve(TooManyTuples);
		tuples.reserve(TooManyTuples);

		for (size_t idx{ 0 }; idx < TooManyTuples; ++idx)
		{
			remoteAddresses.push_back(makeRemoteAddress(static_cast<uint16_t>(10001 + idx)));
			tuples.push_back(makeTuple(remoteAddresses.back()));
		}

		RTC::ICE::IceServer iceServer(
		  std::addressof(listener), std::addressof(shared), usernameFragment, password, ConsentTimeoutSec);

		for (auto& tuple : tuples)
		{
			processRequest(
			  iceServer,
			  tuple,
			  usernameFragment,
			  password,
			  /*useCandidate*/ false,
			  /*nomination*/ std::nullopt);
		}

		REQUIRE(listener.tupleAddedCount == TooManyTuples);
		REQUIRE(listener.tupleRemovedCount == 1);

		// The first tuple is the selected one, so the second one is removed.
		REQUIRE(isSelectedTuple(iceServer, tuples.at(0)));
		REQUIRE(!iceServer.IsValidTuple(std::addressof(tuples.at(1))));

		for (size_t idx{ 0 }; idx < tuples.size(); ++idx)
		{
			if (idx != 1)
			{
				REQUIRE(iceServer.IsValidTuple(std::addressof(tuples.at(idx))));
			}
		}
	}

	SECTION("consent expires if no Binding request is received on time")
	{
		const auto remoteAddress = makeRemoteAddress(10001);

		auto tuple = makeTuple(remoteAddress);

		RTC::ICE::IceServer iceServer(
		  std::addressof(listener), std::addressof(shared), usernameFragment, password, ConsentTimeoutSec);

		processRequest(
		  iceServer,
		  tuple,
		  usernameFragment,
		  password,
		  /*useCandidate*/ true,
		  /*nomination*/ std::nullopt);

		auto* const timer = shared.GetTimer(ConsentCheckTimerLabel);

		REQUIRE(timer != nullptr);

		// Each Binding request restarts the consent timeout.
		nowUs += 20 * 1000 * 1000;

		REQUIRE(!timer->EvaluateHasExpired());

		processRequest(
		  iceServer,
		  tuple,
		  usernameFragment,
		  password,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		nowUs += 20 * 1000 * 1000;

		REQUIRE(!timer->EvaluateHasExpired());
		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::COMPLETED);

		nowUs += 10 * 1000 * 1000;

		REQUIRE(timer->EvaluateHasExpired());
		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::DISCONNECTED);
		REQUIRE(listener.disconnectedCount == 1);
		REQUIRE(listener.tupleRemovedCount == 1);
		REQUIRE(!iceServer.IsValidTuple(std::addressof(tuple)));
		REQUIRE(iceServer.GetSelectedTuple() == nullptr);
		REQUIRE(!timer->IsActive());

		// A new Binding request connects again.
		processRequest(
		  iceServer,
		  tuple,
		  usernameFragment,
		  password,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::CONNECTED);
		REQUIRE(listener.connectedCount == 1);
		REQUIRE(isSelectedTuple(iceServer, tuple));
		REQUIRE(timer->IsActive());
	}

	SECTION("consent timeout is disabled with 0 and clamped otherwise")
	{
		const auto remoteAddress = makeRemoteAddress(10001);

		auto tuple = makeTuple(remoteAddress);

		// Each case is given its own IceServer, which destroys its timer when the
		// case ends.
		const auto runCase =
		  [&listener, &shared, &usernameFragment, &password, &tuple, &processRequest, ConsentCheckTimerLabel](
		    uint8_t consentTimeoutSec, std::optional<int64_t> expectedTimeoutMs) -> void
		{
			RTC::ICE::IceServer iceServer(
			  std::addressof(listener), std::addressof(shared), usernameFragment, password, consentTimeoutSec);

			processRequest(
			  iceServer,
			  tuple,
			  usernameFragment,
			  password,
			  /*useCandidate*/ false,
			  /*nomination*/ std::nullopt);

			REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::CONNECTED);

			const auto* const timer = shared.GetTimer(ConsentCheckTimerLabel);

			if (!expectedTimeoutMs.has_value())
			{
				REQUIRE(timer == nullptr);
			}
			else
			{
				REQUIRE(timer != nullptr);
				REQUIRE(timer->GetTimeoutMs() == expectedTimeoutMs.value());
			}
		};

		runCase(/*consentTimeoutSec*/ 0, /*expectedTimeoutMs*/ std::nullopt);
		runCase(/*consentTimeoutSec*/ 5, /*expectedTimeoutMs*/ 10 * 1000);
		runCase(/*consentTimeoutSec*/ 30, /*expectedTimeoutMs*/ 30 * 1000);
		runCase(/*consentTimeoutSec*/ 70, /*expectedTimeoutMs*/ 60 * 1000);
	}

	SECTION("ICE restart keeps the old credentials until the new ones are used")
	{
		const std::string newUsernameFragment{ "newserverufrag" };
		const std::string newPassword{ "newserverpassword" };
		const auto remoteAddress = makeRemoteAddress(10001);

		auto tuple = makeTuple(remoteAddress);

		RTC::ICE::IceServer iceServer(
		  std::addressof(listener), std::addressof(shared), usernameFragment, password, ConsentTimeoutSec);

		processRequest(
		  iceServer,
		  tuple,
		  usernameFragment,
		  password,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		iceServer.RestartIce(newUsernameFragment, newPassword);

		REQUIRE(
		  listener.addedUsernameFragments ==
		  std::vector<std::string>{ usernameFragment, newUsernameFragment });
		REQUIRE(listener.removedUsernameFragments.empty());

		// The old credentials still work, and the response is protected with the
		// old password.
		processRequest(
		  iceServer,
		  tuple,
		  usernameFragment,
		  password,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		REQUIRE(listener.sentPacketClass == RTC::ICE::StunPacket::Class::SUCCESS_RESPONSE);
		REQUIRE(isSentPacketProtectedWith(password));

		// The new credentials remove the old ones.
		processRequest(
		  iceServer,
		  tuple,
		  newUsernameFragment,
		  newPassword,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		REQUIRE(listener.sentPacketClass == RTC::ICE::StunPacket::Class::SUCCESS_RESPONSE);
		REQUIRE(isSentPacketProtectedWith(newPassword));
		REQUIRE(listener.removedUsernameFragments == std::vector<std::string>{ usernameFragment });

		// The old credentials no longer work.
		processRequest(
		  iceServer,
		  tuple,
		  usernameFragment,
		  password,
		  /*useCandidate*/ false,
		  /*nomination*/ std::nullopt);

		REQUIRE(listener.sentPacketClass == RTC::ICE::StunPacket::Class::ERROR_RESPONSE);
		REQUIRE(listener.sentPacketErrorCode == 401);
		REQUIRE(iceServer.GetState() == RTC::ICE::IceServer::IceState::CONNECTED);
	}
}
