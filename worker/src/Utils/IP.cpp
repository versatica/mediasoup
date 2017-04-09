
#line 1 "src/Utils/IP.rl"
#define MS_CLASS "Utils::IP"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "Utils.hpp"
#include <uv.h>

namespace Utils
{
	int IP::GetFamily(const char* ip, size_t ipLen)
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

		p  = (const unsigned char*)ip;
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
			if (p == pe)
				goto _test_eof;
			switch (cs)
			{
				case 1:
					switch ((*p))
					{
						case 48u:
							goto st2;
						case 49u:
							goto st76;
						case 50u:
							goto st79;
						case 58u:
							goto st83;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto st82;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st85;
					}
					else
						goto st85;
					goto st0;
				st0:
					cs = 0;
					goto _out;
				st2:
					if (++p == pe)
						goto _test_eof2;
				case 2:
					switch ((*p))
					{
						case 46u:
							goto st3;
						case 58u:
							goto st19;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st16;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st16;
					}
					else
						goto st16;
					goto st0;
				st3:
					if (++p == pe)
						goto _test_eof3;
				case 3:
					switch ((*p))
					{
						case 48u:
							goto st4;
						case 49u:
							goto st12;
						case 50u:
							goto st14;
					}
					if (51u <= (*p) && (*p) <= 57u)
						goto st13;
					goto st0;
				st4:
					if (++p == pe)
						goto _test_eof4;
				case 4:
					if ((*p) == 46u)
						goto st5;
					goto st0;
				st5:
					if (++p == pe)
						goto _test_eof5;
				case 5:
					switch ((*p))
					{
						case 48u:
							goto st6;
						case 49u:
							goto st8;
						case 50u:
							goto st10;
					}
					if (51u <= (*p) && (*p) <= 57u)
						goto st9;
					goto st0;
				st6:
					if (++p == pe)
						goto _test_eof6;
				case 6:
					if ((*p) == 46u)
						goto st7;
					goto st0;
				st7:
					if (++p == pe)
						goto _test_eof7;
				case 7:
					switch ((*p))
					{
						case 48u:
							goto tr20;
						case 49u:
							goto tr21;
						case 50u:
							goto tr22;
					}
					if (51u <= (*p) && (*p) <= 57u)
						goto tr23;
					goto st0;
				tr20:
#line 25 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET;
				}
					goto st86;
				tr77:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st86;
				st86:
					if (++p == pe)
						goto _test_eof86;
				case 86:
#line 173 "src/Utils/IP.cpp"
					goto st0;
				tr21:
#line 25 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET;
				}
					goto st87;
				st87:
					if (++p == pe)
						goto _test_eof87;
				case 87:
#line 185 "src/Utils/IP.cpp"
					if (48u <= (*p) && (*p) <= 57u)
						goto tr23;
					goto st0;
				tr23:
#line 25 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET;
				}
					goto st88;
				st88:
					if (++p == pe)
						goto _test_eof88;
				case 88:
#line 199 "src/Utils/IP.cpp"
					if (48u <= (*p) && (*p) <= 57u)
						goto tr20;
					goto st0;
				tr22:
#line 25 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET;
				}
					goto st89;
				st89:
					if (++p == pe)
						goto _test_eof89;
				case 89:
#line 213 "src/Utils/IP.cpp"
					if ((*p) == 53u)
						goto tr120;
					if ((*p) > 52u)
					{
						if (54u <= (*p) && (*p) <= 57u)
							goto tr20;
					}
					else if ((*p) >= 48u)
						goto tr23;
					goto st0;
				tr120:
#line 25 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET;
				}
					goto st90;
				st90:
					if (++p == pe)
						goto _test_eof90;
				case 90:
