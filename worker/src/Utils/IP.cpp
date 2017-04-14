
#line 1 "src/Utils/IP.rl"
#define MS_CLASS "Utils::IP"
// #define MS_LOG_DEV

#include "Utils.hpp"
#include "Logger.hpp"
#include <uv.h>

namespace Utils
{
	int IP::GetFamily(const char *ip, size_t ipLen)
	{
		MS_TRACE();

		int ipFamily = 0;

		/**
		 * Ragel: machine definition.
		 */
		
#line 38 "src/Utils/IP.rl"


		/**
		 * Ragel: %%write data
		 * This generates Ragel's static variables.
		 */
		
#line 31 "src/Utils/IP.cpp"
static const int IPParser_start = 1;






#line 45 "src/Utils/IP.rl"

		// Used by Ragel.
		size_t cs;
		const unsigned char* p;
		const unsigned char* pe;

		p = (const unsigned char*)ip;
		pe = p + ipLen;

		/**
		 * Ragel: %%write init
		 */
		
#line 53 "src/Utils/IP.cpp"
	{
	cs = IPParser_start;
	}

#line 58 "src/Utils/IP.rl"

		/**
		 * Ragel: %%write exec
		 * This updates cs variable needed by Ragel.
		 */
		
#line 65 "src/Utils/IP.cpp"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	switch( (*p) ) {
		case 48u: goto st2;
		case 49u: goto st94;
		case 50u: goto st97;
		case 58u: goto st101;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto st100;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st107;
	} else
		goto st107;
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( (*p) ) {
		case 46u: goto st3;
		case 58u: goto st19;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st16;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st16;
	} else
		goto st16;
	goto st0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	switch( (*p) ) {
		case 48u: goto st4;
		case 49u: goto st12;
		case 50u: goto st14;
	}
	if ( 51u <= (*p) && (*p) <= 57u )
		goto st13;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	if ( (*p) == 46u )
		goto st5;
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	switch( (*p) ) {
		case 48u: goto st6;
		case 49u: goto st8;
		case 50u: goto st10;
	}
	if ( 51u <= (*p) && (*p) <= 57u )
		goto st9;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) == 46u )
		goto st7;
	goto st0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	switch( (*p) ) {
		case 48u: goto tr20;
		case 49u: goto tr21;
		case 50u: goto tr22;
	}
	if ( 51u <= (*p) && (*p) <= 57u )
		goto tr23;
	goto st0;
tr20:
#line 25 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET;
			}
	goto st108;
tr77:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st108;
st108:
	if ( ++p == pe )
		goto _test_eof108;
case 108:
#line 173 "src/Utils/IP.cpp"
	goto st0;
tr21:
#line 25 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET;
			}
	goto st109;
st109:
	if ( ++p == pe )
		goto _test_eof109;
case 109:
#line 185 "src/Utils/IP.cpp"
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr23;
	goto st0;
tr23:
#line 25 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET;
			}
	goto st110;
st110:
	if ( ++p == pe )
		goto _test_eof110;
case 110:
#line 199 "src/Utils/IP.cpp"
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr20;
	goto st0;
tr22:
#line 25 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET;
			}
	goto st111;
st111:
	if ( ++p == pe )
		goto _test_eof111;
case 111:
#line 213 "src/Utils/IP.cpp"
	if ( (*p) == 53u )
		goto tr173;
	if ( (*p) > 52u ) {
		if ( 54u <= (*p) && (*p) <= 57u )
			goto tr20;
	} else if ( (*p) >= 48u )
		goto tr23;
	goto st0;
tr173:
#line 25 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET;
			}
	goto st112;
st112:
	if ( ++p == pe )
		goto _test_eof112;
case 112:
#line 232 "src/Utils/IP.cpp"
	if ( 48u <= (*p) && (*p) <= 53u )
		goto tr20;
	goto st0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	if ( (*p) == 46u )
		goto st7;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st9;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( (*p) == 46u )
		goto st7;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st6;
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( (*p) ) {
		case 46u: goto st7;
		case 53u: goto st11;
	}
	if ( (*p) > 52u ) {
		if ( 54u <= (*p) && (*p) <= 57u )
			goto st6;
	} else if ( (*p) >= 48u )
		goto st9;
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( (*p) == 46u )
		goto st7;
	if ( 48u <= (*p) && (*p) <= 53u )
		goto st6;
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( (*p) == 46u )
		goto st5;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st13;
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( (*p) == 46u )
		goto st5;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st4;
	goto st0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	switch( (*p) ) {
		case 46u: goto st5;
		case 53u: goto st15;
	}
	if ( (*p) > 52u ) {
		if ( 54u <= (*p) && (*p) <= 57u )
			goto st4;
	} else if ( (*p) >= 48u )
		goto st13;
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( (*p) == 46u )
		goto st5;
	if ( 48u <= (*p) && (*p) <= 53u )
		goto st4;
	goto st0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	if ( (*p) == 58u )
		goto st19;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st17;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st17;
	} else
		goto st17;
	goto st0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	if ( (*p) == 58u )
		goto st19;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st18;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st18;
	} else
		goto st18;
	goto st0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	if ( (*p) == 58u )
		goto st19;
	goto st0;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	if ( (*p) == 58u )
		goto tr29;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st20;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st20;
	} else
		goto st20;
	goto st0;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
	if ( (*p) == 58u )
		goto st24;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st21;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st21;
	} else
		goto st21;
	goto st0;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
	if ( (*p) == 58u )
		goto st24;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st22;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st22;
	} else
		goto st22;
	goto st0;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
	if ( (*p) == 58u )
		goto st24;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st23;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st23;
	} else
		goto st23;
	goto st0;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	if ( (*p) == 58u )
		goto st24;
	goto st0;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
	if ( (*p) == 58u )
		goto tr35;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st25;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st25;
	} else
		goto st25;
	goto st0;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
	if ( (*p) == 58u )
		goto st29;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st26;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st26;
	} else
		goto st26;
	goto st0;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	if ( (*p) == 58u )
		goto st29;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st27;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st27;
	} else
		goto st27;
	goto st0;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
	if ( (*p) == 58u )
		goto st29;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st28;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st28;
	} else
		goto st28;
	goto st0;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
	if ( (*p) == 58u )
		goto st29;
	goto st0;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
	if ( (*p) == 58u )
		goto tr41;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st30;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st30;
	} else
		goto st30;
	goto st0;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
	if ( (*p) == 58u )
		goto st34;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st31;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st31;
	} else
		goto st31;
	goto st0;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
	if ( (*p) == 58u )
		goto st34;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st32;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st32;
	} else
		goto st32;
	goto st0;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
	if ( (*p) == 58u )
		goto st34;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st33;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st33;
	} else
		goto st33;
	goto st0;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
	if ( (*p) == 58u )
		goto st34;
	goto st0;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
	if ( (*p) == 58u )
		goto tr47;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st35;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st35;
	} else
		goto st35;
	goto st0;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
	if ( (*p) == 58u )
		goto st39;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st36;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st36;
	} else
		goto st36;
	goto st0;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
	if ( (*p) == 58u )
		goto st39;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st37;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st37;
	} else
		goto st37;
	goto st0;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
	if ( (*p) == 58u )
		goto st39;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st38;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st38;
	} else
		goto st38;
	goto st0;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
	if ( (*p) == 58u )
		goto st39;
	goto st0;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
	if ( (*p) == 58u )
		goto tr53;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st40;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st40;
	} else
		goto st40;
	goto st0;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
	if ( (*p) == 58u )
		goto st44;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st41;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st41;
	} else
		goto st41;
	goto st0;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
	if ( (*p) == 58u )
		goto st44;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st42;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st42;
	} else
		goto st42;
	goto st0;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
	if ( (*p) == 58u )
		goto st44;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st43;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st43;
	} else
		goto st43;
	goto st0;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
	if ( (*p) == 58u )
		goto st44;
	goto st0;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
	switch( (*p) ) {
		case 48u: goto st45;
		case 49u: goto st64;
		case 50u: goto st67;
		case 58u: goto tr62;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto st70;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st71;
	} else
		goto st71;
	goto st0;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st62;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st59;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st59;
	} else
		goto st59;
	goto st0;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
	switch( (*p) ) {
		case 48u: goto st47;
		case 49u: goto st55;
		case 50u: goto st57;
	}
	if ( 51u <= (*p) && (*p) <= 57u )
		goto st56;
	goto st0;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
	if ( (*p) == 46u )
		goto st48;
	goto st0;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
	switch( (*p) ) {
		case 48u: goto st49;
		case 49u: goto st51;
		case 50u: goto st53;
	}
	if ( 51u <= (*p) && (*p) <= 57u )
		goto st52;
	goto st0;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
	if ( (*p) == 46u )
		goto st50;
	goto st0;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
	switch( (*p) ) {
		case 48u: goto tr77;
		case 49u: goto tr78;
		case 50u: goto tr79;
	}
	if ( 51u <= (*p) && (*p) <= 57u )
		goto tr80;
	goto st0;
tr78:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st113;
st113:
	if ( ++p == pe )
		goto _test_eof113;
case 113:
#line 786 "src/Utils/IP.cpp"
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr80;
	goto st0;
tr80:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st114;
st114:
	if ( ++p == pe )
		goto _test_eof114;
case 114:
#line 800 "src/Utils/IP.cpp"
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr77;
	goto st0;
tr79:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st115;
st115:
	if ( ++p == pe )
		goto _test_eof115;
case 115:
#line 814 "src/Utils/IP.cpp"
	if ( (*p) == 53u )
		goto tr174;
	if ( (*p) > 52u ) {
		if ( 54u <= (*p) && (*p) <= 57u )
			goto tr77;
	} else if ( (*p) >= 48u )
		goto tr80;
	goto st0;
tr174:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st116;
st116:
	if ( ++p == pe )
		goto _test_eof116;
case 116:
#line 833 "src/Utils/IP.cpp"
	if ( 48u <= (*p) && (*p) <= 53u )
		goto tr77;
	goto st0;
st51:
	if ( ++p == pe )
		goto _test_eof51;
case 51:
	if ( (*p) == 46u )
		goto st50;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st52;
	goto st0;
st52:
	if ( ++p == pe )
		goto _test_eof52;
case 52:
	if ( (*p) == 46u )
		goto st50;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st49;
	goto st0;
st53:
	if ( ++p == pe )
		goto _test_eof53;
case 53:
	switch( (*p) ) {
		case 46u: goto st50;
		case 53u: goto st54;
	}
	if ( (*p) > 52u ) {
		if ( 54u <= (*p) && (*p) <= 57u )
			goto st49;
	} else if ( (*p) >= 48u )
		goto st52;
	goto st0;
st54:
	if ( ++p == pe )
		goto _test_eof54;
case 54:
	if ( (*p) == 46u )
		goto st50;
	if ( 48u <= (*p) && (*p) <= 53u )
		goto st49;
	goto st0;
