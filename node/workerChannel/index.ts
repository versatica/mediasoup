import { createRequire } from 'node:module';
import { EventEmitter } from 'events';

const buildType = process.env.MEDIASOUP_BUILDTYPE ?? 'Release';
const { WorkerChannel: NativeWorkerChannel } = require(
	`../build/${buildType}/worker-channel.node`
);

export class WorkerChannel extends EventEmitter {
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
			this.emit('data', data);
		});

		this.emitter.on('error', code => {
			this.emit('error', code);
		});
	}

	send(data: Uint8Array): void {
		this.workerChannel.send(data);
	}
}
