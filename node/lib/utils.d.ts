import { ProducerType } from './Producer';
import { Type as FbsRtpParametersType } from './fbs/fbs/rtp-parameters/type';
/**
 * Clones the given object/array.
 */
export declare function clone(data: any): any;
/**
 * Generates a random positive integer.
 */
export declare function generateRandomNumber(): number;
declare type Only<T, U> = {
    [P in keyof T]: T[P];
} & {
    [P in keyof U]?: never;
};
export declare type Either<T, U> = Only<T, U> | Only<U, T>;
/**
 * Get the flatbuffers RtpParameters type of a given Producer.
 */
export declare function getRtpParametersType(producerType: ProducerType, pipe: boolean): FbsRtpParametersType;
/**
 * Parse flatbuffers vector into an array of the given type.
 */
export declare function parseVector<Type>(binary: any, methodName: string, parseFn?: (binary2: any) => Type): Type[];
/**
 * Parse flatbuffers vector of StringUint8 into the corresponding array.
 */
export declare function parseStringUint8Vector(binary: any, methodName: string): {
    key: string;
    value: number;
}[];
/**
 * Parse flatbuffers of Uint16String into the corresponding array.
 */
export declare function parseUint16StringVector(binary: any, methodName: string): {
    key: number;
    value: string;
}[];
/**
 * Parse flatbuffers of Uint32String into the corresponding array.
 */
export declare function parseUint32StringVector(binary: any, methodName: string): {
    key: number;
    value: string;
}[];
export {};
//# sourceMappingURL=utils.d.ts.map