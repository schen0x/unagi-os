#include <Guid/FileInfo.h>
#include <Library/PeCoffGetEntryPointLib.h>
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
  // #@@range_begin(main)
  CHAR8 memmap_buf[4096 * 8];
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
   * The address 1M, if not empty,
   * Find a large enough EfiConventionalMemory region in the MemoryMap to use as
   * kernel_base_addr
   */
  EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
  gBS->AllocatePages(AllocateAddress, EfiLoaderData,
                     (kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr);
  kernel_file->Read(kernel_file, &kernel_file_size, (VOID *)kernel_base_addr);
  Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);

  // UINT64 entry_addr = 0;
  UINT64 entry_addr = *(UINT64 *)(kernel_base_addr + 24);
  // PeCoffLoaderGetEntryPoint((void *)kernel_base_addr, (void **)&entry_addr);
  Print(L"Entry Point:%0lx \n", entry_addr);

  // #@@range_end(read_kernel)

  /**
   * A UEFI OS loader must ensure that it has the system’s current memory map
   * at the time it calls ExitBootServices(). This is done by passing in the
   * current memory map’s MapKey value as returned by
   * EFI_BOOT_SERVICES.GetMemoryMap() . Care must be taken to ensure that the
   * memory map does not change between these two calls. It is suggested that
   * GetMemoryMap() be called immediately before calling ExitBootServices(). If
   * MapKey value is incorrect, ExitBootServices() returns
   * EFI_INVALID_PARAMETER and GetMemoryMap() with ExitBootServices() must be
   * called again. Firmware implementation may choose to do a partial shutdown
   * of the boot services during the first call to ExitBootServices(). A UEFI
   * OS loader should not make calls to any boot service function other than
   * Memory Allocation Services after the first call to ExitBootServices().
   *
   * On success, the UEFI OS loader owns all available memory in the system. In
   * addition, the UEFI OS loader can treat all memory in the map marked as
   * EfiBootServicesCode and EfiBootServicesData as available free memory. No
   * further calls to boot service functions or EFI device-handle-based
   * protocols may be used, and the boot services watchdog timer is disabled.
   *
   * ref:
   * https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-exitbootservices
   *
   * Try exit, if not success, GetMemoryMap then try again, if not, fatal
   */
  // #@@range_begin(exit_bs)
  EFI_STATUS status;
  status = gBS->ExitBootServices(image_handle, memmap.map_key);
  if (EFI_ERROR(status)) {
    status = GetMemoryMap(&memmap);
    if (EFI_ERROR(status)) {
      Print(L"failed to get memory map: %r\n", status);
      while (1)
        ;
    }
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status)) {
      Print(L"Could not exit boot service: %r\n", status);
      while (1)
        ;
    }
  }
  // #@@range_end(exit_bs)

  // #@@range_begin(call_kernel)
  // UINT64 entry_addr = *(UINT64 *)(kernel_base_addr + 24);
  // UINT64 entry_addr = *(UINT64 *)(0x101120);
  typedef void EntryPointType(void);
  EntryPointType *entry_point = (EntryPointType *)entry_addr;
  entry_point();
  // #@@range_end(call_kernel)
  Print(L"All done\n");
  while (1)
    ;
  return EFI_SUCCESS;
}
