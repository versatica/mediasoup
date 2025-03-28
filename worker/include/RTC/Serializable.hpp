#ifndef MS_RTC_SERIALIZABLE_HPP
#define MS_RTC_SERIALIZABLE_HPP

#include "common.hpp"

namespace RTC
{
	/**
	 * Class holding serializable content.
	 *
	 * @remarks
	 * - ICE, RTP, RTCP, SCTP packets and some components in those packets
	 *   inherit from this class.
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
		virtual const uint8_t* GetBuffer() const final;

		/**
		 * Total size of the serializable if it was serialized now.
		 *
		 * @remarks
		 * - Child classes implementing this method should check
		 *   `NeedsSerialization()` and, if not needed, just return
		 *   `GetCurrentSize()`.
		 */
		virtual size_t GetSize() const = 0;

		/**
		 * Whether serialization is needed, meaning that the current `buffer`
		 * doesn't represent the current content of the serializable (due to
		 * modifications not applied yet).
		 */
		virtual bool NeedsSerialization() const final;

		/**
		 * Apply pending changes and serialize the content of the serializable
		 * into a new `buffer`.
		 *
		 * @param buffer - Buffer in which the content will be serialized.
		 *
		 * @throw MediaSoupError - If serialization fails due to invalid
		 *   modifications previously made.
		 *
		 * @remarks
		 * - Child classes invoke `Serialized()` at the end of their `Serialize()`
		 *   implementation.
		 */
		virtual void Serialize(uint8_t* buffer) = 0;

		/**
		 * Methods to be used by classes inheriting from Serializable.
		 */
	protected:
		/**
		 * Initialize serializable size.
		 *
		 * @remarks
		 * - This method must be called when constructing the serializable.
		 *
		 * @throw MediaSoupError - If called twice.
		 *
		 * @remarks
		 * - This method internally sets the `serializationNeeded` flag to false.
		 */
		virtual void SetInitialSize(size_t size) final;

		/**
		 * Current buffer of the serializable, no matter serialization is needed.
		 */
		virtual const uint8_t* GetCurrentBuffer() const final;

		/**
		 * Current serializable size, no matter despite serialization is needed.
		 */
		virtual size_t GetCurrentSize() const final;

		/**
		 * To be called from methods of child classes that affect the serializable
		 * content in a way that serialization is needed.
		 *
		 * @remarks
		 * - Child classes should typicall call this method with `flag` true. They
		 *   may call it with `flag` false only after the finish parsing and
		 *   constructing a class instance.
		 */
		virtual void SetSerializationNeeded(bool flag) final;

		/**
		 * To be called by child classes at the end of their `Serialize()` method.
		 *
		 * @param buffer - Buffer in which the content has been serialized.
		 * @param size - New size of the serializable.
		 *
		 * @remarks
		 * - This method internally sets the `serializationNeeded` flag to false.
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