st55:
	if ( ++p == pe )
		goto _test_eof55;
case 55:
	if ( (*p) == 46u )
		goto st48;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st56;
	goto st0;
st56:
	if ( ++p == pe )
		goto _test_eof56;
case 56:
	if ( (*p) == 46u )
		goto st48;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st47;
	goto st0;
st57:
	if ( ++p == pe )
		goto _test_eof57;
case 57:
	switch( (*p) ) {
		case 46u: goto st48;
		case 53u: goto st58;
	}
	if ( (*p) > 52u ) {
		if ( 54u <= (*p) && (*p) <= 57u )
			goto st47;
	} else if ( (*p) >= 48u )
		goto st56;
	goto st0;
st58:
	if ( ++p == pe )
		goto _test_eof58;
case 58:
	if ( (*p) == 46u )
		goto st48;
	if ( 48u <= (*p) && (*p) <= 53u )
		goto st47;
	goto st0;
st59:
	if ( ++p == pe )
		goto _test_eof59;
case 59:
	if ( (*p) == 58u )
		goto st62;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st60;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st60;
	} else
		goto st60;
	goto st0;
st60:
	if ( ++p == pe )
		goto _test_eof60;
case 60:
	if ( (*p) == 58u )
		goto st62;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st61;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st61;
	} else
		goto st61;
	goto st0;
st61:
	if ( ++p == pe )
		goto _test_eof61;
case 61:
	if ( (*p) == 58u )
		goto st62;
	goto st0;
st62:
	if ( ++p == pe )
		goto _test_eof62;
case 62:
	if ( (*p) == 58u )
		goto tr86;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr85;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr85;
	} else
		goto tr85;
	goto st0;
tr85:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st117;
st117:
	if ( ++p == pe )
		goto _test_eof117;
case 117:
#line 981 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr175;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr175;
	} else
		goto tr175;
	goto st0;
tr175:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st118;
st118:
	if ( ++p == pe )
		goto _test_eof118;
case 118:
#line 1001 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr176;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr176;
	} else
		goto tr176;
	goto st0;
tr176:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st119;
st119:
	if ( ++p == pe )
		goto _test_eof119;
case 119:
#line 1021 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr77;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr77;
	} else
		goto tr77;
	goto st0;
tr86:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st120;
st120:
	if ( ++p == pe )
		goto _test_eof120;
case 120:
#line 1041 "src/Utils/IP.cpp"
	if ( (*p) == 37u )
		goto st63;
	goto st0;
st63:
	if ( ++p == pe )
		goto _test_eof63;
case 63:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr87;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr87;
	} else
		goto tr87;
	goto st0;
tr87:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st121;
st121:
	if ( ++p == pe )
		goto _test_eof121;
case 121:
#line 1068 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr178;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr178;
	} else
		goto tr178;
	goto st0;
tr178:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st122;
st122:
	if ( ++p == pe )
		goto _test_eof122;
case 122:
#line 1088 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr179;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr179;
	} else
		goto tr179;
	goto st0;
tr179:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st123;
st123:
	if ( ++p == pe )
		goto _test_eof123;
case 123:
#line 1108 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr180;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr180;
	} else
		goto tr180;
	goto st0;
tr180:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st124;
st124:
	if ( ++p == pe )
		goto _test_eof124;
case 124:
#line 1128 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr181;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr181;
	} else
		goto tr181;
	goto st0;
tr181:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st125;
st125:
	if ( ++p == pe )
		goto _test_eof125;
case 125:
#line 1148 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr182;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr182;
	} else
		goto tr182;
	goto st0;
tr182:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st126;
st126:
	if ( ++p == pe )
		goto _test_eof126;
case 126:
#line 1168 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr183;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr183;
	} else
		goto tr183;
	goto st0;
tr183:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st127;
st127:
	if ( ++p == pe )
		goto _test_eof127;
case 127:
#line 1188 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr184;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr184;
	} else
		goto tr184;
	goto st0;
tr184:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st128;
st128:
	if ( ++p == pe )
		goto _test_eof128;
case 128:
#line 1208 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr185;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr185;
	} else
		goto tr185;
	goto st0;
tr185:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st129;
st129:
	if ( ++p == pe )
		goto _test_eof129;
case 129:
#line 1228 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr186;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr186;
	} else
		goto tr186;
	goto st0;
tr186:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st130;
st130:
	if ( ++p == pe )
		goto _test_eof130;
case 130:
#line 1248 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr187;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr187;
	} else
		goto tr187;
	goto st0;
tr187:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st131;
st131:
	if ( ++p == pe )
		goto _test_eof131;
case 131:
#line 1268 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr188;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr188;
	} else
		goto tr188;
	goto st0;
tr188:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st132;
st132:
	if ( ++p == pe )
		goto _test_eof132;
case 132:
#line 1288 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr189;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr189;
	} else
		goto tr189;
	goto st0;
tr189:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st133;
st133:
	if ( ++p == pe )
		goto _test_eof133;
case 133:
#line 1308 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr190;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr190;
	} else
		goto tr190;
	goto st0;
tr190:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st134;
st134:
	if ( ++p == pe )
		goto _test_eof134;
case 134:
#line 1328 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr191;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr191;
	} else
		goto tr191;
	goto st0;
tr191:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st135;
st135:
	if ( ++p == pe )
		goto _test_eof135;
case 135:
#line 1348 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr192;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr192;
	} else
		goto tr192;
	goto st0;
tr192:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st136;
st136:
	if ( ++p == pe )
		goto _test_eof136;
case 136:
#line 1368 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr193;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr193;
	} else
		goto tr193;
	goto st0;
tr193:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st137;
st137:
	if ( ++p == pe )
		goto _test_eof137;
case 137:
#line 1388 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr194;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr194;
	} else
		goto tr194;
	goto st0;
tr194:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st138;
st138:
	if ( ++p == pe )
		goto _test_eof138;
case 138:
#line 1408 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr195;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr195;
	} else
		goto tr195;
	goto st0;
tr195:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st139;
st139:
	if ( ++p == pe )
		goto _test_eof139;
case 139:
#line 1428 "src/Utils/IP.cpp"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr77;
	} else if ( (*p) > 90u ) {
		if ( 97u <= (*p) && (*p) <= 122u )
			goto tr77;
	} else
		goto tr77;
	goto st0;
st64:
	if ( ++p == pe )
		goto _test_eof64;
case 64:
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st62;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st65;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st59;
	} else
		goto st59;
	goto st0;
st65:
	if ( ++p == pe )
		goto _test_eof65;
case 65:
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st62;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st66;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st60;
	} else
		goto st60;
	goto st0;
st66:
	if ( ++p == pe )
		goto _test_eof66;
case 66:
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st62;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st61;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st61;
	} else
		goto st61;
	goto st0;
st67:
	if ( ++p == pe )
		goto _test_eof67;
case 67:
	switch( (*p) ) {
		case 46u: goto st46;
		case 53u: goto st68;
		case 58u: goto st62;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto st65;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto st59;
		} else if ( (*p) >= 65u )
			goto st59;
	} else
		goto st69;
	goto st0;
st68:
	if ( ++p == pe )
		goto _test_eof68;
case 68:
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st62;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto st66;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto st60;
		} else if ( (*p) >= 65u )
			goto st60;
	} else
		goto st60;
	goto st0;
st69:
	if ( ++p == pe )
		goto _test_eof69;
case 69:
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st62;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st60;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st60;
	} else
		goto st60;
	goto st0;
st70:
	if ( ++p == pe )
		goto _test_eof70;
case 70:
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st62;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st69;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st59;
	} else
		goto st59;
	goto st0;
tr62:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st140;
st140:
	if ( ++p == pe )
		goto _test_eof140;
case 140:
#line 1574 "src/Utils/IP.cpp"
	if ( (*p) == 37u )
		goto st63;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr107;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr107;
	} else
		goto tr107;
	goto st0;
tr107:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st141;
st141:
	if ( ++p == pe )
		goto _test_eof141;
case 141:
#line 1596 "src/Utils/IP.cpp"
	if ( (*p) == 37u )
		goto st63;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr196;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr196;
	} else
		goto tr196;
	goto st0;
tr196:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st142;
st142:
	if ( ++p == pe )
		goto _test_eof142;
case 142:
#line 1618 "src/Utils/IP.cpp"
	if ( (*p) == 37u )
		goto st63;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr197;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr197;
	} else
		goto tr197;
	goto st0;
tr197:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st143;
st143:
	if ( ++p == pe )
		goto _test_eof143;
case 143:
#line 1640 "src/Utils/IP.cpp"
	if ( (*p) == 37u )
		goto st63;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr86;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr86;
	} else
		goto tr86;
	goto st0;
st71:
	if ( ++p == pe )
		goto _test_eof71;
case 71:
	if ( (*p) == 58u )
		goto st62;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st59;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st59;
	} else
		goto st59;
	goto st0;
tr53:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st144;
st144:
	if ( ++p == pe )
		goto _test_eof144;
case 144:
#line 1677 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 48u: goto tr108;
		case 49u: goto tr109;
		case 50u: goto tr110;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr111;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr112;
	} else
		goto tr112;
	goto st0;
tr108:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st145;
st145:
	if ( ++p == pe )
		goto _test_eof145;
case 145:
#line 1703 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st85;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr199;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr199;
	} else
		goto tr199;
	goto st0;
st72:
	if ( ++p == pe )
		goto _test_eof72;
case 72:
	switch( (*p) ) {
		case 48u: goto st73;
		case 49u: goto st81;
		case 50u: goto st83;
	}
	if ( 51u <= (*p) && (*p) <= 57u )
		goto st82;
	goto st0;
st73:
	if ( ++p == pe )
		goto _test_eof73;
case 73:
	if ( (*p) == 46u )
		goto st74;
	goto st0;
st74:
	if ( ++p == pe )
		goto _test_eof74;
case 74:
	switch( (*p) ) {
		case 48u: goto st75;
		case 49u: goto st77;
		case 50u: goto st79;
	}
	if ( 51u <= (*p) && (*p) <= 57u )
		goto st78;
	goto st0;
st75:
	if ( ++p == pe )
		goto _test_eof75;
case 75:
	if ( (*p) == 46u )
		goto st76;
	goto st0;
st76:
	if ( ++p == pe )
		goto _test_eof76;
case 76:
	switch( (*p) ) {
		case 48u: goto tr86;
		case 49u: goto tr102;
		case 50u: goto tr103;
	}
	if ( 51u <= (*p) && (*p) <= 57u )
		goto tr104;
	goto st0;
tr102:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st146;
st146:
	if ( ++p == pe )
		goto _test_eof146;
case 146:
#line 1778 "src/Utils/IP.cpp"
	if ( (*p) == 37u )
		goto st63;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr104;
	goto st0;
