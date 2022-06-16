#include "ps4.h"
#include "define.h"

#define DEBUG_ENABLED 0

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

int kernel_payload(void* uap)
{
#if DEBUG_ENABLED
    test = 69;
#endif

    kernel_base = ((uint8_t*)(__readmsr(0xC0000082) - 0x1C0));
    
    void(*sceSblSrtcClearTimeDifference)(uint64_t) = (void*)(kernel_base + 0x7c9380);
	void(*sceSblSrtcSetTime)(uint64_t) = (void*)(kernel_base + 0x7c8d80);
	sceSblSrtcClearTimeDifference(15);
	//sceSblSrtcSetTime(2214861963);
    sceSblSrtcSetTime(14861963);

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
        printf("sRet = %d\n", sRet);
        printf("test = %d\n", test);
        printf("kernel_base = %p\n", kernel_base);
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
