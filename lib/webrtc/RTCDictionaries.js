'use strict';

module.exports =
{
	RTCSdpType :
	{
		offer  : 'offer',
		answer : 'answer'
	},
	RTCSignalingState :
	{
		stable          : 'stable',
		haveLocalOffer  : 'have-local-offer',
		haveRemoteOffer : 'have-remote-offer'
	},
	RTCRtpTransceiverDirection :
	{
		sendrecv : 'sendrecv',
		sendonly : 'sendonly',
		recvonly : 'recvonly',
		inactive : 'inactive'
	}
};
