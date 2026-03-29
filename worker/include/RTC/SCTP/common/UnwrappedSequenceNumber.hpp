#ifndef MS_RTC_SCTP_UNWRAPPED_SEQUENCE_NUMBER_HPP
#define MS_RTC_SCTP_UNWRAPPED_SEQUENCE_NUMBER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include <limits>
#include <ostream>
#include <typeinfo>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * UnwrappedSequenceNumber handles wrapping sequence numbers and unwraps
		 * them to an int64_t value space, to allow wrapped sequence numbers to be
		 * easily compared for ordering.
		 *
		 * Sequence numbers are expected to be monotonically increasing, but they
		 * do not need to be unwrapped in order, as long as the difference to the
		 * previous one is not larger than half the range of the wrapped sequence
		 * number.
		 */
		template<typename T>
		class UnwrappedSequenceNumber
		{
		public:
			static_assert(!std::numeric_limits<T>::is_signed, "the wrapped type must be unsigned");
			static_assert(
			  std::numeric_limits<T>::max() < std::numeric_limits<int64_t>::max(),
			  "the wrapped type must be less than the int64_t value space");

			/**
			 * The unwrapper is a sort of factory and converts wrapped sequence
			 * numbers to unwrapped ones.
			 */
			class Unwrapper
			{
			public:
				Unwrapper() = default;

				Unwrapper(const Unwrapper&) = default;

				Unwrapper& operator=(const Unwrapper&) = default;

			public:
				/**
				 * Given a wrapped `value`, and with knowledge of its current last seen
				 * largest number, will return a value that can be compared using normal
				 * operators, such as less-than, greater-than etc.
				 *
				 * This will also update the Unwrapper's state, to track the last seen
				 * largest value.
				 */
				UnwrappedSequenceNumber<T> Unwrap(T value)
				{
					if (!this->lastValue)
					{
						this->lastUnwrapped = value;
					}
					else
					{
						const uint64_t prev = this->lastValue->GetValue();
						const uint64_t curr = value;
						auto delta          = static_cast<int64_t>(curr - prev);
						const auto half     = static_cast<int64_t>(1) << (sizeof(T) * 8 - 1);

						if (delta < -half)
						{
							delta += static_cast<int64_t>(1) << (sizeof(T) * 8);
						}
						else if (delta > half)
						{
							delta -= static_cast<int64_t>(1) << (sizeof(T) * 8);
						}

						this->lastUnwrapped += delta;
					}

					this->lastValue = UnwrappedSequenceNumber<T>(value);

					return UnwrappedSequenceNumber<T>(this->lastUnwrapped);
				}

				/**
				 * Similar to `Unwrap()`, but will not update the Unwrappers's internal
				 * state.
				 */
				UnwrappedSequenceNumber<T> PeekUnwrap(T value) const
				{
					if (!this->lastValue)
					{
						return UnwrappedSequenceNumber<T>(value);
					}

					const uint64_t prev = this->lastValue->GetValue();
					const uint64_t curr = value;
					auto delta          = static_cast<int64_t>(curr - prev);
					const auto half     = static_cast<int64_t>(1) << (sizeof(T) * 8 - 1);

					if (delta < -half)
					{
						delta += static_cast<int64_t>(1) << (sizeof(T) * 8);
					}
					else if (delta > half)
					{
						delta -= static_cast<int64_t>(1) << (sizeof(T) * 8);
					}

					return UnwrappedSequenceNumber<T>(this->lastUnwrapped + delta);
				}

				/**
				 * Resets the Unwrapper to its pristine state. Used when a sequence number
				 * is to be reset to zero.
				 */
				void Reset()
				{
					this->lastUnwrapped = 0;
					this->lastValue.reset();
				}

			private:
				int64_t lastUnwrapped{ 0 };
				std::optional<UnwrappedSequenceNumber<T>> lastValue;
			};

		public:
			/**
			 * Returns a new sequence number based on `value`, and adding `delta`
			 * (which may be negative).
			 */
			static UnwrappedSequenceNumber<T> AddTo(UnwrappedSequenceNumber<T> value, int64_t delta)
			{
				return UnwrappedSequenceNumber<T>(value.value + delta);
			}

			/**
			 * Returns the absolute difference between `lhs` and `rhs`.
			 */
			static T Difference(UnwrappedSequenceNumber<T> lhs, UnwrappedSequenceNumber<T> rhs)
			{
				return (lhs.value > rhs.value) ? (lhs.value - rhs.value) : (rhs.value - lhs.value);
			}

		private:
			static constexpr auto ValueLimit = static_cast<int64_t>(1) << std::numeric_limits<T>::digits;

		public:
			explicit UnwrappedSequenceNumber(int64_t value) : value(value)
			{
			}

		public:
			/**
			 * Returns the wrapped value this type represents.
			 */
			T Wrap() const
			{
				return static_cast<T>(this->value % UnwrappedSequenceNumber::ValueLimit);
			}

			bool operator==(const UnwrappedSequenceNumber<T>& other) const
			{
				return this->value == other.value;
			}

			bool operator!=(const UnwrappedSequenceNumber<T>& other) const
			{
				return this->value != other.value;
			}

			bool operator<(const UnwrappedSequenceNumber<T>& other) const
			{
				return this->value < other.value;
			}

			bool operator>(const UnwrappedSequenceNumber<T>& other) const
			{
				return this->value > other.value;
			}

			bool operator>=(const UnwrappedSequenceNumber<T>& other) const
			{
				return this->value >= other.value;
			}

			bool operator<=(const UnwrappedSequenceNumber<T>& other) const
			{
				return this->value <= other.value;
			}

			// Const accessors for underlying value.

			constexpr const int64_t* operator->() const
			{
				return std::addressof(this->value);
			}

			constexpr const int64_t& operator*() const&
			{
				return this->value;
			}

			constexpr const int64_t&& operator*() const&&
			{
				return std::move(this->value);
			}

			constexpr const int64_t& GetValue() const&
			{
				return this->value;
			}

			constexpr const int64_t&& GetValue() const&&
			{
				return std::move(this->value);
			}

			constexpr explicit operator const int64_t&() const&
			{
				return this->value;
			}

			/**
			 * Increments the value.
			 */
			void Increment()
			{
				++this->value;
			}

			/**
			 * Returns the next value relative to this sequence number.
			 */
			UnwrappedSequenceNumber<T> GetNextValue() const
			{
				return UnwrappedSequenceNumber<T>(this->value + 1);
			}

		private:
			int64_t value{ 0 };
		};

		/**
		 * For logging purposes in Catch2 tests.
		 */
		template<typename T>
		inline std::ostream& operator<<(std::ostream& os, const UnwrappedSequenceNumber<T>& s)
		{
			return os << "{T:" << typeid(T).name() << ", wrapped:" << s.Wrap();
		}

		template<>
		inline std::ostream& operator<<(std::ostream& os, const UnwrappedSequenceNumber<uint8_t>& s)
		{
			return os << "{T:uint8_t, wrapped:" << s.Wrap() << "}";
		}

		template<>
		inline std::ostream& operator<<(std::ostream& os, const UnwrappedSequenceNumber<uint16_t>& s)
		{
			return os << "{T:uint16_t, wrapped:" << s.Wrap() << "}";
		}

		template<>
		inline std::ostream& operator<<(std::ostream& os, const UnwrappedSequenceNumber<uint32_t>& s)
		{
			return os << "{T:uint32_t, wrapped:" << s.Wrap() << "}";
		}
	} // namespace SCTP
} // namespace RTC

#endif
