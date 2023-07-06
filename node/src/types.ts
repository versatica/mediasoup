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
// We cannot export only the type of error classes because those are useless.
export * from './errors';
export type { ScalabilityMode } from './scalabilityModes';

export type AppData =
{
	[key: string]: unknown;
};
