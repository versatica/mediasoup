import { RtpCapabilities, RtpCodecCapability, RtpParameters } from './RtpParameters';
interface RtpMapping {
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
}
/**
 * Generate RTP capabilities for the Router based on the given media codecs and
 * mediasoup supported RTP capabilities.
 */
export declare function generateRouterRtpCapabilities(mediaCodecs?: RtpCodecCapability[]): RTCRtpCapabilities;
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
export declare function getConsumerRtpParameters(consumableParams: RtpParameters, caps: RtpCapabilities): RtpParameters;
/**
 * Generate RTP parameters for a pipe Consumer.
 *
 * It keeps all original consumable encodings, removes RTX support and also
 * other features such as NACK.
 */
export declare function getPipeConsumerRtpParameters(consumableParams: RtpParameters): RtpParameters;
export {};
//# sourceMappingURL=ortc.d.ts.map