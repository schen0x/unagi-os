/**
 * @file usb/xhci/registers.hpp
 *
 * xHCI のレジスタ定義．
 */

#pragma once

#include "register.hpp"

namespace usb::xhci
{
union HCSPARAMS1_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t max_device_slots : 8;
    uint32_t max_interrupters : 11;
    uint32_t : 5;
    /**
     * Affect the Host Controller Operational Registers
     *
     * The MaxPorts value in the HCSPARAMS1 register defines the number of
     * Port e.g. PORTSC, PORTPMSC, and PORTLI register sets). The PORTSC,
     * PORTLI register sets are grouped (consecutive Dwords). Refer to their
     * respective sections for their addressing.
     */
    uint32_t max_ports : 8;
  } __attribute__((packed)) bits;
} __attribute__((packed));

union HCSPARAMS2_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t isochronous_scheduling_threshold : 4;
    uint32_t event_ring_segment_table_max : 4;
    uint32_t : 13;
    uint32_t max_scratchpad_buffers_high : 5;
    uint32_t scratchpad_restore : 1;
    uint32_t max_scratchpad_buffers_low : 5;
  } __attribute__((packed)) bits;
} __attribute__((packed));

union HCSPARAMS3_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t u1_device_eixt_latency : 8;
    uint32_t : 8;
    uint32_t u2_device_eixt_latency : 16;
  } __attribute__((packed)) bits;
} __attribute__((packed));

/* Host Controller Capability Parameters (HCCPARAMS) */
union HCCPARAMS1_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t addressing_capability_64 : 1;
    uint32_t bw_negotiation_capability : 1;
    uint32_t context_size : 1;
    uint32_t port_power_control : 1;
    uint32_t port_indicators : 1;
    uint32_t light_hc_reset_capability : 1;
    uint32_t latency_tolerance_messaging_capability : 1;
    uint32_t no_secondary_sid_support : 1;
    uint32_t parse_all_event_data : 1;
    uint32_t stopped_short_packet_capability : 1;
    uint32_t stopped_edtla_capability : 1;
    uint32_t contiguous_frame_id_capability : 1;
    uint32_t maximum_primary_stream_array_size : 4;
    uint32_t xhci_extended_capabilities_pointer : 16;
  } __attribute__((packed)) bits;
} __attribute__((packed));

union DBOFF_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t : 2;
    uint32_t doorbell_array_offset : 30;
  } __attribute__((packed)) bits;

  uint32_t Offset() const
  {
    return bits.doorbell_array_offset << 2;
  }
} __attribute__((packed));

union RTSOFF_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t : 5;
    uint32_t runtime_register_space_offset : 27;
  } __attribute__((packed)) bits;

  uint32_t Offset() const
  {
    return bits.runtime_register_space_offset << 5;
  }
} __attribute__((packed));

union HCCPARAMS2_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t u3_entry_capability : 1;
    uint32_t configure_endpoint_command_max_exit_latency_too_large_capability : 1;
    uint32_t force_save_context_capability : 1;
    uint32_t compliance_transition_capability : 1;
    uint32_t large_esit_payload_capability : 1;
    uint32_t configuration_information_capability : 1;
    uint32_t : 26;
  } __attribute__((packed)) bits;
} __attribute__((packed));

/**
 * xHCI spec; Section 5.3; Host Controller Capability Registers
 * Base Offset 00h:20h
 * The mimium field size is 1 byte, e.g.,:
 *   - Base_Offset | Size(Bytes) | Mnemonic
 *   - 00h    | 1 |    CAPLENGTH
 *   - 01h    | 1 |    Rsvd
 *   - 02h    | 2 |    HCIVERSION
 *   - 04h    | 4 |    HCSPARAMS1
 *   - ...
 *   - 1Ch    | 4 |    HCCPARAMS2
 *   - 20h    | CAPLENGTH-20h |    Rsvd
 *
 */
