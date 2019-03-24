#include "ass.h"
#include "assert.h"
#include "log.h"
#include "jit.h"

/* Only used for readability. */
#define NO_VALUE (-1)

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
	INSTR_COUNT,
};

static struct instruction instructions[] = {
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
};

static uint8_t *instruction_unknown(enum instruction_e instr, libjit_value value,
			     uint8_t *offset)
{
	(void)instr;
	(void)value;
	(void)offset;

	LIBJIT_DIE("Instruction %d unknown.", instr);
	return NULL;
}

static uint8_t *write_address(uint8_t *offset, void *addr)
{
	*(size_t **)offset = addr;
	return offset + sizeof(size_t);
}

static uint8_t *write_value(uint8_t *offset, libjit_value value, size_t size)
{
	uint8_t *ptr = (uint8_t *)&value;
	for (size_t i = 0; i < size; i++)
		offset[i] = ptr[i];

	return offset + size;
}

static uint8_t *write_instruction(enum instruction_e instr, libjit_value value,
			   uint8_t *offset)
{
	if (instr >= INSTR_COUNT)
		return instruction_unknown(instr, value, offset);

	for (size_t i = 0; i < instructions[instr].opcode_size; i++, offset++)
		*offset = instructions[instr].opcode[i];

	if (instructions[instr].payload_address)
		return write_address(offset, (void *)value);
	if (instructions[instr].payload_value)
		return write_value(offset, value,
				   instructions[instr].payload_size);

	return offset;
}

typedef uint8_t *(*operation_handler)(enum operation operation,
				      libjit_value value, uint8_t *offset);

static enum instruction_e simple_operation_mapping[] = {
	[OPER_ADD] = ADD_RBX_RAX,    [OPER_SUB] = SUB_RBX_RAX,
	[OPER_MULT] = MUL_RBX,	     [OPER_PUSH_A] = PUSH_RAX,
	[OPER_PUSH_B] = PUSH_RBX,    [OPER_POP_A] = POP_RAX,
	[OPER_POP_B] = POP_RBX,	     [OPER_POP_PARAM1] = POP_RDI,
	[OPER_POP_PARAM2] = POP_RSI, [OPER_RET] = RET,
};

static uint8_t *simple_operation_handler(enum operation operation, libjit_value value,
				  uint8_t *offset)
{
	return write_instruction(simple_operation_mapping[operation], value,
				 offset);
}

static uint8_t *handle_division(enum operation operation, libjit_value value,
			 uint8_t *offset)
{
	(void)operation;
	(void)value;

	offset = write_instruction(CLEAR_RDX, NO_VALUE, offset);
	return write_instruction(DIV_RBX, NO_VALUE, offset);
}

static uint8_t *handle_call(enum operation operation, libjit_value value,
		     uint8_t *offset)
{
	(void)operation;

	offset = write_instruction(MOV_IMM_RCX_LARGE, value, offset);
	return write_instruction(CALL_RCX, NO_VALUE, offset);
}

static uint8_t *handle_push_imm(enum operation operation, libjit_value value,
			 uint8_t *offset)
{
	(void)operation;

	offset = write_instruction(MOV_IMM_RCX_LARGE, value, offset);
	return write_instruction(PUSH_RCX, NO_VALUE, offset);
}

static operation_handler operation_handlers[] = {
	[SIMPLE_OPER_BEGIN... SIMPLE_OPER_END] = simple_operation_handler,
	[OPER_DIV] = handle_division,
	[OPER_PUSH_IMM] = handle_push_imm,
	[OPER_CALL] = handle_call,
};

uint8_t *write_operation(enum operation operation, size_t value,
			 uint8_t *offset)
{
	ASSERT(operation < OPER_COUNT, "Operation %d doesn't exist.",
	       operation);
	return operation_handlers[operation](operation, value, offset);
}
