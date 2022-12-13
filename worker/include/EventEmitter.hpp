//
// Created by Eugene Voityuk on 13.12.2022.
//
#ifndef MS_DEP_EVENT_EMITTER_HPP
#define MS_DEP_EVENT_EMITTER_HPP

#include <iostream>
#include <cstdlib>
#include <functional>
#include <vector>

struct BaseEvent
{
	virtual ~BaseEvent() = default;
protected:
	static size_t getNextType();
};

size_t BaseEvent::getNextType()
{
	static size_t type_count = 0;
	return type_count++;
}


template <typename EventType>
struct Event : BaseEvent
{
	static size_t type()
	{
		static size_t t_type = BaseEvent::getNextType();
		return t_type;
	}
	explicit Event(const EventType& event) : event_(event) {}
	const EventType& event_;
};

class EventEmitter
{
private:
	template <typename EventType>
	using call_type = std::function<void(const EventType&)>;

public:
	template <typename EventType>
	void subscribe(const call_type<EventType> callable)
	{
		// When events such as COLLISION don't derive
		// from Event, you have to get the type by
		// using one more level of indirection.
		size_t type = Event<EventType>::type();
		if (type >= m_subscribers.size())
			m_subscribers.resize(type+1);
		m_subscribers[type].push_back(CallbackWrapper<EventType>(callable));
	}

public:
	template <typename EventType>
	void emit(const EventType& event)
	{
		size_t type = Event<EventType>::type();
		if (m_subscribers[type].empty())
			return;
		if (type >= m_subscribers.size())
			m_subscribers.resize(type+1);

		Event<EventType> wrappedEvent(event);
		for (auto& receiver : m_subscribers[type])
			receiver(wrappedEvent);
	}
private:
	template <typename EventType>
	struct CallbackWrapper
	{
		explicit CallbackWrapper(call_type<EventType> callable) : m_callable(callable) {}

		void operator() (const BaseEvent& event) {
			// The event handling code requires a small change too.
			// A reference to the EventType object is stored
			// in Event. You get the EventType reference from the
			// Event and make the final call.
			m_callable(static_cast<const Event<EventType>&>(event).event_);
		}

		call_type<EventType> m_callable;
	};
public:
	void removeAllListeners() {
		m_subscribers.clear();
	}
private:
	std::vector<std::vector<call_type<BaseEvent> > > m_subscribers;
};
#endif // MS_EVENTEMITTER_HPP
