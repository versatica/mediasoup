%%{
	machine grammar;

	CRLF                          = "\r\n";
	SP                            = 0x20;  # Space.
	DIGIT                         = "0".."9";
	ALPHA                         = "a".."z" | "A".."Z";
	alphanum                      = ALPHA | DIGIT;
	HEXDIG                        = DIGIT | "A"i | "B"i | "C"i | "D"i | "E"i | "F"i;
	VCHAR                         = 0x21..0x7E;  # Visible (printing) characters.
	POS_DIGIT                     = "1".."9";
	integer                       = POS_DIGIT DIGIT*;
	# Max unsigned long (in 32 bits) is +4.294.967.295, so just allow 9 digits.
	ULONG                         = DIGIT{1,9};
	decimal_uchar                 = DIGIT | ( 0x31..0x39 DIGIT ) | ( "1" DIGIT{2} ) | ( "2" 0x30..0x34 DIGIT ) |
	                                ( "25" 0x30..0x35 );  # 0-255.

	token                         = ( 0x21 | 0x23..0x27 | 0x2A..0x2B | 0x2D..0x2E | 0x30..0x39 | 0x41..0x5A |
		                              0x5E..0x7E )+;
	byte_string                   = ( 0x01..0x09 | 0x0B..0x0C | 0x0E..0xFF )+;  # Any byte except NULL, CR or LF.
	non_ws_string                 = ( VCHAR | 0x80..0xFF )+;  # String of visible characters.
	text                          = byte_string;

	IPv4address                   = decimal_uchar "." decimal_uchar "." decimal_uchar "." decimal_uchar;
	h16                           = HEXDIG{1,4};
	ls32                          = ( h16 ":" h16 ) | IPv4address;
	scopeId                       = "%" alphanum{1,20};
	IPv6address                   = ( ( h16 ":" ){6} ls32 scopeId? ) |
	                                ( "::" ( h16 ":" ){5} ls32 scopeId? ) |
	                                ( h16? "::" ( h16 ":" ){4} ls32 scopeId? ) |
	                                ( ( ( h16 ":" )? h16 )? "::" ( h16 ":" ){3} ls32 scopeId? ) |
	                                ( ( ( h16 ":" ){,2} h16 )? "::" ( h16 ":" ){2} ls32 scopeId? ) |
	                                ( ( ( h16 ":" ){,3} h16 )? "::" h16 ":" ls32 scopeId? ) |
	                                ( ( ( h16 ":" ){,4} h16 )? "::" ls32 scopeId? ) |
	                                ( ( ( h16 ":" ){,5} h16 )? "::" h16 scopeId? ) |
	                                ( ( ( h16 ":" ){,6} h16 )? "::" scopeId? );
	port                          = DIGIT{1,4} |
	                                "1".."5" DIGIT{4} |
	                                "6" "0".."4" DIGIT{3} |
	                                "6" "5" "0".."4" DIGIT{2} |
	                                "6" "5" "5" "0".."2" DIGIT |
	                                "6" "5" "5" "3" "0".."5";

	domainlabel                   = alphanum | ( alphanum ( alphanum | "-" | "_" )* alphanum );
	toplabel                      = ALPHA | ( ALPHA ( alphanum | "-" | "_" )* alphanum );
	# TODO: Probably relax it and allow "anything".
	FQDN                          = ( domainlabel "." )* toplabel "."?;

	IPv4address_multicast         = IPv4address "/" decimal_uchar ( "/" integer )?;
	IPv6address_multicast         = IPv6address ( "/" integer )?;

	unicast_address               = IPv4address | IPv6address | FQDN;  # NOTE: let's remove non_ws_string.
	multicast_address             = IPv4address_multicast | IPv6address_multicast | FQDN;  # NOTE: let's remove non_ws_string.
}%%
