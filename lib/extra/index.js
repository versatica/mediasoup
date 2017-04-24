'use strict';

// SDP codec parameters that don't follow the camelcase convention of ORTC.
const WELL_KNOWN_SDP_PARAMS_TO_ORTC = new Map([
	// OPUS parameters.
	// https://tools.ietf.org/html/draft-ietf-payload-rtp-opus
	[ 'maxplaybackrate',      'maxPlaybackRate'     ],
	[ 'sprop-maxcapturerate', 'spropMaxCaptureRate' ],
	[ 'maxaveragebitrate',    'maxAverageBitrate'   ],
	[ 'useinbandfec',         'useInbandFec'        ],
	[ 'usedtx',               'useDtx'              ],
	[ 'sprop-maxcapturerate', 'spropMaxCaptureRate' ],
	// FLEXFEC parameters.
	// https://tools.ietf.org/html/draft-ietf-payload-flexible-fec-scheme
	[ 'L',                    'l'                   ],
	[ 'D',                    'd'                   ],
	[ 'ToP',                  'toP'                 ]
]);

// NOTE: SDP params are converted to lowcase.
const WELL_KNOWN_FROM_SDP_PARAMS = new Map();
// NOTE: ORTC params are converted to lowcase.
const WELL_KNOWN_TO_SDP_PARAMS = new Map();

for (let pair of WELL_KNOWN_SDP_PARAMS_TO_ORTC)
{
	let key = pair[0];
	let value = pair[1];

	WELL_KNOWN_FROM_SDP_PARAMS.set(key.toLowerCase(), value);
	WELL_KNOWN_TO_SDP_PARAMS.set(value.toLowerCase(), key);
}

module.exports =
{
	/**
	 * Generate a raw DTLS fingerprint.
	 * @param  {String} sdpFingerprint - SDP fingerprint (uppercase with colons).
	 * @return {String} Raw fingerprint (lowercase without colons).
	 */
	fingerprintFromSDP: (sdpFingerprint) =>
	{
		if (typeof sdpFingerprint !== 'string' || !sdpFingerprint)
			throw new TypeError('wrong argument');

		return sdpFingerprint
			.replace(/:/g, '')
			.toLowerCase();
	},

	/**
	 * Generate a SDP DTLS fingerprint.
	 * @param  {String} rawFingerprint - Raw fingerprint (lowercase without colons).
	 * @return {String} SDP fingerprint (uppercase with colons).
	 */
	fingerprintToSDP: (rawFingerprint) =>
	{
		if (typeof rawFingerprint !== 'string' || !rawFingerprint)
			throw new TypeError('wrong argument');

		return rawFingerprint
			.replace(/../g, (xx) => xx + ':')
			.slice(0, -1)
			.toUpperCase();
	},

	/**
	 * Generate a mediasoup normalized parameter.
	 * @param  {String} param - SDP parameter name.
	 * @return {String} Normalized parameter name.
	 */
	paramFromSDP: (param) =>
	{
		if (typeof param !== 'string' || !param)
			throw new TypeError('wrong argument');

		let wellKnownParam = WELL_KNOWN_FROM_SDP_PARAMS.get(param.toLowerCase());

		if (wellKnownParam)
			return wellKnownParam;

		let newParam = '';

		for (let i = 0, len = param.length; i < len; i++)
		{
			let c = param[i];

			if (c !== '-')
				newParam += c.toLowerCase();
			else if (i < len - 1)
				newParam += param[++i].toUpperCase();
		}

		return newParam;
	},

	/**
	 * Generate a SDP normalized parameter.
	 * @param  {String} param - mediasoup parameter name.
	 * @return {String} SDP parameter name.
	 */
	paramToSDP: (param) =>
	{
		if (typeof param !== 'string' || !param)
			throw new TypeError('wrong argument');

		let wellKnownParam = WELL_KNOWN_TO_SDP_PARAMS.get(param.toLowerCase());

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
