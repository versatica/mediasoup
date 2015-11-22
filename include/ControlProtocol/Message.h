#ifndef MS_CONTROL_PROTOCOL_MESSAGE_H
#define MS_CONTROL_PROTOCOL_MESSAGE_H

#include "common.h"

namespace ControlProtocol
{
	class Message
	{
	public:
		enum class Kind
		{
			Request = 1,
			Response,
			Notification
		};

	public:
		Message(Kind);
		virtual ~Message();

		Kind GetKind();
		virtual bool IsValid();  // TODO: Must be pure virtual.
		void SetTransaction(int32_t transaction);
		int32_t GetTransaction();
		virtual void Dump() = 0;

	protected:
		Kind kind;
		int32_t transaction = 0;
	};

	/* Inlime methods. */

	inline
	Message::Kind Message::GetKind()
	{
		return this->kind;
	}

	inline
	void Message::SetTransaction(int32_t transaction)
	{
		this->transaction = transaction;
	}

	inline
	int32_t Message::GetTransaction()
	{
		return this->transaction;
	}
}  // namespace ControlProtocol

#endif