struct CapabilityRegisters
{
  /**
   * Capability Registers Length (CAPLENGTH)
   *
   * This register is used as an offset to add to register base to find the
   * beginning of the Operational Register Space.
   */
  MemMapRegister<DefaultBitmap<uint8_t>> CAPLENGTH;   // Capability Register Length
  MemMapRegister<DefaultBitmap<uint8_t>> reserved1;   //
  MemMapRegister<DefaultBitmap<uint16_t>> HCIVERSION; // Interface Version Number
  /**
   * Structural Parameters 1 (HCSPARAMS1)
   *
   * This register defines basic structural parameters supported by this xHC
   * implementation: Number of Device Slots support, Interrupters, Root Hub
   * ports, etc.
   */
  MemMapRegister<HCSPARAMS1_Bitmap> HCSPARAMS1; // Structural Parameters 1
  MemMapRegister<HCSPARAMS2_Bitmap> HCSPARAMS2; // Structural Parameters 2
  MemMapRegister<HCSPARAMS3_Bitmap> HCSPARAMS3; // Structural Parameters 3
  MemMapRegister<HCCPARAMS1_Bitmap> HCCPARAMS1; // Capability Parameters 1
  MemMapRegister<DBOFF_Bitmap> DBOFF;           // Doorbell Offset
  MemMapRegister<RTSOFF_Bitmap> RTSOFF;         // Runtime Register Space Offset
  MemMapRegister<HCCPARAMS2_Bitmap> HCCPARAMS2; // Capability Parameters 2
} __attribute__((packed));

union USBCMD_Bitmap {
  uint32_t data[1];
  struct
  {
    /**
     * The xHC continues execution as long as this bit is set to a ‘1’. When
     * this bit is cleared to ‘0’, the xHC completes any current or queued
     * commands or TDs, and any USB transactions associated with them, then
     * halts.
     */
    uint32_t run_stop : 1;
    uint32_t host_controller_reset : 1;
    /**
     * Interrupter Enable (INTE) – RW. Default = ‘0’. This bit provides system
     * software with a means of enabling or disabling the host system
     * interrupts generated by Interrupters. When this bit is a ‘1’, then
     * Interrupter host system interrupt generation is allowed, e.g. the xHC
     * shall issue an interrupt at the next interrupt threshold if the host
     * system interrupt mechanism (e.g. MSI, MSI-X, etc.) is enabled. The
     * interrupt is acknowledged by a host system interrupt specific mechanism.
     */
    uint32_t interrupter_enable : 1;
    uint32_t host_system_error_enable : 1;
    uint32_t : 3;
    uint32_t lignt_host_controller_reset : 1;
    uint32_t controller_save_state : 1;
    uint32_t controller_restore_state : 1;
    uint32_t enable_wrap_event : 1;
    uint32_t enable_u3_mfindex_stop : 1;
    uint32_t stopped_short_packet_enable : 1;
    uint32_t cem_enable : 1;
    uint32_t : 18;
  } __attribute__((packed)) bits;
} __attribute__((packed));

union USBSTS_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t host_controller_halted : 1;
    uint32_t : 1;
    uint32_t host_system_error : 1;
    uint32_t event_interrupt : 1;
    uint32_t port_change_detect : 1;
    uint32_t : 3;
    uint32_t save_state_status : 1;
    uint32_t restore_state_status : 1;
    uint32_t save_restore_error : 1;
    uint32_t controller_not_ready : 1;
    uint32_t host_controller_error : 1;
    uint32_t : 19;
  } __attribute__((packed)) bits;
} __attribute__((packed));

union CRCR_Bitmap {
  uint64_t data[1];
  struct
  {
    uint64_t ring_cycle_state : 1; // RCS - RW; 4.9.3
    uint64_t command_stop : 1;
    uint64_t command_abort : 1;
    uint64_t command_ring_running : 1;
    uint64_t : 2;
    uint64_t command_ring_pointer : 58; // TODO Table 5-24
  } __attribute__((packed)) bits;

  uint64_t Pointer() const
  {
    return bits.command_ring_pointer << 6;
  }

