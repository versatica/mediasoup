import { RtpCapabilities, RtpCodecCapability, RtpHeaderExtension, RtpParameters, RtpCodecParameters, RtcpFeedback, RtpEncodingParameters, RtpHeaderExtensionParameters, RtcpParameters } from './RtpParameters';
import { SctpCapabilities, NumSctpStreams, SctpParameters, SctpStreamParameters } from './SctpParameters';
declare type RtpMapping = {
    codecs: {
        payloadType: number;
        mappedPayloadType: number;
    }[];
    encodings: {
        ssrc?: number;
        rid?: string;
        scalabilityMode?: string;
        mappedSsrc: number;
    }[];
};
/**
 * Validates RtpCapabilities. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateRtpCapabilities(caps: RtpCapabilities): void;
/**
 * Validates RtpCodecCapability. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateRtpCodecCapability(codec: RtpCodecCapability): void;
/**
 * Validates RtcpFeedback. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateRtcpFeedback(fb: RtcpFeedback): void;
/**
 * Validates RtpHeaderExtension. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateRtpHeaderExtension(ext: RtpHeaderExtension): void;
/**
 * Validates RtpParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateRtpParameters(params: RtpParameters): void;
/**
 * Validates RtpCodecParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateRtpCodecParameters(codec: RtpCodecParameters): void;
/**
 * Validates RtpHeaderExtensionParameteters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateRtpHeaderExtensionParameters(ext: RtpHeaderExtensionParameters): void;
/**
 * Validates RtpEncodingParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateRtpEncodingParameters(encoding: RtpEncodingParameters): void;
/**
 * Validates RtcpParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateRtcpParameters(rtcp: RtcpParameters): void;
/**
 * Validates SctpCapabilities. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateSctpCapabilities(caps: SctpCapabilities): void;
/**
 * Validates NumSctpStreams. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateNumSctpStreams(numStreams: NumSctpStreams): void;
/**
 * Validates SctpParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateSctpParameters(params: SctpParameters): void;
/**
 * Validates SctpStreamParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
export declare function validateSctpStreamParameters(params: SctpStreamParameters): void;
/**
 * Generate RTP capabilities for the Router based on the given media codecs and
 * mediasoup supported RTP capabilities.
 */
export declare function generateRouterRtpCapabilities(mediaCodecs?: RtpCodecCapability[]): RtpCapabilities;
/**
 * Get a mapping of codec payloads and encodings of the given Producer RTP
 * parameters as values expected by the Router.
 *
 * It may throw if invalid or non supported RTP parameters are given.
 */
export declare function getProducerRtpParametersMapping(params: RtpParameters, caps: RtpCapabilities): RtpMapping;
/**
 * Generate RTP parameters to be internally used by Consumers given the RTP
 * parameters of a Producer and the RTP capabilities of the Router.
 */
export declare function getConsumableRtpParameters(kind: string, params: RtpParameters, caps: RtpCapabilities, rtpMapping: RtpMapping): RtpParameters;
/**
 * Check whether the given RTP capabilities can consume the given Producer.
 */
export declare function canConsume(consumableParams: RtpParameters, caps: RtpCapabilities): boolean;
/**
 * Generate RTP parameters for a specific Consumer.
 *
 * It reduces encodings to just one and takes into account given RTP capabilities
 * to reduce codecs, codecs' RTCP feedback and header extensions, and also enables
 * or disabled RTX.
 */
export declare function getConsumerRtpParameters(consumableParams: RtpParameters, caps: RtpCapabilities, pipe: boolean): RtpParameters;
/**
 * Generate RTP parameters for a pipe Consumer.
 *
 * It keeps all original consumable encodings and removes support for BWE. If
 * enableRtx is false, it also removes RTX and NACK support.
 */
export declare function getPipeConsumerRtpParameters(consumableParams: RtpParameters, enableRtx?: boolean): RtpParameters;
export {};
//# sourceMappingURL=ortc.d.ts.map