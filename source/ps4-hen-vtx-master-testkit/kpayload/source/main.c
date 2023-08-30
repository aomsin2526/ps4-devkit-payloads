#include <stddef.h>
#include <stdint.h>

#include "sections.h"
#include "sparse.h"
#include "offsets.h"
#include "freebsd_helper.h"
#include "elf_helper.h"
#include "self_helper.h"
#include "sbl_helper.h"
#include "pfs_helper.h"
#include "rif_helper.h"
#include "ccp_helper.h"
#include "amd_helper.h"

size_t (*strlen)(const char *str) PAYLOAD_BSS;
void* (*malloc)(unsigned long size, void* type, int flags) PAYLOAD_BSS;
void (*free)(void* addr, void* type) PAYLOAD_BSS;
void* (*memcpy)(void* dst, const void* src, size_t len) PAYLOAD_BSS;
void* (*memset)(void *s, int c, size_t n) PAYLOAD_BSS;
int (*memcmp)(const void *ptr1, const void *ptr2, size_t num) PAYLOAD_BSS;
int (*sx_xlock)(struct sx *sx, int opts) PAYLOAD_BSS;
int (*sx_xunlock)(struct sx *sx) PAYLOAD_BSS;
int (*fpu_kern_enter)(struct thread *td, struct fpu_kern_ctx *ctx, uint32_t flags) PAYLOAD_BSS;
int (*fpu_kern_leave)(struct thread *td, struct fpu_kern_ctx *ctx) PAYLOAD_BSS;

void* M_TEMP PAYLOAD_BSS;
void* fpu_ctx PAYLOAD_BSS;
uint8_t* mini_syscore_self_binary PAYLOAD_BSS;
struct sbl_map_list_entry** sbl_driver_mapped_pages PAYLOAD_BSS;
struct sx* sbl_pfs_sx PAYLOAD_BSS;
struct proc** allproc PAYLOAD_BSS;

// Fself
void (*sceSblAuthMgrSmStart)(void**) PAYLOAD_BSS;
int (*sceSblServiceMailbox)(unsigned long service_id, uint8_t request[SBL_MSG_SERVICE_MAILBOX_MAX_SIZE], void* response) PAYLOAD_BSS;
int (*sceSblAuthMgrGetSelfInfo)(struct self_context* ctx, struct self_ex_info** info) PAYLOAD_BSS;
int (*sceSblAuthMgrIsLoadable2)(struct self_context* ctx, struct self_auth_info* old_auth_info, int path_id, struct self_auth_info* new_auth_info) PAYLOAD_BSS;
int (*sceSblAuthMgrVerifyHeader)(struct self_context* ctx) PAYLOAD_BSS;

extern int my_sceSblAuthMgrIsLoadable2(struct self_context* ctx, struct self_auth_info* old_auth_info, int path_id, struct self_auth_info* new_auth_info) PAYLOAD_CODE;
extern int my_sceSblAuthMgrVerifyHeader(struct self_context* ctx) PAYLOAD_CODE;
extern int my_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox(unsigned long service_id, uint8_t* request, void* response) PAYLOAD_CODE;
extern int my_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox(unsigned long service_id, uint8_t* request, void* response) PAYLOAD_CODE;

// Fpkg
int (*sceSblPfsKeymgrGenKeys)(union pfs_key_blob* key_blob) PAYLOAD_BSS;
int (*sceSblPfsSetKeys)(uint32_t* ekh, uint32_t* skh, uint8_t* eekpfs, struct ekc* eekc, unsigned int pubkey_ver, unsigned int key_ver, struct pfs_header* hdr, size_t hdr_size, unsigned int type, unsigned int finalized, unsigned int is_disc) PAYLOAD_BSS;
int (*sceSblKeymgrClearKey)(uint32_t kh) PAYLOAD_BSS;
int (*sceSblKeymgrSetKeyForPfs)(union sbl_key_desc* key, unsigned int* handle) PAYLOAD_BSS;
int (*sceSblKeymgrSmCallfunc)(union keymgr_payload* payload) PAYLOAD_BSS;
int (*sceSblDriverSendMsg)(struct sbl_msg* msg, size_t size) PAYLOAD_BSS;
int (*RsaesPkcs1v15Dec2048CRT)(struct rsa_buffer* out, struct rsa_buffer* in, struct rsa_key* key) PAYLOAD_BSS;
int (*AesCbcCfb128Encrypt)(uint8_t* out, const uint8_t* in, size_t data_size, const uint8_t* key, int key_size, uint8_t* iv) PAYLOAD_BSS;
int (*AesCbcCfb128Decrypt)(uint8_t* out, const uint8_t* in, size_t data_size, const uint8_t* key, int key_size, uint8_t* iv) PAYLOAD_BSS;
void (*Sha256Hmac)(uint8_t hash[0x20], const uint8_t* data, size_t data_size, const uint8_t* key, int key_size) PAYLOAD_BSS;

