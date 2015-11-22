
#line 1 "src/ControlProtocol/Parser.rl"
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

namespace ControlProtocol
{
	/**
	 * Ragel: machine definition
	 */
	
#line 57 "src/ControlProtocol/Parser.rl"


	/**
	 * Ragel: %%write data
	 * This generates Ragel's static variables such as:
	 *   static const int MessageParser_start
	 */
	
#line 33 "src/ControlProtocol/Parser.cpp"
static const int MessageParser_start = 1;
static const int MessageParser_first_final = 90;
static const int MessageParser_error = 0;




#line 65 "src/ControlProtocol/Parser.rl"

	Parser::Parser()
	{
		MS_TRACE();

		Reset();
	}

	Parser::~Parser()
	{
		MS_TRACE();
	}

	Message* Parser::Parse(const MS_BYTE* buffer, size_t len)
	{
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
		
#line 75 "src/ControlProtocol/Parser.cpp"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	if ( (*p) == 82u )
		goto st2;
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	if ( (*p) == 101u )
		goto st3;
	goto st0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	if ( (*p) == 113u )
		goto st4;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	if ( (*p) == 117u )
		goto st5;
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( (*p) == 101u )
		goto st6;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) == 115u )
		goto st7;
	goto st0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 116u )
		goto st8;
	goto st0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	if ( (*p) == 32u )
		goto st9;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	switch( (*p) ) {
		case 65u: goto st10;
		case 67u: goto st56;
		case 72u: goto st83;
	}
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	if ( (*p) == 117u )
		goto st11;
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( (*p) == 116u )
		goto st12;
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( (*p) == 104u )
		goto st13;
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( (*p) == 101u )
		goto st14;
	goto st0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	if ( (*p) == 110u )
		goto st15;
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( (*p) == 116u )
		goto st16;
	goto st0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	if ( (*p) == 105u )
		goto st17;
	goto st0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	if ( (*p) == 99u )
		goto st18;
	goto st0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	if ( (*p) == 97u )
		goto st19;
	goto st0;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	if ( (*p) == 116u )
		goto st20;
	goto st0;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
	if ( (*p) == 101u )
		goto st21;
	goto st0;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
	if ( (*p) == 13u )
		goto st22;
	goto st0;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
	if ( (*p) == 10u )
		goto st23;
	goto st0;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr25;
	goto st0;
tr25:
#line 5 "src/ControlProtocol/grammar/RequestAuthenticate.rl"
	{
		this->msg = new ControlProtocol::RequestAuthenticate();
	}
#line 26 "src/ControlProtocol/Parser.rl"
	{
			MARK(this->mark, p);
		}
	goto st24;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
#line 259 "src/ControlProtocol/Parser.cpp"
	if ( (*p) == 13u )
		goto tr26;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st48;
	goto st0;
tr26:
#line 31 "src/ControlProtocol/Parser.rl"
	{
			int32_t transaction = std::atol(PTR_TO_MARK);
			this->msg->SetTransaction(transaction);
		}
	goto st25;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
#line 276 "src/ControlProtocol/Parser.cpp"
	if ( (*p) == 10u )
		goto st26;
	goto st0;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	if ( (*p) == 117u )
		goto st27;
	goto st0;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
	if ( (*p) == 115u )
		goto st28;
	goto st0;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
	if ( (*p) == 101u )
		goto st29;
	goto st0;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
	if ( (*p) == 114u )
		goto st30;
	goto st0;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
	if ( (*p) == 58u )
		goto st31;
	goto st0;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
	if ( (*p) == 32u )
		goto st32;
	goto st0;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
	if ( (*p) == 33u )
		goto tr35;
	if ( (*p) < 45u ) {
		if ( (*p) > 39u ) {
			if ( 42u <= (*p) && (*p) <= 43u )
				goto tr35;
		} else if ( (*p) >= 35u )
			goto tr35;
	} else if ( (*p) > 46u ) {
		if ( (*p) < 65u ) {
			if ( 48u <= (*p) && (*p) <= 57u )
				goto tr35;
		} else if ( (*p) > 90u ) {
			if ( 94u <= (*p) && (*p) <= 126u )
				goto tr35;
		} else
			goto tr35;
	} else
		goto tr35;
	goto st0;
tr35:
#line 26 "src/ControlProtocol/Parser.rl"
	{
			MARK(this->mark, p);
		}
	goto st33;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
