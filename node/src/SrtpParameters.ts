import * as flatbuffers from 'flatbuffers';
import * as FbsSrtpParameters from './fbs/srtp-parameters';

/**
 * SRTP parameters.
 */
export type SrtpParameters = {
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

export function cryptoSuiteFromFbs(
	binary: FbsSrtpParameters.SrtpCryptoSuite
): SrtpCryptoSuite {
	switch (binary) {
		case FbsSrtpParameters.SrtpCryptoSuite.AEAD_AES_256_GCM: {
			return 'AEAD_AES_256_GCM';
		}

		case FbsSrtpParameters.SrtpCryptoSuite.AEAD_AES_128_GCM: {
			return 'AEAD_AES_128_GCM';
		}

		case FbsSrtpParameters.SrtpCryptoSuite.AES_CM_128_HMAC_SHA1_80: {
			return 'AES_CM_128_HMAC_SHA1_80';
		}

		case FbsSrtpParameters.SrtpCryptoSuite.AES_CM_128_HMAC_SHA1_32: {
			return 'AES_CM_128_HMAC_SHA1_32';
		}
	}
}

export function cryptoSuiteToFbs(
	cryptoSuite: SrtpCryptoSuite
): FbsSrtpParameters.SrtpCryptoSuite {
	switch (cryptoSuite) {
		case 'AEAD_AES_256_GCM': {
			return FbsSrtpParameters.SrtpCryptoSuite.AEAD_AES_256_GCM;
		}

		case 'AEAD_AES_128_GCM': {
			return FbsSrtpParameters.SrtpCryptoSuite.AEAD_AES_128_GCM;
		}

		case 'AES_CM_128_HMAC_SHA1_80': {
			return FbsSrtpParameters.SrtpCryptoSuite.AES_CM_128_HMAC_SHA1_80;
		}

		case 'AES_CM_128_HMAC_SHA1_32': {
			return FbsSrtpParameters.SrtpCryptoSuite.AES_CM_128_HMAC_SHA1_32;
		}

		default: {
			throw new TypeError(`invalid SrtpCryptoSuite: ${cryptoSuite}`);
		}
	}
}

export function parseSrtpParameters(
	binary: FbsSrtpParameters.SrtpParameters
): SrtpParameters {
	return {
		cryptoSuite: cryptoSuiteFromFbs(binary.cryptoSuite()),
		keyBase64: binary.keyBase64()!,
	};
}

export function serializeSrtpParameters(
	builder: flatbuffers.Builder,
	srtpParameters: SrtpParameters
): number {
	const keyBase64Offset = builder.createString(srtpParameters.keyBase64);

	return FbsSrtpParameters.SrtpParameters.createSrtpParameters(
		builder,
		cryptoSuiteToFbs(srtpParameters.cryptoSuite),
		keyBase64Offset
	);
}
