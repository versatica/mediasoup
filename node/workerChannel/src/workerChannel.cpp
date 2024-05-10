#include "../include/workerChannel.hpp"
#include "napi.h"

#include <cstring>
#include <iostream>

void deleteMessage(uint8_t* message, uint32_t messageLen, size_t ctx)
{
	delete[] message;
}

ChannelReadFreeFn channelReadFn(
  uint8_t** message,
  uint32_t* messageLen,
  size_t* messageCtx,
  const void* handle,
  ChannelReadCtx channelReadCtx)
{
	auto workerChannel         = static_cast<WorkerChannel*>(channelReadCtx);
	const uv_async_t* uvAsyncT = static_cast<const uv_async_t*>(handle);

	return workerChannel->OnChannelRead(message, messageLen, uvAsyncT);
}

void channelWriteFn(const uint8_t* message, uint32_t messageLen, ChannelWriteCtx channelWriteCtx)
{
	auto workerChannel = static_cast<WorkerChannel*>(channelWriteCtx);

	return workerChannel->OnChannelWrite(message, messageLen);
}

void libmediasoup(WorkerChannel* workerChannel, std::string version, std::vector<std::string> args)
{
	std::vector<char*> argv;

	for (auto& arg : args)
	{
		argv.push_back(arg.data());
	}

	auto result = mediasoup_worker_run(
	  argv.size(),
	  argv.data(),
	  version.data(),
	  0,
	  0,
	  channelReadFn,
	  workerChannel,
	  channelWriteFn,
	  workerChannel);

	if (result != 0)
	{
		workerChannel->OnError(result);
	}
}

Napi::FunctionReference WorkerChannel::constructor;

Napi::Object WorkerChannel::Init(Napi::Env env, Napi::Object exports)
{
	Napi::Function func =
	  DefineClass(env, "WorkerChannel", { InstanceMethod("send", &WorkerChannel::Send) });

	Napi::FunctionReference* constructor = new Napi::FunctionReference();

	// Create a persistent reference to the class constructor. This will allow
	// a function called on a class prototype and a function
	// called on instance of a class to be distinguished from each other.
	*constructor = Napi::Persistent(func);

	exports.Set("WorkerChannel", func);

	// Store the constructor as the add-on instance data. This will allow this
	// add-on to support multiple instances of itself running on multiple worker
	// threads, as well as multiple instances of itself running in different
	// contexts on the same thread.
	//
	// By default, the value set on the environment here will be destroyed when
	// the add-on is unloaded using the `delete` operator, but it is also
	// possible to supply a custom deleter.
	env.SetInstanceData<Napi::FunctionReference>(constructor);

	return exports;
}

WorkerChannel::WorkerChannel(const Napi::CallbackInfo& info) : Napi::ObjectWrap<WorkerChannel>(info)
{
	auto env     = info.Env();
	auto cb      = info[0].As<Napi::Function>();
	auto version = info[1].As<Napi::String>();
	auto params  = info[2].As<Napi::Array>();

	this->emit = Napi::ThreadSafeFunction::New(env, cb, "WorkerChannel", 0, 1);

	std::vector<std::string> args = { "" };

	for (uint32_t i = 0; i < params.Length(); i++)
	{
		Napi::Value v = params[i];
		if (!v.IsString())
		{
			continue;
		}

		auto value = v.As<Napi::String>();

		args.push_back(value.Utf8Value());
	}

	this->thread = std::thread(libmediasoup, this, version.Utf8Value(), args);
}

WorkerChannel::~WorkerChannel()
{
	std::cout << "~WorkerChannel()" << std::endl;
}

void WorkerChannel::Finalize(Napi::Env env)
{
	std::cout << "Finalize()" << std::endl;

	this->emit.Release();
	// this->thread.join();
}

ChannelReadFreeFn WorkerChannel::OnChannelRead(
  uint8_t** message, uint32_t* messageLen, const uv_async_t* handle)
{
	if (!this->handle)
	{
		this->handle = handle;
	}

	if (this->messages.size() == 0)
	{
		return nullptr;
	}

	this->mutex.lock();
	auto* msg = this->messages.front();
	this->messages.pop_front();
	this->mutex.unlock();

	*message = msg;

	return deleteMessage;
}

void WorkerChannel::OnChannelWrite(const uint8_t* message, uint32_t messageLen)
{
	auto copy = new uint8_t[messageLen];

	std::memcpy(copy, message, messageLen);

	auto callback = [copy, messageLen](Napi::Env env, Napi::Function cb)
	{
		auto data = Napi::Buffer<uint8_t>::New(
		  env,
		  const_cast<uint8_t*>(copy),
		  messageLen,
		  [](Napi::Env env, void* data) { delete[] (Napi::Buffer<uint8_t>*)data; });

		cb.Call({ Napi::String::New(env, "data"), data });
	};

	this->emit.NonBlockingCall(callback);
}

void WorkerChannel::OnError(const uint8_t code)
{
	auto callback = [code](Napi::Env env, Napi::Function cb)
	{
		auto value = Napi::Number::New(env, code);

		cb.Call({ Napi::String::New(env, "error"), value });
	};

	this->emit.NonBlockingCall(callback);
}

void WorkerChannel::Send(const Napi::CallbackInfo& info)
{
	// Copy the message into its own memory.
	auto message = info[0].As<Napi::Uint8Array>();
	auto data    = new uint8_t[message.ByteLength()];

	std::memcpy(data, message.Data(), message.ByteLength());

	// Store the message.
	this->mutex.lock();
	this->messages.push_back(data);
	this->mutex.unlock();

	// Notify mediasoup about the new message.
	uv_async_send(const_cast<uv_async_t*>(this->handle));
}
