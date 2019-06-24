#ifndef MS_RTC_SCTP_ASSOCIATION_HPP
#define MS_RTC_SCTP_ASSOCIATION_HPP

#include "common.hpp"
#include "json.hpp"
#include "handles/Timer.hpp"

using json = nlohmann::json;

namespace RTC
{
	class SctpAssociation
	{
	public:
		class Listener
		{
		};

	public:
		SctpAssociation(Listener* listener, uint32_t sctpMaxMessageSize);
		virtual ~SctpAssociation();

	public:
		virtual void FillJson(json& jsonObject) const;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		uint32_t sctpMaxMessageSize{ 0 };
	};
} // namespace RTC

#endif
