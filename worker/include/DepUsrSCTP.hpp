#ifndef MS_DEP_USRSCTP_HPP
#define MS_DEP_USRSCTP_HPP

#include "common.hpp"
#include "RTC/SctpAssociation.hpp"
#include "handles/TimerHandle.hpp"
#include <absl/container/flat_hash_map.h>
#include <vector>

class DepUsrSCTP
{
private:
	/* Struct for storing a pending SCTP message to be sent. */
	struct SendSctpDataItem
	{
		// NOTE: We keep this struct simple, without explicit allocation
		// or deallocation in constructor/destructor, and instead rely on
		// the destructor of the main container SendSctpDataStore.

		SendSctpDataItem() noexcept;

		// SendSctpDataItem(uint8_t* data, size_t len) : data(new uint8_t[len]), len(len)
		// {
		// 	std::memcpy(this->data, data, len);
		// }
		// SendSctpDataItem(uint8_t* data, size_t len);

		// Disable copy constructor because of the dynamically allocated data.
		// SendSctpDataItem(const SendSctpDataItem&) = delete;

		// ~SendSctpDataItem()
		// {
		// 	delete[] this->data;
		// }
		~SendSctpDataItem();

		uint8_t* data{ nullptr };
		size_t len{ 0u };

		int fooId{ 0 };
	};

public:
	/* Struct for storing pending datas to be sent. */
	struct SendSctpDataStore
	{
		explicit SendSctpDataStore(RTC::SctpAssociation* sctpAssociation);

		// Disable copy constructor.
		// SendSctpDataStore(const SendSctpDataStore&) = delete;

		// ~SendSctpDataStore()
		// {
		// 	this->items.clear();
		// }
		~SendSctpDataStore();

		void ClearItems();

		RTC::SctpAssociation* sctpAssociation;

		int fooId{ 0 };

		std::vector<SendSctpDataItem> items;
	};

private:
	class Checker : public TimerHandle::Listener
	{
	public:
		Checker();
		~Checker() override;

	public:
		void Start();
		void Stop();

		/* Pure virtual methods inherited from TimerHandle::Listener. */
	public:
		void OnTimer(TimerHandle* timer) override;

	private:
		TimerHandle* timer{ nullptr };
		uint64_t lastCalledAtMs{ 0u };
	};

public:
	static void ClassInit();
	static void ClassDestroy();
	static void CreateChecker();
	static void CloseChecker();
	static uintptr_t GetNextSctpAssociationId();
	static void RegisterSctpAssociation(RTC::SctpAssociation* sctpAssociation);
	static void DeregisterSctpAssociation(RTC::SctpAssociation* sctpAssociation);
	static RTC::SctpAssociation* RetrieveSctpAssociation(uintptr_t id);
	static void SendSctpData(RTC::SctpAssociation* sctpAssociation, uint8_t* data, size_t len);
	static SendSctpDataStore* GetSendSctpDataStore(uv_async_t* handle);

private:
	thread_local static Checker* checker;
	static uint64_t numSctpAssociations;
	static uintptr_t nextSctpAssociationId;
	static absl::flat_hash_map<uintptr_t, RTC::SctpAssociation*> mapIdSctpAssociation;
	// Map of SendSctpDataStores indexed by uv_async_t*.
	static absl::flat_hash_map<const uv_async_t*, SendSctpDataStore> mapAsyncHandlerSendSctpData;
};

#endif
