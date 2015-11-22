#define MS_CLASS "SignalsHandler"

#include "handles/SignalsHandler.h"
#include "LibUV.h"
#include "MediaSoupError.h"
#include "Logger.h"

/* Static methods for UV callbacks. */

static inline
void on_signal(uv_signal_t* handle, int signum)
{
	static_cast<SignalsHandler*>(handle->data)->onUvSignal(signum);
}

static inline
void on_close(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

SignalsHandler::SignalsHandler(Listener* listener) :
	listener(listener)
{
	MS_TRACE();
}

void SignalsHandler::AddSignal(int signum, std::string name)
{
	MS_TRACE();

	int err;

	uv_signal_t* uvHandle = new uv_signal_t;
	uvHandle->data = (void*)this;

	err = uv_signal_init(LibUV::GetLoop(), uvHandle);
	if (err)
	{
		delete uvHandle;
		MS_THROW_ERROR("uv_signal_init() failed for signal %s: %s", name.c_str(), uv_strerror(err));
	}

	err = uv_signal_start(uvHandle, (uv_signal_cb)on_signal, signum);
	if (err)
		MS_THROW_ERROR("uv_signal_start() failed for signal %s: %s", name.c_str(), uv_strerror(err));

	// Enter the UV handle into the vector.
	this->uvHandles.push_back(uvHandle);

	MS_DEBUG("signal %s added", name.c_str());
}

void SignalsHandler::Close()
{
	MS_TRACE();

	for (auto uvHandle : uvHandles)
	{
		uv_close((uv_handle_t*)uvHandle, (uv_close_cb)on_close);
	}

	// Notify the listener.
	this->listener->onSignalsHandlerClosed(this);

	// And delete this.
	delete this;
}

inline
void SignalsHandler::onUvSignal(int signum)
{
	MS_TRACE();

	// Notify the listener.
	this->listener->onSignal(this, signum);
}
