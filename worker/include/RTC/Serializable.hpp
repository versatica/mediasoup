#ifndef MS_RTC_SERIALIZABLE_HPP
#define MS_RTC_SERIALIZABLE_HPP

#include "common.hpp"

namespace RTC
{
	/**
	 * Class holding serializable content.
	 *
	 * @remarks
	 * - ICE, RTP, RTCP, SCTP packets and some items in those packets inherit
	 *   from this class.
	 * - Typically many of those packets and items may include padding bytes to
	 *   be multiple of 4 bytes. However it's up to each packet or item to deal
	 *   with padding. From the point of view of the Serializable class, the
	 *   length of a Serializable packet or item must include its padding bytes
	 *   (if any).
	 */
	class Serializable
	{
		using ConsolidatedListener = std::function<void()>;

	public:
		/**
		 * @param buffer - The buffer holding the packet.
		 * @param bufferLength - Buffer length.
		 *
		 * @remarks
		 * - In same cases, `bufferLength` is the exact length of the Serializable
		 *   and in other cases `bufferLength` is the maximum length that the
		 *   Serializable can take.
		 * - Always use `GetLength()` to obtain the exact length of the
		 *   Serializable.
		 */
		Serializable(const uint8_t* buffer, size_t bufferLength)
		  : buffer(const_cast<uint8_t*>(buffer)), bufferLength(bufferLength)
		{
		}

		virtual ~Serializable() = default;

	public:
		/**
		 * Print Serializable state.
		 */
		virtual void Dump() const = 0;

		/**
		 * Get a buffer containing the serialized content. Combined with the
		 * `GetLength()` method, the application can obtain the full sequence of
		 * bytes of the Serializable.
		 */
		virtual const uint8_t* GetBuffer() const final
		{
			return this->buffer;
		}

		/**
		 * Maximum length the Serializable can take. This is the `bufferLength`
		 * argument given to the constructor. It's guaranteed to be equal or greater
		 * than value returned by `GetLength()`.
		 */
		virtual const size_t GetBufferLength() const final
		{
			return this->bufferLength;
		}

		/**
		 * Current exact length of the Serializable, including padding bytes (if
		 * any).
		 *
		 * @remarks
		 * - It returns the current value of the `length` member, which can be
		 *   updated by the child class at any time by calling `SetLength()`.
		 * - It's guaranteed to be less or equal to `GetBufferLength()`.
		 */
		virtual const size_t GetLength() const final
		{
			return this->length;
		}

		/**
		 * Whether the Serializable is frozen, meaning that modifications are not
		 * allowed.
		 */
		virtual bool IsFrozen() const final
		{
			return this->frozen;
		}

		/**
		 * Freeze the Serializable, meaning that modifications are not allowed
		 * on it. If a modification is attempted it will throw MediasoupError.
		 *
		 * @remarks
		 * - Subclasses should invoke `Serializable::AssertNotFrozen()` in public
		 *   methods that modify their internal state/content.
		 * - When calling `Serialize()` the `frozen` flag will remain the same as
		 *   it was.
		 * - When calling `Clone()` on a Serializable, the new instance returned
		 *   will keep same `frozen` flag value than the original Serializable.
		 * - In both cases, `Serialize()` and `Clone`, the Serializable class may
		 *   explicitly revert the `frozen` flag to false (by calling `Unfreeze()`
		 *   on its own but it should only do it in the instance itself and not in
		 *   any interal `Serializable` member.
		 */
		virtual void Freeze() final
		{
			this->frozen = true;
		}

		/**
		 * Serialize the Serializable into a new buffer. This method copies the
		 * bytes of the internal buffer into the new buffer and makes `GetBuffer()`
		 * point to the new one.
		 *
		 * @param buffer - The new buffer in which the Serializable will be
		 *   serialized.
		 * @param bufferLength - New buffer length.
		 *
		 * @remarks
		 * In addition to call this method in Serializable parent class, the
		 * `Serialize()` implementation in the subclass must also reassign any
		 * pointers it holds and make them point to the proper position in the new
		 * buffer.
		 *
		 * @throw MediaSoupError - If given `bufferLength` is lower than the
		 *   current exact length of the Serializable.
		 */
		virtual void Serialize(uint8_t* buffer, size_t bufferLength);

