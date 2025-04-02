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
		 * Maximum length the Serializable can take. This is the `bufferLength`
		 * argument given to the constructor.
		 */
		virtual const size_t GetBufferLength() const final
		{
			return this->bufferLength;
		}

		/**
		 * Update the buffer length of the Serializable.
		 **
		 * @remarks
		 * - The child class must invoke this method after parsing completes in
		 *   case it couldn't anticipate its expected exact length. Specially
		 *   useful when parsing variable-length items within a packet.
		 * - The application can also invoke this method on the Serializable if
		 *   it has expanded or reduced the length of the buffer currently assigned
		 *   to the Serializable.
		 *
		 * @throw MediaSoupError - If given `bufferLength` is lower than the
		 * current exact length of the Serializable.
		 */
		virtual void SetBufferLength(size_t bufferLength) final;

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
		 * The subclass must override this method if it hold pointers or allocated
		 * memory. In that case, the overridden method must manually invoke
		 * `SetBuffer()` and `SetBufferLenght()`.
		 *
		 * @throw MediaSoupError - If given `bufferLength` is lower than the
		 * current exact length of the Serializable.
		 */
		virtual void Serialize(const uint8_t* buffer, size_t bufferLength);

		/**
		 * Clone the Serializable into a new buffer. This method returns a new
		 * instance of Serializable which doesn't share any memory with the original
		 * one.
		 *
		 * @param buffer - The new buffer in which the cloned Serializable will be
		 *   serialized.
		 * @param bufferLength - New buffer length.
		 *
		 * @remarks
		 * - The subclass need to clone any pointer or allocated memory it holds
		 * internally.
		 * - The `Clone()` method of the subclass must manually invoke `SetLength()`.
		 *
		 * @throw MediaSoupError - If given `bufferLength` is lower than the
		 * current exact length of the Serializable.
		 */
		virtual std::unique_ptr<Serializable> Clone(const uint8_t* buffer, size_t bufferLength) const = 0;

		/**
		 * Methods to be used by classes inheriting from Serializable.
		 */
	protected:
		/**
		 * Method to be called by the child class in case it overrides the
		 * `Serialize()` method.
		 */
		virtual void SetBuffer(const uint8_t* buffer) final
		{
			this->buffer = const_cast<uint8_t*>(buffer);
		}

		/**
		 * Method to be called by the child class to update the current exact
		 * length of the Serializable.
		 *
		 * @remarks
		 * The child class must invoke this method after parsing completes and
		 * after every change in the Serializable content that affects its current
		 * length.
		 *
		 * @throw MediaSoupError - If given `length` is larger than the buffer
		 * length of the Serializable.
		 */
		virtual void SetLength(size_t length) final;

	private:
		// Buffer holding the Serializable content.
		uint8_t* buffer{ nullptr };
		// Length of the buffer. This is the maximum length the Serializable can
		// take.
		size_t bufferLength{ 0u };
		// Serializable exact length (includes padding bytes).
		size_t length{ 0u };
	};
} // namespace RTC

#endif
