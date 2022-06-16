#include <stddef.h>
#include <stdint.h>

#include "sections.h"
#include "sparse.h"
#include "offsets.h"
#include "freebsd_helper.h"
#include "elf_helper.h"
#include "self_helper.h"
#include "sbl_helper.h"
#include "amd_helper.h"

#define PAGE_SIZE 0x4000

extern void* (*malloc)(unsigned long size, void* type, int flags) PAYLOAD_BSS;
extern void (*free)(void* addr, void* type) PAYLOAD_BSS;
extern void* (*memcpy)(void* dst, const void* src, size_t len) PAYLOAD_BSS;

extern void* M_TEMP PAYLOAD_BSS;
extern struct sbl_map_list_entry** sbl_driver_mapped_pages PAYLOAD_BSS;
extern uint8_t* mini_syscore_self_binary PAYLOAD_BSS;

extern int (*sceSblServiceMailbox)(unsigned long service_id, uint8_t request[SBL_MSG_SERVICE_MAILBOX_MAX_SIZE], void* response) PAYLOAD_BSS;
extern int (*sceSblAuthMgrGetSelfInfo)(struct self_context* ctx, struct self_ex_info** info) PAYLOAD_BSS;
extern void (*sceSblAuthMgrSmStart)(void**) PAYLOAD_BSS;
extern int (*sceSblAuthMgrIsLoadable2)(struct self_context* ctx, struct self_auth_info* old_auth_info, int path_id, struct self_auth_info* new_auth_info) PAYLOAD_BSS;
extern int (*sceSblAuthMgrVerifyHeader)(struct self_context* ctx) PAYLOAD_BSS;

