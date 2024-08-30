export type * from './index';
export type * from './Worker';
export type * from './WebRtcServer';
export type * from './Router';
export type * from './Transport';
export type * from './WebRtcTransport';
export type * from './PlainTransport';
export type * from './PipeTransport';
export type * from './DirectTransport';
export type * from './Producer';
export type * from './Consumer';
export type * from './DataProducer';
export type * from './DataConsumer';
export type * from './RtpObserver';
export type * from './ActiveSpeakerObserver';
export type * from './AudioLevelObserver';
export type * from './RtpParameters';
export type * from './SctpParameters';
export type * from './SrtpParameters';
export type * from './scalabilityModes';
export type * from './errors';

export type AppData = {
	[key: string]: unknown;
};

type Only<T, U> = {
	[P in keyof T]: T[P];
} & {
	[P in keyof U]?: never;
};

export type Either<T, U> = Only<T, U> | Only<U, T>;
