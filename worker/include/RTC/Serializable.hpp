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
	 *   with padding. From the point of view of the Serializable class, the size
	 *   of a serializable packet or item must include the padding in that packet
	 *   or item (if any).
	 */
	class Serializable
	{
	protected:
		/**
		 * @param buffer - The buffer holding the packet.
		 */
		Serializable(uint8_t* buffer);

		virtual ~Serializable();

	public:
		/**
		 * Get a buffer containing the serialized content. Combined with
		 * `GetSize()`, the application can obtain the full sequence of bytes
		 * of the serializable.
		 *
		 * @remarks
		 * The application must check `NeedsSerialization()` (and call `Serialize()`
		 * if serialization is needed) before calling this method.
		 *
		 * @throw MediaSoupError - If serialization is needed (this is, if there
		 *   are pending changes to apply so the current buffer doesn't correspond
		 *   to the real content of the serializable).
		 */
		virtual const uint8_t* GetBuffer() const final;

		/**
		 * Total size of the serializable if it was serialized now.
		 *
		 * @remarks
		 * Child classes implementing this method should check `NeedsSerialization()`
		 * and, if serialization is not needed, just return `GetCurrentSize()`.
		 */
		virtual size_t GetSize() const = 0;

		/**
		 * Whether serialization is needed, meaning that the internal buffer
		 * no longer represents the current content of the serializable (due to
		 * modifications made after the serializable was created or after it was
		 * last modified).
		 */
		virtual bool NeedsSerialization() const final;

		/**
		 * Apply pending changes and serialize the content of the serializable
		 * into the given new `buffer`. Once serialized, the previous buffer of
		 * the serializable is no longer used and can be reused for other purposes.
		 *
		 * @param buffer - Buffer in which the content will be serialized.
		 *
		 * @remarks
		 * - `buffer` cannot be the same as the one currently used by the
		 *   serializable. A separate buffer must be given.
		 * - Child classes must call `Serialized()` at the end of their
		 *   `Serialize()` method implementation.
		 *
		 * @throw MediaSoupError - If serialization fails due to invalid
		 *   modifications previously made.
		 */
		virtual void Serialize(uint8_t* buffer) = 0;

		/**
		 * Methods to be used by classes inheriting from Serializable.
		 */
	protected:
		/**
		 * Method that must be called when constructing a serializable by parsing
		 * a buffer.
		 *
		 * @param size - The serializable size computed during the parsing process.
		 *
		 * @remarks
		 * It's recommented that classes inheriting from Serializable expose a
		 * static `Parse()` method that reads from a buffer and creates an instance
		 * of the class.
		 *
		 * While inspecting the buffer, the parser usually calls methods on the
		 * ongoing serializable that affects its internal state and hence the
		 * `serializationNeeded` flag is set to true. However that's not true
		 * because, obviously, no serialization is needed. So this method
		 * internally sets the `serializationNeeded` flag to false.
		 *
		 * In addition to that, in some cases the parser cannot anticipate how many
		 * bytes the serializable will take until the parsing process completes.
		 * That's why this method must be called with the computed serializable
		 * size as argument once the parsing process is done.
		 *
		 * @throw MediaSoupError - If called twice.
		 */
		virtual void Parsed(size_t size) final;

		/**
		 * Current buffer of the serializable, no matter serialization is needed.
		 */
		virtual const uint8_t* GetCurrentBuffer() const final;

		/**
		 * Current serializable size, no matter serialization is needed.
		 */
		virtual size_t GetCurrentSize() const final;

		/**
		 * To be called from child classes' methods that affect the serializable
		 * content in a way that serialization is needed.
		 */
		virtual void SetSerializationNeeded() final;

		/**
		 * To be called by child classes at the end of their `Serialize()` method.
		 *
		 * @param buffer - Buffer in which the content has been serialized.
		 * @param size - New size of the serializable.
		 *
		 * @remarks
		 * This method internally sets the `serializationNeeded` flag to false.
		 */
		virtual void Serialized(uint8_t* buffer, size_t size) final;

	private:
		// Buffer holding the serializable content.
		uint8_t* buffer{ nullptr };
		// Serializable size.
		size_t size{ 0u };

	private:
		// Whether serialization is needed due to recent modifications.
		bool serializationNeeded{ false };
	};
} // namespace RTC

#endif
