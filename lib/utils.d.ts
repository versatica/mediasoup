declare const randomNumberGenerator: (min?: number | null | undefined, max?: number | null | undefined, integer?: boolean | null | undefined) => number;
/**
 * Clones the given object/array.
 */
export declare function clone(data: any): any;
/**
 * Generates a random positive integer.
 */
export { randomNumberGenerator as generateRandomNumber };
/**
 * Get IPv4 IP addresses from all network interfaces available
 */
export declare function getIpv4IPs(): string[];
/**
 * Get IPv4 and IPv6 IP addresses from all network interfaces available
 */
export declare function getAllIPs(): string[];
export declare function flatten<T>(arr: Array<T | T[]>): T[];
//# sourceMappingURL=utils.d.ts.map