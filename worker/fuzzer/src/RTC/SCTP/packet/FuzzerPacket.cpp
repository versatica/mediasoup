#include "RTC/SCTP/packet/FuzzerPacket.hpp"
#include "RTC/SCTP/packet/Packet.hpp"

thread_local static uint8_t PacketSerializeBuffer[65536];
thread_local static uint8_t PacketCloneBuffer[65536];

void Fuzzer::RTC::SCTP::Packet::Fuzz(const uint8_t* data, size_t len)
{
	::RTC::SCTP::Packet* packet = ::RTC::SCTP::Packet::Parse(data, len);

	if (!packet)
	{
		return;
	}

	packet->GetSourcePort();
	packet->GetDestinationPort();
	packet->GetVerificationTag();
	packet->GetChecksum();
	packet->ValidateCRC32cChecksum();
	packet->HasChunks();
	packet->GetChunksCount();
	packet->GetChunkAt(0);
	packet->GetChunkAt(1);
	packet->GetChunkAt(2);
	packet->GetChunkAt(666);

	packet->Serialize(PacketSerializeBuffer, len);

	packet->GetSourcePort();
	packet->SetSourcePort(12345);
	packet->GetDestinationPort();
	packet->SetDestinationPort(54321);
	packet->GetVerificationTag();
	packet->SetVerificationTag(12345678);
	packet->GetChecksum();
	packet->ValidateCRC32cChecksum();
	packet->SetChecksum(999999);
	packet->SetCRC32cChecksum();
	packet->ValidateCRC32cChecksum();
	packet->HasChunks();
	packet->GetChunksCount();
	packet->GetChunkAt(0);
	packet->GetChunkAt(1);
	packet->GetChunkAt(2);
	packet->GetChunkAt(666);

	auto* clonedPacket = packet->Clone(PacketCloneBuffer, len);

	delete packet;

	clonedPacket->GetSourcePort();
	clonedPacket->SetSourcePort(12345);
	clonedPacket->GetDestinationPort();
	clonedPacket->SetDestinationPort(54321);
	clonedPacket->GetVerificationTag();
	clonedPacket->SetVerificationTag(12345678);
	clonedPacket->GetChecksum();
	clonedPacket->ValidateCRC32cChecksum();
	clonedPacket->SetChecksum(999999);
	clonedPacket->SetCRC32cChecksum();
	clonedPacket->ValidateCRC32cChecksum();
	clonedPacket->HasChunks();
	clonedPacket->GetChunksCount();
	clonedPacket->GetChunkAt(0);
	clonedPacket->GetChunkAt(1);
	clonedPacket->GetChunkAt(2);
	clonedPacket->GetChunkAt(666);

	clonedPacket->Serialize(PacketSerializeBuffer, len);

	delete clonedPacket;
}
