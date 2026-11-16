#ifndef _FAULT_H
#define _FAULT_H

#include "types.h"

// A page fault decoder.
// When the MMU cannot translate an address, or a descriptor forbids an access,
// the CPU takes an instruction abort (on an instruction fetch) or a data abort
// (on a load or store). These functions decode such an exception from ESR_EL1
// and FAR_EL1 and print it.

// What kind of fault it was: the fault status code (FSC) in ESR_EL1.
enum fault_type {
  FAULT_TRANSLATION,   // no valid descriptor for the address at that level
  FAULT_ACCESS_FLAG,   // the descriptor has AF = 0
  FAULT_PERMISSION,    // the descriptor's AP, UXN, or PXN bits forbid it
  FAULT_ADDRESS_SIZE,  // a table address had bits set above what the CPU
                       // supports (usually: a descriptor built from garbage)
  FAULT_ALIGNMENT,     // e.g., an unaligned access to device memory
  FAULT_EXTERNAL,      // the memory system rejected the access
  FAULT_TLB_CONFLICT,  // the TLB held two translations for the address
  FAULT_OTHER,         // anything else: see the FSC in esr
};

// One instruction or data abort, in decoded form.
struct fault {
  uint64 esr;            // ESR_EL1, as passed to fault_decode()
  uint64 addr;           // FAR_EL1: the virtual address that the access used
  uint64 pc;             // ELR_EL1: the address of the faulting instruction
  bool from_el0;         // true if a process faulted, false if the kernel did
  bool instruction;      // true for an instruction fetch, false for a load
                         // or store
  bool write;            // true for a store (data aborts only)
  enum fault_type type;  // what went wrong
  int level;             // the ARM level (0-3) at which the table walk was
                         // when it failed, or -1 if the type has no level
};

// Decodes the exception that ESR_EL1 value esr describes. If it is an
// instruction or data abort, fills in *f (reading FAR_EL1 for the address),
// and returns true. For any other exception (a system call, say), it returns
// false, and leaves *f alone.
//
// FAR_EL1 describes the most recent abort, so call this before your exception
// handler does anything that could fault again.
//
// uint64 esr: the value of ESR_EL1 for this exception
// uint64 elr: the value of ELR_EL1 for this exception
// struct fault* f: where to store the decoded fault
//
// returns: whether the exception was an instruction or data abort
bool fault_decode(uint64 esr, uint64 elr, struct fault* f);

// Prints a one-line description of fault f (including the line break), e.g.:
//
//   translation fault at ARM level 3 (last-level table): EL0 wrote to
//   0x0000000010000000, pc 0x0000000000010a4c
//
// (on one line). Print who faulted first, if you want to: for example, the
// PID and name of the process that you are about to kill.
void fault_print(const struct fault* f);

#endif  // _FAULT_H