		/**
		 * Clone the Serializable into a new buffer. This method returns a new
		 * instance of the Serializable subclass which doesn't share any memory
		 * with the original one.
		 *
		 * @param buffer - The buffer for the cloned Serializable.
		 * @param bufferLength - Buffer length.
		 *
		 * @remarks
		 * - The subclass must redeclare this method with a pointer to the subclass
		 *   as return type.
		 * - The subclass need to clone any pointer or allocated memory it holds
		 *   internally.
		 * - In subclasses that do not hold other Serializable instances and do not
		 *   allocate memory by themselves, the Clone() mthod can be as follows:
		 *   ```c++
		 *   SerializableSubclass* SerializableSubclass::Clone(uint8_t* buffer, size_t bufferLength) const
		 *   {
		 *     auto* cloned = new SerializableSubclass(buffer, bufferLength);
		 *
		 *     Serializable::CloneInto(cloned);
		 *
		 *     return clonedItem;
		 *   }
		 *   ```
		 * - In addition to call this method in Serializable parent class, the
		 *  `Clone()` implementation in the subclass must also replicate any
		 *   pointers it holds and make them point to the proper position in the
		 *   new buffer.
		 *
		 * @throw MediaSoupError - If given `bufferLength` is lower than the
		 *   current exact length of the Serializable.
		 */
		virtual Serializable* Clone(uint8_t* buffer, size_t bufferLength) const = 0;

		/**
		 * The application must call this method on a Serializable when it's been
		 * constructed within a parent object that needs to know when this
		 * Serializable is done to recompute its total length and internal pointers.
		 *
		 * @remarks
		 * This method freezes the Serializable.
		 *
		 * @throw MediaSoupError - If `SetConsolidatedListener()` was not called
		 *   first.
		 *
		 * @see SetConsolidatedListener()
		 */
		virtual void Consolidate() final;

		/**
		 * Methods to be used by classes inheriting from Serializable.
		 */
	protected:
		/**
		 * Method to be called by the child class in case it overrides the
		 * `Serialize()` method.
		 */
		virtual void SetBuffer(uint8_t* buffer) final
		{
			this->buffer = buffer;
		}

		/**
		 * Update the buffer length of the Serializable.
		 **
		 * @remarks
		 * - The child class must invoke this method after parsing completes in
		 *   case it couldn't anticipate its expected exact length. Specially
		 *   useful when parsing variable-length items within a packet.
		 *
		 * @throw
		 * - MediaSoupError - If given `bufferLength` is lower than the current
		 *   exact length of the Serializable.
		 * - MediaSoupError - If 0 is given.
		 */
		virtual void SetBufferLength(size_t bufferLength) final;

		/**
		 * Method to be called by the child class to update the current exact
		 * length of the Serializable.
		 *
		 * @remarks
		 * The child class must invoke this method after parsing completes and
		 * after every change in the Serializable content that affects its current
		 * length.
		 *
		 * @throw
		 * - MediaSoupError - If given `length` is larger than the buffer length of
		 *   the Serializable.
		 * - MediaSoupError - If 0 is given.
		 */
		virtual void SetLength(size_t length) final;

		/**
		 * Clone Serializable into the given Serializable. Subclasses can invoke
		 * this method within their Clone() implementation.
		 *
		 * @remarks
		 * If this method throws (due to the buffer length of the given Serializable
		 * being too small, then it deletes the given `serializable` pointer and
		 * then throws, meaning that the subclass must not delete it in case it does
		 * try/catch.
		 *
		 * @throw MediaSoupError - If the buffer length of the given `serializable`
		 *   is too small.
		 */
		virtual void CloneInto(Serializable* serializable) const final;

		/**
		 * Fill the last `padding` bytes of the buffer with zeros.
		 *
		 * @remarks This method does NOT add bytes to the buffer.
		 */
		virtual void FillPadding(uint8_t padding) final;

		/**
		 * Set a listener that will be invoked when calling `Consolidate()` on this
		 * Serializable.
		 *
		 * @see Consolidate()
		 *
		 * @example
		 * ```c++
		 * chunk->SetConsolidatedListener([this, chunk]()
		 * {
		 *   SetLength(GetLength() + chunk->GetLength());
		 * });
		 * ```
		 */
		virtual void SetConsolidatedListener(ConsolidatedListener&& listener) final;

		/**
		 * Assert that the Serializable is not frozen, otherwise it throws a
		 * MediasoupError error.
		 */
		virtual void AssertNotFrozen() const final;

	private:
		// Buffer holding the Serializable content.
		uint8_t* buffer{ nullptr };
		// Length of the buffer. This is the maximum length the Serializable can
		// take.
		size_t bufferLength{ 0u };
		// Serializable exact length (includes padding bytes).
		size_t length{ 0u };
		// Whether the Serializable is frozen, meaning that modifications are not
		// allowed.
		bool frozen{ false };
		// Event listener invoked when the Serializable is consolidated.
		ConsolidatedListener consolidatedListener{ nullptr };
	};
} // namespace RTC

#endif
