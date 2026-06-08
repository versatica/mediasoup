import type * as flatbuffers from 'flatbuffers';
import type {
	SctpStreamParameters,
	SctpParameters,
} from './sctpParametersTypes';
import * as FbsSctpParameters from './fbs/sctp-parameters';

export function parseSctpParameters(
	binary: FbsSctpParameters.SctpParameters
): SctpParameters {
	return {
		port: binary.port(),
		maxSendMessageSize: binary.maxSendMessageSize(),
		maxReceiveMessageSize: binary.maxReceiveMessageSize(),
		sendBufferSize: binary.sendBufferSize(),
		perStreamSendQueueLimit: binary.perStreamSendQueueLimit(),
		maxReceiverWindowBufferSize: binary.maxReceiverWindowBufferSize(),
		isDataChannel: binary.isDataChannel(),

		// TODO: SCTP: For backwards compatibility. Remove them in the future.
		OS: 65535,
		MIS: 65535,
		maxMessageSize: binary.maxReceiveMessageSize(),
	};
}

export function serializeSctpStreamParameters(
	builder: flatbuffers.Builder,
	parameters: SctpStreamParameters
): number {
	return FbsSctpParameters.SctpStreamParameters.createSctpStreamParameters(
		builder,
		parameters.streamId,
		parameters.ordered!,
		typeof parameters.maxPacketLifeTime === 'number'
			? parameters.maxPacketLifeTime
			: null,
		typeof parameters.maxRetransmits === 'number'
			? parameters.maxRetransmits
			: null
	);
}

export function parseSctpStreamParameters(
	parameters: FbsSctpParameters.SctpStreamParameters
): SctpStreamParameters {
	return {
		streamId: parameters.streamId(),
		ordered: parameters.ordered()!,
		maxPacketLifeTime:
			parameters.maxPacketLifeTime() !== null
				? parameters.maxPacketLifeTime()!
				: undefined,
		maxRetransmits:
			parameters.maxRetransmits() !== null
				? parameters.maxRetransmits()!
				: undefined,
	};
}
