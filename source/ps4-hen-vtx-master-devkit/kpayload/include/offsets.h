#ifndef __OFFSETS_H
#define __OFFSETS_H

// 5.05
#define	KERN_XFAST_SYSCALL		0x1C0

// data
#define M_TEMP_addr                     0x16f55f0
#define fpu_ctx_addr                    0x2b94040
#define mini_syscore_self_binary_addr   0x14C9D48 // skip
#define sbl_driver_mapped_pages_addr    0x2b66208
#define sbl_pfs_sx_addr                 0x2b665d8
#define allproc_addr                    0x27aadf8

// common
#define strlen_addr                     0x4bc9f0
#define malloc_addr                     0x167eb0
#define free_addr                       0x1680c0
#define memcpy_addr                     0x267340
#define memset_addr                     0x3f4410
#define memcmp_addr                     0x0630d0
#define sx_xlock_addr                   0x1492d0
#define sx_xunlock_addr                 0x149490
#define fpu_kern_enter_addr             0x235ee0
#define fpu_kern_leave_addr             0x235fe0

// Fself
#define sceSblAuthMgrSmStart_addr       0x7d65d0
#define sceSblServiceMailbox_addr       0x7c7230
#define sceSblAuthMgrGetSelfInfo_addr   0x7d1a30
#define sceSblAuthMgrIsLoadable2_addr   0x7d11e0
#define sceSblAuthMgrVerifyHeader_addr  0x7d7830

// Fpkg
#define sceSblPfsKeymgrGenKeys_addr     0x7c2170
#define sceSblPfsSetKeys_addr           0x7b3c90
#define sceSblKeymgrClearKey_addr       0x7c2800
#define sceSblKeymgrSetKeyForPfs_addr   0x7c2470
#define sceSblKeymgrSmCallfunc_addr     0x7c2f90
#define sceSblDriverSendMsg_addr        0x7b24e0
#define RsaesPkcs1v15Dec2048CRT_addr    0x27f680
#define AesCbcCfb128Encrypt_addr        0x4a41c0
#define AesCbcCfb128Decrypt_addr        0x4a43f0
#define Sha256Hmac_addr                 0x3900d0

// Patch
#define proc_rwmem_addr                 0x3dd6b0
#define vmspace_acquire_ref_addr        0x212e30
#define vmspace_free_addr               0x212c60
#define vm_map_lock_read_addr           0x212fe0
#define vm_map_unlock_read_addr         0x213030
#define vm_map_lookup_entry_addr        0x213600

// Fself hooks
#define sceSblAuthMgrIsLoadable2_hook                             0x7d3091
#define sceSblAuthMgrVerifyHeader_hook1                           0x7d37ec
#define sceSblAuthMgrVerifyHeader_hook2                           0x7d4408
#define sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x7d7e7b
#define sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x7d8a92

// Fpkg hooks
#define sceSblKeymgrSmCallfunc_npdrm_decrypt_isolated_rif_hook    0x7e1410
#define sceSblKeymgrSmCallfunc_npdrm_decrypt_rif_new_hook         0x7e21ef
#define sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x7b8d55
#define mountpfs__sceSblPfsSetKeys_hook1                          0x8433e5
#define mountpfs__sceSblPfsSetKeys_hook2                          0x843614

// SceShellCore patches

// call sceKernelIsGenuineCEX
#define sceKernelIsGenuineCEX_patch1    0x1755ab
#define sceKernelIsGenuineCEX_patch2    0x7a0b9b
#define sceKernelIsGenuineCEX_patch3    0x7ecda3
#define sceKernelIsGenuineCEX_patch4    0x951a0b

// call nidf_libSceDipsw
#define nidf_libSceDipsw_patch1         0x1755d7
#define nidf_libSceDipsw_patch2         0x23cddb
#define nidf_libSceDipsw_patch3         0x7a0bc7
#define nidf_libSceDipsw_patch4         0x951a37

// enable fpkg
//#define enable_fpkg_patch               0x3E0602
 
// debug pkg free string
//#define fake_free_patch                 0xEA96A7

// make pkgs installer working with external hdd
//#define pkg_installer_patch		0x9312A1

#endif
