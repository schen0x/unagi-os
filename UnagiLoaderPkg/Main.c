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
struct MemoryMap {
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
const CHAR16 *GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
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
EFI_STATUS SaveMemoryMap(struct MemoryMap *map, EFI_FILE_PROTOCOL *file) {
  CHAR8 buf[256];
  UINTN len;

  CHAR8 *header =
      "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  len = AsciiStrLen(header);
  file->Write(file, &len, header);

  Print(L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer,
        map->map_size);

  EFI_PHYSICAL_ADDRESS iter;
  int i;
  for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
       iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
       iter += map->descriptor_size, i++) {
    EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)iter;
    len = AsciiSPrint(buf, sizeof(buf), "%u, %x, %-ls, %08lx, %lx, %lx\n", i,
                      desc->Type, GetMemoryTypeUnicode(desc->Type),
                      desc->PhysicalStart, desc->NumberOfPages,
                      desc->Attribute & 0xffffflu);
    file->Write(file, &len, buf);
  }

  return EFI_SUCCESS;
}
// #@@range_end(save_memory_map)

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL **root) {
  EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

  gBS->OpenProtocol(image_handle, &gEfiLoadedImageProtocolGuid,
                    (VOID **)&loaded_image, image_handle, NULL,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  gBS->OpenProtocol(loaded_image->DeviceHandle,
                    &gEfiSimpleFileSystemProtocolGuid, (VOID **)&fs,
                    image_handle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}

/**
 * Graphics Output Protocol
 * It has basically the same functions as VESA, you can query the modes, set the
 * modes. It also provides an efficient BitBlitter function, which you can't use
 * from your OS unfortunately. GOP is an EFI Boot Time Service, meaning you
 * can't access it after you call ExitBootServices(). However, the framebuffer
 * provided by GOP persists, so you can continue to use it for graphics output
 * in your OS.
 */
EFI_STATUS OpenGOP(EFI_HANDLE image_handle,
                   EFI_GRAPHICS_OUTPUT_PROTOCOL **gop) {
  UINTN num_gop_handles = 0;
  EFI_HANDLE *gop_handles = NULL;
  gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL,
                          &num_gop_handles, &gop_handles);

  gBS->OpenProtocol(gop_handles[0], &gEfiGraphicsOutputProtocolGuid,
                    (VOID **)gop, image_handle, NULL,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  FreePool(gop_handles);

  return EFI_SUCCESS;
}

EFI_STATUS gopQueryAndSet(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop) {

  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *gopModeInfo = NULL;
  gBS->AllocatePages(AllocateAnyPages, EfiConventionalMemory,
                     (sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION) + 0xfff) /
                         0x1000,
                     (EFI_PHYSICAL_ADDRESS *)gopModeInfo);
  gBS->SetMem(
      (void *)gopModeInfo,
      ((sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION) + 0xfff) / 0x1000) *
          0x1000,
      0);

  for (UINT32 modeNum = 0; modeNum < gop->Mode->MaxMode; modeNum++) {
    /* OUT SizeOfInfo; A pointer to the size, in bytes, of the Info */
    UINTN sizeOfInfo = 0;
    gop->QueryMode(gop, modeNum, &sizeOfInfo, &gopModeInfo);
    Print(L"ModeNum: %ld, XY: %ld x %ld, ppl: %ld, pxFmt: %ld", modeNum,
          gopModeInfo->HorizontalResolution, gopModeInfo->VerticalResolution,
          gopModeInfo->PixelsPerScanLine, gopModeInfo->PixelFormat);
  }
  /* 1366x768, ppl 1366, pxFmt 1 */
  EFI_STATUS status0;
  status0 = gop->SetMode(gop, 16);
  return EFI_SUCCESS;
}