extern int my_sceSblKeymgrSmCallfunc_npdrm_decrypt_isolated_rif(union keymgr_payload* payload) PAYLOAD_CODE;
extern int my_sceSblKeymgrSmCallfunc_npdrm_decrypt_rif_new(union keymgr_payload* payload) PAYLOAD_CODE;
extern int my_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg(struct sbl_msg* msg, size_t size) PAYLOAD_CODE;
extern int my_mountpfs__sceSblPfsSetKeys(uint32_t* ekh, uint32_t* skh, uint8_t* eekpfs, struct ekc* eekc, unsigned int pubkey_ver, unsigned int key_ver, struct pfs_header* hdr, size_t hdr_size, unsigned int type, unsigned int finalized, unsigned int is_disc) PAYLOAD_CODE;

// Patch
int (*proc_rwmem)(struct proc *p, struct uio *uio) PAYLOAD_BSS;
struct vmspace *(*vmspace_acquire_ref)(struct proc *p) PAYLOAD_BSS;
void (*vmspace_free)(struct vmspace *vm) PAYLOAD_BSS;
void (*vm_map_lock_read)(struct vm_map *map) PAYLOAD_BSS;
void (*vm_map_unlock_read)(struct vm_map *map) PAYLOAD_BSS;
int (*vm_map_lookup_entry)(struct vm_map *map, uint64_t address, struct vm_map_entry **entries) PAYLOAD_BSS;

// initialization, etc

extern void install_fself_hooks(void) PAYLOAD_CODE;
extern void install_fpkg_hooks(void)  PAYLOAD_CODE;
extern int shellcore_fpkg_patch(void) PAYLOAD_CODE;

#define resolve(name) name = (void *)(kernbase + name##_addr)
PAYLOAD_CODE void resolve_kdlsym()
{
  uint64_t kernbase = getkernbase();

  // data
  resolve(M_TEMP);
  resolve(fpu_ctx);
  resolve(mini_syscore_self_binary);
  resolve(sbl_driver_mapped_pages);
  resolve(sbl_pfs_sx);
  resolve(allproc);

  // common
  resolve(strlen);
  resolve(malloc);
  resolve(free);
  resolve(memcpy);
  resolve(memset);
  resolve(memcmp);
  resolve(sx_xlock);
  resolve(sx_xunlock);
  resolve(fpu_kern_enter);
  resolve(fpu_kern_leave);

  // Fself
  resolve(sceSblAuthMgrSmStart);
  resolve(sceSblServiceMailbox);
  resolve(sceSblAuthMgrGetSelfInfo);
  resolve(sceSblAuthMgrIsLoadable2);
  resolve(sceSblAuthMgrVerifyHeader);

  // Fpkg
  resolve(sceSblPfsKeymgrGenKeys);
  resolve(sceSblPfsSetKeys);
  resolve(sceSblKeymgrClearKey);
  resolve(sceSblKeymgrSetKeyForPfs);
  resolve(sceSblKeymgrSmCallfunc);
  resolve(sceSblDriverSendMsg);
  resolve(RsaesPkcs1v15Dec2048CRT);
  resolve(AesCbcCfb128Encrypt);
  resolve(AesCbcCfb128Decrypt);
  resolve(Sha256Hmac);

  // Patch
  resolve(proc_rwmem);
  resolve(vmspace_acquire_ref);
  resolve(vmspace_free);
  resolve(vm_map_lock_read);
  resolve(vm_map_unlock_read);
  resolve(vm_map_lookup_entry);
}

PAYLOAD_CODE int my_entrypoint()
{
  resolve_kdlsym();
  install_fself_hooks();
  install_fpkg_hooks();
  //return shellcore_fpkg_patch();
    return 22;
}

struct
{
  uint64_t signature;
  void* entrypoint;
}
payload_header PAYLOAD_HEADER =
{
  0x5041594C4F414458ull,
  &my_entrypoint,
};

int _start()
{
  return 0;
}