  void SetPointer(uint64_t value)
  {
    bits.command_ring_pointer = value >> 6;
  }
} __attribute__((packed));

union DCBAAP_Bitmap {
  uint64_t data[1];
  struct
  {
    uint64_t : 6;
    uint64_t device_context_base_address_array_pointer : 26;
  } __attribute__((packed)) bits;

  uint64_t Pointer() const
  {
    return bits.device_context_base_address_array_pointer << 6;
  }

  void SetPointer(uint64_t value)
  {
    bits.device_context_base_address_array_pointer = value >> 6;
  }
} __attribute__((packed));

union CONFIG_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t max_device_slots_enabled : 8;
    uint32_t u3_entry_enable : 1;
    uint32_t configuration_information_enable : 1;
    uint32_t : 22;
  } __attribute__((packed)) bits;
} __attribute__((packed));

/**
 * xHCI spec; Section 5.4 Host Controller Operational Registers
 *
 * The base address of this register space is referred to as Operational Base.
 * The Operational Base shall be Dword aligned and is calculated by adding the
 * value of the Capability Registers Length (CAPLENGTH) register (refer to
 * Section 5.3.1) to the Capability Base address. All registers are multiples
 * of 32 bits in length.
 * - ...
 * - 38h CONFIG
 * - 3C-3FFh RsvdZ
 * - 400-13FFh        Port Register Set (1-MaxPorts)
 */
struct OperationalRegisters
{
  /**
   * 5.4.1 USB Command Register (USBCMD)
   *   - Writing to the register causes a command to be executed.
   *   - Basic commands, e.g. Run/Stop (1 to Run, 0 to Stop); Reset
   */
  MemMapRegister<USBCMD_Bitmap> USBCMD;
  /* bit 0 HCHalted (HCH) - RO; isHalted The bit is 0 whenever R/S bit is 1; */
  MemMapRegister<USBSTS_Bitmap> USBSTS;
  MemMapRegister<DefaultBitmap<uint32_t>> PAGESIZE;
  uint32_t reserved1[2];
  /**
   * Device Notification Control
   * This register is used by software to enable or disable the reporting of
   * the reception of specific USB Device Notification Transaction Packets. A
   * Notification Enable (Nx, where x = 0 to 15) flag is defined for each of
   * the 16 possible de vice notification types. If a flag is set for a
   * specific notification type, a Device Notification Event shall be generated
   * when the respective notification packet is received. After reset all
   * notifications are disabled. Refer to section 6.4.2.7. This register shall
   * be written as a Dword. Byte writes produce undefined results.
   */
  MemMapRegister<DefaultBitmap<uint32_t>> DNCTRL;
  /**
   * 5.4.5 Command Ring Control Register
   */
  MemMapRegister<CRCR_Bitmap> CRCR;
  uint32_t reserved2[4];
  MemMapRegister<DCBAAP_Bitmap> DCBAAP;
  MemMapRegister<CONFIG_Bitmap> CONFIG;
} __attribute__((packed));

union PORTSC_Bitmap {
  uint32_t data[1];
  struct
  {
    /**
     * Port Enabled/Disabled (PED) – RW1CS. (Read Write 1 to clear sticky)
     * Default = ‘0’. ‘1’ = Enabled. ‘0’ = Disabled. Ports may only be enabled
     * by the xHC. Software cannot enable a port by writing a ‘1’ to this flag.
     * A port may be disabled by software writing a ‘1’ to this flag.
     */
    uint32_t current_connect_status : 1;
    uint32_t port_enabled_disabled : 1;
    uint32_t : 1;
    uint32_t over_current_active : 1;
    uint32_t port_reset : 1;
    uint32_t port_link_state : 4;
    uint32_t port_power : 1;
    uint32_t port_speed : 4;
    uint32_t port_indicator_control : 2;
    uint32_t port_link_state_write_strobe : 1;
    uint32_t connect_status_change : 1;
    uint32_t port_enabled_disabled_change : 1;
    uint32_t warm_port_reset_change : 1;
    uint32_t over_current_change : 1;
    uint32_t port_reset_change : 1;
    uint32_t port_link_state_change : 1;
    uint32_t port_config_error_change : 1;
    uint32_t cold_attach_status : 1;
    uint32_t wake_on_connect_enable : 1;
    uint32_t wake_on_disconnect_enable : 1;
    uint32_t wake_on_over_current_enable : 1;
    uint32_t : 2;
    uint32_t device_removable : 1;
    uint32_t warm_port_reset : 1;
  } __attribute__((packed)) bits;
} __attribute__((packed));

