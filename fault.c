#include "vm/fault.h"

#include "aarch64.h"
#include "printf.h"

// ISS bit 10 (FnV): FAR_EL1 does not hold the address. Only external aborts
// can set it.
#define ESR_EL1_ISS_FNV (1 << 10)

// More fault status codes (ESR_EL1 bits 5:0), besides those in aarch64.h
#define FSC_ADDRESS_SIZE 0x00    // bits 1:0 hold the level
#define FSC_EXTERNAL 0x10        // not on a table walk: no level
#define FSC_EXTERNAL_WALK 0x14   // on a table walk: bits 1:0 hold the level
#define FSC_ALIGNMENT 0x21
#define FSC_TLB_CONFLICT 0x30

bool fault_decode(uint64 esr, uint64 elr, struct fault* f) {
  uint64 ec = (esr >> ESR_EL1_EC_OFFSET) & ESR_EL1_EC_MASK;
  bool iabt = (ec == ESR_EL1_EC_IABT_LOWER || ec == ESR_EL1_EC_IABT_SAME);
  bool dabt = (ec == ESR_EL1_EC_DABT_LOWER || ec == ESR_EL1_EC_DABT_SAME);
  if (!iabt && !dabt) {
    return false;
  }

  uint64 far;
  asm volatile("mrs %0, FAR_EL1" : "=r"(far));

  f->esr = esr;
  f->addr = far;
  f->pc = elr;
  f->from_el0 = (ec == ESR_EL1_EC_IABT_LOWER || ec == ESR_EL1_EC_DABT_LOWER);
  f->instruction = iabt;
  f->write = dabt && (esr & ESR_EL1_ISS_WNR);

  uint64 fsc = esr & ESR_EL1_FSC_MASK;
  int level = (int)ESR_EL1_FSC_LEVEL(fsc);
  f->level = -1;
  if (fsc == FSC_EXTERNAL) {
    f->type = FAULT_EXTERNAL;
  } else if (fsc == FSC_ALIGNMENT) {
    f->type = FAULT_ALIGNMENT;
  } else if (fsc == FSC_TLB_CONFLICT) {
    f->type = FAULT_TLB_CONFLICT;
  } else {
    // The remaining codes we know carry the level in bits 1:0.
    f->level = level;
    switch (ESR_EL1_FSC_TYPE(fsc)) {
      case ESR_EL1_FSC_TRANSLATION:
        f->type = FAULT_TRANSLATION;
        break;
      case ESR_EL1_FSC_ACCESS_FLAG:
        f->type = FAULT_ACCESS_FLAG;
        break;
      case ESR_EL1_FSC_PERMISSION:
        f->type = FAULT_PERMISSION;
        break;
      case FSC_ADDRESS_SIZE:
        f->type = FAULT_ADDRESS_SIZE;
        break;
      case FSC_EXTERNAL_WALK:
        f->type = FAULT_EXTERNAL;
        break;
      default:
        f->type = FAULT_OTHER;
        f->level = -1;
        break;
    }
  }
  return true;
}

static const char* type_name(enum fault_type type) {
  switch (type) {
    case FAULT_TRANSLATION:
      return "translation fault";
    case FAULT_ACCESS_FLAG:
      return "access flag fault";
    case FAULT_PERMISSION:
      return "permission fault";
    case FAULT_ADDRESS_SIZE:
      return "address size fault";
    case FAULT_ALIGNMENT:
      return "alignment fault";
    case FAULT_EXTERNAL:
      return "synchronous external abort";
    case FAULT_TLB_CONFLICT:
      return "TLB conflict abort";
    default:
      return "unknown fault";
  }
}

// Our names for the tables at ARM levels 0-3
static const char* table_names[] = {"top-level", "2nd-level", "3rd-level",
                                    "last-level"};

void fault_print(const struct fault* f) {
  printf("%s", type_name(f->type));
  if (f->level >= 0 && f->level <= 3) {
    printf(" at ARM level %d (%s table)", f->level, table_names[f->level]);
  }
  if (f->type == FAULT_OTHER) {
    printf(" (FSC 0x%x)", (int)(f->esr & ESR_EL1_FSC_MASK));
  }

  const char* access = f->instruction ? "fetched an instruction at"
                       : f->write     ? "wrote to"
                                      : "read from";
  printf(": EL%d %s ", f->from_el0 ? 0 : 1, access);
  if (f->esr & ESR_EL1_ISS_FNV) {
    printf("an unknown address");
  } else {
    printf("%p", f->addr);
  }
  printf(", pc %p\r\n", f->pc);
}
