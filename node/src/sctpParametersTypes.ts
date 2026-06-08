export type SctpParameters = {
	/**
	 * SCTP source port of the transport.
	 */
	port: number;

	/**
	 * Maximum size for SCTP messages sent by DataConsumers (in bytes).
	 */
	maxSendMessageSize: number;

	/**
	 * Maximum size for SCTP messages received by DataProducers (in bytes).
	 */
	maxReceiveMessageSize: number;

	/**
	 * Maximum SCTP send buffer used by DataConsumers (in bytes).
	 */
	sendBufferSize: number;

	/**
	 * Per stream send queue size limit. Similar to `sendBufferSize`, but
	 * limiting the size of individual streams.
	 */
	perStreamSendQueueLimit: number;

	/**
	 * Maximum received window buffer size (in bytes).
	 */
	maxReceiverWindowBufferSize: number;

	/**
	 * Whether this is a WebRTC DataChannel based SCTP association. Only `true`
	 * in WebRTC transports.
	 */
	isDataChannel: boolean;

	// TODO: SCTP: For backwards compatibility. Remove them in the future.
	OS: number;
	MIS: number;
	maxMessageSize: number;
};

export type SctpNegotiatedCapabilities = {
	negotiatedMaxOutboundStreams: number;
	negotiatedMaxInboundStreams: number;
};

/**
 * SCTP stream parameters describe the reliability of a certain SCTP stream.
 * If ordered is true then maxPacketLifeTime and maxRetransmits must be
 * false.
 * If ordered if false, only one of maxPacketLifeTime or maxRetransmits
 * can be true.
 */
export type SctpStreamParameters = {
	/**
	 * SCTP stream id.
	 */
	streamId: number;

	/**
	 * Whether data messages must be received in order. If true the messages will
	 * be sent reliably. Default true.
	 */
	ordered?: boolean;

	/**
	 * When ordered is false indicates the time (in milliseconds) after which a
	 * SCTP packet will stop being retransmitted.
	 */
	maxPacketLifeTime?: number;

	/**
	 * When ordered is false indicates the maximum number of times a packet will
	 * be retransmitted.
	 */
	maxRetransmits?: number;
};