union PORTPMSC_Bitmap {
  uint32_t data[1];
  struct
  { // definition for USB3 protocol
    uint32_t u1_timeout : 8;
    uint32_t u2_timeout : 8;
    uint32_t force_link_pm_accept : 1;
    uint32_t : 15;
  } __attribute__((packed)) bits_usb3;
} __attribute__((packed));

union PORTLI_Bitmap {
  uint32_t data[1];
  struct
  { // definition for USB3 protocol
    uint32_t link_error_count : 16;
    uint32_t rx_lane_count : 4;
    uint32_t tx_lane_count : 4;
    uint32_t : 8;
  } __attribute__((packed)) bits_usb3;
} __attribute__((packed));

union PORTHLPMC_Bitmap {
  uint32_t data[1];
  struct
  { // definition for USB2 protocol
    uint32_t host_initiated_resume_duration_mode : 2;
    uint32_t l1_timeout : 8;
    uint32_t best_effort_service_latency_deep : 4;
    uint32_t : 18;
  } __attribute__((packed)) bits_usb2;
} __attribute__((packed));

/**
 * 5.4 Host Controller Operational Registers
 *   - at the tail, 400-13FFh        Port Register Set (1-MaxPorts)
 */
struct PortRegisterSet
{
  MemMapRegister<PORTSC_Bitmap> PORTSC;
  MemMapRegister<PORTPMSC_Bitmap> PORTPMSC;
  MemMapRegister<PORTLI_Bitmap> PORTLI;
  MemMapRegister<PORTHLPMC_Bitmap> PORTHLPMC;
} __attribute__((packed));

using PortRegisterSetArray = ArrayWrapper<PortRegisterSet>;

union IMAN_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t interrupt_pending : 1;
    uint32_t interrupt_enable : 1;
    uint32_t : 30;
  } __attribute__((packed)) bits;
} __attribute__((packed));

union IMOD_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t interrupt_moderation_interval : 16;
    uint32_t interrupt_moderation_counter : 16;
  } __attribute__((packed)) bits;
} __attribute__((packed));

/**
 * 5.5.2.3.1 Event Ring Segment Table Size Register (ERSTSZ)
 *
 * Address:       Runtime Base + 028h + (32 * Interrupter)
 *                where: Interrupter is 0, 1, 2, 3, … 1023
 * Default Value: 0000 0000 0000 0000h
 * Attribute:     RW
 * Size:          32 bits
 */
union ERSTSZ_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t event_ring_segment_table_size : 16;
    uint32_t : 16;
  } __attribute__((packed)) bits;

  uint16_t Size() const
  {
    return bits.event_ring_segment_table_size;
  }

  void SetSize(uint16_t value)
  {
    bits.event_ring_segment_table_size = value;
  }
} __attribute__((packed));

union ERSTBA_Bitmap {
  uint64_t data[1];
  struct
  {
    uint64_t : 6;
    uint64_t event_ring_segment_table_base_address : 58;
  } __attribute__((packed)) bits;

  uint64_t Pointer() const
  {
    return bits.event_ring_segment_table_base_address << 6;
  }

  void SetPointer(uint64_t value)
  {
    bits.event_ring_segment_table_base_address = value >> 6;
  }
} __attribute__((packed));

