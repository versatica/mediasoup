#ifndef MS_SIGNALS_HANDLER_H
#define	MS_SIGNALS_HANDLER_H

#include <vector>
#include <string>
#include <uv.h>

class SignalsHandler
{
public:
	class Listener
	{
	public:
		virtual void onSignal(SignalsHandler* signalsHandler, int signum) = 0;
		virtual void onSignalsHandlerClosed(SignalsHandler* signalsHandler) = 0;
	};

public:
	SignalsHandler(Listener* listener);

	void AddSignal(int signum, std::string name);
	void Close();

/* Callbacks fired by UV events. */
public:
	void onUvSignal(int signum);

private:
	// Allocated by this:
	typedef std::vector<uv_signal_t*> UvHandles;
	UvHandles uvHandles;
	// Passed by argument:
	Listener* listener = nullptr;
};

#endif
