/**
 * SRTP parameters.
 */
export declare type SrtpParameters = {
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
export declare type SrtpCryptoSuite = 'AES_CM_128_HMAC_SHA1_80' | 'AES_CM_128_HMAC_SHA1_32';
//# sourceMappingURL=SrtpParameters.d.ts.map