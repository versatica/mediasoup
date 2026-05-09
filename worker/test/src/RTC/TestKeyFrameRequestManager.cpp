#include "common.hpp"
#include "mocks/include/MockShared.hpp"
#include "RTC/KeyFrameRequestManager.hpp"
#include <catch2/catch_test_macros.hpp>

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
	mocks::MockShared shared(/*getTimeMs*/
	                         []()
	                         {
		                         return 1000;
	                         });

	SECTION("key frame requested once, not received on time")
	{
		listener.Reset();
		RTC::KeyFrameRequestManager keyFrameRequestManager(
		  std::addressof(listener), std::addressof(shared), 1000);

		keyFrameRequestManager.KeyFrameNeeded(1111);

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 1);
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

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 1);
	}

	SECTION("key frame is received on time")
	{
		listener.Reset();
		RTC::KeyFrameRequestManager keyFrameRequestManager(
		  std::addressof(listener), std::addressof(shared), 500);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameReceived(1111);

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 1);
	}

	SECTION("key frame is forced, no received on time")
	{
		listener.Reset();
		RTC::KeyFrameRequestManager keyFrameRequestManager(
		  std::addressof(listener), std::addressof(shared), 500);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.ForceKeyFrameNeeded(1111);

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 2);
	}

	SECTION("key frame is forced, received on time")
	{
		listener.Reset();
		RTC::KeyFrameRequestManager keyFrameRequestManager(
		  std::addressof(listener), std::addressof(shared), 500);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.ForceKeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameReceived(1111);

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 2);
	}
}
