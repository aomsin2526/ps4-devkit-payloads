#include "ps4.h"
#include "define.h"
#include "kern-resolver/kernel.h"

//#define DEBUG_ENABLED 1
//#define TESTKIT 1

volatile int g_debugSocket;
char kprintfbuffer[BUF_SIZE];
char bprintfbuffer[BUF_SIZE];
volatile int bprintflength;

volatile int (*sys_sendto)(void *td,void *uap);

uint64_t __readmsr(unsigned long __register)
{
	unsigned long __edx;
	unsigned long __eax;
	__asm__ ("rdmsr" : "=d"(__edx), "=a"(__eax) : "c"(__register));
	return (((uint64_t)__edx) << 32) | (uint64_t)__eax;
}

int createDebugSocket()
{
	struct sockaddr_in server;
	server.sin_len = sizeof(server);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = IP(192, 168, 3, 27);
	server.sin_port = sceNetHtons(9023);
	memset(server.sin_zero, 0, sizeof(server.sin_zero));

	int sock = sceNetSocket("debug", AF_INET, SOCK_STREAM, 0);
	sceNetConnect(sock, (struct sockaddr *)&server, sizeof(server));
	int flag = 1;
	sceNetSetsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

	return sock;
}

#if DEBUG_ENABLED
int test = 0;
#endif

uint8_t* kernel_base = 0;

#define asm __asm__
#define CR0_WP (1 << 16)

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
    struct thread *td;

    uint32_t offset1 = 0x1C0;
    uint32_t offset4 = 0x00488570; // 5.05d

    kernel_base = ((uint8_t*)(__readmsr(0xC0000082) - offset1));
    
    void(*icc_nvs_write)(int, int, int, void*) = (void*)(kernel_base + offset4);
    
    unsigned long uart = 0x1;
  
    icc_nvs_write(4, 0x31F, 1, &uart);
    
    return 0;
}

int _main(void)
{
    initKernel();	
	initLibc();
	initNetwork();
    initPthread();

    g_debugSocket = 0;

#if DEBUG_ENABLED
	g_debugSocket = createDebugSocket();
#endif

	sys_sendto = 0;
	bprintflength = 0;
	memset(bprintfbuffer, 0, sizeof(bprintfbuffer));

#if DEBUG_ENABLED
	printf("hello!\n");

    int pid = syscall(20);
    printf("pid = %d\n", pid); 
#endif

    {   
#if DEBUG_ENABLED
        printf("syscall 11\n");
#endif
        int sRet = syscall(11, kernel_payload, 0);
#if DEBUG_ENABLED
        bflush();
        printf("sRet = %d\n", sRet);
        printf("test = %d\n", test);
#endif
    }

#if DEBUG_ENABLED
    printf("exit!\n");
#endif

err:
    if (g_debugSocket != 0)
	    sceNetSocketClose(g_debugSocket);
	return 0;
}
