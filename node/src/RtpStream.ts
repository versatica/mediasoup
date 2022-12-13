import * as FbsRtpStream from './fbs/rtpStream_generated';

type BaseStreamStats = {
	timestamp: number;
	ssrc: number;
	rtxSsrc?: number;
	rid?: string;
	kind: string;
	mimeType: string;
	packetsLost: number;
	fractionLost: number;
	packetsDiscarded: number;
	packetsRetransmitted: number;
	packetsRepaired: number;
	nackCount: number;
	nackPacketCount: number;
	pliCount: number;
	firCount: number;
	score: number;
	roundTripTime?: number;
	rtxPacketsDiscarded?: number;
};

type RecvStreamStats = BaseStreamStats & {
	type: string;
	jitter: number;
	packetCount: number;
	byteCount: number;
	bitrate: number;
	bitrateByLayer?: any;
};

type SendStreamStats = BaseStreamStats & {
	type: string;
	packetCount: number;
	byteCount: number;
	bitrate: number;
};

export function parseStreamStats(binary: FbsRtpStream.Stats)
	: RecvStreamStats | SendStreamStats
{
	if (binary.dataType() === FbsRtpStream.StatsData.RecvStats)
		return parseRecvStreamStats(binary);
	else
		return parseSendStreamStats(binary);
}

export function parseRecvStreamStats(binary: FbsRtpStream.Stats): RecvStreamStats
{
	const recvStats = new FbsRtpStream.RecvStats();
	const baseStats = new FbsRtpStream.BaseStats();

	binary.data(recvStats);
	recvStats.base()!.data(baseStats);

	const base = parseBaseStreamStats(baseStats);

	return {
		...base,
		type           : 'inbound-rtp',
		jitter         : recvStats.jitter(),
		byteCount      : Number(recvStats.byteCount()),
		packetCount    : Number(recvStats.packetCount()),
		bitrate        : Number(recvStats.bitrate()),
		bitrateByLayer : parseBitrateByLayer(recvStats)
	};
}

export function parseSendStreamStats(binary: FbsRtpStream.Stats): SendStreamStats
{
	const sendStats = new FbsRtpStream.SendStats();
	const baseStats = new FbsRtpStream.BaseStats();

	binary.data(sendStats);
	sendStats.base()!.data(baseStats);

	const base = parseBaseStreamStats(baseStats);

	return {
		...base,
		type           : 'outbound-rtp',
		byteCount      : Number(sendStats.byteCount()),
		packetCount    : Number(sendStats.packetCount()),
		bitrate        : Number(sendStats.bitrate())
	};
}

function parseBaseStreamStats(binary: FbsRtpStream.BaseStats): BaseStreamStats
{
	return {
		timestamp : Number(binary.timestamp()),
		ssrc      : binary.ssrc(),
		rtxSsrc   : binary.rtxSsrc() ?? undefined,
		rid       : binary.rid() ?? undefined,
		kind      : binary.kind() === FbsRtpStream.MediaKind.AUDIO ?
			'audio' :
			'video',
		mimeType             : binary.mimeType()!,
		packetsLost          : Number(binary.packetsLost()),
		fractionLost         : Number(binary.fractionLost()),
		packetsDiscarded     : Number(binary.packetsDiscarded()),
		packetsRetransmitted : Number(binary.packetsRetransmitted()),
		packetsRepaired      : Number(binary.packetsRepaired()),
		nackCount            : Number(binary.nackCount()),
		nackPacketCount      : Number(binary.nackPacketCount()),
		pliCount             : Number(binary.pliCount()),
		firCount             : Number(binary.firCount()),
		score                : binary.score(),
		roundTripTime        : binary.roundTripTime(),
		rtxPacketsDiscarded  : binary.rtxPacketsDiscarded() ?
			Number(binary.rtxPacketsDiscarded()) :
			undefined
	};
}

function parseBitrateByLayer(binary: FbsRtpStream.RecvStats): any
{
	if (binary.bitrateByLayerLength() === 0)
	{
		return {};
	}

	const bitRateByLayer: {[key: string]: number} = {};

	for (let i=0; i < binary.bitrateByLayerLength(); ++i)
	{
		const layer: string = binary.bitrateByLayer(i)!.layer()!;
		const bitrate = binary.bitrateByLayer(i)!.bitrate();

		bitRateByLayer[layer] = Number(bitrate);
	}
}