#line 232 "src/Utils/IP.cpp"
					if (48u <= (*p) && (*p) <= 53u)
						goto tr20;
					goto st0;
				st8:
					if (++p == pe)
						goto _test_eof8;
				case 8:
					if ((*p) == 46u)
						goto st7;
					if (48u <= (*p) && (*p) <= 57u)
						goto st9;
					goto st0;
				st9:
					if (++p == pe)
						goto _test_eof9;
				case 9:
					if ((*p) == 46u)
						goto st7;
					if (48u <= (*p) && (*p) <= 57u)
						goto st6;
					goto st0;
				st10:
					if (++p == pe)
						goto _test_eof10;
				case 10:
					switch ((*p))
					{
						case 46u:
							goto st7;
						case 53u:
							goto st11;
					}
					if ((*p) > 52u)
					{
						if (54u <= (*p) && (*p) <= 57u)
							goto st6;
					}
					else if ((*p) >= 48u)
						goto st9;
					goto st0;
				st11:
					if (++p == pe)
						goto _test_eof11;
				case 11:
					if ((*p) == 46u)
						goto st7;
					if (48u <= (*p) && (*p) <= 53u)
						goto st6;
					goto st0;
				st12:
					if (++p == pe)
						goto _test_eof12;
				case 12:
					if ((*p) == 46u)
						goto st5;
					if (48u <= (*p) && (*p) <= 57u)
						goto st13;
					goto st0;
				st13:
					if (++p == pe)
						goto _test_eof13;
				case 13:
					if ((*p) == 46u)
						goto st5;
					if (48u <= (*p) && (*p) <= 57u)
						goto st4;
					goto st0;
				st14:
					if (++p == pe)
						goto _test_eof14;
				case 14:
					switch ((*p))
					{
						case 46u:
							goto st5;
						case 53u:
							goto st15;
					}
					if ((*p) > 52u)
					{
						if (54u <= (*p) && (*p) <= 57u)
							goto st4;
					}
					else if ((*p) >= 48u)
						goto st13;
					goto st0;
				st15:
					if (++p == pe)
						goto _test_eof15;
				case 15:
					if ((*p) == 46u)
						goto st5;
					if (48u <= (*p) && (*p) <= 53u)
						goto st4;
					goto st0;
				st16:
					if (++p == pe)
						goto _test_eof16;
				case 16:
					if ((*p) == 58u)
						goto st19;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st17;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st17;
					}
					else
						goto st17;
					goto st0;
				st17:
					if (++p == pe)
						goto _test_eof17;
				case 17:
					if ((*p) == 58u)
						goto st19;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st18;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st18;
					}
					else
						goto st18;
					goto st0;
				st18:
					if (++p == pe)
						goto _test_eof18;
				case 18:
					if ((*p) == 58u)
						goto st19;
					goto st0;
				st19:
					if (++p == pe)
						goto _test_eof19;
				case 19:
					if ((*p) == 58u)
						goto tr29;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st20;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st20;
					}
					else
						goto st20;
					goto st0;
				st20:
					if (++p == pe)
						goto _test_eof20;
				case 20:
					if ((*p) == 58u)
						goto st24;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st21;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st21;
					}
					else
						goto st21;
					goto st0;
				st21:
					if (++p == pe)
						goto _test_eof21;
				case 21:
					if ((*p) == 58u)
						goto st24;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st22;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st22;
					}
					else
						goto st22;
					goto st0;
				st22:
					if (++p == pe)
						goto _test_eof22;
				case 22:
					if ((*p) == 58u)
						goto st24;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st23;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st23;
					}
					else
						goto st23;
					goto st0;
				st23:
					if (++p == pe)
						goto _test_eof23;
				case 23:
					if ((*p) == 58u)
						goto st24;
					goto st0;
				st24:
					if (++p == pe)
						goto _test_eof24;
				case 24:
					if ((*p) == 58u)
						goto tr35;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st25;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st25;
					}
					else
						goto st25;
					goto st0;
				st25:
					if (++p == pe)
						goto _test_eof25;
				case 25:
					if ((*p) == 58u)
						goto st29;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st26;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st26;
					}
					else
						goto st26;
					goto st0;
				st26:
					if (++p == pe)
						goto _test_eof26;
				case 26:
					if ((*p) == 58u)
						goto st29;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st27;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st27;
					}
					else
						goto st27;
					goto st0;
				st27:
					if (++p == pe)
						goto _test_eof27;
				case 27:
					if ((*p) == 58u)
						goto st29;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st28;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st28;
					}
					else
						goto st28;
					goto st0;
				st28:
					if (++p == pe)
						goto _test_eof28;
				case 28:
					if ((*p) == 58u)
						goto st29;
					goto st0;
				st29:
					if (++p == pe)
						goto _test_eof29;
				case 29:
					if ((*p) == 58u)
						goto tr41;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st30;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st30;
					}
					else
						goto st30;
					goto st0;
				st30:
					if (++p == pe)
						goto _test_eof30;
				case 30:
					if ((*p) == 58u)
						goto st34;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st31;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st31;
					}
					else
						goto st31;
					goto st0;
				st31:
					if (++p == pe)
						goto _test_eof31;
				case 31:
					if ((*p) == 58u)
						goto st34;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st32;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st32;
					}
					else
						goto st32;
					goto st0;
				st32:
					if (++p == pe)
						goto _test_eof32;
				case 32:
					if ((*p) == 58u)
						goto st34;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st33;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st33;
					}
					else
						goto st33;
					goto st0;
				st33:
					if (++p == pe)
						goto _test_eof33;
				case 33:
					if ((*p) == 58u)
						goto st34;
					goto st0;
				st34:
					if (++p == pe)
						goto _test_eof34;
				case 34:
					if ((*p) == 58u)
						goto tr47;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st35;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st35;
					}
					else
						goto st35;
					goto st0;
				st35:
					if (++p == pe)
						goto _test_eof35;
				case 35:
					if ((*p) == 58u)
						goto st39;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st36;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st36;
					}
					else
						goto st36;
					goto st0;
				st36:
					if (++p == pe)
						goto _test_eof36;
				case 36:
					if ((*p) == 58u)
						goto st39;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st37;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st37;
					}
					else
						goto st37;
					goto st0;
				st37:
					if (++p == pe)
						goto _test_eof37;
				case 37:
					if ((*p) == 58u)
						goto st39;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st38;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st38;
					}
					else
						goto st38;
					goto st0;
				st38:
					if (++p == pe)
						goto _test_eof38;
				case 38:
					if ((*p) == 58u)
						goto st39;
					goto st0;
				st39:
					if (++p == pe)
						goto _test_eof39;
				case 39:
					if ((*p) == 58u)
						goto tr53;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st40;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st40;
					}
					else
						goto st40;
					goto st0;
				st40:
					if (++p == pe)
						goto _test_eof40;
				case 40:
					if ((*p) == 58u)
						goto st44;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st41;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st41;
					}
					else
						goto st41;
					goto st0;
				st41:
					if (++p == pe)
						goto _test_eof41;
				case 41:
					if ((*p) == 58u)
						goto st44;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st42;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st42;
					}
					else
						goto st42;
					goto st0;
				st42:
					if (++p == pe)
						goto _test_eof42;
				case 42:
					if ((*p) == 58u)
						goto st44;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st43;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st43;
					}
					else
						goto st43;
					goto st0;
				st43:
					if (++p == pe)
						goto _test_eof43;
				case 43:
					if ((*p) == 58u)
						goto st44;
					goto st0;
				st44:
					if (++p == pe)
						goto _test_eof44;
				case 44:
					switch ((*p))
					{
						case 48u:
							goto st45;
						case 49u:
							goto st63;
						case 50u:
							goto st66;
						case 58u:
							goto tr62;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto st69;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st70;
					}
					else
						goto st70;
					goto st0;
				st45:
					if (++p == pe)
						goto _test_eof45;
				case 45:
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st62;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st59;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st59;
					}
					else
						goto st59;
					goto st0;
				st46:
					if (++p == pe)
						goto _test_eof46;
				case 46:
					switch ((*p))
					{
						case 48u:
							goto st47;
						case 49u:
							goto st55;
						case 50u:
							goto st57;
					}
					if (51u <= (*p) && (*p) <= 57u)
						goto st56;
					goto st0;
				st47:
					if (++p == pe)
						goto _test_eof47;
				case 47:
					if ((*p) == 46u)
						goto st48;
					goto st0;
				st48:
					if (++p == pe)
						goto _test_eof48;
				case 48:
					switch ((*p))
					{
						case 48u:
							goto st49;
						case 49u:
							goto st51;
						case 50u:
							goto st53;
					}
					if (51u <= (*p) && (*p) <= 57u)
						goto st52;
					goto st0;
				st49:
					if (++p == pe)
						goto _test_eof49;
				case 49:
					if ((*p) == 46u)
						goto st50;
					goto st0;
				st50:
					if (++p == pe)
						goto _test_eof50;
				case 50:
					switch ((*p))
					{
						case 48u:
							goto tr77;
						case 49u:
							goto tr78;
						case 50u:
							goto tr79;
					}
					if (51u <= (*p) && (*p) <= 57u)
						goto tr80;
					goto st0;
				tr78:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st91;
				st91:
					if (++p == pe)
						goto _test_eof91;
				case 91:
#line 786 "src/Utils/IP.cpp"
					if (48u <= (*p) && (*p) <= 57u)
						goto tr80;
					goto st0;
				tr80:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st92;
				st92:
					if (++p == pe)
						goto _test_eof92;
				case 92:
#line 800 "src/Utils/IP.cpp"
					if (48u <= (*p) && (*p) <= 57u)
						goto tr77;
					goto st0;
				tr79:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st93;
				st93:
					if (++p == pe)
						goto _test_eof93;
				case 93:
