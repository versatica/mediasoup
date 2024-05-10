import { WorkerChannel } from './';

const version = 'v3';
const args = [''];
const workerChannel = new WorkerChannel(version, args);

workerChannel.on('data', value => {
	console.log('### DATA event ... value: ', value);
});

// Wait for the thread to be created.
setTimeout(() => {
	const uint8 = new Uint8Array(2);

	uint8[0] = 42;

	workerChannel.send(uint8);
}, 1000);