tr104:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st147;
st147:
	if ( ++p == pe )
		goto _test_eof147;
case 147:
#line 1794 "src/Utils/IP.cpp"
	if ( (*p) == 37u )
		goto st63;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto tr86;
	goto st0;
tr103:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st148;
st148:
	if ( ++p == pe )
		goto _test_eof148;
case 148:
#line 1810 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 53u: goto tr201;
	}
	if ( (*p) > 52u ) {
		if ( 54u <= (*p) && (*p) <= 57u )
			goto tr86;
	} else if ( (*p) >= 48u )
		goto tr104;
	goto st0;
tr201:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st149;
st149:
	if ( ++p == pe )
		goto _test_eof149;
case 149:
#line 1831 "src/Utils/IP.cpp"
	if ( (*p) == 37u )
		goto st63;
	if ( 48u <= (*p) && (*p) <= 53u )
		goto tr86;
	goto st0;
st77:
	if ( ++p == pe )
		goto _test_eof77;
case 77:
	if ( (*p) == 46u )
		goto st76;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st78;
	goto st0;
st78:
	if ( ++p == pe )
		goto _test_eof78;
case 78:
	if ( (*p) == 46u )
		goto st76;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st75;
	goto st0;
st79:
	if ( ++p == pe )
		goto _test_eof79;
case 79:
	switch( (*p) ) {
		case 46u: goto st76;
		case 53u: goto st80;
	}
	if ( (*p) > 52u ) {
		if ( 54u <= (*p) && (*p) <= 57u )
			goto st75;
	} else if ( (*p) >= 48u )
		goto st78;
	goto st0;
st80:
	if ( ++p == pe )
		goto _test_eof80;
case 80:
	if ( (*p) == 46u )
		goto st76;
	if ( 48u <= (*p) && (*p) <= 53u )
		goto st75;
	goto st0;
st81:
	if ( ++p == pe )
		goto _test_eof81;
case 81:
	if ( (*p) == 46u )
		goto st74;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st82;
	goto st0;
st82:
	if ( ++p == pe )
		goto _test_eof82;
case 82:
	if ( (*p) == 46u )
		goto st74;
	if ( 48u <= (*p) && (*p) <= 57u )
		goto st73;
	goto st0;
st83:
	if ( ++p == pe )
		goto _test_eof83;
case 83:
	switch( (*p) ) {
		case 46u: goto st74;
		case 53u: goto st84;
	}
	if ( (*p) > 52u ) {
		if ( 54u <= (*p) && (*p) <= 57u )
			goto st73;
	} else if ( (*p) >= 48u )
		goto st82;
	goto st0;
st84:
	if ( ++p == pe )
		goto _test_eof84;
case 84:
	if ( (*p) == 46u )
		goto st74;
	if ( 48u <= (*p) && (*p) <= 53u )
		goto st73;
	goto st0;
tr199:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st150;
st150:
	if ( ++p == pe )
		goto _test_eof150;
case 150:
#line 1929 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st85;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr202;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr202;
	} else
		goto tr202;
	goto st0;
tr202:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st151;
st151:
	if ( ++p == pe )
		goto _test_eof151;
case 151:
#line 1953 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st85;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr203;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr203;
	} else
		goto tr203;
	goto st0;
tr203:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st152;
st152:
	if ( ++p == pe )
		goto _test_eof152;
case 152:
#line 1977 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st85;
	}
	goto st0;
st85:
	if ( ++p == pe )
		goto _test_eof85;
case 85:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr107;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr107;
	} else
		goto tr107;
	goto st0;
tr109:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st153;
st153:
	if ( ++p == pe )
		goto _test_eof153;
case 153:
#line 2006 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st85;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr204;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr199;
	} else
		goto tr199;
	goto st0;
tr204:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st154;
st154:
	if ( ++p == pe )
		goto _test_eof154;
case 154:
#line 2031 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st85;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr205;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr202;
	} else
		goto tr202;
	goto st0;
tr205:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st155;
st155:
	if ( ++p == pe )
		goto _test_eof155;
case 155:
#line 2056 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st85;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr203;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr203;
	} else
		goto tr203;
	goto st0;
tr110:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st156;
st156:
	if ( ++p == pe )
		goto _test_eof156;
case 156:
#line 2081 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr206;
		case 58u: goto st85;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr204;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr199;
		} else if ( (*p) >= 65u )
			goto tr199;
	} else
		goto tr207;
	goto st0;
tr206:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st157;
st157:
	if ( ++p == pe )
		goto _test_eof157;
case 157:
#line 2110 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st85;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr205;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr202;
		} else if ( (*p) >= 65u )
			goto tr202;
	} else
		goto tr202;
	goto st0;
tr207:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st158;
st158:
	if ( ++p == pe )
		goto _test_eof158;
case 158:
#line 2138 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st85;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr202;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr202;
	} else
		goto tr202;
	goto st0;
tr111:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st159;
st159:
	if ( ++p == pe )
		goto _test_eof159;
case 159:
#line 2163 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st85;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr207;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr199;
	} else
		goto tr199;
	goto st0;
tr112:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st160;
st160:
	if ( ++p == pe )
		goto _test_eof160;
case 160:
#line 2188 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st85;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr199;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr199;
	} else
		goto tr199;
	goto st0;
tr47:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st161;
st161:
	if ( ++p == pe )
		goto _test_eof161;
case 161:
#line 2212 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 48u: goto tr113;
		case 49u: goto tr114;
		case 50u: goto tr115;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr116;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr117;
	} else
		goto tr117;
	goto st0;
tr113:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st162;
st162:
	if ( ++p == pe )
		goto _test_eof162;
case 162:
#line 2238 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st86;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr208;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr208;
	} else
		goto tr208;
	goto st0;
tr208:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st163;
st163:
	if ( ++p == pe )
		goto _test_eof163;
case 163:
#line 2263 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st86;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr210;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr210;
	} else
		goto tr210;
	goto st0;
tr210:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st164;
st164:
	if ( ++p == pe )
		goto _test_eof164;
case 164:
#line 2287 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st86;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr211;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr211;
	} else
		goto tr211;
	goto st0;
tr211:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st165;
st165:
	if ( ++p == pe )
		goto _test_eof165;
case 165:
#line 2311 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st86;
	}
	goto st0;
st86:
	if ( ++p == pe )
		goto _test_eof86;
case 86:
	switch( (*p) ) {
		case 48u: goto tr108;
		case 49u: goto tr109;
		case 50u: goto tr110;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr111;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr112;
	} else
		goto tr112;
	goto st0;
tr114:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st166;
st166:
	if ( ++p == pe )
		goto _test_eof166;
case 166:
#line 2345 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st86;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr212;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr208;
	} else
		goto tr208;
	goto st0;
tr212:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st167;
st167:
	if ( ++p == pe )
		goto _test_eof167;
case 167:
#line 2370 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st86;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr213;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr210;
	} else
		goto tr210;
	goto st0;
tr213:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st168;
st168:
	if ( ++p == pe )
		goto _test_eof168;
case 168:
#line 2395 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st86;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr211;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr211;
	} else
		goto tr211;
	goto st0;
tr115:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st169;
st169:
	if ( ++p == pe )
		goto _test_eof169;
case 169:
#line 2420 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr214;
		case 58u: goto st86;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr212;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr208;
		} else if ( (*p) >= 65u )
			goto tr208;
	} else
		goto tr215;
	goto st0;
tr214:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st170;
st170:
	if ( ++p == pe )
		goto _test_eof170;
case 170:
#line 2449 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st86;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr213;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr210;
		} else if ( (*p) >= 65u )
			goto tr210;
	} else
		goto tr210;
	goto st0;
tr215:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st171;
st171:
	if ( ++p == pe )
		goto _test_eof171;
case 171:
#line 2477 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st86;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr210;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr210;
	} else
		goto tr210;
	goto st0;
tr116:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st172;
st172:
	if ( ++p == pe )
		goto _test_eof172;
case 172:
#line 2502 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st86;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr215;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr208;
	} else
		goto tr208;
	goto st0;
tr117:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st173;
st173:
	if ( ++p == pe )
		goto _test_eof173;
case 173:
#line 2527 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st86;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr208;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr208;
	} else
		goto tr208;
	goto st0;
tr41:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st174;
st174:
	if ( ++p == pe )
		goto _test_eof174;
case 174:
#line 2551 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 48u: goto tr118;
		case 49u: goto tr119;
		case 50u: goto tr120;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr121;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr122;
	} else
		goto tr122;
	goto st0;
tr118:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st175;
st175:
	if ( ++p == pe )
		goto _test_eof175;
case 175:
#line 2577 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st87;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr216;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr216;
	} else
		goto tr216;
	goto st0;
tr216:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st176;
st176:
	if ( ++p == pe )
		goto _test_eof176;
case 176:
#line 2602 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st87;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr218;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr218;
	} else
		goto tr218;
	goto st0;
tr218:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st177;
st177:
	if ( ++p == pe )
		goto _test_eof177;
case 177:
#line 2626 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st87;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr219;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr219;
	} else
		goto tr219;
	goto st0;
tr219:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st178;
st178:
	if ( ++p == pe )
		goto _test_eof178;
case 178:
#line 2650 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st87;
	}
	goto st0;
st87:
	if ( ++p == pe )
		goto _test_eof87;
case 87:
	switch( (*p) ) {
		case 48u: goto tr113;
		case 49u: goto tr114;
		case 50u: goto tr115;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr116;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr117;
	} else
		goto tr117;
	goto st0;
tr119:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st179;
st179:
	if ( ++p == pe )
		goto _test_eof179;
case 179:
#line 2684 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st87;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr220;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr216;
	} else
		goto tr216;
	goto st0;
tr220:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st180;
st180:
	if ( ++p == pe )
		goto _test_eof180;
case 180:
#line 2709 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st87;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr221;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr218;
	} else
		goto tr218;
	goto st0;
tr221:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st181;
st181:
	if ( ++p == pe )
		goto _test_eof181;
case 181:
#line 2734 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st87;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr219;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr219;
	} else
		goto tr219;
	goto st0;
tr120:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st182;
st182:
	if ( ++p == pe )
		goto _test_eof182;
case 182:
#line 2759 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr222;
		case 58u: goto st87;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr220;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr216;
		} else if ( (*p) >= 65u )
			goto tr216;
	} else
		goto tr223;
	goto st0;
tr222:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st183;
st183:
	if ( ++p == pe )
		goto _test_eof183;
case 183:
#line 2788 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st87;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr221;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr218;
		} else if ( (*p) >= 65u )
			goto tr218;
	} else
		goto tr218;
	goto st0;
tr223:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st184;
st184:
	if ( ++p == pe )
		goto _test_eof184;
case 184:
#line 2816 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st87;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr218;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr218;
	} else
		goto tr218;
	goto st0;
tr121:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st185;
st185:
	if ( ++p == pe )
		goto _test_eof185;