#line 356 "src/ControlProtocol/Parser.cpp"
	switch( (*p) ) {
		case 13u: goto tr36;
		case 33u: goto st33;
	}
	if ( (*p) < 45u ) {
		if ( (*p) > 39u ) {
			if ( 42u <= (*p) && (*p) <= 43u )
				goto st33;
		} else if ( (*p) >= 35u )
			goto st33;
	} else if ( (*p) > 46u ) {
		if ( (*p) < 65u ) {
			if ( 48u <= (*p) && (*p) <= 57u )
				goto st33;
		} else if ( (*p) > 90u ) {
			if ( 94u <= (*p) && (*p) <= 126u )
				goto st33;
		} else
			goto st33;
	} else
		goto st33;
	goto st0;
tr36:
#line 10 "src/ControlProtocol/grammar/RequestAuthenticate.rl"
	{
		static_cast<RequestAuthenticate*>(this->msg)->SetUser(PTR_TO_MARK, LEN_FROM_MARK);
	}
	goto st34;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
#line 389 "src/ControlProtocol/Parser.cpp"
	if ( (*p) == 10u )
		goto st35;
	goto st0;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
	if ( (*p) == 112u )
		goto st36;
	goto st0;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
	if ( (*p) == 97u )
		goto st37;
	goto st0;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
	if ( (*p) == 115u )
		goto st38;
	goto st0;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
	if ( (*p) == 115u )
		goto st39;
	goto st0;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
	if ( (*p) == 119u )
		goto st40;
	goto st0;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
	if ( (*p) == 100u )
		goto st41;
	goto st0;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
	if ( (*p) == 58u )
		goto st42;
	goto st0;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
	if ( (*p) == 32u )
		goto st43;
	goto st0;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
	if ( (*p) == 33u )
		goto tr47;
	if ( (*p) < 45u ) {
		if ( (*p) > 39u ) {
			if ( 42u <= (*p) && (*p) <= 43u )
				goto tr47;
		} else if ( (*p) >= 35u )
			goto tr47;
	} else if ( (*p) > 46u ) {
		if ( (*p) < 65u ) {
			if ( 48u <= (*p) && (*p) <= 57u )
				goto tr47;
		} else if ( (*p) > 90u ) {
			if ( 94u <= (*p) && (*p) <= 126u )
				goto tr47;
		} else
			goto tr47;
	} else
		goto tr47;
	goto st0;
tr47:
#line 26 "src/ControlProtocol/Parser.rl"
	{
			MARK(this->mark, p);
		}
	goto st44;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
#line 483 "src/ControlProtocol/Parser.cpp"
	switch( (*p) ) {
		case 13u: goto tr48;
		case 33u: goto st44;
	}
	if ( (*p) < 45u ) {
		if ( (*p) > 39u ) {
			if ( 42u <= (*p) && (*p) <= 43u )
				goto st44;
		} else if ( (*p) >= 35u )
			goto st44;
	} else if ( (*p) > 46u ) {
		if ( (*p) < 65u ) {
			if ( 48u <= (*p) && (*p) <= 57u )
				goto st44;
		} else if ( (*p) > 90u ) {
			if ( 94u <= (*p) && (*p) <= 126u )
				goto st44;
		} else
			goto st44;
	} else
		goto st44;
	goto st0;
tr78:
#line 31 "src/ControlProtocol/Parser.rl"
	{
			int32_t transaction = std::atol(PTR_TO_MARK);
			this->msg->SetTransaction(transaction);
		}
	goto st45;
tr48:
#line 15 "src/ControlProtocol/grammar/RequestAuthenticate.rl"
	{
		static_cast<RequestAuthenticate*>(this->msg)->SetPasswd(PTR_TO_MARK, LEN_FROM_MARK);
	}
	goto st45;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
#line 523 "src/ControlProtocol/Parser.cpp"
	if ( (*p) == 10u )
		goto st46;
	goto st0;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
	if ( (*p) == 13u )
		goto st47;
	goto st0;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
	if ( (*p) == 10u )
		goto tr52;
	goto st0;
tr52:
#line 37 "src/ControlProtocol/Parser.rl"
	{
			{p++; cs = 90; goto _out;}
		}
	goto st90;
st90:
	if ( ++p == pe )
		goto _test_eof90;
case 90:
#line 551 "src/ControlProtocol/Parser.cpp"
	goto st0;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
	if ( (*p) == 13u )
		goto tr26;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st49;
	goto st0;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
	if ( (*p) == 13u )
		goto tr26;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st50;
	goto st0;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
	if ( (*p) == 13u )
		goto tr26;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st51;
	goto st0;