/**
 * 5.5.2.3.3 Event Ring Dequeue Pointer Register (ERDP)
 *
 * Address:       Runtime Base + 038h + (32 * Interrupter)
 *                where: Interrupter is 0, 1, 2, 3, … 1023
 * Default Value: 0000 0000 0000 0000h
 * Attribute:     RW
 * Size:          64 bits
 *
 * The Event Ring Dequeue Pointer Register is written by **software** to define the Event Ring Dequeue Pointer location
 * to the xHC. Software updates this pointer when it is finished the evaluation of an Event(s) on the Event Ring.
 *
 * Before enqueuing an Event, the xHC shall check that space is available on the Event Ring. This check can be skipped
 * if the xHC is currently enqueuing Event TRBs in a different ERST segment than the one that software is using to
 * dequeue Events.
 */
union ERDP_Bitmap {
  uint64_t data[1];
  struct
  {
    uint64_t dequeue_erst_segment_index : 3;
    uint64_t event_handler_busy : 1;
    uint64_t event_ring_dequeue_pointer : 60;
  } __attribute__((packed)) bits;

  uint64_t Pointer() const
  {
    return bits.event_ring_dequeue_pointer << 4;
  }

  void SetPointer(uint64_t value)
  {
    bits.event_ring_dequeue_pointer = value >> 4;
  }
} __attribute__((packed));

struct InterrupterRegisterSet
{
  MemMapRegister<IMAN_Bitmap> IMAN;
  MemMapRegister<IMOD_Bitmap> IMOD;
  MemMapRegister<ERSTSZ_Bitmap> ERSTSZ;
  uint32_t reserved;
  MemMapRegister<ERSTBA_Bitmap> ERSTBA;
  MemMapRegister<ERDP_Bitmap> ERDP;
} __attribute__((packed));

using InterrupterRegisterSetArray = ArrayWrapper<InterrupterRegisterSet>;

union Doorbell_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t db_target : 8;
    uint32_t : 8;
    uint32_t db_stream_id : 16;
  } __attribute__((packed)) bits;
} __attribute__((packed));

class DoorbellRegister
{
  MemMapRegister<Doorbell_Bitmap> reg_;

public:
  void Ring(uint8_t target, uint16_t stream_id = 0)
  {
    Doorbell_Bitmap value{};
    value.bits.db_target = target;
    value.bits.db_stream_id = stream_id;
    reg_.Write(value);
  }
};

using DoorbellRegisterArray = ArrayWrapper<DoorbellRegister>;

/** @brief 拡張レジスタの共通ヘッダ構造 */
union ExtendedRegister_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t capability_id : 8;
    uint32_t next_pointer : 8;
    uint32_t value : 16;
  } __attribute__((packed)) bits;
} __attribute__((packed));

class ExtendedRegisterList
{
public:
  using ValueType = MemMapRegister<ExtendedRegister_Bitmap>;

  class Iterator
  {
  public:
    Iterator(ValueType *reg) : reg_{reg}
    {
    }
    auto operator->() const
    {
      return reg_;
    }
    auto &operator*() const
    {
      return *reg_;
    }
    bool operator==(Iterator lhs) const
    {
      return reg_ == lhs.reg_;
    }
    bool operator!=(Iterator lhs) const
    {
      return reg_ != lhs.reg_;
    }
    Iterator &operator++();

  private:
    ValueType *reg_;
  };

  ExtendedRegisterList(uint64_t mmio_base, HCCPARAMS1_Bitmap hccp);

  Iterator begin() const
  {
    return first_;
  }
  Iterator end() const
  {
    return Iterator{nullptr};
  }

private:
  const Iterator first_;
};

/**** 個別の拡張レジスタ定義 ****/
/**
 * xHCI spec, Section 7.1.1; USB Legacy Support Capability (USBLEGSUP)
 */
union USBLEGSUP_Bitmap {
  uint32_t data[1];
  struct
  {
    uint32_t capability_id : 8;
    uint32_t next_pointer : 8;
    uint32_t hc_bios_owned_semaphore : 1;
    uint32_t : 7;
    uint32_t hc_os_owned_semaphore : 1;
    uint32_t : 7;
  } __attribute__((packed)) bits;
} __attribute__((packed));
} // namespace usb::xhci
