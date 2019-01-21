#include "common.hpp"
#include "DepLibUV.hpp"
#include "catch.hpp"
#include "RTC/KeyFrameRequestManager.hpp"
#include "handles/Timer.hpp"

using namespace RTC;

SCENARIO("KeyFrameRequestManager", "[rtp][keyframe]")
{
	// Start LibUV.
	DepLibUV::ClassInit();

	class TestKeyFrameRequestManagerListener : public KeyFrameRequestManager::Listener
	{
	public:
		void OnKeyFrameNeeded(uint32_t /*ssrc*/) override
		{
			this->onKeyFrameNeededTimesCalled++;
		}

	public:
		size_t onKeyFrameNeededTimesCalled{ 0 };
	};

	static TestKeyFrameRequestManagerListener listener;

	SECTION("KeyFrame requested once, not received on time")
	{
		KeyFrameRequestManager keyFrameRequestManager(&listener);

		keyFrameRequestManager.KeyFrameNeeded(1111);

		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 1);
	}

	SECTION("KeyFrame requested many times, not received on time")
	{
		KeyFrameRequestManager keyFrameRequestManager(&listener);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameNeeded(1111);

		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 2);
	}

	SECTION("KeyFrame is received on time")
	{
		KeyFrameRequestManager keyFrameRequestManager(&listener);

		keyFrameRequestManager.KeyFrameNeeded(1111);
		keyFrameRequestManager.KeyFrameReceived(1111);

		DepLibUV::RunLoop();

		REQUIRE(listener.onKeyFrameNeededTimesCalled == 3);
	}
}
