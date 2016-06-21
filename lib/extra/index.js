'use strict';

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
