#include "../src/frame_buffer_config.hpp"
#include "Protocol/GraphicsOutput.h"
#include "Uefi/UefiBaseType.h"
#include <Guid/FileInfo.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Uefi.h>

// #@@range_begin(struct_memory_map)
struct MemoryMap
{
  UINTN buffer_size;
  /**
   * MemoryDesc buffer[map_size/sizeof(MemoryDesc)]
   */
  VOID *buffer;
  /**
   * [IN] buffer size in bytes that can be written
   * [OUT] buffer size in bytes that is written
   */
  UINTN map_size;
  /**
   * The Unique Identifier of the current Memory Map
   */
  UINTN map_key;
  /**
   * Maybe larger than a MemoryDesc
   */
  UINTN descriptor_size;
  /**
   * MemoryDesc version
   */
  UINT32 descriptor_version;
};
// #@@range_end(struct_memory_map)

// #@@range_begin(get_memory_type)
const CHAR16 *GetMemoryTypeUnicode(EFI_MEMORY_TYPE type)
{
  switch (type)
  {
  case EfiReservedMemoryType:
    return L"EfiReservedMemoryType";
  case EfiLoaderCode:
    return L"EfiLoaderCode";
  case EfiLoaderData:
    return L"EfiLoaderData";
  case EfiBootServicesCode:
    return L"EfiBootServicesCode";
  case EfiBootServicesData:
    return L"EfiBootServicesData";
  case EfiRuntimeServicesCode:
    return L"EfiRuntimeServicesCode";
  case EfiRuntimeServicesData:
    return L"EfiRuntimeServicesData";
  case EfiConventionalMemory:
    return L"EfiConventionalMemory";
  case EfiUnusableMemory:
    return L"EfiUnusableMemory";
  case EfiACPIReclaimMemory:
    return L"EfiACPIReclaimMemory";
  case EfiACPIMemoryNVS:
    return L"EfiACPIMemoryNVS";
  case EfiMemoryMappedIO:
    return L"EfiMemoryMappedIO";
  case EfiMemoryMappedIOPortSpace:
    return L"EfiMemoryMappedIOPortSpace";
  case EfiPalCode:
    return L"EfiPalCode";
  case EfiPersistentMemory:
    return L"EfiPersistentMemory";
  case EfiMaxMemoryType:
    return L"EfiMaxMemoryType";
  default:
    return L"InvalidMemoryType";
  }
}
// #@@range_end(get_memory_type)

/**
 * Loop through all MemoryMapDesc and write to a file in CSV
 */
