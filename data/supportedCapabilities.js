'use strict';

/**
 * This file is not used by the JavaScript engine, but copied into the C++
 * source code once JSONized by gulp.
 */

let supportedCapabilities =
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

module.exports = supportedCapabilities;
