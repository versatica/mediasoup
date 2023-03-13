import * as flatbuffers from 'flatbuffers';
import * as FbsTransport from './fbs/transport';

/**
 * SRTP parameters.
 */
export type SrtpParameters =
{
	/**
	 * Encryption and authentication transforms to be used.
	 */
	cryptoSuite: SrtpCryptoSuite;

	/**
	 * SRTP keying material (master key and salt) in Base64.
	 */
	keyBase64: string;
};

/**
 * SRTP crypto suite.
 */
export type SrtpCryptoSuite =
	| 'AEAD_AES_256_GCM'
	| 'AEAD_AES_128_GCM'
	| 'AES_CM_128_HMAC_SHA1_80'
	| 'AES_CM_128_HMAC_SHA1_32';

export function parseSrtpParameters(binary: FbsTransport.SrtpParameters): SrtpParameters
{
	return {
		cryptoSuite : binary.cryptoSuite()! as SrtpCryptoSuite,
		keyBase64   : binary.keyBase64()!
	};
}

export function serializeSrtpParameters(
	builder:flatbuffers.Builder, srtpParameters:SrtpParameters
): number
{
	const cryptoSuiteOffset = builder.createString(srtpParameters.cryptoSuite);
	const keyBase64Offset = builder.createString(srtpParameters.keyBase64);

	// NOTE: This check should not be needed since
	// FbsTransport.SrtpParameters.createSrtpParameters() should throw.
	// In absence of 'keyBase64' it does not throw in Linux.
	// TODO: Remove once https://github.com/google/flatbuffers/issues/7739
	// is merged.
	if (!srtpParameters.cryptoSuite)
	{
		throw new TypeError('missing cryptoSuite');
	}
	if (!srtpParameters.keyBase64)
	{
		throw new TypeError('missing keyBase64');
	}

	return FbsTransport.SrtpParameters.createSrtpParameters(
		builder, cryptoSuiteOffset, keyBase64Offset
	);
}
