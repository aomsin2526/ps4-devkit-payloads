#define TESTKIT 1

#include "ps4.h"

#define asm __asm__
#define CR0_WP (1 << 16)

uint64_t __readmsr(unsigned long __register)
{
	unsigned long __edx;
	unsigned long __eax;
	__asm__ ("rdmsr" : "=d"(__edx), "=a"(__eax) : "c"(__register));
	return (((uint64_t)__edx) << 32) | (uint64_t)__eax;
}

static inline u64 cr0_read(void)
{
    u64 reg;
    asm volatile("mov %0, cr0;" : "=r" (reg));
    return reg;
}

static inline void cr0_write(u64 val)
{
    asm volatile("mov cr0, %0;" :: "r" (val));
}

static inline u64 write_protect_disable()
{
    u64 cr0 = cr0_read();
    cr0_write(cr0 & ~CR0_WP);
    return cr0;
}

static inline void write_protect_restore(u64 cr0)
{
    // Use only WP bit of input
    cr0_write(cr0_read() | (cr0 & CR0_WP));
}

int kernel_payload(void* uap)
{
    //*(int*)0 = 0;
    
    uint32_t offset1 = 0x1C0;

#if TESTKIT
    uint32_t offset2 = 0x639198;
    uint32_t offset3 = 0x63919c;
    
    uint32_t offset4 = 0x639160;
#else
#error devkit not supported!!
#endif
    
    uint8_t* kernel_base = 0;
    kernel_base = ((uint8_t*)(__readmsr(0xC0000082) - offset1));
    
    u64 cr0 = write_protect_disable();

    {
        uint8_t* ptr = (uint8_t*)(kernel_base + offset2);

        ptr[0] = 0x90;
        ptr[1] = 0x90;
    }

    {
        uint8_t* ptr = (uint8_t*)(kernel_base + offset3);

        ptr[0] = 0x90;
        ptr[1] = 0x90;
    }

    uint32_t(*sceSblDevActSetStatus)(int) = (void*)(kernel_base + offset4);
    sceSblDevActSetStatus(0);

    write_protect_restore(cr0);
    
    return 0;
}

int _main(void)
{
    //*(int*)0 = 0;
    syscall(11, kernel_payload, 0);

    return 0;
}
