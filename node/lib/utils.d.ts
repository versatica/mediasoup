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
export {};
//# sourceMappingURL=utils.d.ts.map