st51:
	if ( ++p == pe )
		goto _test_eof51;
case 51:
	if ( (*p) == 13u )
		goto tr26;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st52;
	goto st0;
st52:
	if ( ++p == pe )
		goto _test_eof52;
case 52:
	if ( (*p) == 13u )
		goto tr26;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st53;
	goto st0;
st53:
	if ( ++p == pe )
		goto _test_eof53;
case 53:
	if ( (*p) == 13u )
		goto tr26;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st54;
	goto st0;
st54:
	if ( ++p == pe )
		goto _test_eof54;
case 54:
	if ( (*p) == 13u )
		goto tr26;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st55;
	goto st0;
st55:
	if ( ++p == pe )
		goto _test_eof55;
case 55:
	if ( (*p) == 13u )
		goto tr26;
	goto st0;
st56:
	if ( ++p == pe )
		goto _test_eof56;
case 56:
	if ( (*p) == 114u )
		goto st57;
	goto st0;
st57:
	if ( ++p == pe )
		goto _test_eof57;
case 57:
	if ( (*p) == 101u )
		goto st58;
	goto st0;
st58:
	if ( ++p == pe )
		goto _test_eof58;
case 58:
	if ( (*p) == 97u )
		goto st59;
	goto st0;
st59:
	if ( ++p == pe )
		goto _test_eof59;
case 59:
	if ( (*p) == 116u )
		goto st60;
	goto st0;
st60:
	if ( ++p == pe )
		goto _test_eof60;
case 60:
	if ( (*p) == 101u )
		goto st61;
	goto st0;
st61:
	if ( ++p == pe )
		goto _test_eof61;
case 61:
	if ( (*p) == 67u )
		goto st62;
	goto st0;
st62:
	if ( ++p == pe )
		goto _test_eof62;
case 62:
	if ( (*p) == 111u )
		goto st63;
	goto st0;
st63:
	if ( ++p == pe )
		goto _test_eof63;
case 63:
	if ( (*p) == 110u )
		goto st64;
	goto st0;
st64:
	if ( ++p == pe )
		goto _test_eof64;
case 64:
	if ( (*p) == 102u )
		goto st65;
	goto st0;
st65:
	if ( ++p == pe )
		goto _test_eof65;
case 65:
	if ( (*p) == 101u )
		goto st66;
	goto st0;
st66:
	if ( ++p == pe )
		goto _test_eof66;
case 66:
	if ( (*p) == 114u )
		goto st67;
	goto st0;
st67:
	if ( ++p == pe )
		goto _test_eof67;
case 67:
	if ( (*p) == 101u )
		goto st68;
	goto st0;
st68:
	if ( ++p == pe )
		goto _test_eof68;
case 68:
	if ( (*p) == 110u )
		goto st69;
	goto st0;
st69:
	if ( ++p == pe )
		goto _test_eof69;
case 69:
	if ( (*p) == 99u )
		goto st70;
	goto st0;
st70:
	if ( ++p == pe )
		goto _test_eof70;
case 70:
	if ( (*p) == 101u )
		goto st71;
	goto st0;
st71:
	if ( ++p == pe )
		goto _test_eof71;
case 71:
	if ( (*p) == 13u )
		goto st72;
	goto st0;
st72:
	if ( ++p == pe )
		goto _test_eof72;
case 72:
	if ( (*p) == 10u )
		goto st73;
	goto st0;
st73:
	if ( ++p == pe )
		goto _test_eof73;
case 73:
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr77;
	goto st0;
tr77:
#line 5 "src/ControlProtocol/grammar/RequestCreateConference.rl"
	{
		this->msg = new ControlProtocol::RequestCreateConference();
	}
#line 26 "src/ControlProtocol/Parser.rl"
	{
			MARK(this->mark, p);
		}
	goto st74;
tr93:
#line 5 "src/ControlProtocol/grammar/RequestHello.rl"
	{
		this->msg = new ControlProtocol::RequestHello();
	}
#line 26 "src/ControlProtocol/Parser.rl"
	{
			MARK(this->mark, p);
		}
	goto st74;
st74:
	if ( ++p == pe )
		goto _test_eof74;
case 74:
#line 773 "src/ControlProtocol/Parser.cpp"
	if ( (*p) == 13u )
		goto tr78;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st75;
	goto st0;
st75:
	if ( ++p == pe )
		goto _test_eof75;
case 75:
	if ( (*p) == 13u )
		goto tr78;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st76;
	goto st0;
