#include "ps4.h"
#include "defines.h"
#include "debug.h"

#define	KERN_XFAST_SYSCALL	0x1C0		// 5.05
#define KERN_PRISON_0		0x12DA8A8
#define KERN_ROOTVNODE		0x250CC90

#define KERN_PMAP_PROTECT	0x3A1990
#define KERN_PMAP_PROTECT_P	0x3A19D4
#define KERN_PMAP_STORE		0x2516928

#define DT_HASH_SEGMENT		0xD61988

extern char kpayload[];
unsigned kpayload_size;

int payload_result = 0;

int install_payload(struct thread *td, struct install_payload_args* args)
{
  struct ucred* cred;
  struct filedesc* fd;

  fd = td->td_proc->p_fd;
  cred = td->td_proc->p_ucred;

  uint8_t* kernel_base = (uint8_t*)(__readmsr(0xC0000082) - KERN_XFAST_SYSCALL);
  uint8_t* kernel_ptr = (uint8_t*)kernel_base;
  void** got_prison0 =   (void**)&kernel_ptr[KERN_PRISON_0];
  void** got_rootvnode = (void**)&kernel_ptr[KERN_ROOTVNODE];

  void (*pmap_protect)(void * pmap, uint64_t sva, uint64_t eva, uint8_t pr) = (void *)(kernel_base + KERN_PMAP_PROTECT);
  void *kernel_pmap_store = (void *)(kernel_base + KERN_PMAP_STORE);

  uint8_t* payload_data = args->payload_info->buffer;
  size_t payload_size = args->payload_info->size;
  uint8_t* payload_buffer = (uint8_t*)&kernel_base[DT_HASH_SEGMENT];

  if (!payload_data)
  {
    return -1;
  }

  cred->cr_uid = 0;
  cred->cr_ruid = 0;
  cred->cr_rgid = 0;
  cred->cr_groups[0] = 0;

  cred->cr_prison = *got_prison0;
  fd->fd_rdir = fd->fd_jdir = *got_rootvnode;

  // escalate ucred privs, needed for access to the filesystem ie* mounting & decrypting files
  void *td_ucred = *(void **)(((char *)td) + 304); // p_ucred == td_ucred
	
  // sceSblACMgrIsSystemUcred
  uint64_t *sonyCred = (uint64_t *)(((char *)td_ucred) + 96);
  *sonyCred = 0xffffffffffffffff;
	
  // sceSblACMgrGetDeviceAccessType
  uint64_t *sceProcType = (uint64_t *)(((char *)td_ucred) + 88);
  *sceProcType = 0x3801000000000013; // Max access
	
  // sceSblACMgrHasSceProcessCapability
  uint64_t *sceProcCap = (uint64_t *)(((char *)td_ucred) + 104);
  *sceProcCap = 0xffffffffffffffff; // Sce Process

  // Disable write protection
  uint64_t cr0 = readCr0();
  writeCr0(cr0 & ~X86_CR0_WP);

  // install kpayload
  memset(payload_buffer, 0, PAGE_SIZE);
  memcpy(payload_buffer, payload_data, payload_size);

  uint64_t sss = ((uint64_t)payload_buffer) & ~(uint64_t)(PAGE_SIZE-1);
  uint64_t eee = ((uint64_t)payload_buffer + payload_size + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE-1);
  kernel_base[KERN_PMAP_PROTECT_P] = 0xEB;
  pmap_protect(kernel_pmap_store, sss, eee, 7);
  kernel_base[KERN_PMAP_PROTECT_P] = 0x75;

  // Restore write protection
  writeCr0(cr0);

  int (*payload_entrypoint)(void *early_printf, void *sys_kexec_ptr);
  *((void**)&payload_entrypoint) =
    (void*)(&payload_buffer[0]);

  payload_result = payload_entrypoint((void *)(kernel_base + 0x584DE0), NULL);
  return payload_result;
}

