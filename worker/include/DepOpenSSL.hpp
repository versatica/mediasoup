#ifndef MS_DEP_OPENSSL_HPP
#define MS_DEP_OPENSSL_HPP

#include <cstring>

class DepOpenSSL
{
public:
	static void ClassInit();

	static void DetectAESNI();

	struct CPUIDinfo {
			unsigned int EAX;
			unsigned int EBX;
			unsigned int ECX;
			unsigned int EDX;
	};

private:
	static int HasIntelCpu();
	static int HasAESNI();
  static void cpuid_info(CPUIDinfo *info, const unsigned int func, const unsigned int subfunc);
};


inline int DepOpenSSL::HasIntelCpu() {
    DepOpenSSL::CPUIDinfo info;
    DepOpenSSL::cpuid_info(&info, 0, 0);
    if (std::memcmp((char *) (&info.EBX), "Genu", 4) == 0
            && std::memcmp((char *) (&info.EDX), "ineI", 4) == 0
            && std::memcmp((char *) (&info.ECX), "ntel", 4) == 0) {

        return 1;
    }

    return 0;
}


inline int DepOpenSSL::HasAESNI() {
    if (!HasIntelCpu())
        return 0;

    CPUIDinfo info;
    DepOpenSSL::cpuid_info(&info, 1, 0);

    static const unsigned int AESNI_FLAG = (1 << 25);
    if ((info.ECX & AESNI_FLAG) == AESNI_FLAG)
        return 1;

    return 0;
}


inline void DepOpenSSL::cpuid_info(CPUIDinfo *info, unsigned int func, unsigned int subfunc) {
    __asm__ __volatile__ (
            "cpuid"
            : "=a"(info->EAX), "=b"(info->EBX), "=c"(info->ECX), "=d"(info->EDX)
            : "a"(func), "c"(subfunc)
    );
}

#endif