// #@@range_begin(save_memory_map)
EFI_STATUS SaveMemoryMap(struct MemoryMap *map, EFI_FILE_PROTOCOL *file)
{
  CHAR8 buf[256];
  UINTN len;

  CHAR8 *header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  len = AsciiStrLen(header);
  file->Write(file, &len, header);

  // Print(L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer,
  // map->map_size);

  EFI_PHYSICAL_ADDRESS iter;
  int i;
  for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0; iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
       iter += map->descriptor_size, i++)
  {
    EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)iter;
    len = AsciiSPrint(buf, sizeof(buf), "%u, %x, %-ls, %08lx, %lx, %lx\n", i, desc->Type,
                      GetMemoryTypeUnicode(desc->Type), desc->PhysicalStart, desc->NumberOfPages,
                      desc->Attribute & 0xffffflu);
    file->Write(file, &len, buf);
  }

  return EFI_SUCCESS;
}
// #@@range_end(save_memory_map)

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL **root)
{
  EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

  gBS->OpenProtocol(image_handle, &gEfiLoadedImageProtocolGuid, (VOID **)&loaded_image, image_handle, NULL,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  gBS->OpenProtocol(loaded_image->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&fs, image_handle, NULL,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}
struct FrameBufferConfig frameBufferConfig = {0};

/**
 * Graphics Output Protocol
 * It has basically the same functions as VESA, you can query the modes, set
 * the modes. It also provides an efficient BitBlitter function, which you
 * can't use from your OS unfortunately. GOP is an EFI Boot Time Service,
 * meaning you can't access it after you call ExitBootServices(). However,
 * the framebuffer provided by GOP persists, so you can continue to use it
 * for graphics output in your OS.
 */
EFI_STATUS
OpenGOP(EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL **gop)
{
  UINTN num_gop_handles = 0;
  EFI_HANDLE *gop_handles = NULL;
  gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &num_gop_handles, &gop_handles);

  gBS->OpenProtocol(gop_handles[0], &gEfiGraphicsOutputProtocolGuid, (VOID **)gop, image_handle, NULL,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  FreePool(gop_handles);

  return EFI_SUCCESS;
}

EFI_STATUS gopQueryAndSet(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop)
{

  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *gopModeInfo = NULL;
  gBS->AllocatePages(AllocateAnyPages, EfiConventionalMemory,
                     (sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION) + 0xfff) / 0x1000,
                     (EFI_PHYSICAL_ADDRESS *)gopModeInfo);
  gBS->SetMem((void *)gopModeInfo, ((sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION) + 0xfff) / 0x1000) * 0x1000, 0);

  for (UINT32 modeNum = 0; modeNum < gop->Mode->MaxMode; modeNum++)
  {
    /* OUT SizeOfInfo; A pointer to the size, in bytes, of the Info */
    UINTN sizeOfInfo = 0;
    gop->QueryMode(gop, modeNum, &sizeOfInfo, &gopModeInfo);
    // Print(L"%ld:%ldx%ld,%ld,%ld;", modeNum,
    // gopModeInfo->HorizontalResolution,
    //      gopModeInfo->VerticalResolution, gopModeInfo->PixelsPerScanLine,
    //      gopModeInfo->PixelFormat);
  }
  /**
   * It seems QEMU may not support a resolution? And screen will skew and bugs out at 1366x768 (mode 16)
   */
  EFI_STATUS status0;
  status0 = gop->SetMode(gop, 17);

  return status0;
}

const CHAR16 *GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt)
{
  switch (fmt)
  {
  case PixelRedGreenBlueReserved8BitPerColor:
    return L"PixelRedGreenBlueReserved8BitPerColor";
  case PixelBlueGreenRedReserved8BitPerColor:
    return L"PixelBlueGreenRedReserved8BitPerColor";
  case PixelBitMask:
    return L"PixelBitMask";
  case PixelBltOnly:
    return L"PixelBltOnly";
  case PixelFormatMax:
    return L"PixelFormatMax";
  default:
    return L"InvalidPixelFormat";
  }
}
/**
 *
 * @retval EFI_INVALID_PARAMETER Null pointers
 * @retval EFI_UNSUPPORTED       The pixel format is not supported
 */