case 185:
#line 2841 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st87;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr223;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr216;
	} else
		goto tr216;
	goto st0;
tr122:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st186;
st186:
	if ( ++p == pe )
		goto _test_eof186;
case 186:
#line 2866 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st87;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr216;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr216;
	} else
		goto tr216;
	goto st0;
tr35:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st187;
st187:
	if ( ++p == pe )
		goto _test_eof187;
case 187:
#line 2890 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 48u: goto tr224;
		case 49u: goto tr225;
		case 50u: goto tr226;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr227;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr228;
	} else
		goto tr228;
	goto st0;
tr224:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st188;
st188:
	if ( ++p == pe )
		goto _test_eof188;
case 188:
#line 2916 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st88;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr229;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr229;
	} else
		goto tr229;
	goto st0;
tr229:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st189;
st189:
	if ( ++p == pe )
		goto _test_eof189;
case 189:
#line 2941 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st88;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr231;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr231;
	} else
		goto tr231;
	goto st0;
tr231:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st190;
st190:
	if ( ++p == pe )
		goto _test_eof190;
case 190:
#line 2965 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st88;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr232;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr232;
	} else
		goto tr232;
	goto st0;
tr232:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st191;
st191:
	if ( ++p == pe )
		goto _test_eof191;
case 191:
#line 2989 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st88;
	}
	goto st0;
st88:
	if ( ++p == pe )
		goto _test_eof88;
case 88:
	switch( (*p) ) {
		case 48u: goto tr118;
		case 49u: goto tr119;
		case 50u: goto tr120;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr121;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr122;
	} else
		goto tr122;
	goto st0;
tr225:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st192;
st192:
	if ( ++p == pe )
		goto _test_eof192;
case 192:
#line 3023 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st88;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr233;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr229;
	} else
		goto tr229;
	goto st0;
tr233:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st193;
st193:
	if ( ++p == pe )
		goto _test_eof193;
case 193:
#line 3048 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st88;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr234;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr231;
	} else
		goto tr231;
	goto st0;
tr234:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st194;
st194:
	if ( ++p == pe )
		goto _test_eof194;
case 194:
#line 3073 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st88;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr232;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr232;
	} else
		goto tr232;
	goto st0;
tr226:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st195;
st195:
	if ( ++p == pe )
		goto _test_eof195;
case 195:
#line 3098 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr235;
		case 58u: goto st88;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr233;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr229;
		} else if ( (*p) >= 65u )
			goto tr229;
	} else
		goto tr236;
	goto st0;
tr235:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st196;
st196:
	if ( ++p == pe )
		goto _test_eof196;
case 196:
#line 3127 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st88;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr234;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr231;
		} else if ( (*p) >= 65u )
			goto tr231;
	} else
		goto tr231;
	goto st0;
tr236:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st197;
st197:
	if ( ++p == pe )
		goto _test_eof197;
case 197:
#line 3155 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st88;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr231;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr231;
	} else
		goto tr231;
	goto st0;
tr227:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st198;
st198:
	if ( ++p == pe )
		goto _test_eof198;
case 198:
#line 3180 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st88;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr236;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr229;
	} else
		goto tr229;
	goto st0;
tr228:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st199;
st199:
	if ( ++p == pe )
		goto _test_eof199;
case 199:
#line 3205 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st88;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr229;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr229;
	} else
		goto tr229;
	goto st0;
tr29:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st200;
st200:
	if ( ++p == pe )
		goto _test_eof200;
case 200:
#line 3229 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 48u: goto tr237;
		case 49u: goto tr238;
		case 50u: goto tr239;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr240;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr241;
	} else
		goto tr241;
	goto st0;
tr237:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st201;
st201:
	if ( ++p == pe )
		goto _test_eof201;
case 201:
#line 3255 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st89;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr242;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr242;
	} else
		goto tr242;
	goto st0;
tr242:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st202;
st202:
	if ( ++p == pe )
		goto _test_eof202;
case 202:
#line 3280 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st89;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr244;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr244;
	} else
		goto tr244;
	goto st0;
tr244:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st203;
st203:
	if ( ++p == pe )
		goto _test_eof203;
case 203:
#line 3304 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st89;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr245;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr245;
	} else
		goto tr245;
	goto st0;
tr245:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st204;
st204:
	if ( ++p == pe )
		goto _test_eof204;
case 204:
#line 3328 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st89;
	}
	goto st0;
st89:
	if ( ++p == pe )
		goto _test_eof89;
case 89:
	switch( (*p) ) {
		case 48u: goto tr123;
		case 49u: goto tr124;
		case 50u: goto tr125;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr126;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr127;
	} else
		goto tr127;
	goto st0;
tr123:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st205;
st205:
	if ( ++p == pe )
		goto _test_eof205;
case 205:
#line 3362 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st90;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr246;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr246;
	} else
		goto tr246;
	goto st0;
tr246:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st206;
st206:
	if ( ++p == pe )
		goto _test_eof206;
case 206:
#line 3387 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st90;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr248;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr248;
	} else
		goto tr248;
	goto st0;
tr248:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st207;
st207:
	if ( ++p == pe )
		goto _test_eof207;
case 207:
#line 3411 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st90;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr249;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr249;
	} else
		goto tr249;
	goto st0;
tr249:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st208;
st208:
	if ( ++p == pe )
		goto _test_eof208;
case 208:
#line 3435 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st90;
	}
	goto st0;
st90:
	if ( ++p == pe )
		goto _test_eof90;
case 90:
	switch( (*p) ) {
		case 48u: goto tr128;
		case 49u: goto tr129;
		case 50u: goto tr130;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr131;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr132;
	} else
		goto tr132;
	goto st0;
tr128:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st209;
st209:
	if ( ++p == pe )
		goto _test_eof209;
case 209:
#line 3469 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st91;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr250;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr250;
	} else
		goto tr250;
	goto st0;
tr250:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st210;
st210:
	if ( ++p == pe )
		goto _test_eof210;
case 210:
#line 3494 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st91;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr252;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr252;
	} else
		goto tr252;
	goto st0;
tr252:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st211;
st211:
	if ( ++p == pe )
		goto _test_eof211;
case 211:
#line 3518 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st91;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr253;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr253;
	} else
		goto tr253;
	goto st0;
tr253:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st212;
st212:
	if ( ++p == pe )
		goto _test_eof212;
case 212:
#line 3542 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st91;
	}
	goto st0;
st91:
	if ( ++p == pe )
		goto _test_eof91;
case 91:
	switch( (*p) ) {
		case 48u: goto tr133;
		case 49u: goto tr134;
		case 50u: goto tr135;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr136;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr137;
	} else
		goto tr137;
	goto st0;
tr133:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st213;
st213:
	if ( ++p == pe )
		goto _test_eof213;
case 213:
#line 3576 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st92;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr254;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr254;
	} else
		goto tr254;
	goto st0;
tr254:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st214;
st214:
	if ( ++p == pe )
		goto _test_eof214;
case 214:
#line 3601 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st92;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr256;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr256;
	} else
		goto tr256;
	goto st0;
tr256:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st215;
st215:
	if ( ++p == pe )
		goto _test_eof215;
case 215:
#line 3625 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st92;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr257;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr257;
	} else
		goto tr257;
	goto st0;
tr257:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st216;
st216:
	if ( ++p == pe )
		goto _test_eof216;
case 216:
#line 3649 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st92;
	}
	goto st0;
st92:
	if ( ++p == pe )
		goto _test_eof92;
case 92:
	switch( (*p) ) {
		case 48u: goto tr138;
		case 49u: goto tr139;
		case 50u: goto tr140;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr141;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr142;
	} else
		goto tr142;
	goto st0;
tr138:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st217;
st217:
	if ( ++p == pe )
		goto _test_eof217;
case 217:
#line 3683 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr258;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr258;
	} else
		goto tr258;
	goto st0;
tr258:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st218;
st218:
	if ( ++p == pe )
		goto _test_eof218;
case 218:
#line 3708 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr260;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr260;
	} else
		goto tr260;
	goto st0;
tr260:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st219;
st219:
	if ( ++p == pe )
		goto _test_eof219;
case 219:
#line 3732 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr261;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr261;
	} else
		goto tr261;
	goto st0;
tr261:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st220;
st220:
	if ( ++p == pe )
		goto _test_eof220;
case 220:
#line 3756 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st93;
	}
	goto st0;
st93:
	if ( ++p == pe )
		goto _test_eof93;
case 93:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr85;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr85;
	} else
		goto tr85;
	goto st0;
tr139:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st221;
st221:
	if ( ++p == pe )
		goto _test_eof221;
case 221:
#line 3785 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr262;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr258;
	} else
		goto tr258;
	goto st0;
tr262:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st222;
st222:
	if ( ++p == pe )
		goto _test_eof222;
case 222:
#line 3810 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr263;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr260;
	} else
		goto tr260;
	goto st0;
tr263:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st223;
st223:
	if ( ++p == pe )
		goto _test_eof223;
case 223:
#line 3835 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr261;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr261;
	} else
		goto tr261;
	goto st0;
tr140:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st224;
st224:
	if ( ++p == pe )
		goto _test_eof224;
case 224:
#line 3860 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 53u: goto tr264;
		case 58u: goto st93;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr262;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr258;
		} else if ( (*p) >= 65u )
			goto tr258;
	} else
		goto tr265;
	goto st0;
tr264:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st225;
st225:
	if ( ++p == pe )
		goto _test_eof225;
case 225:
#line 3889 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr263;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr260;
		} else if ( (*p) >= 65u )
			goto tr260;
	} else
		goto tr260;
	goto st0;
tr265:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st226;
st226:
	if ( ++p == pe )
		goto _test_eof226;
case 226:
#line 3917 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr260;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr260;
	} else
		goto tr260;
	goto st0;
tr141:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st227;
st227:
	if ( ++p == pe )
		goto _test_eof227;
case 227:
#line 3942 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr265;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr258;
	} else
		goto tr258;
	goto st0;
tr142:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st228;
st228:
	if ( ++p == pe )
		goto _test_eof228;
case 228:
#line 3967 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr258;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr258;
	} else
		goto tr258;
	goto st0;
tr134:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st229;
st229:
	if ( ++p == pe )
		goto _test_eof229;
case 229:
#line 3991 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st92;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr266;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr254;
	} else
		goto tr254;
	goto st0;
tr266:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st230;
st230:
	if ( ++p == pe )
		goto _test_eof230;
case 230:
#line 4016 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st92;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr267;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr256;
	} else
		goto tr256;
	goto st0;
tr267:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st231;
st231:
	if ( ++p == pe )
		goto _test_eof231;
case 231:
#line 4041 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st92;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr257;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr257;
	} else
		goto tr257;
	goto st0;
tr135:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st232;
st232:
	if ( ++p == pe )
		goto _test_eof232;
