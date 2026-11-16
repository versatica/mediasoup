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
	 *   from this class (or will inherit).
	 * - Typically many of those packets and items may include padding bytes to
	 *   be multiple of 4 bytes. However it's up to each packet or item to deal
	 *   with padding. From the point of view of the Serializable class, the
	 *   length of a Serializable packet or item must include its padding bytes
	 *   (if any).
	 */
	class Serializable
	{
	public:
		using BufferReleasedListener = std::function<void(const Serializable*, uint8_t* buffer)>;

	protected:
		/**
		 * Listener set by the parent Serializable in which this one is being built
		 * in place. It's invoked with `consolidate` set to true by `Consolidate()`
		 * and with `consolidate` set to false by `Discard()`.
		 */
		using BuildInPlaceListener = std::function<void(bool consolidate)>;

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
		Serializable(const uint8_t* buffer, size_t bufferLength);

		virtual ~Serializable();

	public:
		/**
		 * Print Serializable state with given indentation.
		 */
		virtual void Dump(int indentation = 0) const = 0;

		/**
		 * Get a buffer containing the serialized content. Combined with the
		 * `GetLength()` method, the application can obtain the full sequence of
		 * bytes of the Serializable in network byte order.
		 */
		virtual const uint8_t* GetBuffer() const final
		{
			return this->buffer;
		}

		/**
		 * Maximum length the Serializable can take. It's guaranteed to be equal or
		 * greater than value returned by `GetLength()`.
		 */
		virtual size_t GetBufferLength() const final
		{
			return this->bufferLength;
		}

		/**
		 * Current exact length of the Serializable, including padding bytes (if
		 * any).
		 */
		virtual size_t GetLength() const final
		{
			return this->length;
		}

		/**
		 * Current available length of the Serializable.
		 */
		virtual size_t GetAvailableLength() const final
		{
			return this->bufferLength - this->length;
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
		 * - In addition to call this method in Serializable parent class, the
		 *   `Serialize()` implementation in the subclass must also reassign any
		 *   pointers it holds and make them point to the proper position in the
		 *   new buffer.
		 *
		 * @throw
		 * - MediaSoupError - If some item is being built in place within this
		 *   Serializable (see `NeedsConsolidation()`).
		 * - MediaSoupError - If given `bufferLength` is lower than the current
		 *   exact length of the Serializable.
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
		 * @throw
		 * - MediaSoupError - If some item is being built in place within this
		 *   Serializable (see `NeedsConsolidation()`).
		 * - MediaSoupError - If given `bufferLength` is lower than the current
		 *   exact length of the Serializable.
		 */
		virtual Serializable* Clone(uint8_t* buffer, size_t bufferLength) const = 0;

		/**
		 * Set a listener that will be invoked when the current buffer is released,
		 * meaning that this Serializable no longer uses it.
		 *
		 * @remarks
		 * - The caller should call this method with `nullptr` as argument in case
		 *   the lifetime of the previously passed `listener` ends before the
		 *   Serializable is destroyed or serialized. Otherwise the Serializable
		 *   will invoke a listener that was already destroyed.
		 */
		virtual void SetBufferReleasedListener(BufferReleasedListener* listener) final;

		/**
		 * The application must call this method on a Serializable when it's been
		 * constructed within a parent Serializable object that needs to know when
		 * this Serializable is done to recompute its total length and internal
		 * pointers.
		 *
		 * @remarks
		 * - The listener is removed before being invoked, so this method or
		 *   `Discard()` can only be called once per `SetBuildInPlaceListener()`
		 *   call.
		 * - Once consolidated, the Serializable MUST NOT be modified, since the
		 *   parent does not track later changes (for example, a change in its
		 *   length would leave the parent with a wrong length).
		 *
		 * @throw
		 * - MediaSoupError - If `SetBuildInPlaceListener()` was not called first.
		 * - MediaSoupError - If this method or `Discard()` was already called.
		 * - MediaSoupError - If some item is being built in place within this
		 *   Serializable (see `NeedsConsolidation()`), in which case nothing
		 *   changes and this method can be called again.
		 *
		 * @see SetBuildInPlaceListener()
		 */
		virtual void Consolidate() const final;

		/**
		 * The application can call this method, instead of `Consolidate()`, on a
		 * Serializable that's been constructed within a parent Serializable object
		 * in order to remove it from the parent, which then deletes it. The parent
		 * is left as it was before the Serializable was built in place.
		 *
		 * @remarks
		 * - The Serializable is deleted by the parent, so it MUST NOT be used
		 *   after calling this method (unless it throws).
		 * - The listener is removed before being invoked, so this method or
		 *   `Consolidate()` can only be called once per `SetBuildInPlaceListener()`
		 *   call.
		 *
		 * @throw
		 * - MediaSoupError - If `SetBuildInPlaceListener()` was not called first.
		 * - MediaSoupError - If this method or `Consolidate()` was already called.
		 * - MediaSoupError - If some item is being built in place within this
		 *   Serializable (see `NeedsConsolidation()`), in which case nothing
		 *   changes and this method can be called again.
		 *
		 * @see SetBuildInPlaceListener()
		 */
		virtual void Discard() const final;

		/**
		 * Whether some item is being built in place within this Serializable and
		 * `Consolidate()` or `Discard()` wasn't invoked on it yet.
		 */
		virtual bool NeedsConsolidation() const final
		{
			return this->needsConsolidation;
		}

		/**
		 * Methods to be used by classes inheriting from Serializable.
		 */
	protected:
		/**
		 * Change the buffer of the Serializable.
		 */
		virtual void SetBuffer(uint8_t* buffer) final;

		/**
		 * Update the buffer length of the Serializable.
		 *
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
		 * - The child class must invoke this method after parsing completes and
		 *   after every change in the Serializable content that affects its
		 *   current length.
		 *
		 * @throw
		 * - MediaSoupError - If given `length` is larger than the buffer length of
		 *   the Serializable.
		 */
		virtual void SetLength(size_t length) final;

		/**
		 * Clone the Serializable into the given Serializable.
		 *
		 * @remarks
		 * - If this method throws, then it deletes the given `serializable`
		 *   pointer before throwing, meaning that the subclass must not delete it
		 *   in case it captured the error.
		 *
		 * @throw
		 * - MediaSoupError - If some item is being built in place within this
		 *   Serializable (see `NeedsConsolidation()`).
		 * - MediaSoupError - If the buffer length of the given `serializable` is
		 *   too small.
		 */
		virtual void CloneInto(Serializable* serializable) const;

		/**
		 * Fill the last given `padding` number of bytes of the buffer with zeros.
		 *
		 * @remarks
		 * - This method does NOT add bytes to the buffer.
		 */
		virtual void FillPadding(uint8_t padding) final;

		/**
		 * Set a listener that will be invoked when calling `Consolidate()` or
		 * `Discard()` on this Serializable.
		 *
		 * @see Consolidate()
		 * @see Discard()
		 */
		virtual void SetBuildInPlaceListener(BuildInPlaceListener&& listener) final;

		/**
		 * To be called by the parent Serializable with `true` when it starts
		 * building an item in place within it, and with `false` once that item is
		 * consolidated or discarded.
		 *
		 * @see NeedsConsolidation()
		 */
		virtual void SetNeedsConsolidation(bool flag) final;

		/**
		 * @throw
		 * - MediaSoupError - If some item is being built in place within this
		 *   Serializable.
		 *
		 * @see NeedsConsolidation()
		 */
		virtual void AssertDoesNotNeedConsolidation() const final;

	private:
		// Buffer holding the Serializable content.
		uint8_t* buffer;
		// Length of the buffer. This is the maximum length the Serializable can
		// take.
		size_t bufferLength;
		// Serializable current exact length (includes padding bytes).
		size_t length{ 0 };
		// Event listener invoked when the current buffer is released (no longer
		// used by this Serializable).
		BufferReleasedListener* bufferReleasedListener{ nullptr };
		// Event listener invoked when the Serializable is consolidated or
		// discarded. It's removed by the const `Consolidate()` and `Discard()`
		// methods before invoking it.
		mutable BuildInPlaceListener buildInPlaceListener;
		// Whether some item is being built in place within this Serializable and
		// `Consolidate()` or `Discard()` wasn't invoked on it yet.
		bool needsConsolidation{ false };
	};
} // namespace RTC

#endif
