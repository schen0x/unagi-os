/**
 * @file usb/xhci/trb.hpp
 *
 * 3.2.9 Control Transfers
 *
 * USB Control transfers minimally require two transaction stages on the bus:
 * Setup and Status. Optionally, contains a Data stage in between.
 * Status stages
 * The xHCI defines 3 types of TDs:
 *   - Setup Stage
 *   - Data Stage
 *   - Status Stage
 * Software "constructs" a control transfer by placing either 2 or 3 TDs on the Transfer Ring before ringing the
 * doorbell
 *
 * A TD can contains multiple TRBs (Scatter/Gather Transfer), until the last TRB where the Chain (CH) flag is 0
 *
 */

#pragma once

#include "usb/xhci/context.hpp"
#include <array>
#include <cstdint>

namespace usb::xhci
{
extern const std::array<const char *, 37> kTRBCompletionCodeToName;
extern const std::array<const char *, 64> kTRBTypeToName;

/**
 * Transfer Request Blocks (TRBs)
 * ref:
 * - Figure 3-4: Transfer Ring
 * - 4.9.2 Enqueue and Dequeue Pointers
 *
 *   - Data Buffer Pointer (Address or data) (64)
 *   - Status (32)
 *   - Control (32)
 */
union TRB {
  std::array<uint32_t, 4> data{};
  struct
  {
    uint64_t parameter;
    uint32_t status;
    uint32_t cycle_bit : 1;
    uint32_t evaluate_next_trb : 1;
    uint32_t : 8;
    uint32_t trb_type : 6;
    uint32_t control : 16;
  } __attribute__((packed)) bits;
};

/**
 * A Setup Stage TD generates a USB SETUP transaction, which is used to
 * transmit information to the control endpoint of a USB device.
 */
union SetupStageTRB {
  static const unsigned int Type = 2;
  static const unsigned int kNoDataStage = 0;
  static const unsigned int kOutDataStage = 2;
  static const unsigned int kInDataStage = 3;

  std::array<uint32_t, 4> data{};
  struct
  {
    uint32_t request_type : 8;
    uint32_t request : 8;
    uint32_t value : 16;

    uint32_t index : 16;
    uint32_t length : 16;

    uint32_t trb_transfer_length : 17;
    uint32_t : 5;
    uint32_t interrupter_target : 10;

    uint32_t cycle_bit : 1;
    uint32_t : 4;
    uint32_t interrupt_on_completion : 1;
    uint32_t immediate_data : 1;
    uint32_t : 3;
    uint32_t trb_type : 6;
    uint32_t transfer_type : 2;
    uint32_t : 14;
  } __attribute__((packed)) bits;

  SetupStageTRB()
  {
    bits.trb_type = Type;
    bits.immediate_data = true;
    bits.trb_transfer_length = 8;
  }
};

/**
 * A Data Stage TD consists of a Data Stage TRB followed by zero or more Normal
 * TRBs. If the data is not physically contiguous, Normal TRBs may be chained
 * to the Data Stage TRB.
 */
union DataStageTRB {
  static const unsigned int Type = 3;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint64_t data_buffer_pointer;

    uint32_t trb_transfer_length : 17;
    uint32_t td_size : 5;
    uint32_t interrupter_target : 10;

    uint32_t cycle_bit : 1;
    uint32_t evaluate_next_trb : 1;
    uint32_t interrupt_on_short_packet : 1;
    uint32_t no_snoop : 1;
    uint32_t chain_bit : 1;
    uint32_t interrupt_on_completion : 1;
    uint32_t immediate_data : 1;
    uint32_t : 3;
    uint32_t trb_type : 6;
    uint32_t direction : 1;
    uint32_t : 15;
  } __attribute__((packed)) bits;

  DataStageTRB()
  {
    bits.trb_type = Type;
  }

  void *Pointer() const
  {
    return reinterpret_cast<void *>(bits.data_buffer_pointer);
  }

  void SetPointer(const void *p)
  {
    bits.data_buffer_pointer = reinterpret_cast<uint64_t>(p);
  }
};

/**
 * A Data Stage TD consists of a Data Stage TRB followed by zero or more Normal
 * TRBs. If the data is not physically contiguous, Normal TRBs may be chained
 * to the Data Stage TRB.
 */
union NormalTRB {
  static const unsigned int Type = 1;
  std::array<uint32_t, 4> data{};
  struct
  {
    /* uintptr_t Base of data buffer */
    uint64_t data_buffer_pointer;
    /* length of data */
    uint32_t trb_transfer_length : 17;
    uint32_t td_size : 5;
    uint32_t interrupter_target : 10;

    uint32_t cycle_bit : 1;
    uint32_t evaluate_next_trb : 1;
    uint32_t interrupt_on_short_packet : 1;
    uint32_t no_snoop : 1;
    /* (CH) If set, is `Scatter/Gather Transfer`; send Multi-TRB Transfer
     * Descriptors (TDs), the CH of the last TD is 0
     */
    uint32_t chain_bit : 1;
    uint32_t interrupt_on_completion : 1;
    uint32_t immediate_data : 1;
    uint32_t : 2;
    uint32_t block_event_interrupt : 1;
    uint32_t trb_type : 6;
    uint32_t : 16;
  } __attribute__((packed)) bits;

