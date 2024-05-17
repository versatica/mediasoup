import { EventEmitter } from 'events';
import { EnhancedEventEmitter } from '../../src/enhancedEvents';

const buildType = process.env.MEDIASOUP_BUILDTYPE ?? 'Release';

// eslint-disable-next-line @typescript-eslint/no-var-requires
const { WorkerChannel: NativeWorkerChannel } = require(
	`../build/${buildType}/worker-channel.node`
);

export type WorkerChannelEvents = {
	data: [Uint8Array];
	error: [number];
};

export class WorkerChannel extends EnhancedEventEmitter<WorkerChannelEvents> {
	private emitter: EventEmitter;
	private workerChannel: typeof NativeWorkerChannel;

	constructor(version: string, args: string[]) {
		super();

		this.emitter = new EventEmitter();
		this.workerChannel = new NativeWorkerChannel(
			this.emitter.emit.bind(this.emitter),
			version,
			args
		);

		this.emitter.on('data', data => {
			this.safeEmit('data', data);
		});

		this.emitter.on('error', code => {
			this.safeEmit('error', code);
		});
	}

	send(data: Uint8Array): void {
		this.workerChannel.send(data);
	}
}