case 232:
#line 4066 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr268;
		case 58u: goto st92;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr266;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr254;
		} else if ( (*p) >= 65u )
			goto tr254;
	} else
		goto tr269;
	goto st0;
tr268:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st233;
st233:
	if ( ++p == pe )
		goto _test_eof233;
case 233:
#line 4095 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st92;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr267;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr256;
		} else if ( (*p) >= 65u )
			goto tr256;
	} else
		goto tr256;
	goto st0;
tr269:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st234;
st234:
	if ( ++p == pe )
		goto _test_eof234;
case 234:
#line 4123 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st92;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr256;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr256;
	} else
		goto tr256;
	goto st0;
tr136:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st235;
st235:
	if ( ++p == pe )
		goto _test_eof235;
case 235:
#line 4148 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st92;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr269;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr254;
	} else
		goto tr254;
	goto st0;
tr137:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st236;
st236:
	if ( ++p == pe )
		goto _test_eof236;
case 236:
#line 4173 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st92;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr254;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr254;
	} else
		goto tr254;
	goto st0;
tr129:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st237;
st237:
	if ( ++p == pe )
		goto _test_eof237;
case 237:
#line 4197 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st91;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr270;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr250;
	} else
		goto tr250;
	goto st0;
tr270:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st238;
st238:
	if ( ++p == pe )
		goto _test_eof238;
case 238:
#line 4222 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st91;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr271;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr252;
	} else
		goto tr252;
	goto st0;
tr271:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st239;
st239:
	if ( ++p == pe )
		goto _test_eof239;
case 239:
#line 4247 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st91;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr253;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr253;
	} else
		goto tr253;
	goto st0;
tr130:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st240;
st240:
	if ( ++p == pe )
		goto _test_eof240;
case 240:
#line 4272 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr272;
		case 58u: goto st91;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr270;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr250;
		} else if ( (*p) >= 65u )
			goto tr250;
	} else
		goto tr273;
	goto st0;
tr272:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st241;
st241:
	if ( ++p == pe )
		goto _test_eof241;
case 241:
#line 4301 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st91;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr271;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr252;
		} else if ( (*p) >= 65u )
			goto tr252;
	} else
		goto tr252;
	goto st0;
tr273:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st242;
st242:
	if ( ++p == pe )
		goto _test_eof242;
case 242:
#line 4329 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st91;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr252;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr252;
	} else
		goto tr252;
	goto st0;
tr131:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st243;
st243:
	if ( ++p == pe )
		goto _test_eof243;
case 243:
#line 4354 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st91;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr273;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr250;
	} else
		goto tr250;
	goto st0;
tr132:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st244;
st244:
	if ( ++p == pe )
		goto _test_eof244;
case 244:
#line 4379 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st91;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr250;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr250;
	} else
		goto tr250;
	goto st0;
tr124:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st245;
st245:
	if ( ++p == pe )
		goto _test_eof245;
case 245:
#line 4403 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st90;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr274;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr246;
	} else
		goto tr246;
	goto st0;
tr274:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st246;
st246:
	if ( ++p == pe )
		goto _test_eof246;
case 246:
#line 4428 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st90;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr275;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr248;
	} else
		goto tr248;
	goto st0;
tr275:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st247;
st247:
	if ( ++p == pe )
		goto _test_eof247;
case 247:
#line 4453 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st90;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr249;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr249;
	} else
		goto tr249;
	goto st0;
tr125:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st248;
st248:
	if ( ++p == pe )
		goto _test_eof248;
case 248:
#line 4478 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr276;
		case 58u: goto st90;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr274;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr246;
		} else if ( (*p) >= 65u )
			goto tr246;
	} else
		goto tr277;
	goto st0;
tr276:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st249;
st249:
	if ( ++p == pe )
		goto _test_eof249;
case 249:
#line 4507 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st90;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr275;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr248;
		} else if ( (*p) >= 65u )
			goto tr248;
	} else
		goto tr248;
	goto st0;
tr277:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st250;
st250:
	if ( ++p == pe )
		goto _test_eof250;
case 250:
#line 4535 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st90;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr248;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr248;
	} else
		goto tr248;
	goto st0;
tr126:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st251;
st251:
	if ( ++p == pe )
		goto _test_eof251;
case 251:
#line 4560 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st90;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr277;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr246;
	} else
		goto tr246;
	goto st0;
tr127:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st252;
st252:
	if ( ++p == pe )
		goto _test_eof252;
case 252:
#line 4585 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st90;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr246;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr246;
	} else
		goto tr246;
	goto st0;
tr238:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st253;
st253:
	if ( ++p == pe )
		goto _test_eof253;
case 253:
#line 4609 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st89;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr278;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr242;
	} else
		goto tr242;
	goto st0;
tr278:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st254;
st254:
	if ( ++p == pe )
		goto _test_eof254;
case 254:
#line 4634 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st89;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr279;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr244;
	} else
		goto tr244;
	goto st0;
tr279:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st255;
st255:
	if ( ++p == pe )
		goto _test_eof255;
case 255:
#line 4659 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st89;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr245;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr245;
	} else
		goto tr245;
	goto st0;
tr239:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st256;
st256:
	if ( ++p == pe )
		goto _test_eof256;
case 256:
#line 4684 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr280;
		case 58u: goto st89;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr278;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr242;
		} else if ( (*p) >= 65u )
			goto tr242;
	} else
		goto tr281;
	goto st0;
tr280:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st257;
st257:
	if ( ++p == pe )
		goto _test_eof257;
case 257:
#line 4713 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st89;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr279;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr244;
		} else if ( (*p) >= 65u )
			goto tr244;
	} else
		goto tr244;
	goto st0;
tr281:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st258;
st258:
	if ( ++p == pe )
		goto _test_eof258;
case 258:
#line 4741 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st89;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr244;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr244;
	} else
		goto tr244;
	goto st0;
tr240:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st259;
st259:
	if ( ++p == pe )
		goto _test_eof259;
case 259:
#line 4766 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st89;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr281;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr242;
	} else
		goto tr242;
	goto st0;
tr241:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st260;
st260:
	if ( ++p == pe )
		goto _test_eof260;
case 260:
#line 4791 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st89;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr242;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr242;
	} else
		goto tr242;
	goto st0;
st94:
	if ( ++p == pe )
		goto _test_eof94;
case 94:
	switch( (*p) ) {
		case 46u: goto st3;
		case 58u: goto st19;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st95;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st16;
	} else
		goto st16;
	goto st0;
st95:
	if ( ++p == pe )
		goto _test_eof95;
case 95:
	switch( (*p) ) {
		case 46u: goto st3;
		case 58u: goto st19;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st96;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st17;
	} else
		goto st17;
	goto st0;
st96:
	if ( ++p == pe )
		goto _test_eof96;
case 96:
	switch( (*p) ) {
		case 46u: goto st3;
		case 58u: goto st19;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st18;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st18;
	} else
		goto st18;
	goto st0;
st97:
	if ( ++p == pe )
		goto _test_eof97;
case 97:
	switch( (*p) ) {
		case 46u: goto st3;
		case 53u: goto st98;
		case 58u: goto st19;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto st95;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto st16;
		} else if ( (*p) >= 65u )
			goto st16;
	} else
		goto st99;
	goto st0;
st98:
	if ( ++p == pe )
		goto _test_eof98;
case 98:
	switch( (*p) ) {
		case 46u: goto st3;
		case 58u: goto st19;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto st96;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto st17;
		} else if ( (*p) >= 65u )
			goto st17;
	} else
		goto st17;
	goto st0;
st99:
	if ( ++p == pe )
		goto _test_eof99;
case 99:
	switch( (*p) ) {
		case 46u: goto st3;
		case 58u: goto st19;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st17;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st17;
	} else
		goto st17;
	goto st0;
st100:
	if ( ++p == pe )
		goto _test_eof100;
case 100:
	switch( (*p) ) {
		case 46u: goto st3;
		case 58u: goto st19;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st99;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st16;
	} else
		goto st16;
	goto st0;
st101:
	if ( ++p == pe )
		goto _test_eof101;
case 101:
	if ( (*p) == 58u )
		goto tr147;
	goto st0;
tr147:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st261;
st261:
	if ( ++p == pe )
		goto _test_eof261;
case 261:
#line 4948 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 48u: goto tr282;
		case 49u: goto tr283;
		case 50u: goto tr284;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr285;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr286;
	} else
		goto tr286;
	goto st0;
tr282:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st262;
st262:
	if ( ++p == pe )
		goto _test_eof262;
case 262:
#line 4974 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st102;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr287;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr287;
	} else
		goto tr287;
	goto st0;
tr287:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st263;
st263:
	if ( ++p == pe )
		goto _test_eof263;
case 263:
#line 4999 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st102;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr289;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr289;
	} else
		goto tr289;
	goto st0;
tr289:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st264;
st264:
	if ( ++p == pe )
		goto _test_eof264;
case 264:
#line 5023 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st102;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr290;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr290;
	} else
		goto tr290;
	goto st0;
tr290:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st265;
st265:
	if ( ++p == pe )
		goto _test_eof265;
case 265:
#line 5047 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st102;
	}
	goto st0;
st102:
	if ( ++p == pe )
		goto _test_eof102;
case 102:
	switch( (*p) ) {
		case 48u: goto tr148;
		case 49u: goto tr149;
		case 50u: goto tr150;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr151;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr152;
	} else
		goto tr152;
	goto st0;
tr148:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st266;
st266:
	if ( ++p == pe )
		goto _test_eof266;
case 266:
#line 5081 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st103;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr291;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr291;
	} else
		goto tr291;
	goto st0;
tr291:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st267;
st267:
	if ( ++p == pe )
		goto _test_eof267;
case 267:
#line 5106 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st103;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr293;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr293;
	} else
		goto tr293;
	goto st0;
tr293:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st268;
st268:
	if ( ++p == pe )
		goto _test_eof268;
case 268:
#line 5130 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st103;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr294;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr294;
	} else
		goto tr294;
	goto st0;
tr294:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st269;
st269:
	if ( ++p == pe )
		goto _test_eof269;
case 269:
#line 5154 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st103;
	}
	goto st0;
st103:
	if ( ++p == pe )
		goto _test_eof103;
case 103:
	switch( (*p) ) {
		case 48u: goto tr153;
		case 49u: goto tr154;
		case 50u: goto tr155;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr156;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr157;
	} else
		goto tr157;
	goto st0;
tr153:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st270;
st270:
	if ( ++p == pe )
		goto _test_eof270;
case 270:
#line 5188 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st104;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr295;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr295;
	} else
		goto tr295;
	goto st0;
tr295:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st271;
st271:
	if ( ++p == pe )
		goto _test_eof271;
case 271:
#line 5213 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st104;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr297;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr297;
	} else
		goto tr297;
	goto st0;
tr297:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st272;
st272:
	if ( ++p == pe )
		goto _test_eof272;
