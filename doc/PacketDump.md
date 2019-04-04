# Packet Dump

For example, after SRTP decrypting a RTP or SRTP packet, print it in hexadecimal in a Wireshark friendly format:

```c++
void OnRtpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
{
  // [...]

  std::printf("-------------- <decrypted RTP> -----------------\n");

  for (size_t i{0}; i < len; ++i)
  {
    if (i % 8 == 0)
      std::printf("\n%06X ", (unsigned int)i);

    std::printf("%02X ", (unsigned char)data[i]);
  }

  std::printf("\n-------------- </decrypted RTP> ----------------\n");
  std::fflush(stdout);
}
```
