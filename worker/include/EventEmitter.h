//
// Created by Eugene Voityuk on 14.12.2022.
//

#ifndef MEDIASOUP_EVENTEMITTER_H
#define MEDIASOUP_EVENTEMITTER_H
#include <cstdlib>
#include <functional>
#include <iostream>
#include <type_traits>
#include <vector>

class BaseEvent {
public:
	~BaseEvent() = default;
	static size_t getNextType()
	{
		static size_t type_count = 0;
		return type_count++;
	}
};


template <typename EventType>
struct Event : BaseEvent
{
	static size_t type()
	{
		static size_t t_type = BaseEvent::getNextType();
		return t_type;
	}
	Event(const EventType& event) : event(event) {}
	const EventType& event;
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
		CallbackWrapper(call_type<EventType> callable) : m_callable(callable) {}

		void operator() (const BaseEvent& event) {
			// The event handling code requires a small change too.
			// A reference to the EventType object is stored
			// in Event. You get the EventType reference from the
			// Event and make the final call.
			m_callable(static_cast<const Event<EventType>&>(event).event);
		}

		call_type<EventType> m_callable;
	};
public:
	void removeAllListeners() {
		m_subscribers.clear();
	}
private:
	std::vector<std::vector<call_type<BaseEvent>>> m_subscribers;
};
#endif // MEDIASOUP_EVENTEMITTER_H