case 272:
#line 5237 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st104;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr298;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr298;
	} else
		goto tr298;
	goto st0;
tr298:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st273;
st273:
	if ( ++p == pe )
		goto _test_eof273;
case 273:
#line 5261 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st104;
	}
	goto st0;
st104:
	if ( ++p == pe )
		goto _test_eof104;
case 104:
	switch( (*p) ) {
		case 48u: goto tr158;
		case 49u: goto tr159;
		case 50u: goto tr160;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr161;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr162;
	} else
		goto tr162;
	goto st0;
tr158:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st274;
st274:
	if ( ++p == pe )
		goto _test_eof274;
case 274:
#line 5295 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st105;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr299;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr299;
	} else
		goto tr299;
	goto st0;
tr299:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st275;
st275:
	if ( ++p == pe )
		goto _test_eof275;
case 275:
#line 5320 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st105;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr301;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr301;
	} else
		goto tr301;
	goto st0;
tr301:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st276;
st276:
	if ( ++p == pe )
		goto _test_eof276;
case 276:
#line 5344 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st105;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr302;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr302;
	} else
		goto tr302;
	goto st0;
tr302:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st277;
st277:
	if ( ++p == pe )
		goto _test_eof277;
case 277:
#line 5368 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st105;
	}
	goto st0;
st105:
	if ( ++p == pe )
		goto _test_eof105;
case 105:
	switch( (*p) ) {
		case 48u: goto tr163;
		case 49u: goto tr164;
		case 50u: goto tr165;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr166;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr167;
	} else
		goto tr167;
	goto st0;
tr163:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st278;
st278:
	if ( ++p == pe )
		goto _test_eof278;
case 278:
#line 5402 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st106;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr303;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr303;
	} else
		goto tr303;
	goto st0;
tr303:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st279;
st279:
	if ( ++p == pe )
		goto _test_eof279;
case 279:
#line 5427 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st106;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr305;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr305;
	} else
		goto tr305;
	goto st0;
tr305:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st280;
st280:
	if ( ++p == pe )
		goto _test_eof280;
case 280:
#line 5451 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st106;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr306;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr306;
	} else
		goto tr306;
	goto st0;
tr306:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st281;
st281:
	if ( ++p == pe )
		goto _test_eof281;
case 281:
#line 5475 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st106;
	}
	goto st0;
st106:
	if ( ++p == pe )
		goto _test_eof106;
case 106:
	switch( (*p) ) {
		case 48u: goto tr168;
		case 49u: goto tr169;
		case 50u: goto tr170;
	}
	if ( (*p) < 65u ) {
		if ( 51u <= (*p) && (*p) <= 57u )
			goto tr171;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr172;
	} else
		goto tr172;
	goto st0;
tr168:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st282;
st282:
	if ( ++p == pe )
		goto _test_eof282;
case 282:
#line 5509 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr307;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr307;
	} else
		goto tr307;
	goto st0;
tr307:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st283;
st283:
	if ( ++p == pe )
		goto _test_eof283;
case 283:
#line 5533 "src/Utils/IP.cpp"
	if ( (*p) == 58u )
		goto st93;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr308;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr308;
	} else
		goto tr308;
	goto st0;
tr308:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st284;
st284:
	if ( ++p == pe )
		goto _test_eof284;
case 284:
#line 5555 "src/Utils/IP.cpp"
	if ( (*p) == 58u )
		goto st93;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr309;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr309;
	} else
		goto tr309;
	goto st0;
tr309:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st285;
st285:
	if ( ++p == pe )
		goto _test_eof285;
case 285:
#line 5577 "src/Utils/IP.cpp"
	if ( (*p) == 58u )
		goto st93;
	goto st0;
tr169:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st286;
st286:
	if ( ++p == pe )
		goto _test_eof286;
case 286:
#line 5591 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr310;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr307;
	} else
		goto tr307;
	goto st0;
tr310:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st287;
st287:
	if ( ++p == pe )
		goto _test_eof287;
case 287:
#line 5615 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr311;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr308;
	} else
		goto tr308;
	goto st0;
tr311:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st288;
st288:
	if ( ++p == pe )
		goto _test_eof288;
case 288:
#line 5639 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr309;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr309;
	} else
		goto tr309;
	goto st0;
tr170:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st289;
st289:
	if ( ++p == pe )
		goto _test_eof289;
case 289:
#line 5663 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 46u: goto st46;
		case 53u: goto tr312;
		case 58u: goto st93;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr310;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr307;
		} else if ( (*p) >= 65u )
			goto tr307;
	} else
		goto tr313;
	goto st0;
tr312:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st290;
st290:
	if ( ++p == pe )
		goto _test_eof290;
case 290:
#line 5691 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr311;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr308;
		} else if ( (*p) >= 65u )
			goto tr308;
	} else
		goto tr308;
	goto st0;
tr313:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st291;
st291:
	if ( ++p == pe )
		goto _test_eof291;
case 291:
#line 5718 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr308;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr308;
	} else
		goto tr308;
	goto st0;
tr171:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st292;
st292:
	if ( ++p == pe )
		goto _test_eof292;
case 292:
#line 5742 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 46u: goto st46;
		case 58u: goto st93;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr313;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr307;
	} else
		goto tr307;
	goto st0;
tr172:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st293;
st293:
	if ( ++p == pe )
		goto _test_eof293;
case 293:
#line 5766 "src/Utils/IP.cpp"
	if ( (*p) == 58u )
		goto st93;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr307;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr307;
	} else
		goto tr307;
	goto st0;
tr164:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st294;
st294:
	if ( ++p == pe )
		goto _test_eof294;
case 294:
#line 5788 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st106;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr314;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr303;
	} else
		goto tr303;
	goto st0;
tr314:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st295;
st295:
	if ( ++p == pe )
		goto _test_eof295;
case 295:
#line 5813 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st106;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr315;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr305;
	} else
		goto tr305;
	goto st0;
tr315:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st296;
st296:
	if ( ++p == pe )
		goto _test_eof296;
case 296:
#line 5838 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st106;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr306;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr306;
	} else
		goto tr306;
	goto st0;
tr165:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st297;
st297:
	if ( ++p == pe )
		goto _test_eof297;
case 297:
#line 5863 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 53u: goto tr316;
		case 58u: goto st106;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr314;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr303;
		} else if ( (*p) >= 65u )
			goto tr303;
	} else
		goto tr317;
	goto st0;
tr316:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st298;
st298:
	if ( ++p == pe )
		goto _test_eof298;
case 298:
#line 5892 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st106;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr315;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr305;
		} else if ( (*p) >= 65u )
			goto tr305;
	} else
		goto tr305;
	goto st0;
tr317:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st299;
st299:
	if ( ++p == pe )
		goto _test_eof299;
case 299:
#line 5920 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st106;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr305;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr305;
	} else
		goto tr305;
	goto st0;
tr166:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st300;
st300:
	if ( ++p == pe )
		goto _test_eof300;
case 300:
#line 5945 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st46;
		case 58u: goto st106;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr317;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr303;
	} else
		goto tr303;
	goto st0;
tr167:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st301;
st301:
	if ( ++p == pe )
		goto _test_eof301;
case 301:
#line 5970 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st106;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr303;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr303;
	} else
		goto tr303;
	goto st0;
tr159:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st302;
st302:
	if ( ++p == pe )
		goto _test_eof302;
case 302:
#line 5994 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st105;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr318;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr299;
	} else
		goto tr299;
	goto st0;
tr318:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st303;
st303:
	if ( ++p == pe )
		goto _test_eof303;
case 303:
#line 6019 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st105;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr319;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr301;
	} else
		goto tr301;
	goto st0;
tr319:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st304;
st304:
	if ( ++p == pe )
		goto _test_eof304;
case 304:
#line 6044 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st105;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr302;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr302;
	} else
		goto tr302;
	goto st0;
tr160:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st305;
st305:
	if ( ++p == pe )
		goto _test_eof305;
case 305:
#line 6069 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr320;
		case 58u: goto st105;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr318;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr299;
		} else if ( (*p) >= 65u )
			goto tr299;
	} else
		goto tr321;
	goto st0;
tr320:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st306;
st306:
	if ( ++p == pe )
		goto _test_eof306;
case 306:
#line 6098 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st105;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr319;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr301;
		} else if ( (*p) >= 65u )
			goto tr301;
	} else
		goto tr301;
	goto st0;
tr321:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st307;
st307:
	if ( ++p == pe )
		goto _test_eof307;
case 307:
#line 6126 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st105;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr301;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr301;
	} else
		goto tr301;
	goto st0;
tr161:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st308;
st308:
	if ( ++p == pe )
		goto _test_eof308;
case 308:
#line 6151 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st105;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr321;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr299;
	} else
		goto tr299;
	goto st0;
tr162:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st309;
st309:
	if ( ++p == pe )
		goto _test_eof309;
case 309:
#line 6176 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st105;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr299;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr299;
	} else
		goto tr299;
	goto st0;
tr154:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st310;
st310:
	if ( ++p == pe )
		goto _test_eof310;
case 310:
#line 6200 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st104;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr322;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr295;
	} else
		goto tr295;
	goto st0;
tr322:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st311;
st311:
	if ( ++p == pe )
		goto _test_eof311;
case 311:
#line 6225 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st104;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr323;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr297;
	} else
		goto tr297;
	goto st0;
tr323:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st312;
st312:
	if ( ++p == pe )
		goto _test_eof312;
case 312:
#line 6250 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st104;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr298;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr298;
	} else
		goto tr298;
	goto st0;
tr155:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st313;
st313:
	if ( ++p == pe )
		goto _test_eof313;
case 313:
#line 6275 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr324;
		case 58u: goto st104;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr322;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr295;
		} else if ( (*p) >= 65u )
			goto tr295;
	} else
		goto tr325;
	goto st0;
tr324:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st314;
st314:
	if ( ++p == pe )
		goto _test_eof314;
case 314:
#line 6304 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st104;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr323;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr297;
		} else if ( (*p) >= 65u )
			goto tr297;
	} else
		goto tr297;
	goto st0;
tr325:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st315;
st315:
	if ( ++p == pe )
		goto _test_eof315;
case 315:
#line 6332 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st104;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr297;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr297;
	} else
		goto tr297;
	goto st0;
tr156:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st316;
st316:
	if ( ++p == pe )
		goto _test_eof316;
case 316:
#line 6357 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st104;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr325;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr295;
	} else
		goto tr295;
	goto st0;
tr157:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st317;
st317:
	if ( ++p == pe )
		goto _test_eof317;
case 317:
#line 6382 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st104;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr295;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr295;
	} else
		goto tr295;
	goto st0;
tr149:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st318;
st318:
	if ( ++p == pe )
		goto _test_eof318;
case 318:
#line 6406 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st103;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr326;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr291;
	} else
		goto tr291;
	goto st0;