  NormalTRB()
  {
    bits.trb_type = Type;
  }

  void *Pointer() const
  {
    return reinterpret_cast<TRB *>(bits.data_buffer_pointer);
  }

  void SetPointer(const void *p)
  {
    bits.data_buffer_pointer = reinterpret_cast<uint64_t>(p);
  }
};

/**
 * A Status Stage TD is required to complete a control transfer by retrieving
 * the completion status of the USB SETUP transaction from the USB device. The
 * Status Stage TD is always the last TD in a control transfer sequence. A
 * Status Stage TD always consists of a single Status Stage TRB and may include
 * an Event Data TRB. Refer to section 8.5.3.1 of the USB2 specification and
 * section 8.12.2.1 of the USB3 specification for more information on status
 * reporting.
 *
 * - The "direction" field value is the opposite to that in the SetupStageTRB
 * - The "length" field is 0
 *
 */
union StatusStageTRB {
  static const unsigned int Type = 4;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint64_t : 64;

    uint32_t : 22;
    uint32_t interrupter_target : 10;

    uint32_t cycle_bit : 1;
    uint32_t evaluate_next_trb : 1;
    uint32_t : 2;
    uint32_t chain_bit : 1;
    uint32_t interrupt_on_completion : 1;
    uint32_t : 4;
    uint32_t trb_type : 6;
    uint32_t direction : 1;
    uint32_t : 15;
  } __attribute__((packed)) bits;

  StatusStageTRB()
  {
    bits.trb_type = Type;
  }
};

/**
 * 3.3 Command Interface
 * To manage the xHC and the devices attached to it, the xHC provides an independent Command Ring interface. A work item on a Command Ring is called a Command Descriptor (CD). Command Ring operation is very similar to that of Transfer Rings, software issues a command to the xHC by placing a CD on the Command Ring then rings the Host Controller doorbell. The size of the Command Ring can be modified using the same Link TRB mechanism that Transfer Rings use.
 *
 */

union LinkTRB {
  static const unsigned int Type = 6;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint64_t : 4;
    uint64_t ring_segment_pointer : 60;

    uint32_t : 22;
    uint32_t interrupter_target : 10;

    uint32_t cycle_bit : 1;
    uint32_t toggle_cycle : 1;
    uint32_t : 2;
    uint32_t chain_bit : 1;
    uint32_t interrupt_on_completion : 1;
    uint32_t : 4;
    uint32_t trb_type : 6;
    uint32_t : 16;
  } __attribute__((packed)) bits;

  LinkTRB(const TRB *ring_segment_pointer)
  {
    bits.trb_type = Type;
    SetPointer(ring_segment_pointer);
  }

  TRB *Pointer() const
  {
    return reinterpret_cast<TRB *>(bits.ring_segment_pointer << 4);
  }

  void SetPointer(const TRB *p)
  {
    bits.ring_segment_pointer = reinterpret_cast<uint64_t>(p) >> 4;
  }
};

union NoOpTRB {
  static const unsigned int Type = 8;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint64_t : 64;

    uint32_t : 22;
    uint32_t interrupter_target : 10;

    uint32_t cycle_bit : 1;
    uint32_t evaluate_next_trb : 1;
    uint32_t : 2;
    uint32_t chain_bit : 1;
    uint32_t interrupt_on_completion : 1;
    uint32_t : 4;
    uint32_t trb_type : 6;
    uint32_t : 16;
  } __attribute__((packed)) bits;

  NoOpTRB()
  {
    bits.trb_type = Type;
  }
};

union EnableSlotCommandTRB {
  static const unsigned int Type = 9;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint32_t : 32;

    uint32_t : 32;

    uint32_t : 32;

    uint32_t cycle_bit : 1;
    uint32_t : 9;
    uint32_t trb_type : 6;
    uint32_t slot_type : 5;
    uint32_t : 11;
  } __attribute__((packed)) bits;

  EnableSlotCommandTRB()
  {
    bits.trb_type = Type;
  }
};

union AddressDeviceCommandTRB {
  static const unsigned int Type = 11;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint64_t : 4;
    uint64_t input_context_pointer : 60;

    uint32_t : 32;

    uint32_t cycle_bit : 1;
    uint32_t : 8;
    uint32_t block_set_address_request : 1;
    uint32_t trb_type : 6;
    uint32_t : 8;
    uint32_t slot_id : 8;
  } __attribute__((packed)) bits;

  AddressDeviceCommandTRB(const InputContext *input_context, uint8_t slot_id)
  {
    bits.trb_type = Type;
    bits.slot_id = slot_id;
    SetPointer(input_context);
  }

  InputContext *Pointer() const
  {
    return reinterpret_cast<InputContext *>(bits.input_context_pointer << 4);
  }

  void SetPointer(const InputContext *p)
  {
    bits.input_context_pointer = reinterpret_cast<uint64_t>(p) >> 4;
  }
};

