import * as flatbuffers from 'flatbuffers';
import { RtpEncodingParameters, RtpParameters } from '../RtpParameters';
import { ProducerType } from '../Producer';
import { Type as FbsRtpParametersType } from './fbs/rtp-parameters/type';
export declare function getRtpParametersType(producerType: ProducerType, pipe: boolean): FbsRtpParametersType;
export declare function serializeRtpParameters(builder: flatbuffers.Builder, rtpParameters: RtpParameters): number;
export declare function serializeRtpEncodingParameters(builder: flatbuffers.Builder, rtpEncodingParameters: RtpEncodingParameters[]): number;
//# sourceMappingURL=utils.d.ts.map