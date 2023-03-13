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

	return FbsTransport.SrtpParameters.createSrtpParameters(
		builder, cryptoSuiteOffset, keyBase64Offset
	);
}
