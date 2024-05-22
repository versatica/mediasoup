import { EventEmitter } from 'events';
import { EnhancedEventEmitter } from '../../enhancedEvents';

const buildType = process.env.MEDIASOUP_BUILDTYPE ?? 'Release';

/**
 * NOTE: The following path is reachable from:
 * - current file:       node/src/workerChannel/src/index.ts
 * - transpiled JS file: node/lib/workerChannel/src/index.js
 */
// eslint-disable-next-line @typescript-eslint/no-var-requires
const { WorkerChannel: NativeWorkerChannel } = require(
	`../../../src/workerChannel/build/${buildType}/worker-channel.node`
);

export type WorkerChannelEvents = {
	data: [Buffer];
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

	close() {
		// By setting the instance to `undefined`, the garbage collector will clean up
		// the native instance, calling its `Finalize()` method accordingly.
		// Without it, the tests will never finish due to remaining open handles.
		this.workerChannel = undefined;
	}

	send(data: Uint8Array): void {
		this.workerChannel.send(data);
	}
}