st76:
	if ( ++p == pe )
		goto _test_eof76;
case 76:
	if ( (*p) == 13u )
		goto tr78;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st77;
	goto st0;
st77:
	if ( ++p == pe )
		goto _test_eof77;
case 77:
	if ( (*p) == 13u )
		goto tr78;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st78;
	goto st0;
st78:
	if ( ++p == pe )
		goto _test_eof78;
case 78:
	if ( (*p) == 13u )
		goto tr78;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st79;
	goto st0;
st79:
	if ( ++p == pe )
		goto _test_eof79;
case 79:
	if ( (*p) == 13u )
		goto tr78;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st80;
	goto st0;
st80:
	if ( ++p == pe )
		goto _test_eof80;
case 80:
	if ( (*p) == 13u )
		goto tr78;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st81;
	goto st0;
st81:
	if ( ++p == pe )
		goto _test_eof81;
case 81:
	if ( (*p) == 13u )
		goto tr78;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st82;
	goto st0;
st82:
	if ( ++p == pe )
		goto _test_eof82;
case 82:
	if ( (*p) == 13u )
		goto tr78;
	goto st0;
st83:
	if ( ++p == pe )
		goto _test_eof83;
case 83:
	if ( (*p) == 101u )
		goto st84;
	goto st0;
st84:
	if ( ++p == pe )
		goto _test_eof84;
case 84:
	if ( (*p) == 108u )
		goto st85;
	goto st0;
st85:
	if ( ++p == pe )
		goto _test_eof85;
case 85:
	if ( (*p) == 108u )
		goto st86;
	goto st0;
st86:
	if ( ++p == pe )
		goto _test_eof86;
case 86:
	if ( (*p) == 111u )
		goto st87;
	goto st0;
st87:
	if ( ++p == pe )
		goto _test_eof87;
case 87:
	if ( (*p) == 13u )
		goto st88;
	goto st0;
st88:
	if ( ++p == pe )
		goto _test_eof88;
case 88:
	if ( (*p) == 10u )
		goto st89;
	goto st0;
st89:
	if ( ++p == pe )
		goto _test_eof89;
