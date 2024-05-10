#include "DepLibUV.hpp"
#include "common.hpp"   // mediasoup header file.
#include "lib.hpp"      // mediasoup header file.
#include <deque>
#include <mutex>
#include <napi.h>
#include <thread>

class WorkerChannel : public Napi::ObjectWrap<WorkerChannel>
{
public:
	static Napi::Object Init(Napi::Env env, Napi::Object exports);
	WorkerChannel(const Napi::CallbackInfo& info);
	~WorkerChannel();

	// Called when the wrapped native instance is freed.
	void Finalize(Napi::Env env);
	ChannelReadFreeFn OnChannelRead(uint8_t** message, uint32_t* messageLen, const uv_async_t* handle);
	void OnChannelWrite(const uint8_t* message, uint32_t messageLen);
	void OnError(const uint8_t code);

private:
	static Napi::FunctionReference constructor;
	std::thread thread;
	Napi::ThreadSafeFunction emit;

	const uv_async_t* handle;
	std::mutex mutex;
	std::deque<uint8_t*> messages;

	void Send(const Napi::CallbackInfo& info);
};
