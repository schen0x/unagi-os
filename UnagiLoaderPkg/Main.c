//#include <X64/ProcessorBind.h>
#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/PrintLib.h>
#include  <Protocol/LoadedImage.h>
#include  <Protocol/SimpleFileSystem.h>
#include  <Protocol/DiskIo2.h>
#include  <Protocol/BlockIo.h>

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table) {
  Print(L"Hello, Unagi!\n");
  while (1);
  return EFI_SUCCESS;
}

// #@@range_begin(struct_memory_map)
struct MemoryMap {
  UINTN buffer_size;
  /**
   * MemoryDesc buffer[map_size/sizeof(MemoryDesc)]
   */
  VOID* buffer;
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

/**
 * Call UEFI gBS->GetMemoryMap and store the result in map
 * according to UEFI spec
 */
// #@@range_begin(get_memory_map)
EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  map->map_size = map->buffer_size;
  return gBS->GetMemoryMap(
      &map->map_size,
      (EFI_MEMORY_DESCRIPTOR*)map->buffer,
      &map->map_key,
      &map->descriptor_size,
      &map->descriptor_version);
}
// #@@range_end(get_memory_map)
