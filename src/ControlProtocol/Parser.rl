#define MS_CLASS "ControlProtocol::Parser"

#include "ControlProtocol/Parser.h"
#include "ControlProtocol/messages.h"
#include "Logger.h"
#include <cstdlib>  // std::atol()


#define MARK(M, P) (M = (P) - buffer)
#define LEN(AT, P) ((P) - buffer - (AT))
#define LEN_FROM_MARK (p - buffer - this->mark)
#define PTR_TO(F) (buffer + (F))
#define PTR_TO_MARK (const char*)(buffer + this->mark)


namespace ControlProtocol {


/**
 * Ragel: machine definition
 */
%%{
	machine MessageParser;

	alphtype unsigned char;

	action mark {
		MARK(this->mark, p);
	}

	action on_transaction {
		int32_t transaction = std::atol(PTR_TO_MARK);
		this->msg->SetTransaction(transaction);
	}

	action on_message {
		fbreak;
	}

	include core_grammar "../core_grammar.rl";

	transaction             = ULONG >mark %on_transaction;

	include RequestHello "grammar/RequestHello.rl";
	include RequestAuthenticate "grammar/RequestAuthenticate.rl";
	include RequestCreateConference "grammar/RequestCreateConference.rl";

	Request                 = RequestHello |
	                          RequestAuthenticate |
	                          RequestCreateConference;

	#Message                 = Request | Response | Notification;
	Message                 = Request @on_message;

	main := Message;
}%%


/**
 * Ragel: %%write data
 * This generates Ragel's static variables such as:
 *   static const int MessageParser_start
 */
%%write data;


Parser::Parser() {
	MS_TRACE();

	Reset();
}


Parser::~Parser() {
	MS_TRACE();
}


Message* Parser::Parse(const MS_BYTE* buffer, size_t len) {
	MS_TRACE();

	// Used by Ragel:
	const unsigned char* p;
	const unsigned char* pe;
	// const unsigned char* eof;

	MS_ASSERT(this->parsedLen <= len, "parsedLen pasts end of buffer");

	p = (const unsigned char*)buffer + this->parsedLen;
	pe = (const unsigned char*)buffer + len;
	// eof = nullptr;  // Ragel manual pag 36 (eof variable).

	/**
	 * Ragel: %%write exec
	 * This updates cs variable needed by Ragel (so this->cs at the end) and p.
	 */
	%%write exec;

	this->parsedLen = p - (const unsigned char*)buffer;

	MS_ASSERT(p <= pe, "buffer overflow after parsing execute");
	MS_ASSERT(this->parsedLen <= len, "parsedLen longer than length");
	MS_ASSERT(this->mark < len, "mark is after buffer end");

	// Parsing succedded.
	if (this->cs == (size_t)MessageParser_first_final) {
		MS_DEBUG("parsing finished OK");

		MS_ASSERT(this->msg != nullptr, "parsing OK but msg is NULL");

		// Check the message semantics.
		if(! this->msg->IsValid()) {
			MS_ERROR("invalid message");
			delete this->msg;
			return (this->msg = nullptr);
		}

		return this->msg;
	}

	// Parsing error.
	else if (this->cs == (size_t)MessageParser_error) {
		MS_ERROR("parsing error at position %zd", this->parsedLen);

		// Delete this->msg if it exists.
		if (this->msg) {
			delete this->msg;
			this->msg = nullptr;
		}

		return nullptr;
	}

	// Parsing not finished.
	else {
		MS_DEBUG("parsing not finished");

		return nullptr;
	}
}


bool Parser::HasError() {
	MS_TRACE();

	return (this->cs == (size_t)MessageParser_error);
}


size_t Parser::GetParsedLen() {
	MS_TRACE();

	return this->parsedLen;
}


void Parser::Reset() {
	MS_TRACE();

	this->parsedLen = 0;
	this->msg = nullptr;
	this->cs = 0;
	this->mark = 0;

	/**
	 * Ragel: %%write init
	 * This sets cs variable needed by Ragel (so this->cs at the end).
	 */
	%%write init;
}


void Parser::Dump() {
	MS_DEBUG("[cs: %zu | parsedLen: %zu | mark: %zu | error?: %s | finished?: %s]",
		this->cs, this->parsedLen, this->mark,
		HasError() ? "true" : "false",
		(this->cs >= (size_t)MessageParser_first_final) ? "true" : "false");
}


}  // namespace ControlProtocol