case 89:
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr93;
	goto st0;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof16: cs = 16; goto _test_eof; 
	_test_eof17: cs = 17; goto _test_eof; 
	_test_eof18: cs = 18; goto _test_eof; 
	_test_eof19: cs = 19; goto _test_eof; 
	_test_eof20: cs = 20; goto _test_eof; 
	_test_eof21: cs = 21; goto _test_eof; 
	_test_eof22: cs = 22; goto _test_eof; 
	_test_eof23: cs = 23; goto _test_eof; 
	_test_eof24: cs = 24; goto _test_eof; 
	_test_eof25: cs = 25; goto _test_eof; 
	_test_eof26: cs = 26; goto _test_eof; 
	_test_eof27: cs = 27; goto _test_eof; 
	_test_eof28: cs = 28; goto _test_eof; 
	_test_eof29: cs = 29; goto _test_eof; 
	_test_eof30: cs = 30; goto _test_eof; 
	_test_eof31: cs = 31; goto _test_eof; 
	_test_eof32: cs = 32; goto _test_eof; 
	_test_eof33: cs = 33; goto _test_eof; 
	_test_eof34: cs = 34; goto _test_eof; 
	_test_eof35: cs = 35; goto _test_eof; 
	_test_eof36: cs = 36; goto _test_eof; 
	_test_eof37: cs = 37; goto _test_eof; 
	_test_eof38: cs = 38; goto _test_eof; 
	_test_eof39: cs = 39; goto _test_eof; 
	_test_eof40: cs = 40; goto _test_eof; 
	_test_eof41: cs = 41; goto _test_eof; 
	_test_eof42: cs = 42; goto _test_eof; 
	_test_eof43: cs = 43; goto _test_eof; 
	_test_eof44: cs = 44; goto _test_eof; 
	_test_eof45: cs = 45; goto _test_eof; 
	_test_eof46: cs = 46; goto _test_eof; 
	_test_eof47: cs = 47; goto _test_eof; 
	_test_eof90: cs = 90; goto _test_eof; 
	_test_eof48: cs = 48; goto _test_eof; 
	_test_eof49: cs = 49; goto _test_eof; 
	_test_eof50: cs = 50; goto _test_eof; 
	_test_eof51: cs = 51; goto _test_eof; 
	_test_eof52: cs = 52; goto _test_eof; 
	_test_eof53: cs = 53; goto _test_eof; 
	_test_eof54: cs = 54; goto _test_eof; 
	_test_eof55: cs = 55; goto _test_eof; 
	_test_eof56: cs = 56; goto _test_eof; 
	_test_eof57: cs = 57; goto _test_eof; 
	_test_eof58: cs = 58; goto _test_eof; 
	_test_eof59: cs = 59; goto _test_eof; 
	_test_eof60: cs = 60; goto _test_eof; 
	_test_eof61: cs = 61; goto _test_eof; 
	_test_eof62: cs = 62; goto _test_eof; 
	_test_eof63: cs = 63; goto _test_eof; 
	_test_eof64: cs = 64; goto _test_eof; 
	_test_eof65: cs = 65; goto _test_eof; 
	_test_eof66: cs = 66; goto _test_eof; 
	_test_eof67: cs = 67; goto _test_eof; 
	_test_eof68: cs = 68; goto _test_eof; 
	_test_eof69: cs = 69; goto _test_eof; 
	_test_eof70: cs = 70; goto _test_eof; 
	_test_eof71: cs = 71; goto _test_eof; 
	_test_eof72: cs = 72; goto _test_eof; 
	_test_eof73: cs = 73; goto _test_eof; 
	_test_eof74: cs = 74; goto _test_eof; 
	_test_eof75: cs = 75; goto _test_eof; 
	_test_eof76: cs = 76; goto _test_eof; 
	_test_eof77: cs = 77; goto _test_eof; 
	_test_eof78: cs = 78; goto _test_eof; 
	_test_eof79: cs = 79; goto _test_eof; 
	_test_eof80: cs = 80; goto _test_eof; 
	_test_eof81: cs = 81; goto _test_eof; 
	_test_eof82: cs = 82; goto _test_eof; 
	_test_eof83: cs = 83; goto _test_eof; 
	_test_eof84: cs = 84; goto _test_eof; 
	_test_eof85: cs = 85; goto _test_eof; 
	_test_eof86: cs = 86; goto _test_eof; 
	_test_eof87: cs = 87; goto _test_eof; 
	_test_eof88: cs = 88; goto _test_eof; 
	_test_eof89: cs = 89; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 98 "src/ControlProtocol/Parser.rl"

		this->parsedLen = p - (const unsigned char*)buffer;

		MS_ASSERT(p <= pe, "buffer overflow after parsing execute");
		MS_ASSERT(this->parsedLen <= len, "parsedLen longer than length");
		MS_ASSERT(this->mark < len, "mark is after buffer end");

		// Parsing succedded.
		if (this->cs == (size_t)MessageParser_first_final)
		{
			MS_DEBUG("parsing finished OK");

			MS_ASSERT(this->msg != nullptr, "parsing OK but msg is NULL");

			// Check the message semantics.
			if (!this->msg->IsValid())
			{
				MS_ERROR("invalid message");
				delete this->msg;
				return (this->msg = nullptr);
			}

			return this->msg;
		}

		// Parsing error.
		else if (this->cs == (size_t)MessageParser_error)
		{
			MS_ERROR("parsing error at position %zd", this->parsedLen);

			// Delete this->msg if it exists.
			if (this->msg)
			{
				delete this->msg;
				this->msg = nullptr;
			}

			return nullptr;
		}

		// Parsing not finished.
		else
		{
			MS_DEBUG("parsing not finished");

			return nullptr;
		}
	}

	bool Parser::HasError()
	{
		MS_TRACE();

		return (this->cs == (size_t)MessageParser_error);
	}

	size_t Parser::GetParsedLen()
	{
		MS_TRACE();

		return this->parsedLen;
	}

	void Parser::Reset()
	{
		MS_TRACE();

		this->parsedLen = 0;
		this->msg = nullptr;
		this->cs = 0;
		this->mark = 0;

		/**
		 * Ragel: %%write init
		 * This sets cs variable needed by Ragel (so this->cs at the end).
		 */
		
#line 1071 "src/ControlProtocol/Parser.cpp"
	{
	cs = MessageParser_start;
	}

#line 175 "src/ControlProtocol/Parser.rl"
	}

	void Parser::Dump()
	{
		MS_DEBUG("[cs: %zu | parsedLen: %zu | mark: %zu | error?: %s | finished?: %s]",
			this->cs, this->parsedLen, this->mark,
			HasError() ? "true" : "false",
			(this->cs >= (size_t)MessageParser_first_final) ? "true" : "false");
	}
}  // namespace ControlProtocol
