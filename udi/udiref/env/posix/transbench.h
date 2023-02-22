
#define REPEAT2(a,b,c)  a,b,c, a,b,c
#define REPEAT4(a,b,c)  REPEAT2(a,b,c), REPEAT2(a,b,c)
#define REPEAT5(a,b,c)  REPEAT2(a,b,c), REPEAT2(a,b,c), a,b,c
#define REPEAT10(a,b,c) REPEAT5(a,b,c), REPEAT5(a,b,c)
#define REPEAT20(a,b,c) REPEAT10(a,b,c), REPEAT10(a,b,c)
#define REPEAT50(a,b,c) REPEAT20(a,b,c), REPEAT20(a,b,c), REPEAT10(a,b,c)
#define REPEAT100(a,b,c) REPEAT50(a,b,c), REPEAT50(a,b,c)
#define REPEAT200(a,b,c) REPEAT100(a,b,c), REPEAT100(a,b,c)
#define REPEAT500(a,b,c) REPEAT200(a,b,c), REPEAT200(a,b,c), REPEAT100(a,b,c)
#define REPEAT1000(a,b,c) REPEAT500(a,b,c), REPEAT500(a,b,c)
#define REPEAT2k(a,b,c) REPEAT1000(a,b,c), REPEAT1000(a,b,c)
#define REPEAT5k(a,b,c) REPEAT2k(a,b,c), REPEAT2k(a,b,c), REPEAT1000(a,b,c)
#define REPEAT10k(a,b,c) REPEAT5k(a,b,c), REPEAT5k(a,b,c)

#define BUILD_ALL_SIZES(name, a, b) \
udi_pio_trans_t name[] = { \
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 1 } , \
	REPEAT10k({a,UDI_PIO_1BYTE,b}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 2 } , \
	REPEAT10k({a,UDI_PIO_2BYTE,b}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 3 } , \
	REPEAT10k({a,UDI_PIO_4BYTE,b}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 4 } , \
	REPEAT10k({a,UDI_PIO_8BYTE,b}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 5 } , \
	REPEAT10k({a,UDI_PIO_16BYTE,b}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 6 } , \
	REPEAT10k({a,UDI_PIO_32BYTE,b}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
}
#define BUILD_ALL_SAME(name, a, b, c) \
udi_pio_trans_t name[] = { \
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 1 } , \
	REPEAT10k({a,b,c}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 2 } , \
	REPEAT10k({a,b,c}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 3 } , \
	REPEAT10k({a,b,c}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 4 } , \
	REPEAT10k({a,b,c}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 5 } , \
	REPEAT10k({a,b,c}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
	{ UDI_PIO_LABEL, UDI_PIO_2BYTE, 6 } , \
	REPEAT10k({a,b,c}), \
	{ UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0 }, \
	\
}

BUILD_ALL_SAME(load_imm_list, UDI_PIO_LOAD_IMM + UDI_PIO_R0, UDI_PIO_2BYTE,
	       0x1234);
BUILD_ALL_SIZES(xor_list, UDI_PIO_XOR + UDI_PIO_R0, UDI_PIO_R7);
BUILD_ALL_SIZES(shift_left_list, UDI_PIO_SHIFT_LEFT + UDI_PIO_R0, UDI_PIO_R7);
BUILD_ALL_SIZES(in_direct_list, UDI_PIO_IN + UDI_PIO_DIRECT + UDI_PIO_R0, 0);
BUILD_ALL_SIZES(in_mem_list, UDI_PIO_IN + UDI_PIO_MEM + UDI_PIO_R0, 0);

udi_pio_trans_t end_list[] = {
	{UDI_PIO_END_IMM + UDI_PIO_R0, UDI_PIO_2BYTE, 0}
};
