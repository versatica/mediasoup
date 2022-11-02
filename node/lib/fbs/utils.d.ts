import { ProducerType } from '../Producer';
import { Type as FbsRtpParametersType } from './fbs/rtp-parameters/type';
export declare function getRtpParametersType(producerType: ProducerType, pipe: boolean): FbsRtpParametersType;
/**
 * Parse vector into an array of the given type.
 */
export declare function parseVector<Type>(binary: any, methodName: string): Type[];
/**
 * Parse an vector of StringUint8 into the corresponding array.
 */
export declare function parseStringUint8Vector(binary: any, methodName: string): {
    key: string;
    value: number;
}[];
/**
 * Parse a vector of Uint32String into the corresponding array.
 */
export declare function parseUint32StringVector(binary: any, methodName: string): {
    key: number;
    value: string;
}[];
//# sourceMappingURL=utils.d.ts.map