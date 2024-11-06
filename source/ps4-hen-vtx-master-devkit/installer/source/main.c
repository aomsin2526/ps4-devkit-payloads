#include "ps4.h"
#include "defines.h"
#include "debug.h"

#define PS4_UPDATE_FULL_PATH "/update/PS4UPDATE.PUP"
#define PS4_UPDATE_TEMP_PATH "/update/PS4UPDATE.PUP.net.temp"

#define	KERN_XFAST_SYSCALL	0x1C0		// 5.05
#define KERN_PRISON_0		0x12DA8A8
#define KERN_ROOTVNODE		0x250CC90

#define KERN_PMAP_PROTECT	0x3A1990
#define KERN_PMAP_PROTECT_P	0x3A19D4
#define KERN_PMAP_STORE		0x2516928

#define DT_HASH_SEGMENT		0xD61988

extern char kpayload[];
extern unsigned kpayload_size;

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
  struct payload_header* payload_header = (struct payload_header*)payload_data;
  uint8_t* payload_buffer = (uint8_t*)&kernel_base[DT_HASH_SEGMENT];

  if (!payload_data ||
      payload_size < sizeof(payload_header) ||
      payload_header->signature != 0x5041594C4F414458ull)
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

  // debug settings patches 5.05
  *(char *)(kernel_base + 0x1F1B806) |= 0x14;
  *(char *)(kernel_base + 0x1F1B829) |= 3;
  *(char *)(kernel_base + 0x1F1B82A) |= 1;
  *(char *)(kernel_base + 0x1F1B848) |= 1;

  // debug menu error patches 5.05
  *(uint32_t *)(kernel_base + 0x660f58) = 0;
  *(uint32_t *)(kernel_base + 0x66206c) = 0;

/*
  // flatz disable pfs signature check 5.05
  *(uint32_t *)(kernel_base + 0x6A2700) = 0x90C3C031;

  // flatz enable debug RIFs 5.05
  *(uint32_t *)(kernel_base + 0x64B2B0) = 0x90C301B0;
  *(uint32_t *)(kernel_base + 0x64B2D0) = 0x90C301B0;*/

  // spoof fw

#if 0

  uint32_t spoof_version = 0x99999999;

  *(uint32_t *)(kernel_base + 0x16e6d00) = spoof_version;
  *(uint32_t *)(kernel_base + 0x1ced2d0) = spoof_version;

#endif

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

  int (*payload_entrypoint)();
  *((void**)&payload_entrypoint) =
    (void*)(&payload_buffer[payload_header->entrypoint_offset]);

  payload_result = payload_entrypoint();
  return payload_result;
}

static inline void patch_update(void)
{
  unlink(PS4_UPDATE_FULL_PATH);
  unlink(PS4_UPDATE_TEMP_PATH);

  //mkdir(PS4_UPDATE_FULL_PATH, 0777);
  //mkdir(PS4_UPDATE_TEMP_PATH, 0777);
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

  //patch_update();

#if 0

  initSysUtil();

  char buf[512] = {0};
  sprintf(buf, "Welcome to PS4HEN");

  notify(buf);

#endif

  printfsocket("Done.\n");

#ifdef DEBUG_SOCKET
  closeDebugSocket();
#endif

  return result;
}