EFI_STATUS SetKernelFrameBufferConfig(FrameBufferConfig *fbc, const EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *m)
{
  if (!fbc || !m)
    return EFI_INVALID_PARAMETER;
  if (m->Info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)
    fbc->pixel_format = kPixelRGBResv8BitPerColor;
  else if (m->Info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
    fbc->pixel_format = kPixelBGRResv8BitPerColor;
  else
    return EFI_UNSUPPORTED;

  fbc->frame_buffer_base = m->FrameBufferBase;
  fbc->horizontal_resolution = m->Info->HorizontalResolution;
  fbc->vertical_resolution = m->Info->VerticalResolution;
  fbc->pixels_per_scan_line = m->Info->PixelsPerScanLine;
  return EFI_SUCCESS;
}

/**
 * Call UEFI gBS->GetMemoryMap and store the result in map
 * according to UEFI spec
 */
// #@@range_begin(get_memory_map)
EFI_STATUS GetMemoryMap(struct MemoryMap *map)
{
  if (map->buffer == NULL)
  {
    return EFI_BUFFER_TOO_SMALL;
  }

  map->map_size = map->buffer_size;
  return gBS->GetMemoryMap(&map->map_size, (EFI_MEMORY_DESCRIPTOR *)map->buffer, &map->map_key, &map->descriptor_size,
                           &map->descriptor_version);
}
// #@@range_end(get_memory_map)

typedef struct ELF64_HEADER
{
  unsigned char e_ident[16];
  UINT16 e_type;
  UINT16 e_machine;
  UINT32 e_version;
  UINTN e_entry;
  UINTN e_phoff;
  UINTN e_shoff;
  UINT32 e_flags;
  UINT16 e_ehsize;
  UINT16 e_phentsize;
  UINT16 e_phnum;
  UINT16 e_shentsize;
  UINT16 e_shnum;
  UINT16 e_shstrndx;
} ELF64_HEADER;

/**
 * ELF Program Header
 * It is found at file offset e_phoff, and consists of e_phnum entries, each
 * with size e_phentsize.
 */
typedef struct ELF64_PGN_HEADER
{
  /* 0x00000001	PT_LOAD	Loadable segment. */
  UINT32 p_type;
  /**
   * Segment-dependent flags (position for 64-bit structure).
   * PF_X 1, PF_W 2, PF_R 4 => .text R E == 0x5
   */
  UINT32 p_flags;
  /* Offset of the segment in the file image. */
  UINTN p_offset;
  /* Virtual address of the segment in memory. */
  UINTN p_vaddr;
  /* On systems where physical address is relevant, reserved for segment's
   * physical address. */
  UINTN p_paddr;
  /* Size in bytes of the segment in the file image. May be 0. */
  UINTN p_filesz;
  /* Size in bytes of the segment in memory. May be 0. */
  UINTN p_memsz;
  /* 0 and 1 specify no alignment. Otherwise should be a positive, integral
   * power of 2, with p_vaddr equating p_offset modulus p_align. */
  UINTN p_align;
} ELF64_PGN_HEADER;

/**
 * Load raw elf in the raw_elf_addr, into kernel_base_addr
 *
 * When loading an elf, even when the elf is stripped, the eh.e_entry is always
 * available
 * TODO how do actual loader works?
 * For now check p_flags, find the R E segment and treat as .text
 *
 * https://krinkinmu.github.io/2020/11/15/loading-elf-image.html
 * [load_elf_binary,
 * Linux](https://elixir.bootlin.com/linux/v3.18/source/fs/binfmt_elf.c#L571)
 */
static EFI_STATUS load_elf_image(EFI_PHYSICAL_ADDRESS kernel_base_addr, EFI_PHYSICAL_ADDRESS raw_elf_addr)
{
  ELF64_HEADER eh = {0};
  eh.e_entry = *(UINTN *)(raw_elf_addr + 0x18);      // e.g. 0x101120
  eh.e_phentsize = *(UINT16 *)(raw_elf_addr + 0x36); //
  eh.e_phnum = *(UINT16 *)(raw_elf_addr + 0x38);     // e.g., 0x4
  eh.e_phoff = *(UINTN *)(raw_elf_addr + 0x20);      // e.g., 0x40
                                                     //
  /* Go through all program headers and load their content into memory */
  for (UINTN i = 0; i < eh.e_phnum; i++)
  {
    ELF64_PGN_HEADER eph = {0};
    eph = *((ELF64_PGN_HEADER *)(eh.e_phoff + raw_elf_addr) + i);
    UINT32 PT_LOAD = 1;
    if (eph.p_type != PT_LOAD)
      continue;

    gBS->CopyMem((void *)(eph.p_vaddr), (void *)(raw_elf_addr + eph.p_offset), eph.p_filesz);
    Print(L"FoundPT_LOAD,dst:0x%lx, %ldbytes ", eph.p_vaddr, eph.p_filesz);
    /* dst, src, bytes */
    gBS->CopyMem((void *)(eph.p_vaddr), (void *)(raw_elf_addr + eph.p_offset), eph.p_filesz);
    Print(L"dst->0x%lx", *(UINT64 *)eph.p_vaddr);
  }
  return EFI_SUCCESS;
}

/**
 * The signature of the Entry point is as per the UEFI specification
 *   - Name defined in the ".INF" file
 *
 * The second argument, the EFI_SYSTEM_TABLE contains the standard output and
 * input handles, plus pointers to the EFI_BOOT_SERVICES and
 * EFI_RUNTIME_SERVICES tables
 *
 * gBS == system_table->BootServices
 *
 * ref:
 * https://uefi.org/specs/UEFI/2.10/04_EFI_System_Table.html?highlight=boot_service
 * https://tianocore-docs.github.io/edk2-ModuleWriteGuide/draft/4_uefi_applications/42_write_uefi_application_entry_point.html
 */
EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
  // Print(L"Hello, Unagi!\n");
  DEBUG((EFI_D_INFO, "UefiMain Entry: 0x%08x\r\n", (CHAR16 *)UefiMain));

  /**
   * Get memory map, write to a file in the image
   */
  // #@@range_begin(main)
  CHAR8 memmap_buf[4096 * 16];
  struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
  GetMemoryMap(&memmap);

  EFI_FILE_PROTOCOL *root_dir;
  OpenRootDir(image_handle, &root_dir);

  EFI_FILE_PROTOCOL *memmap_file;
  root_dir->Open(root_dir, &memmap_file, L"\\memmap", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                 0);

  SaveMemoryMap(&memmap, memmap_file);
  memmap_file->Close(memmap_file);
  // #@@range_end(main)

  /**
   * Graphics Output Protocol
   * 	- Get gop handle
   */
  // #@@range_begin(gop)
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  OpenGOP(image_handle, &gop);
  // #@@range_end(gop)

  // #@@range_begin(read_kernel)
  EFI_FILE_PROTOCOL *kernel_file;

  /* If the filename starts with a “\” the relative location is the root
   * directory that This resides on; */
  root_dir->Open(root_dir, &kernel_file, L"\\kernel.elf", EFI_FILE_MODE_READ, 0);

  /**
   * 12 CHAR16 for the filename, which is counted as 0 or 1 (NULL) byte in
   * the EFI_FILE_INFO
   */
  UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
  UINT8 file_info_buffer[file_info_size];
  kernel_file->GetInfo(kernel_file, &gEfiFileInfoGuid, &file_info_size, file_info_buffer);

  EFI_FILE_INFO *file_info = (EFI_FILE_INFO *)file_info_buffer;
  UINTN kernel_file_size = file_info->FileSize;
  Print(L"Kernel_file_size: 0x%lx;", kernel_file_size);

  /**
   * TODO the correct things to do is to read the kernel.elf segment headers,
   * and alloc based on the segment size.
   * But now just get large enough spaces to hold the segments.
   *
   * There still seems to be wired things going on regarding the page types
   * So make sure the address is indeed allocated.
   *
   * At 1M there are 0x700 free pages == 7MB according to the ./misc/memmap
   * We take 4MB == 0x400
   *
   * Find a large enough EfiConventionalMemory region in the MemoryMap to use as
   * kernel_base_addr
   */
  EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
  gBS->AllocatePages(AllocateAddress, EfiConventionalMemory, 0x400, &kernel_base_addr);
  gBS->SetMem((void *)kernel_base_addr, ((kernel_file_size + 0xfff) / 0x1000 + 20) * 0x1000, 0);

  /**
   * The raw elf file
   */
  EFI_PHYSICAL_ADDRESS raw_elf_addr;
  gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, (kernel_file_size + 0xfff) / 0x1000, &raw_elf_addr);
  gBS->SetMem((void *)raw_elf_addr, ((kernel_file_size + 0xfff) / 0x1000) * 0x1000, 0);

  kernel_file->Read(kernel_file, &kernel_file_size, (VOID *)raw_elf_addr);
  /* Observed that *raw_elf_addr == start of the raw file */
  Print(L"Kernel: 0x%0lx (%lu bytes) -> Base:0x%lx->0x%lx\n", raw_elf_addr, kernel_file_size, kernel_base_addr,
        *(UINT64 *)kernel_base_addr);

  load_elf_image(kernel_base_addr, raw_elf_addr);
  /* TODO Check if the elf is loaded correctly in a consistant way */
  // asm("int3");

  gopQueryAndSet(gop);
  EFI_STATUS s_fbc;
  s_fbc = SetKernelFrameBufferConfig(&frameBufferConfig, gop->Mode);
  if (EFI_ERROR(s_fbc))
  {
    Print(L"Cannot set mode: %d, fallback to mode 1\n, stop", gop->Mode->Info->PixelFormat);
    gop->SetMode(gop, 1);
    asm("hlt");
  }

  Print(L"screen:%ldx%ld:%ld,%ld;", frameBufferConfig.horizontal_resolution, frameBufferConfig.vertical_resolution,
        frameBufferConfig.pixel_format, frameBufferConfig.pixels_per_scan_line);

  // struct FrameBufferConfig __conf = {(UINT8 *)gop->Mode->FrameBufferBase,
  //                                   gop->Mode->Info->PixelsPerScanLine,
  //                                   gop->Mode->Info->HorizontalResolution,
  //                                   gop->Mode->Info->VerticalResolution, 0};
  // frameBufferConfig = __conf;

  // #@@range_end(read_kernel)

  /**
   * A UEFI OS loader must ensure that it has the system’s current memory
   * map at the time it calls ExitBootServices(). This is done by passing in
   * the current memory map’s MapKey value as returned by
   * EFI_BOOT_SERVICES.GetMemoryMap() . Care must be taken to ensure that
   * the memory map does not change between these two calls. It is suggested
   * that GetMemoryMap() be called immediately before calling
   * ExitBootServices(). If MapKey value is incorrect, ExitBootServices()
   * returns EFI_INVALID_PARAMETER and GetMemoryMap() with
   * ExitBootServices() must be called again. Firmware implementation may
   * choose to do a partial shutdown of the boot services during the first
   * call to ExitBootServices(). A UEFI OS loader should not make calls to
   * any boot service function other than Memory Allocation Services after
   * the first call to ExitBootServices().
   *
   * On success, the UEFI OS loader owns all available memory in the system.
   * In addition, the UEFI OS loader can treat all memory in the map marked
   * as EfiBootServicesCode and EfiBootServicesData as available free
   * memory. No further calls to boot service functions or EFI
   * device-handle-based protocols may be used, and the boot services
   * watchdog timer is disabled.
   *
   * ref:
   * https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-exitbootservices
   *
   * Try exit, if not success, GetMemoryMap then try again, if not, fatal
   */
  // #@@range_begin(exit_bs)
  EFI_STATUS status;
  GetMemoryMap(&memmap);
  status = gBS->ExitBootServices(image_handle, memmap.map_key);
  if (EFI_ERROR(status))
  {
    status = GetMemoryMap(&memmap);
    if (EFI_ERROR(status))
    {
      Print(L"failed to get memory map: %r\n", status);
      while (1)
        ;
    }
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status))
    {
      Print(L"Could not exit boot service: %r\n", status);
      while (1)
        ;
    }
  }
  //#@ @range_end(exit_bs)

  // #@@range_begin(call_kernel)
  /**
   * Get the e_entry field in the elf header
   * During linking, offset of KernelMain() is written to the entry_addr
   */
  UINTN entry_addr = *(UINT64 *)(raw_elf_addr + 0x18);
  // asm("int3");

  typedef UINT64 __attribute__((sysv_abi)) EntryPointType(const struct FrameBufferConfig *);

  EntryPointType *entry_point = (EntryPointType *)entry_addr;
  entry_point(&frameBufferConfig);
  // #@@range_end(call_kernel)
  // Print(L"All done\n");
  while (1)
    ;
  return EFI_SUCCESS;
}
