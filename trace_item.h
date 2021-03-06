
#ifndef TRACE_ITEM_H
#define TRACE_ITEM_H

// this is tpts

//added a "squashed" to the enum for use in the branch prediction
enum trace_item_type {
	ti_NOP = 0,
	
	ti_RTYPE,
	ti_ITYPE,
	ti_LOAD,
	ti_STORE,
	ti_BRANCH,
	ti_JTYPE,
	ti_SPECIAL,
	ti_JRTYPE,
	
	ti_SQUASHED
};

struct trace_item {
	unsigned char type;			// see above
	unsigned char sReg_a;			// 1st operand
	unsigned char sReg_b;			// 2nd operand
	unsigned char dReg;			// dest. operand
	unsigned int PC;			// program counter
	unsigned int Addr;			// mem. address
};

#endif
