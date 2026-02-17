export type RtpStreamRecvStats = BaseRtpStreamStats & {
	type: string;
	packetCount: number;
	byteCount: number;
	bitrate: number;
	bitrateByLayer: BitrateByLayer;
};

export type RtpStreamSendStats = BaseRtpStreamStats & {
	type: string;
	packetCount: number;
	byteCount: number;
	bitrate: number;
};

export type BaseRtpStreamStats = {
	timestamp: number;
	ssrc: number;
	rtxSsrc?: number;
	rid?: string;
	kind: string;
	mimeType: string;
	packetsLost: number;
	fractionLost: number;
	jitter: number;
	packetsDiscarded: number;
	packetsRetransmitted: number;
	packetsRepaired: number;
	nackCount: number;
	nackPacketCount: number;
	pliCount: number;
	firCount: number;
	roundTripTime?: number;
	rtxPacketsDiscarded?: number;
	score: number;
};

export type BitrateByLayer = { [key: string]: number };
