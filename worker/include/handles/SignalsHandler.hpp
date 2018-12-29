#ifndef MS_SIGNALS_HANDLER_HPP
#define MS_SIGNALS_HANDLER_HPP

#include <uv.h>
#include <string>
#include <vector>

class SignalsHandler
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnSignal(SignalsHandler* signalsHandler, int signum) = 0;
	};

public:
	explicit SignalsHandler(Listener* listener);
	~SignalsHandler();

public:
	void Close();
	void AddSignal(int signum, const std::string& name);

	/* Callbacks fired by UV events. */
public:
	void OnUvSignal(int signum);

private:
	// Passed by argument.
	Listener* listener{ nullptr };
	// Allocated by this.
	std::vector<uv_signal_t*> uvHandles;
	// Others.
	bool closed{ false };
};

#endif