tr326:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st319;
st319:
	if ( ++p == pe )
		goto _test_eof319;
case 319:
#line 6431 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st103;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr327;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr293;
	} else
		goto tr293;
	goto st0;
tr327:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st320;
st320:
	if ( ++p == pe )
		goto _test_eof320;
case 320:
#line 6456 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st103;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr294;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr294;
	} else
		goto tr294;
	goto st0;
tr150:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st321;
st321:
	if ( ++p == pe )
		goto _test_eof321;
case 321:
#line 6481 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr328;
		case 58u: goto st103;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr326;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr291;
		} else if ( (*p) >= 65u )
			goto tr291;
	} else
		goto tr329;
	goto st0;
tr328:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st322;
st322:
	if ( ++p == pe )
		goto _test_eof322;
case 322:
#line 6510 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st103;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr327;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr293;
		} else if ( (*p) >= 65u )
			goto tr293;
	} else
		goto tr293;
	goto st0;
tr329:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st323;
st323:
	if ( ++p == pe )
		goto _test_eof323;
case 323:
#line 6538 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st103;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr293;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr293;
	} else
		goto tr293;
	goto st0;
tr151:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st324;
st324:
	if ( ++p == pe )
		goto _test_eof324;
case 324:
#line 6563 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st103;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr329;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr291;
	} else
		goto tr291;
	goto st0;
tr152:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st325;
st325:
	if ( ++p == pe )
		goto _test_eof325;
case 325:
#line 6588 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st103;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr291;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr291;
	} else
		goto tr291;
	goto st0;
tr283:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st326;
st326:
	if ( ++p == pe )
		goto _test_eof326;
case 326:
#line 6612 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st102;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr330;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr287;
	} else
		goto tr287;
	goto st0;
tr330:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st327;
st327:
	if ( ++p == pe )
		goto _test_eof327;
case 327:
#line 6637 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st102;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr331;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr289;
	} else
		goto tr289;
	goto st0;
tr331:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st328;
st328:
	if ( ++p == pe )
		goto _test_eof328;
case 328:
#line 6662 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st102;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr290;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr290;
	} else
		goto tr290;
	goto st0;
tr284:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st329;
st329:
	if ( ++p == pe )
		goto _test_eof329;
case 329:
#line 6687 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 53u: goto tr332;
		case 58u: goto st102;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 52u )
			goto tr330;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr287;
		} else if ( (*p) >= 65u )
			goto tr287;
	} else
		goto tr333;
	goto st0;
tr332:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st330;
st330:
	if ( ++p == pe )
		goto _test_eof330;
case 330:
#line 6716 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st102;
	}
	if ( (*p) < 54u ) {
		if ( 48u <= (*p) && (*p) <= 53u )
			goto tr331;
	} else if ( (*p) > 57u ) {
		if ( (*p) > 70u ) {
			if ( 97u <= (*p) && (*p) <= 102u )
				goto tr289;
		} else if ( (*p) >= 65u )
			goto tr289;
	} else
		goto tr289;
	goto st0;
tr333:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st331;
st331:
	if ( ++p == pe )
		goto _test_eof331;
case 331:
#line 6744 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st102;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr289;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr289;
	} else
		goto tr289;
	goto st0;
tr285:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st332;
st332:
	if ( ++p == pe )
		goto _test_eof332;
case 332:
#line 6769 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 46u: goto st72;
		case 58u: goto st102;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr333;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr287;
	} else
		goto tr287;
	goto st0;
tr286:
#line 30 "src/Utils/IP.rl"
	{
				ipFamily = AF_INET6;
			}
	goto st333;
st333:
	if ( ++p == pe )
		goto _test_eof333;
case 333:
#line 6794 "src/Utils/IP.cpp"
	switch( (*p) ) {
		case 37u: goto st63;
		case 58u: goto st102;
	}
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr287;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr287;
	} else
		goto tr287;
	goto st0;
st107:
	if ( ++p == pe )
		goto _test_eof107;
