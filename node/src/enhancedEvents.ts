/* eslint-disable @typescript-eslint/no-explicit-any */

import { EventEmitter, once } from 'node:events';

type Events = Record<string, any[]>;

export class EnhancedEventEmitter<
	E extends Events = Events,
	BuiltInEvents extends Events = {
		listenererror: [keyof E, Error];
	},
	E2 extends Events = E & BuiltInEvents,
> extends EventEmitter {
	constructor() {
		super();

		this.setMaxListeners(Infinity);
	}

	override emit<K extends keyof E2 & string>(
		eventName: K,
		...args: E2[K]
	): boolean {
		return super.emit(eventName, ...args);
	}

	/**
	 * Special addition to the EventEmitter API.
	 */
	safeEmit<K extends keyof E2 & string>(eventName: K, ...args: E2[K]): boolean {
		try {
			return super.emit(eventName, ...args);
		} catch (error) {
			try {
				super.emit('listenererror', eventName, error);
			} catch (error2) {
				// Ignore it.
			}

			return Boolean(super.listenerCount(eventName));
		}
	}

	override on<K extends keyof E2 & string>(
		eventName: K,
		listener: (...args: E2[K]) => void
	): this {
		super.on(eventName, listener as (...args: any[]) => void);

		return this;
	}

	override off<K extends keyof E2 & string>(
		eventName: K,
		listener: (...args: E2[K]) => void
	): this {
		super.off(eventName, listener as (...args: any[]) => void);

		return this;
	}

	override addListener<K extends keyof E2 & string>(
		eventName: K,
		listener: (...args: E2[K]) => void
	): this {
		super.on(eventName, listener as (...args: any[]) => void);

		return this;
	}

	override prependListener<K extends keyof E2 & string>(
		eventName: K,
		listener: (...args: E2[K]) => void
	): this {
		super.prependListener(eventName, listener as (...args: any[]) => void);

		return this;
	}

	override once<K extends keyof E2 & string>(
		eventName: K,
		listener: (...args: E2[K]) => void
	): this {
		super.once(eventName, listener as (...args: any[]) => void);

		return this;
	}

	override prependOnceListener<K extends keyof E2 & string>(
		eventName: K,
		listener: (...args: E2[K]) => void
	): this {
		super.prependOnceListener(eventName, listener as (...args: any[]) => void);

		return this;
	}

	override removeListener<K extends keyof E2 & string>(
		eventName: K,
		listener: (...args: E2[K]) => void
	): this {
		super.off(eventName, listener as (...args: any[]) => void);

		return this;
	}

	override removeAllListeners<K extends keyof E2 & string>(
		eventName?: K
	): this {
		super.removeAllListeners(eventName);

		return this;
	}

	override listenerCount<K extends keyof E2 & string>(eventName: K): number {
		return super.listenerCount(eventName);
	}

	// eslint-disable-next-line @typescript-eslint/no-unsafe-function-type
	override listeners<K extends keyof E2 & string>(eventName: K): Function[] {
		return super.listeners(eventName);
	}

	// eslint-disable-next-line @typescript-eslint/no-unsafe-function-type
	override rawListeners<K extends keyof E2 & string>(eventName: K): Function[] {
		return super.rawListeners(eventName);
	}
}

/**
 * TypeScript version of events.once():
 *   https://nodejs.org/api/events.html#eventsonceemitter-name-options
 *
 * Usage example:
 * ```ts
 * await enhancedOnce<ConsumerEvents>(videoConsumer, 'producerpause');
 * ```
 */
export async function enhancedOnce<E extends Events = Events>(
	emmiter: EnhancedEventEmitter<E>,
	eventName: keyof E & string,
	options?: any
): Promise<any[]> {
	return once(emmiter, eventName, options);
}
