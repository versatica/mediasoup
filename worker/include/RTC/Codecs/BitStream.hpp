#include "common.hpp"

namespace RTC
{
	namespace Codecs
	{
		class BitStream
		{
		public:
			BitStream(const uint8_t* data, size_t len);
			~BitStream() = default;

			uint8_t GetBit();
			uint32_t GetBits(size_t count);
			uint32_t GetLeftBits() const;
			void SkipBits(size_t count);

		private:
			const uint8_t* data;
			uint32_t len{ 0 };
			uint32_t offset{ 0 };
		};
	} // namespace Codecs
} // namespace RTC
