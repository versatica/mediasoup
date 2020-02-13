#include "common.hpp"
#include "DepLibUV.hpp"
#include "RTC/KeyFrameRequestManager.hpp"
#include "handles/Timer.hpp"
#include <catch.hpp>

using namespace RTC;

SCENARIO("KeyFrameRequestManager", "[rtp][keyframe]")
{
	// Start LibUV.
	DepLibUV::ClassInit();

	class TestKeyFrameRequestManagerListener : public KeyFrameRequestManager::Listener
	{
	public:
		void OnKeyFrameNeeded(KeyFrameRequestManager* /*keyFrameRequestManager */, uint32_t /*ssrc*/) override
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

	static TestKeyFrameRequestManagerListener listener;

	SECTION("key frame requested once, not received on time")
	{
		listener.Reset();
		KeyFrameRequestManager keyFrameRequestManager(&listener, 1000);

		keyFrameRequestManager.KeyFrameNeeded(1111);

		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 2);
	}

	SECTION("key frame requested many times, not received on time")
	{
		listener.Reset();
		KeyFrameRequestManager keyFrameRequestManager(&listener, 500);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameNeeded(1111);

		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 2);
	}

	SECTION("key frame is received on time")
	{
		listener.Reset();
		KeyFrameRequestManager keyFrameRequestManager(&listener, 500);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameReceived(1111);

		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 1);
	}

	SECTION("key frame is forced, no received on time")
	{
		listener.Reset();
		KeyFrameRequestManager keyFrameRequestManager(&listener, 500);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.ForceKeyFrameNeeded(1111);

		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 3);
	}

	SECTION("key frame is forced, received on time")
	{
		listener.Reset();
		KeyFrameRequestManager keyFrameRequestManager(&listener, 500);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.ForceKeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameReceived(1111);

		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 2);
	}
}
