#ifndef MS_MOCKS_MOCK_BACKOFF_TIMER_HANDLE_HPP
#define MS_MOCKS_MOCK_BACKOFF_TIMER_HANDLE_HPP

#include "common.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"

namespace mocks
{
	// Forward declaration.
	class MockShared;

	class MockBackoffTimerHandle : public BackoffTimerHandleInterface
	{
		// Only MockShared class can invoke the constructor.
		friend class mocks::MockShared;

	private:
		explicit MockBackoffTimerHandle(BackoffTimerHandleOptions options)
		  : label(std::move(options.label)),
		    baseTimeoutMs(options.baseTimeoutMs),
		    backoffAlgorithm(options.backoffAlgorithm),
		    maxBackoffTimeoutMs(options.maxBackoffTimeoutMs),
		    maxRestarts(options.maxRestarts)
		{
			SetBaseTimeoutMs(baseTimeoutMs);
		}

	public:
		MockBackoffTimerHandle& operator=(const MockBackoffTimerHandle&) = delete;

		MockBackoffTimerHandle(const MockBackoffTimerHandle&) = delete;

		~MockBackoffTimerHandle() override = default;

	public:
		void Start() override
		{
			this->running = true;
		}

		void Stop() override
		{
			this->running = false;
		}

		void SetBaseTimeoutMs(uint64_t baseTimeoutMs) override
		{
			this->baseTimeoutMs = baseTimeoutMs;
		}

		bool IsRunning() const override
		{
			return this->running;
		}

		const std::string GetLabel() const override
		{
			return this->label;
		}

		std::optional<size_t> GetMaxRestarts() const override
		{
			return this->maxRestarts;
		}

		size_t GetExpirationCount() const override
		{
			return this->expirationCount;
		}

	private:
		uint64_t ComputeNextTimeoutMs() const
		{
			auto expirationCount = this->expirationCount;

			switch (this->backoffAlgorithm)
			{
				case BackoffAlgorithm::FIXED:
				{
					return this->baseTimeoutMs;
				}

				case BackoffAlgorithm::EXPONENTIAL:
				{
					auto timeoutMs = this->baseTimeoutMs;

					while (expirationCount > 0 && timeoutMs < BackoffTimerHandleInterface::MaxTimeoutMs)
					{
						timeoutMs *= 2;
						--expirationCount;

						if (this->maxBackoffTimeoutMs.has_value() && timeoutMs > this->maxBackoffTimeoutMs.value())
						{
							return this->maxBackoffTimeoutMs.value();
						}
					}

					return std::min<uint64_t>(timeoutMs, BackoffTimerHandleInterface::MaxTimeoutMs);
				}

					NO_DEFAULT_GCC();
			}
		}

	private:
		const std::string label;
		uint64_t baseTimeoutMs{ 0 };
		BackoffAlgorithm backoffAlgorithm;
		std::optional<uint64_t> maxBackoffTimeoutMs;
		std::optional<size_t> maxRestarts;
		bool running{ false };
		size_t expirationCount{ 0 };
	};
} // namespace mocks

#endif
