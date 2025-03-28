#ifndef MS_RTC_SERIALIZABLE_HPP
#define MS_RTC_SERIALIZABLE_HPP

#include "common.hpp"

namespace RTC
{
	/**
	 * Class holding serializable content with optional padding.
	 *
	 * @remarks
	 * - ICE, RTP, RTCP, SCTP packets and some components in those packets
	 *   inherit from this class.
	 */
	class Serializable
	{
	protected:
		Serializable(uint8_t* buffer);

		virtual ~Serializable();

	public:
		/**
		 * Print serializable information.
		 */
		virtual void Dump() const = 0;

		/**
		 * Get a buffer containing the serialized content.
		 *
		 * @remarks
		 * - The caller must check `NeedsSerialization()` (and `Serialize()` if
		 *   needed) before calling this method.
		 *
		 * @throw MediaSoupError - If serialization is needed (this is, if there
		 *   are pending changes to apply).
		 */
		const uint8_t* GetBuffer() const;

		/**
		 * Total size of the serializable (including padding bytes if any).
		 *
		 * @remarks
		 * - The caller must check `NeedsSerialization()` (and `Serialize()` if
		 *   needed) before calling this method.
		 *
		 * @throw MediaSoupError - If serialization is needed (this is, if there
		 *   are pending changes to apply).
		 */
		size_t GetSize() const;

		/**
		 * Computes the content size of the serializable (padding bytes excluded).
		 */
		virtual size_t ComputeContentSize() const = 0;

		/**
		 * Whether serialization is needed, meaning that the current `buffer`
		 * doesn't represent the current content of the serializable (due to
		 * modifications not applied yet). Calling `Serialize()` or `GetBuffer()`
		 * will serialize the packet or the item.
		 */
		bool NeedsSerialization() const;

		/**
		 * Apply pending changes and serialize the content of the serializable
		 * into a new `buffer`.
		 *
		 * @param buffer - Buffer in which the content will be serialized.
		 * @param padTo4Bytes - Whether padding bytes should be added for the size
		 *   of the serializable to be multiple of 4 bytes.
		 *
		 * @throw MediaSoupError - If serialization fails due to invalid content
		 *   previously added.
		 *
		 * @privateRemarks
		 * - Child classes invoke Serializable::Serialized()` at the end of their
		 *   `Serialize()` implementation.
		 */
		virtual void Serialize(uint8_t* buffer, bool padTo4Bytes) = 0;

		/**
		 * Methods to be used by classes inheriting from Serializable.
		 */
	protected:
		/**
		 * Initialize content size (padding bytes excluded).
		 *
		 * @remarks
		 * - This method must be called when constructing the serializable.
		 *
		 * @throw MediaSoupError - If called twice.
		 */
		void SetInitialContentSize(size_t contentSize);

		/**
		 * Initialize padding.
		 *
		 * @remarks
		 * - This method must be called when constructing the serializable.
		 *
		 * @throw MediaSoupError - If called twice.
		 */
		void SetInitialPadding(uint8_t padding);

		/**
		 * Current buffer of the serializable, despite serialization is needed.
		 */
		const uint8_t* GetCurrentBuffer() const;

		/**
		 * Current content size (padding bytes excluded), despite serialization is
		 * needed.
		 */
		size_t GetCurrentContentSize() const;

		/**
		 * Current padding, despite serialization is needed.
		 */
		uint8_t GetCurrentPadding() const;

		/**
		 * To be called by child classes from public methods that affect the packet
		 * or item content in a way that serialization is needed.
		 */
		void SetSerializationNeeded();

		/**
		 * To be called by child classes at the end of their `Serialize()` method.
		 *
		 * @param buffer - Buffer in which the content has been serialized.
		 * @param contentSize - New content size (padding excluded).
		 * @param padding - New padding size.
		 *
		 * @remarks
		 * - This method fills padding bytes (if any) with zeroes.
		 */
		void Serialized(uint8_t* buffer, size_t contentSize, uint8_t padding);

	private:
		// Buffer holding the serializable content.
		uint8_t* buffer{ nullptr };
		// Serializable content size (padding excluded).
		uint8_t contentSize{ 0u };
		// Number of bytes of padding.
		uint8_t padding{ 0u };

	private:
		// Whether serialization is needed due to recent modifications.
		bool serializationNeeded{ false };
	};
} // namespace RTC

#endif
