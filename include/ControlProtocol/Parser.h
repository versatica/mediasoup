#ifndef MS_CONTROL_PROTOCOL_PARSER_H
#define	MS_CONTROL_PROTOCOL_PARSER_H

#include "common.h"
#include "ControlProtocol/Message.h"

namespace ControlProtocol
{
	class Parser
	{
	public:
		Parser();
		virtual ~Parser();

		/**
		 * Parses the given buffer.
		 *
		 * @param  buffer The buffer to parse.
		 * @param  len    The lenght of the buffer.
		 *
		 * @return        Returns a pointer to ControlProtocol::Message if parsing succeded,
		 *                or nullptr if the message is not completed or parsing failed.
		 */
		Message* Parse(const MS_BYTE* buffer, size_t len);

		/**
		 * Checks whether the ongoing parsing has error or not.
		 *
		 * @return  true if there is parsing error, false otherwise.
		 */
		bool HasError();

		/**
		 * Get the length of the complete or uncomplete message parsed or being parsed.
		 *
		 * @return  The number of chars parsed in the current message.
		 */
		size_t GetParsedLen();

		/**
		 * Resets the parser internals. Must be called before parsing a new message
		 * (regardless the previous one succeded or not).
		 */
		void Reset();

		/**
		 * Prints current status of the parser.
		 */
		void Dump();

	protected:
		size_t parsedLen;

	private:
		// Allocated by this:
		ControlProtocol::Message* msg = nullptr;
		// Others:
		size_t cs = 0;  // Needed for Ragel.
		size_t mark = 0;
	};
}  // namespace ControlProtocol

#endif