static const uint8_t s_auth_info_for_exec[] PAYLOAD_RDATA =
{
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, 0x20,
  0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x40, 0x00, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00,
  0x00, 0x40, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t s_auth_info_for_dynlib[] PAYLOAD_RDATA =
{
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x30, 0x00, 0x30,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
  0x00, 0x40, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

PAYLOAD_CODE static inline void* alloc(uint32_t size)
{
  return malloc(size, M_TEMP, 2);
}

PAYLOAD_CODE static inline void dealloc(void* addr)
{
  free(addr, M_TEMP);
}

PAYLOAD_CODE static struct sbl_map_list_entry* sceSblDriverFindMappedPageListByGpuVa(vm_offset_t gpu_va)
{
  struct sbl_map_list_entry* entry;
  if (!gpu_va)
  {
    return NULL;
  }
  entry = *sbl_driver_mapped_pages;
  while (entry)
  {
    if (entry->gpu_va == gpu_va)
    {
      return entry;
    }
    entry = entry->next;
  }
  return NULL;
}

PAYLOAD_CODE static inline vm_offset_t sceSblDriverGpuVaToCpuVa(vm_offset_t gpu_va, size_t* num_page_groups)
{
  struct sbl_map_list_entry* entry = sceSblDriverFindMappedPageListByGpuVa(gpu_va);
  if (!entry)
  {
    return 0;
  }
  if (num_page_groups)
  {
    *num_page_groups = entry->num_page_groups;
  }
  return entry->cpu_va;
}

PAYLOAD_CODE static inline int sceSblAuthMgrGetSelfAuthInfoFake(struct self_context* ctx, struct self_auth_info* info)
{
  struct self_header* hdr;
  struct self_fake_auth_info* fake_info;

  if (ctx->format == SELF_FORMAT_SELF)
  {
    hdr = (struct self_header*)ctx->header;
    fake_info = (struct self_fake_auth_info*)(ctx->header + hdr->header_size + hdr->meta_size - 0x100);
    if (fake_info->size == sizeof(fake_info->info))
    {
      memcpy(info, &fake_info->info, sizeof(*info));
      return 0;
    }
    return -37;
  }
  else
  {
    return -35;
  }
}

PAYLOAD_CODE static inline int is_fake_self(struct self_context* ctx)
{
  struct self_ex_info* ex_info;
  if (ctx && ctx->format == SELF_FORMAT_SELF)
  {
    if (sceSblAuthMgrGetSelfInfo(ctx, &ex_info))
    {
      return 0;
    }
    return ex_info->ptype == SELF_PTYPE_FAKE;
  }
  return 0;
}

PAYLOAD_CODE static inline int sceSblAuthMgrGetElfHeader(struct self_context* ctx, struct elf64_ehdr** ehdr)
{
  struct self_header* self_hdr;
  struct elf64_ehdr* elf_hdr;
  size_t pdata_size;

  if (ctx->format == SELF_FORMAT_ELF)
  {
    elf_hdr = (struct elf64_ehdr*)ctx->header;
    if (ehdr)
    {
      *ehdr = elf_hdr;
    }
    return 0;
  }
  else if (ctx->format == SELF_FORMAT_SELF)
  {
    self_hdr = (struct self_header*)ctx->header;
    pdata_size = self_hdr->header_size - sizeof(struct self_entry) * self_hdr->num_entries - sizeof(struct self_header);
    if (pdata_size >= sizeof(struct elf64_ehdr) && (pdata_size & 0xF) == 0)
    {
      elf_hdr = (struct elf64_ehdr*)((uint8_t*)self_hdr + sizeof(struct self_header) + sizeof(struct self_entry) * self_hdr->num_entries);
      if (ehdr)
      {
        *ehdr = elf_hdr;
      }
      return 0;
    }
    return -37;
  }
  return -35;
}

PAYLOAD_CODE static inline int build_self_auth_info_fake(struct self_context* ctx, struct self_auth_info* parent_auth_info, struct self_auth_info* auth_info)
{
  struct self_auth_info fake_auth_info;
  struct elf64_ehdr* ehdr = NULL;
  int result;

  if (!ctx || !parent_auth_info || !auth_info)
  {
    result = 1;
    goto error;
  }

  result = sceSblAuthMgrGetElfHeader(ctx, &ehdr);
  if (result)
  {
    result = 2;
    goto error;
  }

  if (!ehdr)
  {
    result = 3;
    goto error;
  }

  result = sceSblAuthMgrGetSelfAuthInfoFake(ctx, &fake_auth_info);
  if (result)
  {
    switch (ehdr->type)
    {
      case ELF_ET_EXEC:
      case ELF_ET_SCE_EXEC:
      case ELF_ET_SCE_EXEC_ASLR:
      {
        memcpy(&fake_auth_info, s_auth_info_for_exec, sizeof(fake_auth_info));
        result = 0;
        break;
      }

      case ELF_ET_SCE_DYNAMIC:
      {
        memcpy(&fake_auth_info, s_auth_info_for_dynlib, sizeof(fake_auth_info));
        result = 0;
        break;
      }

      default:
      {
        result = 4;
        goto error;
      }
    }
  }

  if (auth_info)
  {
    memcpy(auth_info, &fake_auth_info, sizeof(*auth_info));
  }

error:
  return result;
}

PAYLOAD_CODE static inline int auth_self_header(struct self_context* ctx)
{
  struct self_header* hdr;
  unsigned int old_total_header_size, new_total_header_size;
  int old_format;
  uint8_t* tmp;
  int is_unsigned;
  int result;

  is_unsigned = ctx->format == SELF_FORMAT_ELF || is_fake_self(ctx);
  if (is_unsigned)
  {
    old_format = ctx->format;
    old_total_header_size = ctx->total_header_size;

    /* take a header from mini-syscore.elf */
    //hdr = (struct self_header*)mini_syscore_self_binary;

    static const long int my_self_header_size = 1136;
    static const unsigned char my_self_header[1136] = {
    0x4F, 0x15, 0x3D, 0x1D, 0x00, 0x01, 0x01, 0x12, 0x01, 0x28, 0x00, 0x00, 0x00, 0x02, 0x70, 0x02,
    0xB9, 0x9F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xD8, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD8, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0E, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xC0, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6C, 0xAD, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x8A, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0E, 0x2C, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x8A, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xC0, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x7F, 0x45, 0x4C, 0x46, 0x02, 0x01, 0x01, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x3E, 0x00, 0x01, 0x00, 0x00, 0x00, 0x70, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x38, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x6C, 0xAD, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6C, 0xAD, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xC0, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x81, 0x36, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x51, 0xE5, 0x74, 0x64, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xC0, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x55, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x04, 0x00, 0x00,
    0x20, 0xAE, 0xA1, 0x9B, 0x45, 0x86, 0xF4, 0x99, 0xCC, 0x7D, 0x86, 0xE4, 0xBB, 0xE8, 0x2A, 0x71,
    0x46, 0x2B, 0xC3, 0x3B, 0xEA, 0x25, 0x31, 0x3C, 0x1B, 0x0A, 0x87, 0xFA, 0xCB, 0x8F, 0x21, 0xE9,
    0x00, 0x06, 0x82, 0xB8, 0xD6, 0xF3, 0x60, 0x79, 0x0F, 0xED, 0x33, 0xEC, 0xBC, 0x9A, 0x09, 0x3B,
    0xEB, 0xCA, 0x5F, 0x6D, 0x9F, 0x5D, 0x07, 0x67, 0xBF, 0x28, 0x56, 0xC3, 0x27, 0xE9, 0x8D, 0xD4,
    0x51, 0x87, 0x8F, 0xB7, 0xC5, 0x26, 0xA6, 0x36, 0x19, 0x2C, 0x9B, 0xDE, 0xE0, 0x28, 0x89, 0x85,
    0x77, 0x56, 0xFB, 0x81, 0x20, 0x3F, 0xC4, 0x10, 0xB7, 0xAC, 0xBC, 0xB3, 0xCB, 0xA8, 0x71, 0x73,
    0x46, 0x4E, 0x60, 0xC2, 0x35, 0x9F, 0xC5, 0xE9, 0x9C, 0x5B, 0x90, 0xA1, 0x65, 0x08, 0xCE, 0x32,
    0xC7, 0x70, 0x2B, 0x26, 0xFC, 0xD0, 0x4D, 0x0D, 0xAF, 0x4B, 0x08, 0x02, 0xE5, 0xC9, 0x2C, 0x41,
    0x31, 0x2D, 0x6F, 0x8B, 0xCB, 0xFE, 0xB0, 0x04, 0x61, 0x94, 0x87, 0x13, 0xEF, 0x69, 0x5C, 0x16,
    0xD3, 0x44, 0xCD, 0xE9, 0x62, 0x92, 0x74, 0xB0, 0xD3, 0x4D, 0xC0, 0x25, 0x8D, 0x86, 0x00, 0xD5,
    0x3D, 0x09, 0x3A, 0xE0, 0xD0, 0xC0, 0xE9, 0xA8, 0x0D, 0x4F, 0xA2, 0x02, 0x5E, 0xF8, 0x15, 0x07,
    0x82, 0xB8, 0x23, 0xC3, 0x95, 0x25, 0x56, 0x1E, 0x9F, 0xD5, 0x43, 0xC6, 0x5F, 0xCD, 0x08, 0x5B,
    0xEC, 0xFF, 0x5D, 0xF1, 0x67, 0x83, 0xE1, 0x48, 0x3B, 0xC6, 0x23, 0x07, 0x83, 0xF5, 0x90, 0xF0,
    0xD5, 0x7B, 0x65, 0xCF, 0xCF, 0xB1, 0x72, 0x8E, 0x43, 0xCF, 0x5E, 0xEB, 0x99, 0xB7, 0x43, 0xB0,
    0x84, 0x34, 0x06, 0xCB, 0x01, 0x79, 0x77, 0x42, 0x74, 0x22, 0xAC, 0x03, 0xEF, 0x28, 0x30, 0x52,
    0x90, 0x45, 0xFB, 0x07, 0x08, 0xE2, 0x67, 0x49, 0xEE, 0x50, 0xF4, 0x4D, 0x09, 0x58, 0x44, 0x99,
    0x42, 0x21, 0xD5, 0x92, 0x4A, 0xC4, 0x23, 0xA4, 0x60, 0x05, 0xB1, 0xAA, 0x61, 0xFC, 0x0A, 0x80,
    0x36, 0x17, 0xA6, 0x5D, 0x6B, 0x15, 0x32, 0xBB, 0x04, 0xC4, 0x40, 0xD4, 0xB9, 0x02, 0x26, 0xEB,
    0x1B, 0x46, 0x5E, 0x13, 0x59, 0xEF, 0xFA, 0x78, 0xE0, 0xE2, 0x98, 0x02, 0x9F, 0xCC, 0xED, 0x46,
    0xCD, 0x56, 0xD4, 0x2A, 0x59, 0x8E, 0x36, 0xED, 0x1D, 0x0D, 0x94, 0xB9, 0x1F, 0x51, 0xCF, 0x00,
    0x89, 0xDD, 0x9D, 0xC8, 0x14, 0x8A, 0xE8, 0x2B, 0x3A, 0x69, 0xFE, 0xCE, 0x55, 0x56, 0x38, 0x2A,
    0x42, 0x68, 0x3B, 0x68, 0xAE, 0x29, 0xA7, 0x49, 0xAF, 0xCB, 0xA2, 0x07, 0xED, 0xEE, 0x1A, 0x97,
    0x4A, 0x19, 0xDF, 0xCC, 0x9B, 0x57, 0x60, 0xDB, 0xA4, 0x80, 0x9B, 0x22, 0xF5, 0xAF, 0xB9, 0xAC,
    0x99, 0x8B, 0x73, 0x33, 0xD2, 0xAC, 0xB3, 0xD3, 0x62, 0x6F, 0x61, 0x99, 0xFF, 0x36, 0x59, 0x1F,
    0x2B, 0x0C, 0xFD, 0x88, 0xC2, 0x4B, 0xE7, 0xFC, 0xD4, 0xBD, 0x19, 0xA6, 0x92, 0x47, 0xBC, 0xD2,
    0x25, 0xD8, 0xEA, 0x02, 0x52, 0x16, 0x74, 0xB6, 0x14, 0x26, 0x96, 0x68, 0xEE, 0x86, 0xEC, 0x5D,
    0x10, 0x2F, 0x3C, 0x50, 0xC3, 0xE2, 0xDF, 0x00, 0x16, 0x30, 0xBB, 0xF9, 0xCE, 0x24, 0x4D, 0x37,
    0x94, 0x60, 0xEE, 0x93, 0xD4, 0x0D, 0x96, 0xA2, 0x24, 0x95, 0x75, 0x37, 0xAD, 0x48, 0x13, 0xB8,
    0x17, 0x5C, 0x55, 0xF2, 0x56, 0x1C, 0xA4, 0xBC, 0x80, 0x5D, 0x50, 0x3B, 0xBC, 0xA5, 0xE9, 0xD9,
    0x94, 0xBB, 0xFD, 0xE4, 0x19, 0x2A, 0x86, 0x56, 0x6D, 0xD8, 0x07, 0xEF, 0x82, 0x99, 0xFE, 0xEA,
    0x01, 0x26, 0xE1, 0x08, 0xA7, 0xE9, 0x46, 0xA5, 0xBE, 0x30, 0x36, 0x4E, 0xE8, 0xF9, 0x0E, 0x92,
    0x60, 0xDF, 0x52, 0xEC, 0xA5, 0xE2, 0x94, 0x62, 0xED, 0x21, 0xC9, 0x5F, 0x60, 0x0A, 0xE2, 0x84,
    0x66, 0x7E, 0x58, 0x72, 0x61, 0x38, 0xD2, 0x75, 0x15, 0x7E, 0x4A, 0x1B, 0x1D, 0x6B, 0x13, 0xA7,
    0x11, 0x83, 0x46, 0xD6, 0x00, 0xED, 0xE7, 0x60, 0x4D, 0x47, 0xCA, 0x27, 0x4A, 0x0A, 0x7B, 0x38,
    0xF9, 0xBB, 0x3F, 0xAB, 0x95, 0xE8, 0x2B, 0xDB, 0x54, 0xE6, 0x68, 0xFD, 0x06, 0x3A, 0x17, 0x7E,
    0x9B, 0xD7, 0xA6, 0xF1, 0xCD, 0x32, 0x70, 0x7E, 0xFC, 0x1C, 0x81, 0xFF, 0x62, 0x79, 0xCC, 0xC6,
    0x58, 0xC1, 0xBF, 0xE2, 0x36, 0x58, 0xA5, 0xE8, 0x83, 0x39, 0x27, 0xB3, 0x45, 0x80, 0xFD, 0x70,
    0x0E, 0xAE, 0xF0, 0x8B, 0x73, 0x55, 0x13, 0x45, 0x78, 0xAC, 0x6E, 0xC8, 0x01, 0xBB, 0x60, 0x6B,
    0x9D, 0x2F, 0xF6, 0x5F, 0x02, 0x33, 0x1C, 0xA1, 0x13, 0x3A, 0xB7, 0x57, 0xF5, 0x48, 0x1D, 0xCF,
    0xFE, 0xE3, 0x5D, 0x86, 0x24, 0xB1, 0xFC, 0xE0, 0xEE, 0xDF, 0x34, 0x57, 0x4C, 0xC7, 0xB3, 0xA3,
    0x1D, 0xCC, 0xBC, 0xCA, 0x5D, 0xD1, 0x7D, 0xD9, 0x2F, 0x8C, 0x1F, 0x2D, 0xCC, 0x32, 0xED, 0x80
    };

    hdr = (struct self_header*)my_self_header;

    new_total_header_size = hdr->header_size + hdr->meta_size;

    tmp = (uint8_t*)alloc(new_total_header_size);
    if (!tmp)
    {
      result = ENOMEM;
      goto error;
    }

    /* temporarily swap an our header with a header from a real SELF file */
    memcpy(tmp, ctx->header, new_total_header_size);
    memcpy(ctx->header, hdr, new_total_header_size);

    /* it's now SELF, not ELF or whatever... */
    ctx->format = SELF_FORMAT_SELF;
    ctx->total_header_size = new_total_header_size;

    /* call the original method using a real SELF file */
    result = sceSblAuthMgrVerifyHeader(ctx);

    /* restore everything we did before */
    memcpy(ctx->header, tmp, new_total_header_size);
    ctx->format = old_format;
    ctx->total_header_size = old_total_header_size;

    dealloc(tmp);
  }
  else
  {
    result = sceSblAuthMgrVerifyHeader(ctx);
  }

error:
  return result;
}

PAYLOAD_CODE int my_sceSblAuthMgrIsLoadable2(struct self_context* ctx, struct self_auth_info* old_auth_info, int path_id, struct self_auth_info* new_auth_info)
{
  if (ctx->format == SELF_FORMAT_ELF || is_fake_self(ctx))
  {
    return build_self_auth_info_fake(ctx, old_auth_info, new_auth_info);
  }
  else
  {
    return sceSblAuthMgrIsLoadable2(ctx, old_auth_info, path_id, new_auth_info);
  }
}

PAYLOAD_CODE int my_sceSblAuthMgrVerifyHeader(struct self_context* ctx)
{
  void* dummy;
  sceSblAuthMgrSmStart(&dummy);
  return auth_self_header(ctx);
}

PAYLOAD_CODE int my_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox(unsigned long service_id, uint8_t* request, void* response)
{
  register struct self_context* ctx __asm ("r14"); // 5.05
  int is_unsigned = ctx && (ctx->format == SELF_FORMAT_ELF || is_fake_self(ctx));

  if (is_unsigned)
  {
    *(int*)(response + 0x04) = 0; /* setting error field to zero, thus we have no errors */
    return 0;
  }
  return sceSblServiceMailbox(service_id, request, response);
}

PAYLOAD_CODE int my_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox(unsigned long service_id, uint8_t* request, void* response)
{
  uint8_t* frame = (uint8_t*)__builtin_frame_address(1);
  struct self_context* ctx = *(struct self_context**)(frame - 0x1C8); // 5.05
  vm_offset_t segment_data_gpu_va = *(unsigned long*)(request + 0x08);
  vm_offset_t cur_data_gpu_va = *(unsigned long*)(request + 0x50);
  vm_offset_t cur_data2_gpu_va = *(unsigned long*)(request + 0x58);
  unsigned int data_offset = *(unsigned int*)(request + 0x44);
  unsigned int data_size = *(unsigned int*)(request + 0x48);
  vm_offset_t segment_data_cpu_va, cur_data_cpu_va, cur_data2_cpu_va;
  unsigned int size1;

  int is_unsigned = ctx && (ctx->format == SELF_FORMAT_ELF || is_fake_self(ctx));
  int result;

  if (is_unsigned)
  {
    /* looking into lists of GPU's mapped memory regions */
    segment_data_cpu_va = sceSblDriverGpuVaToCpuVa(segment_data_gpu_va, NULL);
    cur_data_cpu_va = sceSblDriverGpuVaToCpuVa(cur_data_gpu_va, NULL);
    cur_data2_cpu_va = cur_data2_gpu_va ? sceSblDriverGpuVaToCpuVa(cur_data2_gpu_va, NULL) : 0;

    if (segment_data_cpu_va && cur_data_cpu_va)
    {
      if (cur_data2_gpu_va && cur_data2_gpu_va != cur_data_gpu_va && data_offset > 0)
      {
        /* data spans two consecutive memory's pages, so we need to copy twice */
        size1 = PAGE_SIZE - data_offset;
        memcpy((char*)segment_data_cpu_va, (char*)cur_data_cpu_va + data_offset, size1);
        memcpy((char*)segment_data_cpu_va + size1, (char*)cur_data2_cpu_va, data_size - size1);
      }
      else
      {
        memcpy((char*)segment_data_cpu_va, (char*)cur_data_cpu_va + data_offset, data_size);
      }
    }

    *(int*)(request + 0x04) = 0; /* setting error field to zero, thus we have no errors */
    result = 0;
  }
  else
  {
    result = sceSblServiceMailbox(service_id, request, response);
  }

  return result;
}

PAYLOAD_CODE void install_fself_hooks()
{
  uint64_t flags, cr0;
  uint64_t kernbase = getkernbase();

  cr0 = readCr0();
  writeCr0(cr0 & ~X86_CR0_WP);
  flags = intr_disable();

  KCALL_REL32(kernbase, sceSblAuthMgrIsLoadable2_hook, (uint64_t)my_sceSblAuthMgrIsLoadable2);
  KCALL_REL32(kernbase, sceSblAuthMgrVerifyHeader_hook1, (uint64_t)my_sceSblAuthMgrVerifyHeader);
  KCALL_REL32(kernbase, sceSblAuthMgrVerifyHeader_hook2, (uint64_t)my_sceSblAuthMgrVerifyHeader);
  KCALL_REL32(kernbase, sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook, (uint64_t)my_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox);
  KCALL_REL32(kernbase, sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook, (uint64_t)my_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox);

  intr_restore(flags);
  writeCr0(cr0);
}
