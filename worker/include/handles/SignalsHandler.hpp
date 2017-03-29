#ifndef MS_SIGNALS_HANDLER_HPP
#define	MS_SIGNALS_HANDLER_HPP

#include <vector>
#include <string>
#include <uv.h>

class SignalsHandler
{
public:
	class Listener
	{
	public:
		virtual ~Listener() {};

	public:
		virtual void onSignal(SignalsHandler* signalsHandler, int signum) = 0;
	};

public:
	explicit SignalsHandler(Listener* listener);

private:
	~SignalsHandler() {};

public:
	void Destroy();
	void AddSignal(int signum, std::string name);

/* Callbacks fired by UV events. */
public:
	void onUvSignal(int signum);

private:
	// Passed by argument.
	Listener* listener = nullptr;
	// Allocated by this.
	std::vector<uv_signal_t*> uvHandles;
};

#endif
