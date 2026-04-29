#include "common.hpp"
#include "DepLibUV.hpp"
#include "mocks/include/MockShared.hpp"
#include "RTC/KeyFrameRequestManager.hpp"
#include <catch2/catch_test_macros.hpp>
#include <memory>

SCENARIO("KeyFrameRequestManager", "[rtp][keyframe]")
{
	class TestKeyFrameRequestManagerListener : public RTC::KeyFrameRequestManager::Listener
	{
	public:
		void OnKeyFrameNeeded(RTC::KeyFrameRequestManager* /*keyFrameRequestManager */, uint32_t /*ssrc*/) override
		{
			this->onKeyFrameNeededTimesCalled++;
		}

		void Reset()
		{
			this->onKeyFrameNeededTimesCalled = 0;
		}

	public:
		size_t onKeyFrameNeededTimesCalled{ 0 };
	};

	TestKeyFrameRequestManagerListener listener;
	mocks::MockShared shared;

	SECTION("key frame requested once, not received on time")
	{
		listener.Reset();
		RTC::KeyFrameRequestManager keyFrameRequestManager(
		  std::addressof(listener), std::addressof(shared), 1000);

		keyFrameRequestManager.KeyFrameNeeded(1111);

		// Must run the loop here to consume the timer before doing the check.
		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 2);
	}

	SECTION("key frame requested many times, not received on time")
	{
		listener.Reset();
		RTC::KeyFrameRequestManager keyFrameRequestManager(
		  std::addressof(listener), std::addressof(shared), 500);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameNeeded(1111);

		// Must run the loop here to consume the timer before doing the check.
		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 2);
	}

	SECTION("key frame is received on time")
	{
		listener.Reset();
		RTC::KeyFrameRequestManager keyFrameRequestManager(
		  std::addressof(listener), std::addressof(shared), 500);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameReceived(1111);

		// Must run the loop here to consume the timer before doing the check.
		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 1);
	}

	SECTION("key frame is forced, no received on time")
	{
		listener.Reset();
		RTC::KeyFrameRequestManager keyFrameRequestManager(
		  std::addressof(listener), std::addressof(shared), 500);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.ForceKeyFrameNeeded(1111);

		// Must run the loop here to consume the timer before doing the check.
		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 3);
	}

	SECTION("key frame is forced, received on time")
	{
		listener.Reset();
		RTC::KeyFrameRequestManager keyFrameRequestManager(
		  std::addressof(listener), std::addressof(shared), 500);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.ForceKeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameReceived(1111);

		// Must run the loop here to consume the timer before doing the check.
		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 2);
	}

	// Must run the loop to wait for UV timers and close them.
	// NOTE: Not really needed in this file since we run it in each SECTION in
	// purpose.
	DepLibUV::RunLoop();
}
