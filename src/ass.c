#include "ass.h"
#include "log.h"
#include "jit.h"

struct instruction {
	uint8_t opcode[15];
	const char *str;
	size_t opcode_size;
	bool payload_address;
	bool payload_value;
	size_t payload_size;
};

enum instruction_e {
	ADD_RBX_RAX = 0,
	SUB_RBX_RAX,
	MUL_RBX,
	CLEAR_RDX,
	DIV_RBX,
	PUSH_LARGE,
	POP_RAX,
	POP_RBX,
	POP_RDI,
	POP_RSI,
	PUSH_RAX,
	PUSH_RBX,
	PUSH_RCX,
	MOV_RAX_RDI,
	MOV_IMM_RAX,
	MOV_IMM_RCX_LARGE,
	CALL_RCX,
	RET,
};

struct instruction instructions[] = {
	[ADD_RBX_RAX] = {
		.opcode = {0x48, 0x01, 0xd8},
		.str = "ADD_RBX_RAX",
		.opcode_size = 3,
		.payload_address = false,
		.payload_value = false,
	},
	[SUB_RBX_RAX] = {
		.opcode = {0x48, 0x29, 0xd8},
		.str = "SUB_RBX_RAX",
		.opcode_size = 3,
		.payload_address = false,
		.payload_value = false,
	},
	[MUL_RBX] = {
		.opcode = {0x48, 0xf7, 0xeb},
		.str = "MUL_RBX",
		.opcode_size = 3,
		.payload_address = false,
		.payload_value = false,
	},
	[CLEAR_RDX] = {
		.opcode = {0x99},
		.str = "CLEAR_RDX",
		.opcode_size = 1,
		.payload_address = false,
		.payload_value = false,
	},
	[DIV_RBX] = {
		.opcode = {0x48, 0xf7, 0xfb},
		.str = "DIV_RBX",
		.opcode_size = 3,
		.payload_address = false,
		.payload_value = false,
	},
	[PUSH_LARGE] = {
		.opcode = {0x68},
		.str = "PUSH_LARGE",
		.opcode_size = 1,
		.payload_address = false,
		.payload_value = false,
	},
	[POP_RAX] = {
		.opcode = {0x58},
		.str = "POP_RAX",
		.opcode_size = 1,
		.payload_address = false,
		.payload_value = false,
	},
	[POP_RBX] = {
		.opcode = {0x5b},
		.str = "POP_RBX",
		.opcode_size = 1,
		.payload_address = false,
		.payload_value = false,
	},
	[POP_RDI] = {
		.opcode = {0x5f},
		.str = "POP_RDI",
		.opcode_size = 1,
		.payload_address = false,
		.payload_value = false,
	},
	[POP_RSI] = {
		.opcode = {0x5e},
		.str = "POP_RSI",
		.opcode_size = 1,
		.payload_address = false,
		.payload_value = false,
	},
	[PUSH_RAX] = {
		.opcode = {0x50},
		.str = "PUSH_RAX",
		.opcode_size = 1,
		.payload_address = false,
		.payload_value = false,
	},
	[PUSH_RBX] = {
		.opcode = {0x53},
		.str = "PUSH_RBX",
		.opcode_size = 1,
		.payload_address = false,
		.payload_value = false,
	},
	[PUSH_RCX] = {
		.opcode = {0x51},
		.str = "PUSH_RCX",
		.opcode_size = 1,
		.payload_address = false,
		.payload_value = false,
	},
	[MOV_RAX_RDI] = {
		.opcode = {0x48, 0x89, 0xc7},
		.str = "MOV_RAX_RDI",
		.opcode_size = 3,
		.payload_address = false,
		.payload_value = false,
	},
	[MOV_IMM_RAX] = {
		.opcode = {0x48, 0xc7, 0xc0},
		.str = "MOV_IMM_RAX",
		.opcode_size = 3,
		.payload_address = false,
		.payload_value = true,
		.payload_size = 4,
	},
	[MOV_IMM_RCX_LARGE] = {
		.opcode = {0x48, 0xb9},
		.str = "MOV_IMM_RCX_LARGE",
		.opcode_size = 2,
		.payload_address = true,
		.payload_value = false,
		.payload_size = 8,
	},
	[CALL_RCX] = {
		.opcode = {0xff, 0xd1},
		.str = "CALL_RCX",
		.opcode_size = 2,
		.payload_address = false,
		.payload_value = false,
	},
	[RET] = {
		.opcode = {0xc3},
		.str = "RET",
		.opcode_size = 1,
		.payload_address = false,
		.payload_value = false,
	},
}
;

