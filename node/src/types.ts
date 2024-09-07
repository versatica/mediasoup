export type { Observer, ObserverEvents, LogEventListeners } from './index';
export type * from './RtpParameters';
export type * from './SctpParameters';
export type * from './SrtpParameters';
export type * from './scalabilityModes';

export * from './Worker';
export * from './WebRtcServer';
export * from './Router';
export * from './Transport';
export * from './WebRtcTransport';
export * from './PlainTransport';
export * from './PipeTransport';
export * from './DirectTransport';
export * from './Producer';
export * from './Consumer';
export * from './DataProducer';
export * from './DataConsumer';
export * from './RtpObserver';
export * from './ActiveSpeakerObserver';
export * from './AudioLevelObserver';
export * from './errors';

export type AppData = {
	[key: string]: unknown;
};

type Only<T, U> = {
	[P in keyof T]: T[P];
} & {
	[P in keyof U]?: never;
};

export type Either<T, U> = Only<T, U> | Only<U, T>;