int usbthing()
{
	printfsocket("Open bzImage file\n");

	FILE *fkernel = NULL;

    if (fkernel == NULL)
    {
        fkernel = fopen("/mnt/usb0/bzImage", "r");
        if (fkernel != NULL)
            notify("Found bzImage from usb0.");
    }

    if (fkernel == NULL)
    {
        fkernel = fopen("/mnt/usb1/bzImage", "r");
        if (fkernel != NULL)
            notify("Found bzImage from usb1.");
    }

    if (fkernel == NULL)
    {
        fkernel = fopen("/mnt/usb2/bzImage", "r");
        if (fkernel != NULL)
            notify("Found bzImage from usb2.");
    }

    if (fkernel == NULL)
    {
        fkernel = fopen("/user/system/boot/bzImage", "r");
        if (fkernel != NULL)
            notify("Found bzImage from /user/system/boot/.");
    }

    printfsocket("Open initramfs file\n");

    FILE *finitramfs = NULL;

    if (finitramfs == NULL)
    {
        finitramfs = fopen("/mnt/usb0/initramfs.cpio.gz", "r");
        if (finitramfs != NULL)
            notify("Found initramfs.cpio.gz from usb0.");
    }

    if (finitramfs == NULL)
    {
        finitramfs = fopen("/mnt/usb1/initramfs.cpio.gz", "r");
        if (finitramfs != NULL)
            notify("Found initramfs.cpio.gz from usb1.");
    }

    if (finitramfs == NULL)
    {
        finitramfs = fopen("/mnt/usb2/initramfs.cpio.gz", "r");
        if (finitramfs != NULL)
            notify("Found initramfs.cpio.gz from usb2.");
    }

    if (finitramfs == NULL)
    {
        finitramfs = fopen("/user/system/boot/initramfs.cpio.gz", "r");
        if (finitramfs != NULL)
            notify("Found initramfs.cpio.gz from /user/system/boot/.");
    }

    if (fkernel == NULL || finitramfs == NULL)
    {
        notify("bzImage or initramfs not found.");

        if (fkernel != NULL) fclose(fkernel);
        if (finitramfs != NULL) fclose(finitramfs);

        return 0;
    }

	fseek(fkernel, 0L, SEEK_END);
	int kernelsize = ftell(fkernel);
	fseek(fkernel, 0L, SEEK_SET);

	fseek(finitramfs, 0L, SEEK_END);
	int initramfssize = ftell(finitramfs);
	fseek(finitramfs, 0L, SEEK_SET);

	printfsocket("kernelsize = %d\n", kernelsize);
	printfsocket("initramfssize = %d\n", initramfssize);

	printfsocket("Checks if the files are here\n");
	if(kernelsize == 0 || initramfssize == 0) {
		notify("kernelsize/initramfssize = 0");
		fclose(fkernel);
		fclose(finitramfs);
		return 0;
	}

	void *kernel, *initramfs;
	char *cmd_line = "panic=0 clocksource=tsc radeon.dpm=0 console=tty0 console=ttyS0,115200n8 "
			"console=uart8250,mmio32,0xd0340000 video=HDMI-A-1:1920x1080-24@60 "
			"consoleblank=0 net.ifnames=0 drm.debug=0";
	
	kernel = malloc(kernelsize);
	initramfs = malloc(initramfssize);

	printfsocket("kernel = %llp\n", kernel);
	printfsocket("initramfs = %llp\n", initramfs);

	fread(kernel, kernelsize, 1, fkernel);
	fread(initramfs, initramfssize, 1, finitramfs);

	fclose(fkernel);
	fclose(finitramfs);

    int vram_gb = 2;

	//Call sys_kexec (153 syscall)
	syscall(153, kernel, kernelsize, initramfs, initramfssize, cmd_line, vram_gb);

	free(kernel);
	free(initramfs);

    return 1;
}



int _main(struct thread *td) {
  int result;

  initKernel();	
  initLibc();

#ifdef DEBUG_SOCKET
  initNetwork();
  initDebugSocket();
#endif

  printfsocket("Starting...\n");

  struct payload_info payload_info;
  payload_info.buffer = (uint8_t *)kpayload;
  payload_info.size = (size_t)kpayload_size;

  errno = 0;

  result = kexec(&install_payload, &payload_info);
  result = !result ? 0 : errno;
  printfsocket("install_payload: %d\n", result);

  initSysUtil();

  char buf[512] = {0};
  sprintf(buf, "Welcome to Linux Loader!");

  notify(buf);

  int loadResult = usbthing();

#ifdef DEBUG_SOCKET
  closeDebugSocket();
#endif

  notify("Done.");

  if (loadResult == 1)
  {
    sceKernelSleep(20);
 
    //Reboot PS4
    int evf = syscall(540, "SceSysCoreReboot");
    syscall(546, evf, 0x4000, 0);
    syscall(541, evf);
    syscall(37, 1, 30);
  }

  return result;
}
