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

	DataBuffer[0] = 0x00;
	DataBuffer[1] = 0x01;
	DataBuffer[2] = 0x02;
	DataBuffer[3] = 0x03;
	DataBuffer[4] = 0x04;
	DataBuffer[5] = 0x05;
	DataBuffer[6] = 0x06;
	DataBuffer[7] = 0x07;
}
