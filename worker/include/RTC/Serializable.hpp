#ifndef MS_RTC_SERIALIZABLE_HPP
#define MS_RTC_SERIALIZABLE_HPP

#include "common.hpp"

namespace RTC
{
	/**
	 * Class holding a serializable data buffer. ICE, RTP, RTCP and SCTP packets
	 * inherit from this class, and also some components in those packets.
	 */
	class Serializable
	{
	protected:
		Serializable(const uint8_t* buffer, size_t size);

		virtual ~Serializable();

	public:
		/**
		 * Serializable dump.
		 */
		virtual void Dump() const = 0;

		/**
		 * Get a buffer containing the serialized content of the packet or item.
		 *
		 * @remarks
		 * - The caller must check `NeedsSerialization()` (and `Serialize()` if
		 *   needed) before calling this method.
		 *
		 * @throw MediaSoupSerializationError - If serialization is needed (this is,
		 *   if there are pending changes to apply).
		 */
		const uint8_t* GetBuffer() const;

		/**
		 * Computes total size of the packet or item (including padding if any) as
		 * if it was serialized now.
		 */
		virtual size_t GetSize() const = 0;

		/**
		 * Get the padding (in bytes) at the end of the packet or item.
		 */
		uint8_t GetPadding() const;

		/**
		 * Pad the packet or item size to 4 bytes. To achieve it, this method
		 * may add or remove bytes of padding.
		 *
		 * @return Packet or item size padded to 4 bytes.
		 *
		 * @remarks
		 * - Serialization maybe needed after calling this method.
		 */
		void PadTo4Bytes();

		/**
		 * Whether serialization is needed, meaning that the current `buffer`
		 * doesn't represent the current content of the packet or item (due to
		 * modifications not applied yet). Calling `Serialize()` or `GetBuffer()`
		 * will serialize the packet or the item.
		 */
		bool NeedsSerialization() const;

		/**
		 * Apply pending changes and serialize the content of the packet or item
		 * into a new `buffer`.
		 *
		 * @param buffer - Buffer in which the content will be serialized.
		 *
		 * @param size - Size of the given buffer.
		 *
		 * @throw MediaSoupSerializationError - If serialization fails due to
		 *   invalid content previously added.
		 *
		 * @throw MediaSoupSerializationError - If given `buffer` doesn't have
		 *   space enough to serialize the content.
		 *
		 * @privateRemarks
		 * - Classes implementing Serializable must invoke `Serialized()` in the
		 *   parent at the end of this `Serialize()` implementation.
		 */
		virtual void Serialize(uint8_t* buffer, size_t size) = 0;

	protected:
		/**
		 * To be called by child classes from public methods that affect the packet
		 * or item content in a way that serialization is needed.
		 */
		void SetSerializationNeeded();

		/**
		 * To be called by child classes at the end of their `Serialize()` method.
		 *
		 * @param buffer - Buffer in which the content has been serialized.
		 *
		 * @param size - New size of the packet or item.
		 */
		void Serialized(const uint8_t* buffer, size_t size);

	protected:
		// Buffer holding the packet or item content.
		uint8_t* buffer{ nullptr };
		// Packet or item total size (it includes padding bytes if any).
		size_t size{ 0u };
		// Number of bytes of padding.
		uint8_t padding{ 0u };

	private:
		// Whether serialization is needed due to recent modifications.
		bool serializationNeeded{ false };
	};
} // namespace RTC

#endif