#line 814 "src/Utils/IP.cpp"
					if ((*p) == 53u)
						goto tr121;
					if ((*p) > 52u)
					{
						if (54u <= (*p) && (*p) <= 57u)
							goto tr77;
					}
					else if ((*p) >= 48u)
						goto tr80;
					goto st0;
				tr121:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st94;
				st94:
					if (++p == pe)
						goto _test_eof94;
				case 94:
#line 833 "src/Utils/IP.cpp"
					if (48u <= (*p) && (*p) <= 53u)
						goto tr77;
					goto st0;
				st51:
					if (++p == pe)
						goto _test_eof51;
				case 51:
					if ((*p) == 46u)
						goto st50;
					if (48u <= (*p) && (*p) <= 57u)
						goto st52;
					goto st0;
				st52:
					if (++p == pe)
						goto _test_eof52;
				case 52:
					if ((*p) == 46u)
						goto st50;
					if (48u <= (*p) && (*p) <= 57u)
						goto st49;
					goto st0;
				st53:
					if (++p == pe)
						goto _test_eof53;
				case 53:
					switch ((*p))
					{
						case 46u:
							goto st50;
						case 53u:
							goto st54;
					}
					if ((*p) > 52u)
					{
						if (54u <= (*p) && (*p) <= 57u)
							goto st49;
					}
					else if ((*p) >= 48u)
						goto st52;
					goto st0;
				st54:
					if (++p == pe)
						goto _test_eof54;
				case 54:
					if ((*p) == 46u)
						goto st50;
					if (48u <= (*p) && (*p) <= 53u)
						goto st49;
					goto st0;
				st55:
					if (++p == pe)
						goto _test_eof55;
				case 55:
					if ((*p) == 46u)
						goto st48;
					if (48u <= (*p) && (*p) <= 57u)
						goto st56;
					goto st0;
				st56:
					if (++p == pe)
						goto _test_eof56;
				case 56:
					if ((*p) == 46u)
						goto st48;
					if (48u <= (*p) && (*p) <= 57u)
						goto st47;
					goto st0;
				st57:
					if (++p == pe)
						goto _test_eof57;
				case 57:
					switch ((*p))
					{
						case 46u:
							goto st48;
						case 53u:
							goto st58;
					}
					if ((*p) > 52u)
					{
						if (54u <= (*p) && (*p) <= 57u)
							goto st47;
					}
					else if ((*p) >= 48u)
						goto st56;
					goto st0;
				st58:
					if (++p == pe)
						goto _test_eof58;
				case 58:
					if ((*p) == 46u)
						goto st48;
					if (48u <= (*p) && (*p) <= 53u)
						goto st47;
					goto st0;
				st59:
					if (++p == pe)
						goto _test_eof59;
				case 59:
					if ((*p) == 58u)
						goto st62;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st60;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st60;
					}
					else
						goto st60;
					goto st0;
				st60:
					if (++p == pe)
						goto _test_eof60;
				case 60:
					if ((*p) == 58u)
						goto st62;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st61;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st61;
					}
					else
						goto st61;
					goto st0;
				st61:
					if (++p == pe)
						goto _test_eof61;
				case 61:
					if ((*p) == 58u)
						goto st62;
					goto st0;
				st62:
					if (++p == pe)
						goto _test_eof62;
				case 62:
					if ((*p) == 58u)
						goto tr77;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr85;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr85;
					}
					else
						goto tr85;
					goto st0;
				tr85:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st95;
				st95:
					if (++p == pe)
						goto _test_eof95;
				case 95:
#line 981 "src/Utils/IP.cpp"
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr122;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr122;
					}
					else
						goto tr122;
					goto st0;
				tr122:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st96;
				st96:
					if (++p == pe)
						goto _test_eof96;
				case 96:
#line 1001 "src/Utils/IP.cpp"
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr123;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr123;
					}
					else
						goto tr123;
					goto st0;
				tr123:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st97;
				st97:
					if (++p == pe)
						goto _test_eof97;
				case 97:
#line 1021 "src/Utils/IP.cpp"
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr77;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr77;
					}
					else
						goto tr77;
					goto st0;
				st63:
					if (++p == pe)
						goto _test_eof63;
				case 63:
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st62;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st64;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st59;
					}
					else
						goto st59;
					goto st0;
				st64:
					if (++p == pe)
						goto _test_eof64;
				case 64:
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st62;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st65;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st60;
					}
					else
						goto st60;
					goto st0;
				st65:
					if (++p == pe)
						goto _test_eof65;
				case 65:
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st62;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st61;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st61;
					}
					else
						goto st61;
					goto st0;
				st66:
					if (++p == pe)
						goto _test_eof66;
				case 66:
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 53u:
							goto st67;
						case 58u:
							goto st62;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 52u)
							goto st64;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto st59;
						}
						else if ((*p) >= 65u)
							goto st59;
					}
					else
						goto st68;
					goto st0;
				st67:
					if (++p == pe)
						goto _test_eof67;
				case 67:
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st62;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 53u)
							goto st65;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto st60;
						}
						else if ((*p) >= 65u)
							goto st60;
					}
					else
						goto st60;
					goto st0;
				st68:
					if (++p == pe)
						goto _test_eof68;
				case 68:
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st62;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st60;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st60;
					}
					else
						goto st60;
					goto st0;
				st69:
					if (++p == pe)
						goto _test_eof69;
				case 69:
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st62;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st68;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st59;
					}
					else
						goto st59;
					goto st0;
				tr62:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st98;
				st98:
					if (++p == pe)
						goto _test_eof98;
				case 98:
#line 1167 "src/Utils/IP.cpp"
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr85;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr85;
					}
					else
						goto tr85;
					goto st0;
				st70:
					if (++p == pe)
						goto _test_eof70;
				case 70:
					if ((*p) == 58u)
						goto st62;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st59;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st59;
					}
					else
						goto st59;
					goto st0;
				tr53:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st99;
				st99:
					if (++p == pe)
						goto _test_eof99;
				case 99:
#line 1202 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 48u:
							goto tr90;
						case 49u:
							goto tr91;
						case 50u:
							goto tr92;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto tr93;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr94;
					}
					else
						goto tr94;
					goto st0;
				tr90:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st100;
				st100:
					if (++p == pe)
						goto _test_eof100;
				case 100:
#line 1227 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st71;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr124;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr124;
					}
					else
						goto tr124;
					goto st0;
				tr124:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st101;
				st101:
					if (++p == pe)
						goto _test_eof101;
				case 101:
#line 1251 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st71;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr126;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr126;
					}
					else
						goto tr126;
					goto st0;
				tr126:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st102;
				st102:
					if (++p == pe)
						goto _test_eof102;
				case 102:
#line 1273 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st71;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr127;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr127;
					}
					else
						goto tr127;
					goto st0;
				tr127:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st103;
				st103:
					if (++p == pe)
						goto _test_eof103;
				case 103:
#line 1295 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st71;
					goto st0;
				st71:
					if (++p == pe)
						goto _test_eof71;
				case 71:
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr85;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr85;
					}
					else
						goto tr85;
					goto st0;
				tr91:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st104;
				st104:
					if (++p == pe)
						goto _test_eof104;
				case 104:
#line 1322 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st71;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr128;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr124;
					}
					else
						goto tr124;
					goto st0;
				tr128:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st105;
				st105:
					if (++p == pe)
						goto _test_eof105;
				case 105:
#line 1346 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st71;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr129;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr126;
					}
					else
						goto tr126;
					goto st0;
				tr129:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st106;
				st106:
					if (++p == pe)
						goto _test_eof106;
				case 106:
#line 1370 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st71;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr127;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr127;
					}
					else
						goto tr127;
					goto st0;
				tr92:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st107;
				st107:
					if (++p == pe)
						goto _test_eof107;
				case 107:
#line 1394 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 53u:
							goto tr130;
						case 58u:
							goto st71;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 52u)
							goto tr128;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr124;
						}
						else if ((*p) >= 65u)
							goto tr124;
					}
					else
						goto tr131;
					goto st0;
				tr130:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st108;
				st108:
					if (++p == pe)
						goto _test_eof108;
				case 108:
#line 1422 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st71;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 53u)
							goto tr129;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr126;
						}
						else if ((*p) >= 65u)
							goto tr126;
					}
					else
						goto tr126;
					goto st0;
				tr131:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st109;
				st109:
					if (++p == pe)
						goto _test_eof109;
				case 109:
#line 1449 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st71;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr126;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr126;
					}
					else
						goto tr126;
					goto st0;
				tr93:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st110;
				st110:
					if (++p == pe)
						goto _test_eof110;
				case 110:
#line 1473 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st71;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr131;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr124;
					}
					else
						goto tr124;
					goto st0;
				tr94:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st111;
				st111:
					if (++p == pe)
						goto _test_eof111;
				case 111:
#line 1497 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st71;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr124;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr124;
					}
					else
						goto tr124;
					goto st0;
				tr47:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st112;
				st112:
					if (++p == pe)
						goto _test_eof112;
				case 112:
#line 1519 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 48u:
							goto tr95;
						case 49u:
							goto tr96;
						case 50u:
							goto tr97;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto tr98;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr99;
					}
					else
						goto tr99;
					goto st0;
				tr95:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st113;
				st113:
					if (++p == pe)
						goto _test_eof113;
				case 113:
#line 1544 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st72;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr132;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr132;
					}
					else
						goto tr132;
					goto st0;
				tr132:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st114;
				st114:
					if (++p == pe)
						goto _test_eof114;
				case 114:
#line 1568 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st72;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr134;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr134;
					}
					else
						goto tr134;
					goto st0;
				tr134:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st115;
				st115:
					if (++p == pe)
						goto _test_eof115;
				case 115:
#line 1590 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st72;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr135;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr135;
					}
					else
						goto tr135;
					goto st0;
				tr135:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st116;
				st116:
					if (++p == pe)
						goto _test_eof116;
				case 116:
#line 1612 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st72;
					goto st0;
				st72:
					if (++p == pe)
						goto _test_eof72;
				case 72:
					switch ((*p))
					{
						case 48u:
							goto tr90;
						case 49u:
							goto tr91;
						case 50u:
							goto tr92;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto tr93;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr94;
					}
					else
						goto tr94;
					goto st0;
				tr96:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st117;
				st117:
					if (++p == pe)
						goto _test_eof117;
				case 117:
#line 1644 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st72;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr136;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr132;
					}
					else
						goto tr132;
					goto st0;
				tr136:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st118;
				st118:
					if (++p == pe)
						goto _test_eof118;
				case 118:
#line 1668 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st72;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr137;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr134;
					}
					else
						goto tr134;
					goto st0;
				tr137:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st119;
				st119:
					if (++p == pe)
						goto _test_eof119;
				case 119:
#line 1692 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st72;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr135;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr135;
					}
					else
						goto tr135;
					goto st0;
				tr97:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st120;
				st120:
					if (++p == pe)
						goto _test_eof120;
				case 120:
#line 1716 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 53u:
							goto tr138;
						case 58u:
							goto st72;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 52u)
							goto tr136;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr132;
						}
						else if ((*p) >= 65u)
							goto tr132;
					}
					else
						goto tr139;
					goto st0;
				tr138:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st121;
				st121:
					if (++p == pe)
						goto _test_eof121;
				case 121:
#line 1744 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st72;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 53u)
							goto tr137;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr134;
						}
						else if ((*p) >= 65u)
							goto tr134;
					}
					else
						goto tr134;
					goto st0;
				tr139:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st122;
				st122:
					if (++p == pe)
						goto _test_eof122;
				case 122:
#line 1771 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st72;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr134;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr134;
					}
					else
						goto tr134;
					goto st0;
				tr98:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st123;
				st123:
					if (++p == pe)
						goto _test_eof123;
				case 123:
#line 1795 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st72;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr139;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr132;
					}
					else
						goto tr132;
					goto st0;
				tr99:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st124;
				st124:
					if (++p == pe)
						goto _test_eof124;
				case 124:
#line 1819 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st72;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr132;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr132;
					}
					else
						goto tr132;
					goto st0;
				tr41:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st125;
				st125:
					if (++p == pe)
						goto _test_eof125;
				case 125:
#line 1841 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 48u:
							goto tr100;
						case 49u:
							goto tr101;
						case 50u:
							goto tr102;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto tr103;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr104;
					}
					else
						goto tr104;
					goto st0;
				tr100:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st126;
				st126:
					if (++p == pe)
						goto _test_eof126;
				case 126:
#line 1866 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st73;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr140;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr140;
					}
					else
						goto tr140;
					goto st0;
				tr140:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st127;
				st127:
					if (++p == pe)
						goto _test_eof127;
				case 127:
#line 1890 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st73;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr142;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr142;
					}
					else
						goto tr142;
					goto st0;
				tr142:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st128;
				st128:
					if (++p == pe)
						goto _test_eof128;
				case 128:
#line 1912 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st73;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr143;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr143;
					}
					else
						goto tr143;
					goto st0;
				tr143:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st129;
				st129:
					if (++p == pe)
						goto _test_eof129;
				case 129:
#line 1934 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st73;
					goto st0;
				st73:
					if (++p == pe)
						goto _test_eof73;
				case 73:
					switch ((*p))
					{
						case 48u:
							goto tr95;
						case 49u:
							goto tr96;
						case 50u:
							goto tr97;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto tr98;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr99;
					}
					else
						goto tr99;
					goto st0;
				tr101:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st130;
				st130:
					if (++p == pe)
						goto _test_eof130;
				case 130:
#line 1966 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st73;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr144;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr140;
					}
					else
						goto tr140;
					goto st0;
				tr144:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st131;
				st131:
					if (++p == pe)
						goto _test_eof131;
				case 131:
#line 1990 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st73;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr145;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr142;
					}
					else
						goto tr142;
					goto st0;
				tr145:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st132;
				st132:
					if (++p == pe)
						goto _test_eof132;
				case 132:
#line 2014 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st73;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr143;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr143;
					}
					else
						goto tr143;
					goto st0;
				tr102:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st133;
				st133:
					if (++p == pe)
						goto _test_eof133;
				case 133:
#line 2038 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 53u:
							goto tr146;
						case 58u:
							goto st73;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 52u)
							goto tr144;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr140;
						}
						else if ((*p) >= 65u)
							goto tr140;
					}
					else
						goto tr147;
					goto st0;
				tr146:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st134;
				st134:
					if (++p == pe)
						goto _test_eof134;
				case 134:
#line 2066 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st73;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 53u)
							goto tr145;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr142;
						}
						else if ((*p) >= 65u)
							goto tr142;
					}
					else
						goto tr142;
					goto st0;
				tr147:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st135;
				st135:
					if (++p == pe)
						goto _test_eof135;
				case 135:
#line 2093 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st73;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr142;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr142;
					}
					else
						goto tr142;
					goto st0;
				tr103:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st136;
				st136:
					if (++p == pe)
						goto _test_eof136;
				case 136:
#line 2117 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st73;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr147;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr140;
					}
					else
						goto tr140;
					goto st0;
				tr104:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st137;
				st137:
					if (++p == pe)
						goto _test_eof137;
				case 137:
#line 2141 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st73;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr140;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr140;
					}
					else
						goto tr140;
					goto st0;
				tr35:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st138;
				st138:
					if (++p == pe)
						goto _test_eof138;
				case 138:
#line 2163 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 48u:
							goto tr105;
						case 49u:
							goto tr106;
						case 50u:
							goto tr107;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto tr108;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr109;
					}
					else
						goto tr109;
					goto st0;
				tr105:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st139;
				st139:
					if (++p == pe)
						goto _test_eof139;
				case 139:
#line 2188 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st74;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr148;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr148;
					}
					else
						goto tr148;
					goto st0;
				tr148:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st140;
				st140:
					if (++p == pe)
						goto _test_eof140;
				case 140:
#line 2212 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st74;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr150;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr150;
					}
					else
						goto tr150;
					goto st0;
				tr150:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st141;
				st141:
					if (++p == pe)
						goto _test_eof141;
				case 141:
#line 2234 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st74;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr151;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr151;
					}
					else
						goto tr151;
					goto st0;
				tr151:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st142;
				st142:
					if (++p == pe)
						goto _test_eof142;
				case 142:
#line 2256 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st74;
					goto st0;
				st74:
					if (++p == pe)
						goto _test_eof74;
				case 74:
					switch ((*p))
					{
						case 48u:
							goto tr100;
						case 49u:
							goto tr101;
						case 50u:
							goto tr102;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto tr103;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr104;
					}
					else
						goto tr104;
					goto st0;
				tr106:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st143;
				st143:
					if (++p == pe)
						goto _test_eof143;
				case 143:
#line 2288 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st74;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr152;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr148;
					}
					else
						goto tr148;
					goto st0;
				tr152:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st144;
				st144:
					if (++p == pe)
						goto _test_eof144;
				case 144:
#line 2312 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st74;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr153;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr150;
					}
					else
						goto tr150;
					goto st0;
				tr153:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st145;
				st145:
					if (++p == pe)
						goto _test_eof145;
				case 145:
#line 2336 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st74;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr151;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr151;
					}
					else
						goto tr151;
					goto st0;
				tr107:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st146;
				st146:
					if (++p == pe)
						goto _test_eof146;
				case 146:
#line 2360 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 53u:
							goto tr154;
						case 58u:
							goto st74;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 52u)
							goto tr152;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr148;
						}
						else if ((*p) >= 65u)
							goto tr148;
					}
					else
						goto tr155;
					goto st0;
				tr154:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st147;
				st147:
					if (++p == pe)
						goto _test_eof147;
				case 147:
#line 2388 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st74;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 53u)
							goto tr153;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr150;
						}
						else if ((*p) >= 65u)
							goto tr150;
					}
					else
						goto tr150;
					goto st0;
				tr155:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st148;
				st148:
					if (++p == pe)
						goto _test_eof148;
				case 148:
#line 2415 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st74;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr150;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr150;
					}
					else
						goto tr150;
					goto st0;
				tr108:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st149;
				st149:
					if (++p == pe)
						goto _test_eof149;
				case 149:
#line 2439 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st74;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr155;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr148;
					}
					else
						goto tr148;
					goto st0;
				tr109:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st150;
				st150:
					if (++p == pe)
						goto _test_eof150;
				case 150:
#line 2463 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st74;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr148;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr148;
					}
					else
						goto tr148;
					goto st0;
				tr29:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st151;
				st151:
					if (++p == pe)
						goto _test_eof151;
				case 151:
#line 2485 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 48u:
							goto tr115;
						case 49u:
							goto tr116;
						case 50u:
							goto tr117;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto tr118;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr119;
					}
					else
						goto tr119;
					goto st0;
				tr115:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st152;
				st152:
					if (++p == pe)
						goto _test_eof152;
				case 152:
#line 2510 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st75;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr156;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr156;
					}
					else
						goto tr156;
					goto st0;
				tr156:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st153;
				st153:
					if (++p == pe)
						goto _test_eof153;
				case 153:
#line 2534 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st75;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr158;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr158;
					}
					else
						goto tr158;
					goto st0;
				tr158:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st154;
				st154:
					if (++p == pe)
						goto _test_eof154;
				case 154:
#line 2556 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st75;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr159;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr159;
					}
					else
						goto tr159;
					goto st0;
				tr159:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st155;
				st155:
					if (++p == pe)
						goto _test_eof155;
				case 155:
#line 2578 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st75;
					goto st0;
				st75:
					if (++p == pe)
						goto _test_eof75;
				case 75:
					switch ((*p))
					{
						case 48u:
							goto tr105;
						case 49u:
							goto tr106;
						case 50u:
							goto tr107;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto tr108;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr109;
					}
					else
						goto tr109;
					goto st0;
				tr116:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st156;
				st156:
					if (++p == pe)
						goto _test_eof156;
				case 156:
#line 2610 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st75;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr160;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr156;
					}
					else
						goto tr156;
					goto st0;
				tr160:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st157;
				st157:
					if (++p == pe)
						goto _test_eof157;
				case 157:
#line 2634 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st75;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr161;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr158;
					}
					else
						goto tr158;
					goto st0;
				tr161:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st158;
				st158:
					if (++p == pe)
						goto _test_eof158;
				case 158:
#line 2658 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st75;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr159;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr159;
					}
					else
						goto tr159;
					goto st0;
				tr117:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st159;
				st159:
					if (++p == pe)
						goto _test_eof159;
				case 159:
#line 2682 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 53u:
							goto tr162;
						case 58u:
							goto st75;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 52u)
							goto tr160;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr156;
						}
						else if ((*p) >= 65u)
							goto tr156;
					}
					else
						goto tr163;
					goto st0;
				tr162:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st160;
				st160:
					if (++p == pe)
						goto _test_eof160;
				case 160:
#line 2710 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st75;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 53u)
							goto tr161;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr158;
						}
						else if ((*p) >= 65u)
							goto tr158;
					}
					else
						goto tr158;
					goto st0;
				tr163:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st161;
				st161:
					if (++p == pe)
						goto _test_eof161;
				case 161:
#line 2737 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st75;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr158;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr158;
					}
					else
						goto tr158;
					goto st0;
				tr118:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st162;
				st162:
					if (++p == pe)
						goto _test_eof162;
				case 162:
#line 2761 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st75;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr163;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr156;
					}
					else
						goto tr156;
					goto st0;
				tr119:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st163;
				st163:
					if (++p == pe)
						goto _test_eof163;
				case 163:
#line 2785 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st75;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr156;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr156;
					}
					else
						goto tr156;
					goto st0;
				st76:
					if (++p == pe)
						goto _test_eof76;
				case 76:
					switch ((*p))
					{
						case 46u:
							goto st3;
						case 58u:
							goto st19;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st77;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st16;
					}
					else
						goto st16;
					goto st0;
				st77:
					if (++p == pe)
						goto _test_eof77;
				case 77:
					switch ((*p))
					{
						case 46u:
							goto st3;
						case 58u:
							goto st19;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st78;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st17;
					}
					else
						goto st17;
					goto st0;
				st78:
					if (++p == pe)
						goto _test_eof78;
				case 78:
					switch ((*p))
					{
						case 46u:
							goto st3;
						case 58u:
							goto st19;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st18;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st18;
					}
					else
						goto st18;
					goto st0;
				st79:
					if (++p == pe)
						goto _test_eof79;
				case 79:
					switch ((*p))
					{
						case 46u:
							goto st3;
						case 53u:
							goto st80;
						case 58u:
							goto st19;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 52u)
							goto st77;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto st16;
						}
						else if ((*p) >= 65u)
							goto st16;
					}
					else
						goto st81;
					goto st0;
				st80:
					if (++p == pe)
						goto _test_eof80;
				case 80:
					switch ((*p))
					{
						case 46u:
							goto st3;
						case 58u:
							goto st19;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 53u)
							goto st78;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto st17;
						}
						else if ((*p) >= 65u)
							goto st17;
					}
					else
						goto st17;
					goto st0;
				st81:
					if (++p == pe)
						goto _test_eof81;
				case 81:
					switch ((*p))
					{
						case 46u:
							goto st3;
						case 58u:
							goto st19;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st17;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st17;
					}
					else
						goto st17;
					goto st0;
				st82:
					if (++p == pe)
						goto _test_eof82;
				case 82:
					switch ((*p))
					{
						case 46u:
							goto st3;
						case 58u:
							goto st19;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st81;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st16;
					}
					else
						goto st16;
					goto st0;
				st83:
					if (++p == pe)
						goto _test_eof83;
				case 83:
					if ((*p) == 58u)
						goto tr114;
					goto st0;
				tr114:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st164;
				st164:
					if (++p == pe)
						goto _test_eof164;
				case 164:
#line 2940 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 48u:
							goto tr164;
						case 49u:
							goto tr165;
						case 50u:
							goto tr166;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto tr167;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr168;
					}
					else
						goto tr168;
					goto st0;
				tr164:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st165;
				st165:
					if (++p == pe)
						goto _test_eof165;
				case 165:
#line 2965 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st84;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr169;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr169;
					}
					else
						goto tr169;
					goto st0;
				tr169:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st166;
				st166:
					if (++p == pe)
						goto _test_eof166;
				case 166:
#line 2989 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st84;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr171;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr171;
					}
					else
						goto tr171;
					goto st0;
				tr171:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st167;
				st167:
					if (++p == pe)
						goto _test_eof167;
				case 167:
#line 3011 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st84;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr172;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr172;
					}
					else
						goto tr172;
					goto st0;
				tr172:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st168;
				st168:
					if (++p == pe)
						goto _test_eof168;
				case 168:
#line 3033 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st84;
					goto st0;
				st84:
					if (++p == pe)
						goto _test_eof84;
				case 84:
					switch ((*p))
					{
						case 48u:
							goto tr115;
						case 49u:
							goto tr116;
						case 50u:
							goto tr117;
					}
					if ((*p) < 65u)
					{
						if (51u <= (*p) && (*p) <= 57u)
							goto tr118;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr119;
					}
					else
						goto tr119;
					goto st0;
				tr165:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st169;
				st169:
					if (++p == pe)
						goto _test_eof169;
				case 169:
#line 3065 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st84;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr173;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr169;
					}
					else
						goto tr169;
					goto st0;
				tr173:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st170;
				st170:
					if (++p == pe)
						goto _test_eof170;
				case 170:
#line 3089 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st84;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr174;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr171;
					}
					else
						goto tr171;
					goto st0;
				tr174:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st171;
				st171:
					if (++p == pe)
						goto _test_eof171;
				case 171:
#line 3113 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st84;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr172;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr172;
					}
					else
						goto tr172;
					goto st0;
				tr166:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st172;
				st172:
					if (++p == pe)
						goto _test_eof172;
				case 172:
#line 3137 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 53u:
							goto tr175;
						case 58u:
							goto st84;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 52u)
							goto tr173;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr169;
						}
						else if ((*p) >= 65u)
							goto tr169;
					}
					else
						goto tr176;
					goto st0;
				tr175:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st173;
				st173:
					if (++p == pe)
						goto _test_eof173;
				case 173:
#line 3165 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st84;
					}
					if ((*p) < 54u)
					{
						if (48u <= (*p) && (*p) <= 53u)
							goto tr174;
					}
					else if ((*p) > 57u)
					{
						if ((*p) > 70u)
						{
							if (97u <= (*p) && (*p) <= 102u)
								goto tr171;
						}
						else if ((*p) >= 65u)
							goto tr171;
					}
					else
						goto tr171;
					goto st0;
				tr176:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st174;
				st174:
					if (++p == pe)
						goto _test_eof174;
				case 174:
#line 3192 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st84;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr171;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr171;
					}
					else
						goto tr171;
					goto st0;
				tr167:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st175;
				st175:
					if (++p == pe)
						goto _test_eof175;
				case 175:
#line 3216 "src/Utils/IP.cpp"
					switch ((*p))
					{
						case 46u:
							goto st46;
						case 58u:
							goto st84;
					}
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr176;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr169;
					}
					else
						goto tr169;
					goto st0;
				tr168:
#line 30 "src/Utils/IP.rl"
				{
					ipFamily = AF_INET6;
				}
					goto st176;
				st176:
					if (++p == pe)
						goto _test_eof176;
				case 176:
#line 3240 "src/Utils/IP.cpp"
					if ((*p) == 58u)
						goto st84;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto tr169;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto tr169;
					}
					else
						goto tr169;
					goto st0;
				st85:
					if (++p == pe)
						goto _test_eof85;
				case 85:
					if ((*p) == 58u)
						goto st19;
					if ((*p) < 65u)
					{
						if (48u <= (*p) && (*p) <= 57u)
							goto st16;
					}
					else if ((*p) > 70u)
					{
						if (97u <= (*p) && (*p) <= 102u)
							goto st16;
					}
					else
						goto st16;
					goto st0;
			}
		_test_eof2:
			cs = 2;
			goto _test_eof;
		_test_eof3:
			cs = 3;
			goto _test_eof;
		_test_eof4:
			cs = 4;
			goto _test_eof;
		_test_eof5:
			cs = 5;
			goto _test_eof;
		_test_eof6:
			cs = 6;
			goto _test_eof;
		_test_eof7:
			cs = 7;
			goto _test_eof;
		_test_eof86:
			cs = 86;
			goto _test_eof;
		_test_eof87:
			cs = 87;
			goto _test_eof;
		_test_eof88:
			cs = 88;
			goto _test_eof;
		_test_eof89:
			cs = 89;
			goto _test_eof;
		_test_eof90:
			cs = 90;
			goto _test_eof;
		_test_eof8:
			cs = 8;
			goto _test_eof;
		_test_eof9:
			cs = 9;
			goto _test_eof;
		_test_eof10:
			cs = 10;
			goto _test_eof;
		_test_eof11:
			cs = 11;
			goto _test_eof;
		_test_eof12:
			cs = 12;
			goto _test_eof;
		_test_eof13:
			cs = 13;
			goto _test_eof;
		_test_eof14:
			cs = 14;
			goto _test_eof;
		_test_eof15:
			cs = 15;
			goto _test_eof;
		_test_eof16:
			cs = 16;
			goto _test_eof;
		_test_eof17:
			cs = 17;
			goto _test_eof;
		_test_eof18:
			cs = 18;
			goto _test_eof;
		_test_eof19:
			cs = 19;
			goto _test_eof;
		_test_eof20:
			cs = 20;
			goto _test_eof;
		_test_eof21:
			cs = 21;
			goto _test_eof;
		_test_eof22:
			cs = 22;
			goto _test_eof;
		_test_eof23:
			cs = 23;
			goto _test_eof;
		_test_eof24:
			cs = 24;
			goto _test_eof;
		_test_eof25:
			cs = 25;
			goto _test_eof;
		_test_eof26:
			cs = 26;
			goto _test_eof;
		_test_eof27:
			cs = 27;
			goto _test_eof;
		_test_eof28:
			cs = 28;
			goto _test_eof;
		_test_eof29:
			cs = 29;
			goto _test_eof;
		_test_eof30:
			cs = 30;
			goto _test_eof;
		_test_eof31:
			cs = 31;
			goto _test_eof;
		_test_eof32:
			cs = 32;
			goto _test_eof;
		_test_eof33:
			cs = 33;
			goto _test_eof;
		_test_eof34:
			cs = 34;
			goto _test_eof;
		_test_eof35:
			cs = 35;
			goto _test_eof;
		_test_eof36:
			cs = 36;
			goto _test_eof;
		_test_eof37:
			cs = 37;
			goto _test_eof;
		_test_eof38:
			cs = 38;
			goto _test_eof;
		_test_eof39:
			cs = 39;
			goto _test_eof;
		_test_eof40:
			cs = 40;
			goto _test_eof;
		_test_eof41:
			cs = 41;
			goto _test_eof;
		_test_eof42:
			cs = 42;
			goto _test_eof;
		_test_eof43:
			cs = 43;
			goto _test_eof;
		_test_eof44:
			cs = 44;
			goto _test_eof;
		_test_eof45:
			cs = 45;
			goto _test_eof;
		_test_eof46:
			cs = 46;
			goto _test_eof;
		_test_eof47:
			cs = 47;
			goto _test_eof;
		_test_eof48:
			cs = 48;
			goto _test_eof;
		_test_eof49:
			cs = 49;
			goto _test_eof;
		_test_eof50:
			cs = 50;
			goto _test_eof;
		_test_eof91:
			cs = 91;
			goto _test_eof;
		_test_eof92:
			cs = 92;
			goto _test_eof;
		_test_eof93:
			cs = 93;
			goto _test_eof;
		_test_eof94:
			cs = 94;
			goto _test_eof;
		_test_eof51:
			cs = 51;
			goto _test_eof;
		_test_eof52:
			cs = 52;
			goto _test_eof;
		_test_eof53:
			cs = 53;
			goto _test_eof;
		_test_eof54:
			cs = 54;
			goto _test_eof;
		_test_eof55:
			cs = 55;
			goto _test_eof;
		_test_eof56:
			cs = 56;
			goto _test_eof;
		_test_eof57:
			cs = 57;
			goto _test_eof;
		_test_eof58:
			cs = 58;
			goto _test_eof;
		_test_eof59:
			cs = 59;
			goto _test_eof;
		_test_eof60:
			cs = 60;
			goto _test_eof;
		_test_eof61:
			cs = 61;
			goto _test_eof;
		_test_eof62:
			cs = 62;
			goto _test_eof;
		_test_eof95:
			cs = 95;
			goto _test_eof;
		_test_eof96:
			cs = 96;
			goto _test_eof;
		_test_eof97:
			cs = 97;
			goto _test_eof;
		_test_eof63:
			cs = 63;
			goto _test_eof;
		_test_eof64:
			cs = 64;
			goto _test_eof;
		_test_eof65:
			cs = 65;
			goto _test_eof;
		_test_eof66:
			cs = 66;
			goto _test_eof;
		_test_eof67:
			cs = 67;
			goto _test_eof;
		_test_eof68:
			cs = 68;
			goto _test_eof;
		_test_eof69:
			cs = 69;
			goto _test_eof;
		_test_eof98:
			cs = 98;
			goto _test_eof;
		_test_eof70:
			cs = 70;
			goto _test_eof;
		_test_eof99:
			cs = 99;
			goto _test_eof;
		_test_eof100:
			cs = 100;
			goto _test_eof;
		_test_eof101:
			cs = 101;
			goto _test_eof;
		_test_eof102:
			cs = 102;
			goto _test_eof;
		_test_eof103:
			cs = 103;
			goto _test_eof;
		_test_eof71:
			cs = 71;
			goto _test_eof;
		_test_eof104:
			cs = 104;
			goto _test_eof;
		_test_eof105:
			cs = 105;
			goto _test_eof;
		_test_eof106:
			cs = 106;
			goto _test_eof;
		_test_eof107:
			cs = 107;
			goto _test_eof;
		_test_eof108:
			cs = 108;
			goto _test_eof;
		_test_eof109:
			cs = 109;
			goto _test_eof;
		_test_eof110:
			cs = 110;
			goto _test_eof;
		_test_eof111:
			cs = 111;
			goto _test_eof;
		_test_eof112:
			cs = 112;
			goto _test_eof;
		_test_eof113:
			cs = 113;
			goto _test_eof;
		_test_eof114:
			cs = 114;
			goto _test_eof;
		_test_eof115:
			cs = 115;
			goto _test_eof;
		_test_eof116:
			cs = 116;
			goto _test_eof;
		_test_eof72:
			cs = 72;
			goto _test_eof;
		_test_eof117:
			cs = 117;
			goto _test_eof;
		_test_eof118:
			cs = 118;
			goto _test_eof;
		_test_eof119:
			cs = 119;
			goto _test_eof;
		_test_eof120:
			cs = 120;
			goto _test_eof;
		_test_eof121:
			cs = 121;
			goto _test_eof;
		_test_eof122:
			cs = 122;
			goto _test_eof;
		_test_eof123:
			cs = 123;
			goto _test_eof;
		_test_eof124:
			cs = 124;
			goto _test_eof;
		_test_eof125:
			cs = 125;
			goto _test_eof;
		_test_eof126:
			cs = 126;
			goto _test_eof;
		_test_eof127:
			cs = 127;
			goto _test_eof;
		_test_eof128:
			cs = 128;
			goto _test_eof;
		_test_eof129:
			cs = 129;
			goto _test_eof;
		_test_eof73:
			cs = 73;
			goto _test_eof;
		_test_eof130:
			cs = 130;
			goto _test_eof;
		_test_eof131:
			cs = 131;
			goto _test_eof;
		_test_eof132:
			cs = 132;
			goto _test_eof;
		_test_eof133:
			cs = 133;
			goto _test_eof;
		_test_eof134:
			cs = 134;
			goto _test_eof;
		_test_eof135:
			cs = 135;
			goto _test_eof;
		_test_eof136:
			cs = 136;
			goto _test_eof;
		_test_eof137:
			cs = 137;
			goto _test_eof;
		_test_eof138:
			cs = 138;
			goto _test_eof;
		_test_eof139:
			cs = 139;
			goto _test_eof;
		_test_eof140:
			cs = 140;
			goto _test_eof;
		_test_eof141:
			cs = 141;
			goto _test_eof;
		_test_eof142:
			cs = 142;
			goto _test_eof;
		_test_eof74:
			cs = 74;
			goto _test_eof;
		_test_eof143:
			cs = 143;
			goto _test_eof;
		_test_eof144:
			cs = 144;
			goto _test_eof;
		_test_eof145:
			cs = 145;
			goto _test_eof;
		_test_eof146:
			cs = 146;
			goto _test_eof;
		_test_eof147:
			cs = 147;
			goto _test_eof;
		_test_eof148:
			cs = 148;
			goto _test_eof;
		_test_eof149:
			cs = 149;
			goto _test_eof;
		_test_eof150:
			cs = 150;
			goto _test_eof;
		_test_eof151:
			cs = 151;
			goto _test_eof;
		_test_eof152:
			cs = 152;
			goto _test_eof;
		_test_eof153:
			cs = 153;
			goto _test_eof;
		_test_eof154:
			cs = 154;
			goto _test_eof;
		_test_eof155:
			cs = 155;
			goto _test_eof;
		_test_eof75:
			cs = 75;
			goto _test_eof;
		_test_eof156:
			cs = 156;
			goto _test_eof;
		_test_eof157:
			cs = 157;
			goto _test_eof;
		_test_eof158:
			cs = 158;
			goto _test_eof;
		_test_eof159:
			cs = 159;
			goto _test_eof;
		_test_eof160:
			cs = 160;
			goto _test_eof;
		_test_eof161:
			cs = 161;
			goto _test_eof;
		_test_eof162:
			cs = 162;
			goto _test_eof;
		_test_eof163:
			cs = 163;
			goto _test_eof;
		_test_eof76:
			cs = 76;
			goto _test_eof;
		_test_eof77:
			cs = 77;
			goto _test_eof;
		_test_eof78:
			cs = 78;
			goto _test_eof;
		_test_eof79:
			cs = 79;
			goto _test_eof;
		_test_eof80:
			cs = 80;
			goto _test_eof;
		_test_eof81:
			cs = 81;
			goto _test_eof;
		_test_eof82:
			cs = 82;
			goto _test_eof;
		_test_eof83:
			cs = 83;
			goto _test_eof;
		_test_eof164:
			cs = 164;
			goto _test_eof;
		_test_eof165:
			cs = 165;
			goto _test_eof;
		_test_eof166:
			cs = 166;
			goto _test_eof;
		_test_eof167:
			cs = 167;
			goto _test_eof;
		_test_eof168:
			cs = 168;
			goto _test_eof;
		_test_eof84:
			cs = 84;
			goto _test_eof;
		_test_eof169:
			cs = 169;
			goto _test_eof;
		_test_eof170:
			cs = 170;
			goto _test_eof;
		_test_eof171:
			cs = 171;
			goto _test_eof;
		_test_eof172:
			cs = 172;
			goto _test_eof;
		_test_eof173:
			cs = 173;
			goto _test_eof;
		_test_eof174:
			cs = 174;
			goto _test_eof;
		_test_eof175:
			cs = 175;
			goto _test_eof;
		_test_eof176:
			cs = 176;
			goto _test_eof;
		_test_eof85:
			cs = 85;
			goto _test_eof;

		_test_eof:
		{
		}
		_out:
		{
		}
		}

#line 64 "src/Utils/IP.rl"

		// Ensure that the parsing has consumed all the given length.
		if (ipLen == (size_t)(p - (const unsigned char*)ip))
			return ipFamily;
		else
			return AF_UNSPEC;
	}

	void IP::GetAddressInfo(const struct sockaddr* addr, int* family, std::string& ip, uint16_t* port)
	{
		MS_TRACE();

		char ipBuffer[INET6_ADDRSTRLEN + 1];
		int err;

		switch (addr->sa_family)
		{
			case AF_INET:
			{
				err =
				    uv_inet_ntop(AF_INET, &((struct sockaddr_in*)addr)->sin_addr, ipBuffer, INET_ADDRSTRLEN);

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
