'use strict';

module.exports =
{
	/**
	 * Generate a raw DTLS fingerprint
	 * @param  {String} sdpFingerprint  SDP fingerprint (uppercase with colons)
	 * @return {String} Raw fingerprint (lowercase without colons)
	 */
	dtlsFingerprintFromSDP: (sdpFingerprint) =>
	{
		if (typeof sdpFingerprint !== 'string' || !sdpFingerprint)
			throw new TypeError('wrong argument');

		return sdpFingerprint
			.replace(/:/g, '')
			.toLowerCase();
	},

	/**
	 * Generate a SDP DTLS fingerprint
	 * @param  {String} rawFingerprint  Raw fingerprint (lowercase without colons)
	 * @return {String} SDP fingerprint (uppercase with colons)
	 */
	dtlsFingerprintToSDP: (rawFingerprint) =>
	{
		if (typeof rawFingerprint !== 'string' || !rawFingerprint)
			throw new TypeError('wrong argument');

		return rawFingerprint
			.replace(/../g, (xx) => xx + ':')
			.toUpperCase()
			.slice(0, -1);
	}
};
