#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include <cstring>             // std::memset

using namespace RTC::SCTP;

thread_local uint8_t FactoryBuffer[];
thread_local uint8_t SerializeBuffer[];
thread_local uint8_t CloneBuffer[];
thread_local uint8_t DataBuffer[];
thread_local uint8_t ThrowBuffer[];

void resetBuffers()
{
	std::memset(FactoryBuffer, 0xAA, sizeof(FactoryBuffer));
	std::memset(SerializeBuffer, 0xBB, sizeof(SerializeBuffer));
	std::memset(CloneBuffer, 0xCC, sizeof(CloneBuffer));
	std::memset(DataBuffer, 0xDD, sizeof(DataBuffer));
	std::memset(ThrowBuffer, 0xEE, sizeof(ThrowBuffer));

	DataBuffer[0]  = 0x00;
	DataBuffer[1]  = 0x01;
	DataBuffer[2]  = 0x02;
	DataBuffer[3]  = 0x03;
	DataBuffer[4]  = 0x04;
	DataBuffer[5]  = 0x05;
	DataBuffer[6]  = 0x06;
	DataBuffer[7]  = 0x07;
	DataBuffer[8]  = 0x08;
	DataBuffer[9]  = 0x09;
	DataBuffer[10] = 0x0A;
	DataBuffer[11] = 0x0B;
	DataBuffer[12] = 0x0C;
	DataBuffer[13] = 0x0D;
	DataBuffer[14] = 0x0E;
	DataBuffer[15] = 0x0F;
}
