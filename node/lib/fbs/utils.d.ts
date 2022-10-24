import { ProducerType } from '../Producer';
import { Type as FbsRtpParametersType } from './fbs/rtp-parameters/type';
export declare function getRtpParametersType(producerType: ProducerType, pipe: boolean): FbsRtpParametersType;
/**
 * Parse vector into an array of the given type.
 */
export declare function parseVector<Type>(binary: any, methodName: string): Type[];
/**
 * Parse an array of StringUint8 into the corresponding object.
 */
export declare function parseMapStringUint8(binary: any, methodName: string): Record<string, number>;
/**
 * Parse an array of Uint32String into the corresponding object.
 */
export declare function parseMapUint32String(binary: any, methodName: string): Record<number, string>;
//# sourceMappingURL=utils.d.ts.map