const CHAR16 *GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt) {
  switch (fmt) {
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
 * Call UEFI gBS->GetMemoryMap and store the result in map
 * according to UEFI spec
 */
// #@@range_begin(get_memory_map)
EFI_STATUS GetMemoryMap(struct MemoryMap *map) {
  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  map->map_size = map->buffer_size;
  return gBS->GetMemoryMap(&map->map_size, (EFI_MEMORY_DESCRIPTOR *)map->buffer,
                           &map->map_key, &map->descriptor_size,
                           &map->descriptor_version);
}
// #@@range_end(get_memory_map)

typedef struct ELF64_HEADER {
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
typedef struct ELF64_PGN_HEADER {
  /* 0x00000001	PT_LOAD	Loadable segment. */
  UINT32 p_type;
  /* Segment-dependent flags (position for 64-bit structure). */
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
 * https://krinkinmu.github.io/2020/11/15/loading-elf-image.html
 * [load_elf_binary,
 * Linux](https://elixir.bootlin.com/linux/v3.18/source/fs/binfmt_elf.c#L571)
 */
static EFI_STATUS load_elf_image(EFI_PHYSICAL_ADDRESS kernel_base_addr,
                                 EFI_PHYSICAL_ADDRESS raw_elf_addr) {
  CHAR16 start_msg[] = L"Loading ELF image...\r\n";
  CHAR16 finish_msg[] = L"Loaded ELF image\r\n";

  Print(L"%S", start_msg);

  ELF64_HEADER eh = {0};
  eh.e_entry = *(UINTN *)(raw_elf_addr + 0x18);      // e.g. 0x101120
  eh.e_phentsize = *(UINT16 *)(raw_elf_addr + 0x36); //
  eh.e_phnum = *(UINT16 *)(raw_elf_addr + 0x38);     // e.g., 0x4
  eh.e_phoff = *(UINTN *)(raw_elf_addr + 0x20);      // e.g., 0x40
  Print(L"e_phnum: %x ", eh.e_phnum);
  Print(L"e_phoff: %lx", eh.e_phoff);
  Print(L"e_entry: %lx", eh.e_entry);
  // Go over all program headers and load their content in memory
  for (UINTN i = 0; i < eh.e_phnum; i++) {
    ELF64_PGN_HEADER eph = {0};
    eph = *((ELF64_PGN_HEADER *)(eh.e_phoff + raw_elf_addr) + i);
    UINT32 PT_LOAD = 1;
    Print(L"p_type: %x ", eph.p_type);      // e.g., 1
    Print(L"p_vaddr: %lx ", eph.p_vaddr);   // e.g., 0x101120
    Print(L"p_offset: %lx ", eph.p_offset); // e.g., 0x120
    if (eph.p_type != PT_LOAD)
      continue;

    if (eph.p_vaddr != eh.e_entry)
      continue;
    /* dst, src, bytes */
    gBS->CopyMem((void *)(eph.p_vaddr), (void *)(raw_elf_addr + eph.p_offset),
                 eph.p_filesz);
  }

  return Print(L"%S", finish_msg);
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
EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle,
                           EFI_SYSTEM_TABLE *system_table) {
  Print(L"Hello, Unagi!\n");
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
  root_dir->Open(
      root_dir, &memmap_file, L"\\memmap",
      EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

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

  // if (EFI_ERROR(status0))
  //  Print(L"cannot set mode");
  Print(L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
        gop->Mode->Info->PixelsPerScanLine);
  Print(L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
        gop->Mode->FrameBufferBase,
        gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
        gop->Mode->FrameBufferSize);

  // #@@range_end(gop)

  // #@@range_begin(read_kernel)
  EFI_FILE_PROTOCOL *kernel_file;
  root_dir->Open(root_dir, &kernel_file, L"\\kernel.elf", EFI_FILE_MODE_READ,
                 0);

  /**
   * 12 CHAR16 for the filename, which is counted as 0 or 1 (NULL) byte in
   * the EFI_FILE_INFO
   */
  UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
  UINT8 file_info_buffer[file_info_size];
  kernel_file->GetInfo(kernel_file, &gEfiFileInfoGuid, &file_info_size,
                       file_info_buffer);

  EFI_FILE_INFO *file_info = (EFI_FILE_INFO *)file_info_buffer;
  UINTN kernel_file_size = file_info->FileSize;

  /**
   * Large enough spaces to load the OS elf?
   *
   * Find a large enough EfiConventionalMemory region in the MemoryMap to use as
   * kernel_base_addr
   */
  EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
  gBS->AllocatePages(AllocateAddress, EfiLoaderCode,
                     (kernel_file_size + 0xfff) / 0x1000 + 20,
                     &kernel_base_addr);
  gBS->SetMem((void *)kernel_base_addr,
              ((kernel_file_size + 0xfff) / 0x1000 + 20) * 0x1000, 0);

  /**
   * The raw elf file
   */
  EFI_PHYSICAL_ADDRESS raw_elf_addr;
  gBS->AllocatePages(AllocateAnyPages, EfiLoaderData,
                     (kernel_file_size + 0xfff) / 0x1000, &raw_elf_addr);
  gBS->SetMem((void *)raw_elf_addr,
              ((kernel_file_size + 0xfff) / 0x1000) * 0x1000, 0);

  kernel_file->Read(kernel_file, &kernel_file_size, (VOID *)raw_elf_addr);
  Print(L"Kernel: 0x%0lx (%lu bytes)\n", raw_elf_addr, kernel_file_size);

  load_elf_image(kernel_base_addr, raw_elf_addr);

  gopQueryAndSet(gop);
  volatile UINT64 fb = gop->Mode->FrameBufferBase;
  volatile UINT64 fbs = gop->Mode->FrameBufferSize;
  // const int w = gop->Mode->Info->HorizontalResolution;
  // const int h = gop->Mode->Info->VerticalResolution;

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
  status = GetMemoryMap(&memmap);
  Print(L"GetMemoryMap Status: 0x%lx\n", status);
  Print(L"memmap:0x%0lx \n", memmap.map_key);
  // DebugPrint(DEBUG_INFO, "ABC");
  status = GetMemoryMap(&memmap);
  status = gBS->ExitBootServices(image_handle, memmap.map_key);
  // if (EFI_ERROR(status)) {
  //  status = GetMemoryMap(&memmap);
  //  if (EFI_ERROR(status)) {
  //    Print(L"failed to get memory map: %r\n", status);
  //    while (1)
  //      ;
  //  }
  //  status = gBS->ExitBootServices(image_handle, memmap.map_key);
  //  if (EFI_ERROR(status)) {
  //    Print(L"Could not exit boot service: %r\n", status);
  //    while (1)
  //      ;
  //  }
  //}
  // #@@range_end(exit_bs)
  //// const int ppl = gop->Mode->Info->PixelsPerScanLine;

  // #@@range_begin(call_kernel)
  const UINT64 p1 = fb;
  (void)p1;
  /**
   * Get the e_entry field in the elf header
   */
  UINTN entry_addr = *(UINT64 *)(raw_elf_addr + 0x18);
  typedef UINT64 __attribute__((ms_abi)) EntryPointType(UINT64, UINT64);
  EntryPointType *entry_point = (EntryPointType *)entry_addr;
  entry_point(fb, fbs);
  // #@@range_end(call_kernel)
  // Print(L"All done\n");
  while (1)
    ;
  return EFI_SUCCESS;
}