union ConfigureEndpointCommandTRB {
  static const unsigned int Type = 12;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint64_t : 4;
    uint64_t input_context_pointer : 60;

    uint32_t : 32;

    uint32_t cycle_bit : 1;
    uint32_t : 8;
    uint32_t deconfigure : 1;
    uint32_t trb_type : 6;
    uint32_t : 8;
    uint32_t slot_id : 8;
  } __attribute__((packed)) bits;

  ConfigureEndpointCommandTRB(const InputContext *input_context, uint8_t slot_id)
  {
    bits.trb_type = Type;
    bits.slot_id = slot_id;
    SetPointer(input_context);
  }

  InputContext *Pointer() const
  {
    return reinterpret_cast<InputContext *>(bits.input_context_pointer << 4);
  }

  void SetPointer(const InputContext *p)
  {
    bits.input_context_pointer = reinterpret_cast<uint64_t>(p) >> 4;
  }
};

union StopEndpointCommandTRB {
  static const unsigned int Type = 15;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint32_t : 32;

    uint32_t : 32;

    uint32_t : 32;

    uint32_t cycle_bit : 1;
    uint32_t : 9;
    uint32_t trb_type : 6;
    uint32_t endpoint_id : 5;
    uint32_t : 2;
    uint32_t suspend : 1;
    uint32_t slot_id : 8;
  } __attribute__((packed)) bits;

  StopEndpointCommandTRB(EndpointID endpoint_id, uint8_t slot_id)
  {
    bits.trb_type = Type;
    bits.endpoint_id = endpoint_id.Address();
    bits.slot_id = slot_id;
  }

  EndpointID EndpointID() const
  {
    return usb::EndpointID{bits.endpoint_id};
  }
};

union NoOpCommandTRB {
  static const unsigned int Type = 23;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint32_t : 32;

    uint32_t : 32;

    uint32_t : 32;

    uint32_t cycle_bit : 1;
    uint32_t : 9;
    uint32_t trb_type : 6;
    uint32_t : 16;
  } __attribute__((packed)) bits;

  NoOpCommandTRB()
  {
    bits.trb_type = Type;
  }
};

union TransferEventTRB {
  static const unsigned int Type = 32;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint64_t trb_pointer : 64;

    uint32_t trb_transfer_length : 24;
    uint32_t completion_code : 8;

    uint32_t cycle_bit : 1;
    uint32_t : 1;
    uint32_t event_data : 1;
    uint32_t : 7;
    uint32_t trb_type : 6;
    uint32_t endpoint_id : 5;
    uint32_t : 3;
    uint32_t slot_id : 8;
  } __attribute__((packed)) bits;

  TransferEventTRB()
  {
    bits.trb_type = Type;
  }

  TRB *Pointer() const
  {
    return reinterpret_cast<TRB *>(bits.trb_pointer);
  }

  void SetPointer(const TRB *p)
  {
    bits.trb_pointer = reinterpret_cast<uint64_t>(p);
  }

  EndpointID EndpointID() const
  {
    return usb::EndpointID{bits.endpoint_id};
  }
};

union CommandCompletionEventTRB {
  static const unsigned int Type = 33;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint64_t : 4;
    uint64_t command_trb_pointer : 60;

    uint32_t command_completion_parameter : 24;
    uint32_t completion_code : 8;

    uint32_t cycle_bit : 1;
    uint32_t : 9;
    uint32_t trb_type : 6;
    uint32_t vf_id : 8;
    uint32_t slot_id : 8;
  } __attribute__((packed)) bits;

  CommandCompletionEventTRB()
  {
    bits.trb_type = Type;
  }

  TRB *Pointer() const
  {
    return reinterpret_cast<TRB *>(bits.command_trb_pointer << 4);
  }

  void SetPointer(TRB *p)
  {
    bits.command_trb_pointer = reinterpret_cast<uint64_t>(p) >> 4;
  }
};

union PortStatusChangeEventTRB {
  static const unsigned int Type = 34;
  std::array<uint32_t, 4> data{};
  struct
  {
    uint32_t : 24;
    uint32_t port_id : 8;

    uint32_t : 32;

    uint32_t : 24;
    uint32_t completion_code : 8;

    uint32_t cycle_bit : 1;
    uint32_t : 9;
    uint32_t trb_type : 6;
  } __attribute__((packed)) bits;

  PortStatusChangeEventTRB()
  {
    bits.trb_type = Type;
  }
};

/** @brief TRBDynamicCast casts a trb pointer to other type of TRB.
 *
 * @param trb  source pointer
 * @return  casted pointer if the source TRB's type is equal to the resulting
 *  type. nullptr otherwise.
 */
template <class ToType, class FromType> ToType *TRBDynamicCast(FromType *trb)
{
  if (ToType::Type == trb->bits.trb_type)
  {
    return reinterpret_cast<ToType *>(trb);
  }
  return nullptr;
}
} // namespace usb::xhci