#define ADD_RBX_RAX (0x00d80148)
#define SUB_RBX_RAX (0x00d82948)
#define MUL_RBX (0x00ebf748)
#define CLEAR_RDX (0x99)
#define DIV_RBX (0x00fbf748)
#define PUSH_SMALL (0x6a)
#define PUSH_LARGE (0x68)
#define POP_RAX (0x58)
#define POP_RBX (0x5b)
#define POP_RDI (0x5f)
#define POP_RSI (0x5e)
#define PUSH_RAX (0x50)
#define PUSH_RBX (0x53)
#define PUSH_RCX (0x51)
#define MOV_RAX_RDI (0x00c78948)
#define MOV_IMM_RAX (0x00c0c748)
#define MOV_IMM_RCX_LARGE (0xb948)
#define CALL_RCX (0xd1ff)
#define RET (0xc3)

typedef uint8_t *(*instruction_handler)(enum instruction_e instr, libjit_value value,
					void *addr, uint8_t *offset);

uint8_t *handle_simple_instr(enum instruction_e instr, libjit_value value, void *addr,
			     uint8_t *offset)
{
	(void)instr;
	(void)value;
	(void)addr;
	(void)offset;
	return NULL;
}

uint8_t *handle_complex_instr(enum instruction_e instr, libjit_value value, void *addr,
			      uint8_t *offset)
{
	(void)instr;
	(void)value;
	(void)addr;
	(void)offset;
	return NULL;
}

uint8_t *instruction_unknown(enum instruction_e instr, libjit_value value, void *addr,
			     uint8_t *offset)
{
	(void)instr;
	(void)value;
	(void)addr;
	(void)offset;

	LIBJIT_DIE("Instruction %d unknown.", instr);
	return NULL;
}

uint8_t *write_instr2(enum instruction_e instr, libjit_value value, void *addr,
		   uint8_t *offset)
{
	(void)value;
	(void)addr;
	for (size_t i = 0; i < instructions[instr].opcode_size; i++, offset++)
		*offset = instructions[instr].opcode[i];

	return offset;
}

uint8_t *write_operation(enum operation operation, libjit_value value,
			 size_t addr, uint8_t *offset)
{
	switch (operation) {
	case OPER_ADD:
		*((uint32_t *)offset) = ADD_RBX_RAX;
		return offset + 3;
	case OPER_SUB:
		*((uint32_t *)offset) = SUB_RBX_RAX;
		return offset + 3;
	case OPER_MULT:
		*((uint32_t *)offset) = MUL_RBX;
		return offset + 3;
	case OPER_DIV:
		*offset = CLEAR_RDX;
		offset++;
		*((uint32_t *)offset) = DIV_RBX;
		return offset + 3;
	case OPER_RET:
		*((uint8_t *)offset) = RET;
		return offset + 1;
	case OPER_PUSH_ADDR:
		value = addr;
		// fall through
	case OPER_PUSH_IMM:
		*(uint16_t *)offset = MOV_IMM_RCX_LARGE;
		offset += 2;
		*(uint64_t *)offset = value;
		offset += 8;
		*offset = PUSH_RCX;
		return offset + 1;
	case OPER_PUSH_A:
		*offset = PUSH_RAX;
		return offset + 1;
	case OPER_PUSH_B:
		*offset = PUSH_RBX;
		return offset + 1;
	case OPER_POP_A:
		*offset = POP_RAX;
		return offset + 1;
	case OPER_POP_B:
		*offset = POP_RBX;
		return offset + 1;
	case OPER_POP_PARAM1:
		*offset = POP_RDI;
		return offset + 1;
	case OPER_POP_PARAM2:
		*offset = POP_RSI;
		return offset + 1;
	case OPER_CALL:
		*(uint16_t *)offset = MOV_IMM_RCX_LARGE;
		offset += 2;
		*(uint64_t *)offset = addr;
		offset += 8;
		*(uint16_t *)offset = CALL_RCX;
		return offset + 2;
	default:
		LIBJIT_DIE("Unhandled instruction\n");
	}

	return 0;
}
