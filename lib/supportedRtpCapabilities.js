'use strict';

let supportedRtpCapabilities =
{
	headerExtensions :
	[
		{
			kind             : '',
			uri              : 'urn:ietf:params:rtp-hdrext:sdes:mid',
			preferredId      : 1,
			preferredEncrypt : false
		}
	],
	fecMechanisms : []
};

module.exports = supportedRtpCapabilities;
