'use strict';

const randomatic = require('randomatic');
const randomNumber = require('random-number');
const clone = require('clone');

const randomNumberGenerator = randomNumber.generator(
	{
		min     : 10000000,
		max     : 99999999,
		integer : true
	});

// In ORTC, codec parameters use camelCase but that provides zero benefit.
// Let keep them for backward compatibility.
const ORTC_TO_SDP_PARAMETERS =  new Map([
	// OPUS parameters.
	// https://tools.ietf.org/html/draft-ietf-payload-rtp-opus
	[ 'maxPlaybackRate',     'maxplaybackrate'      ],
	[ 'spropMaxCaptureRate', 'sprop-maxcapturerate' ],
	[ 'maxAverageBitrate',   'maxaveragebitrate'    ],
	[ 'useInbandFec',        'useinbandfec'         ],
	[ 'useDtx',              'usedtx'               ],
	// FLEXFEC parameters.
	// https://tools.ietf.org/html/draft-ietf-payload-flexible-fec-scheme
	[ 'l',                    'L'                   ],
	[ 'd',                    'D'                   ],
	[ 'toP',                  'ToP'                 ]
]);

// Covert ORTC codec parameters to lowercase.
const ORTC_TO_SDP_PARAMETERS_LOWERCASE = new Map();

for (let pair of ORTC_TO_SDP_PARAMETERS)
{
	let key = pair[0];
	let value = pair[1];

	ORTC_TO_SDP_PARAMETERS_LOWERCASE.set(key.toLowerCase(), value);
}

module.exports =
{
	randomString(length)
	{
		return randomatic('a', length || 8);
	},

	randomNumber: randomNumberGenerator,

	clone(obj)
	{
		if (!obj || typeof obj !== 'object')
			return {};
		else
			return clone(obj, false);
	},

	/**
	 * Generate a SDP normalized parameter.
	 * @param  {String} param - ORTC parameter name.
	 * @return {String} SDP parameter name.
	 */
	paramToSDP(param)
	{
		if (typeof param !== 'string' || !param)
			throw new TypeError('wrong argument');

		let wellKnownParam = ORTC_TO_SDP_PARAMETERS_LOWERCASE.get(param.toLowerCase());

		if (wellKnownParam)
			return wellKnownParam;

		let newParam = '';

		for (let i = 0, len = param.length; i < len; i++)
		{
			let c = param[i];

			if (c === c.toLowerCase())
			{
				newParam += c;
			}
			else
			{
				if (i)
					newParam += '-';
				newParam += c.toLowerCase();
			}
		}

		return newParam;
	}
};
