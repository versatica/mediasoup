/// <reference types="node" />
import { EventEmitter } from 'events';
declare type Events = Record<string, any[]>;
declare type InternalEvents = Record<`@${string}`, any[]>;
export declare class EnhancedEventEmitter<PublicEvents extends Events = Events, E extends PublicEvents = PublicEvents & InternalEvents> extends EventEmitter {
    constructor();
    safeEmit<K extends keyof E & string>(event: K, ...args: E[K]): boolean;
    safeEmitAsPromise<K extends keyof E & string>(event: K, ...args: E[K]): Promise<any>;
    on<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void): this;
    off<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void): this;
    addListener<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void): this;
    prependListener<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void): this;
    once<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void): this;
    prependOnceListener<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void): this;
    removeListener<K extends keyof E & string>(event: K, listener: (...args: E[K]) => void): this;
}
export {};
//# sourceMappingURL=EnhancedEventEmitter.d.ts.map