case 107:
	if ( (*p) == 58u )
		goto st19;
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto st16;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto st16;
	} else
		goto st16;
	goto st0;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof108: cs = 108; goto _test_eof; 
	_test_eof109: cs = 109; goto _test_eof; 
	_test_eof110: cs = 110; goto _test_eof; 
	_test_eof111: cs = 111; goto _test_eof; 
	_test_eof112: cs = 112; goto _test_eof; 
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
	_test_eof48: cs = 48; goto _test_eof; 
	_test_eof49: cs = 49; goto _test_eof; 
	_test_eof50: cs = 50; goto _test_eof; 
	_test_eof113: cs = 113; goto _test_eof; 
	_test_eof114: cs = 114; goto _test_eof; 
	_test_eof115: cs = 115; goto _test_eof; 
	_test_eof116: cs = 116; goto _test_eof; 
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
	_test_eof117: cs = 117; goto _test_eof; 
	_test_eof118: cs = 118; goto _test_eof; 
	_test_eof119: cs = 119; goto _test_eof; 
	_test_eof120: cs = 120; goto _test_eof; 
	_test_eof63: cs = 63; goto _test_eof; 
	_test_eof121: cs = 121; goto _test_eof; 
	_test_eof122: cs = 122; goto _test_eof; 
	_test_eof123: cs = 123; goto _test_eof; 
	_test_eof124: cs = 124; goto _test_eof; 
	_test_eof125: cs = 125; goto _test_eof; 
	_test_eof126: cs = 126; goto _test_eof; 
	_test_eof127: cs = 127; goto _test_eof; 
	_test_eof128: cs = 128; goto _test_eof; 
	_test_eof129: cs = 129; goto _test_eof; 
	_test_eof130: cs = 130; goto _test_eof; 
	_test_eof131: cs = 131; goto _test_eof; 
	_test_eof132: cs = 132; goto _test_eof; 
	_test_eof133: cs = 133; goto _test_eof; 
	_test_eof134: cs = 134; goto _test_eof; 
	_test_eof135: cs = 135; goto _test_eof; 
	_test_eof136: cs = 136; goto _test_eof; 
	_test_eof137: cs = 137; goto _test_eof; 
	_test_eof138: cs = 138; goto _test_eof; 
	_test_eof139: cs = 139; goto _test_eof; 
	_test_eof64: cs = 64; goto _test_eof; 
	_test_eof65: cs = 65; goto _test_eof; 
	_test_eof66: cs = 66; goto _test_eof; 
	_test_eof67: cs = 67; goto _test_eof; 
	_test_eof68: cs = 68; goto _test_eof; 
	_test_eof69: cs = 69; goto _test_eof; 
	_test_eof70: cs = 70; goto _test_eof; 
	_test_eof140: cs = 140; goto _test_eof; 
	_test_eof141: cs = 141; goto _test_eof; 
	_test_eof142: cs = 142; goto _test_eof; 
	_test_eof143: cs = 143; goto _test_eof; 
	_test_eof71: cs = 71; goto _test_eof; 
	_test_eof144: cs = 144; goto _test_eof; 
	_test_eof145: cs = 145; goto _test_eof; 
	_test_eof72: cs = 72; goto _test_eof; 
	_test_eof73: cs = 73; goto _test_eof; 
	_test_eof74: cs = 74; goto _test_eof; 
	_test_eof75: cs = 75; goto _test_eof; 
	_test_eof76: cs = 76; goto _test_eof; 
	_test_eof146: cs = 146; goto _test_eof; 
	_test_eof147: cs = 147; goto _test_eof; 
	_test_eof148: cs = 148; goto _test_eof; 
	_test_eof149: cs = 149; goto _test_eof; 
	_test_eof77: cs = 77; goto _test_eof; 
	_test_eof78: cs = 78; goto _test_eof; 
	_test_eof79: cs = 79; goto _test_eof; 
	_test_eof80: cs = 80; goto _test_eof; 
	_test_eof81: cs = 81; goto _test_eof; 
	_test_eof82: cs = 82; goto _test_eof; 
	_test_eof83: cs = 83; goto _test_eof; 
	_test_eof84: cs = 84; goto _test_eof; 
	_test_eof150: cs = 150; goto _test_eof; 
	_test_eof151: cs = 151; goto _test_eof; 
	_test_eof152: cs = 152; goto _test_eof; 
	_test_eof85: cs = 85; goto _test_eof; 
	_test_eof153: cs = 153; goto _test_eof; 
	_test_eof154: cs = 154; goto _test_eof; 
	_test_eof155: cs = 155; goto _test_eof; 
	_test_eof156: cs = 156; goto _test_eof; 
	_test_eof157: cs = 157; goto _test_eof; 
	_test_eof158: cs = 158; goto _test_eof; 
	_test_eof159: cs = 159; goto _test_eof; 
	_test_eof160: cs = 160; goto _test_eof; 
	_test_eof161: cs = 161; goto _test_eof; 
	_test_eof162: cs = 162; goto _test_eof; 
	_test_eof163: cs = 163; goto _test_eof; 
	_test_eof164: cs = 164; goto _test_eof; 
	_test_eof165: cs = 165; goto _test_eof; 
	_test_eof86: cs = 86; goto _test_eof; 
	_test_eof166: cs = 166; goto _test_eof; 
	_test_eof167: cs = 167; goto _test_eof; 
	_test_eof168: cs = 168; goto _test_eof; 
	_test_eof169: cs = 169; goto _test_eof; 
	_test_eof170: cs = 170; goto _test_eof; 
	_test_eof171: cs = 171; goto _test_eof; 
	_test_eof172: cs = 172; goto _test_eof; 
	_test_eof173: cs = 173; goto _test_eof; 
	_test_eof174: cs = 174; goto _test_eof; 
	_test_eof175: cs = 175; goto _test_eof; 
	_test_eof176: cs = 176; goto _test_eof; 
	_test_eof177: cs = 177; goto _test_eof; 
	_test_eof178: cs = 178; goto _test_eof; 
	_test_eof87: cs = 87; goto _test_eof; 
	_test_eof179: cs = 179; goto _test_eof; 
	_test_eof180: cs = 180; goto _test_eof; 
	_test_eof181: cs = 181; goto _test_eof; 
	_test_eof182: cs = 182; goto _test_eof; 
	_test_eof183: cs = 183; goto _test_eof; 
	_test_eof184: cs = 184; goto _test_eof; 
	_test_eof185: cs = 185; goto _test_eof; 
	_test_eof186: cs = 186; goto _test_eof; 
	_test_eof187: cs = 187; goto _test_eof; 
	_test_eof188: cs = 188; goto _test_eof; 
	_test_eof189: cs = 189; goto _test_eof; 
	_test_eof190: cs = 190; goto _test_eof; 
	_test_eof191: cs = 191; goto _test_eof; 
	_test_eof88: cs = 88; goto _test_eof; 
	_test_eof192: cs = 192; goto _test_eof; 
	_test_eof193: cs = 193; goto _test_eof; 
	_test_eof194: cs = 194; goto _test_eof; 
	_test_eof195: cs = 195; goto _test_eof; 
	_test_eof196: cs = 196; goto _test_eof; 
	_test_eof197: cs = 197; goto _test_eof; 
	_test_eof198: cs = 198; goto _test_eof; 
	_test_eof199: cs = 199; goto _test_eof; 
	_test_eof200: cs = 200; goto _test_eof; 
	_test_eof201: cs = 201; goto _test_eof; 
	_test_eof202: cs = 202; goto _test_eof; 
	_test_eof203: cs = 203; goto _test_eof; 
	_test_eof204: cs = 204; goto _test_eof; 
	_test_eof89: cs = 89; goto _test_eof; 
	_test_eof205: cs = 205; goto _test_eof; 
	_test_eof206: cs = 206; goto _test_eof; 
	_test_eof207: cs = 207; goto _test_eof; 
	_test_eof208: cs = 208; goto _test_eof; 
	_test_eof90: cs = 90; goto _test_eof; 
	_test_eof209: cs = 209; goto _test_eof; 
	_test_eof210: cs = 210; goto _test_eof; 
	_test_eof211: cs = 211; goto _test_eof; 
	_test_eof212: cs = 212; goto _test_eof; 
	_test_eof91: cs = 91; goto _test_eof; 
	_test_eof213: cs = 213; goto _test_eof; 
	_test_eof214: cs = 214; goto _test_eof; 
	_test_eof215: cs = 215; goto _test_eof; 
	_test_eof216: cs = 216; goto _test_eof; 
	_test_eof92: cs = 92; goto _test_eof; 
	_test_eof217: cs = 217; goto _test_eof; 
	_test_eof218: cs = 218; goto _test_eof; 
	_test_eof219: cs = 219; goto _test_eof; 
	_test_eof220: cs = 220; goto _test_eof; 
	_test_eof93: cs = 93; goto _test_eof; 
	_test_eof221: cs = 221; goto _test_eof; 
	_test_eof222: cs = 222; goto _test_eof; 
	_test_eof223: cs = 223; goto _test_eof; 
	_test_eof224: cs = 224; goto _test_eof; 
	_test_eof225: cs = 225; goto _test_eof; 
	_test_eof226: cs = 226; goto _test_eof; 
	_test_eof227: cs = 227; goto _test_eof; 
	_test_eof228: cs = 228; goto _test_eof; 
	_test_eof229: cs = 229; goto _test_eof; 
	_test_eof230: cs = 230; goto _test_eof; 
	_test_eof231: cs = 231; goto _test_eof; 
	_test_eof232: cs = 232; goto _test_eof; 
	_test_eof233: cs = 233; goto _test_eof; 
	_test_eof234: cs = 234; goto _test_eof; 
	_test_eof235: cs = 235; goto _test_eof; 
	_test_eof236: cs = 236; goto _test_eof; 
	_test_eof237: cs = 237; goto _test_eof; 
	_test_eof238: cs = 238; goto _test_eof; 
	_test_eof239: cs = 239; goto _test_eof; 
	_test_eof240: cs = 240; goto _test_eof; 
	_test_eof241: cs = 241; goto _test_eof; 
	_test_eof242: cs = 242; goto _test_eof; 
	_test_eof243: cs = 243; goto _test_eof; 
	_test_eof244: cs = 244; goto _test_eof; 
	_test_eof245: cs = 245; goto _test_eof; 
	_test_eof246: cs = 246; goto _test_eof; 
	_test_eof247: cs = 247; goto _test_eof; 
	_test_eof248: cs = 248; goto _test_eof; 
	_test_eof249: cs = 249; goto _test_eof; 
	_test_eof250: cs = 250; goto _test_eof; 
	_test_eof251: cs = 251; goto _test_eof; 
	_test_eof252: cs = 252; goto _test_eof; 
	_test_eof253: cs = 253; goto _test_eof; 
	_test_eof254: cs = 254; goto _test_eof; 
	_test_eof255: cs = 255; goto _test_eof; 
	_test_eof256: cs = 256; goto _test_eof; 
	_test_eof257: cs = 257; goto _test_eof; 
	_test_eof258: cs = 258; goto _test_eof; 
	_test_eof259: cs = 259; goto _test_eof; 
	_test_eof260: cs = 260; goto _test_eof; 
	_test_eof94: cs = 94; goto _test_eof; 
	_test_eof95: cs = 95; goto _test_eof; 
	_test_eof96: cs = 96; goto _test_eof; 
	_test_eof97: cs = 97; goto _test_eof; 
	_test_eof98: cs = 98; goto _test_eof; 
	_test_eof99: cs = 99; goto _test_eof; 
	_test_eof100: cs = 100; goto _test_eof; 
	_test_eof101: cs = 101; goto _test_eof; 
	_test_eof261: cs = 261; goto _test_eof; 
	_test_eof262: cs = 262; goto _test_eof; 
	_test_eof263: cs = 263; goto _test_eof; 
	_test_eof264: cs = 264; goto _test_eof; 
	_test_eof265: cs = 265; goto _test_eof; 
	_test_eof102: cs = 102; goto _test_eof; 
	_test_eof266: cs = 266; goto _test_eof; 
	_test_eof267: cs = 267; goto _test_eof; 
	_test_eof268: cs = 268; goto _test_eof; 
	_test_eof269: cs = 269; goto _test_eof; 
	_test_eof103: cs = 103; goto _test_eof; 
	_test_eof270: cs = 270; goto _test_eof; 
	_test_eof271: cs = 271; goto _test_eof; 
	_test_eof272: cs = 272; goto _test_eof; 
	_test_eof273: cs = 273; goto _test_eof; 
	_test_eof104: cs = 104; goto _test_eof; 
	_test_eof274: cs = 274; goto _test_eof; 
	_test_eof275: cs = 275; goto _test_eof; 
	_test_eof276: cs = 276; goto _test_eof; 
	_test_eof277: cs = 277; goto _test_eof; 
	_test_eof105: cs = 105; goto _test_eof; 
	_test_eof278: cs = 278; goto _test_eof; 
	_test_eof279: cs = 279; goto _test_eof; 
	_test_eof280: cs = 280; goto _test_eof; 
	_test_eof281: cs = 281; goto _test_eof; 
	_test_eof106: cs = 106; goto _test_eof; 
	_test_eof282: cs = 282; goto _test_eof; 
	_test_eof283: cs = 283; goto _test_eof; 
	_test_eof284: cs = 284; goto _test_eof; 
	_test_eof285: cs = 285; goto _test_eof; 
	_test_eof286: cs = 286; goto _test_eof; 
	_test_eof287: cs = 287; goto _test_eof; 
	_test_eof288: cs = 288; goto _test_eof; 
	_test_eof289: cs = 289; goto _test_eof; 
	_test_eof290: cs = 290; goto _test_eof; 
	_test_eof291: cs = 291; goto _test_eof; 
	_test_eof292: cs = 292; goto _test_eof; 
	_test_eof293: cs = 293; goto _test_eof; 
	_test_eof294: cs = 294; goto _test_eof; 
	_test_eof295: cs = 295; goto _test_eof; 
	_test_eof296: cs = 296; goto _test_eof; 
	_test_eof297: cs = 297; goto _test_eof; 
	_test_eof298: cs = 298; goto _test_eof; 
	_test_eof299: cs = 299; goto _test_eof; 
	_test_eof300: cs = 300; goto _test_eof; 
	_test_eof301: cs = 301; goto _test_eof; 
	_test_eof302: cs = 302; goto _test_eof; 
	_test_eof303: cs = 303; goto _test_eof; 
	_test_eof304: cs = 304; goto _test_eof; 
	_test_eof305: cs = 305; goto _test_eof; 
	_test_eof306: cs = 306; goto _test_eof; 
	_test_eof307: cs = 307; goto _test_eof; 
	_test_eof308: cs = 308; goto _test_eof; 
	_test_eof309: cs = 309; goto _test_eof; 
	_test_eof310: cs = 310; goto _test_eof; 
	_test_eof311: cs = 311; goto _test_eof; 
	_test_eof312: cs = 312; goto _test_eof; 
	_test_eof313: cs = 313; goto _test_eof; 
	_test_eof314: cs = 314; goto _test_eof; 
	_test_eof315: cs = 315; goto _test_eof; 
	_test_eof316: cs = 316; goto _test_eof; 
	_test_eof317: cs = 317; goto _test_eof; 
	_test_eof318: cs = 318; goto _test_eof; 
	_test_eof319: cs = 319; goto _test_eof; 
	_test_eof320: cs = 320; goto _test_eof; 
	_test_eof321: cs = 321; goto _test_eof; 
	_test_eof322: cs = 322; goto _test_eof; 
	_test_eof323: cs = 323; goto _test_eof; 
	_test_eof324: cs = 324; goto _test_eof; 
	_test_eof325: cs = 325; goto _test_eof; 
	_test_eof326: cs = 326; goto _test_eof; 
	_test_eof327: cs = 327; goto _test_eof; 
	_test_eof328: cs = 328; goto _test_eof; 
	_test_eof329: cs = 329; goto _test_eof; 
	_test_eof330: cs = 330; goto _test_eof; 
	_test_eof331: cs = 331; goto _test_eof; 
	_test_eof332: cs = 332; goto _test_eof; 
	_test_eof333: cs = 333; goto _test_eof; 
	_test_eof107: cs = 107; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 64 "src/Utils/IP.rl"

		// Ensure that the parsing has consumed all the given length.
		if (ipLen == (size_t)(p - (const unsigned char*)ip))
			return ipFamily;
		else
			return AF_UNSPEC;
	}

	void IP::GetAddressInfo(const struct sockaddr* addr, int* family, std::string &ip, uint16_t* port)
	{
		MS_TRACE();

		char ipBuffer[INET6_ADDRSTRLEN+1];
		int err;

		switch (addr->sa_family)
		{
			case AF_INET:
			{
				err = uv_inet_ntop(
					AF_INET, &((struct sockaddr_in*)addr)->sin_addr, ipBuffer, INET_ADDRSTRLEN);

				if (err)
					MS_ABORT("uv_inet_ntop() failed: %s", uv_strerror(err));

				*port = (uint16_t)ntohs(((struct sockaddr_in*)addr)->sin_port);

				break;
			}

			case AF_INET6:
			{
				err = uv_inet_ntop(
					AF_INET6, &((struct sockaddr_in6*)addr)->sin6_addr, ipBuffer, INET6_ADDRSTRLEN);

				if (err)
					MS_ABORT("uv_inet_ntop() failed: %s", uv_strerror(err));

				*port = (uint16_t)ntohs(((struct sockaddr_in6*)addr)->sin6_port);

				break;
			}

			default:
				MS_ABORT("unknown network family: %d", (int)addr->sa_family);
		}

		*family = addr->sa_family;
		ip.assign(ipBuffer);
	}
}
