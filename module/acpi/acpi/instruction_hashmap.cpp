#include "instruction_hashmap.h"
#include "instruction_handlers.h"
#include "aml_opcodes.h"

#include <mkmi_log.h>
#include <mkmi_memory.h>


AML_Hashmap *CreateHashmap() {
	AML_Hashmap *hashmap = new AML_Hashmap;
	hashmap->Size = 89;
	hashmap->Entries = new AML_HashmapEntry[hashmap->Size];
	
	hashmap->Entries[0] = (AML_HashmapEntry){AML_ZERO_OP, HandleZeroOp};
	hashmap->Entries[1] = (AML_HashmapEntry){AML_ONE_OP, HandleOneOp};
	hashmap->Entries[2] = (AML_HashmapEntry){AML_ALIAS_OP, HandleAliasOp};
	hashmap->Entries[3] = (AML_HashmapEntry){AML_NAME_OP, HandleNameOp};
	hashmap->Entries[4] = (AML_HashmapEntry){AML_BYTEPREFIX, HandleIntegerOp};
	hashmap->Entries[5] = (AML_HashmapEntry){AML_WORDPREFIX, HandleIntegerOp};
	hashmap->Entries[6] = (AML_HashmapEntry){AML_DWORDPREFIX, HandleIntegerOp};
	hashmap->Entries[7] = (AML_HashmapEntry){AML_QWORDPREFIX, HandleIntegerOp};
	hashmap->Entries[8] = (AML_HashmapEntry){AML_STRINGPREFIX, HandleStringPrefix};
	hashmap->Entries[9] = (AML_HashmapEntry){AML_SCOPE_OP, HandleScopeOp};
	hashmap->Entries[10] = (AML_HashmapEntry){AML_BUFFER_OP, HandleBufferOp};
	hashmap->Entries[11] = (AML_HashmapEntry){AML_PACKAGE_OP, HandlePackageOp};
	hashmap->Entries[12] = (AML_HashmapEntry){AML_VARPACKAGE_OP, NULL};
	hashmap->Entries[13] = (AML_HashmapEntry){AML_METHOD_OP, NULL};
	hashmap->Entries[14] = (AML_HashmapEntry){AML_EXTERNAL_OP, NULL};
	hashmap->Entries[15] = (AML_HashmapEntry){AML_DUAL_PREFIX, NULL};
	hashmap->Entries[16] = (AML_HashmapEntry){AML_MULTI_PREFIX, NULL};
	hashmap->Entries[17] = (AML_HashmapEntry){AML_EXTOP_PREFIX, HandleExtendedOp};
	hashmap->Entries[18] = (AML_HashmapEntry){AML_ROOT_CHAR, NULL};
	hashmap->Entries[19] = (AML_HashmapEntry){AML_PARENT_CHAR, NULL};
	hashmap->Entries[20] = (AML_HashmapEntry){AML_LOCAL0_OP, NULL};
	hashmap->Entries[21] = (AML_HashmapEntry){AML_LOCAL1_OP, NULL};
	hashmap->Entries[22] = (AML_HashmapEntry){AML_LOCAL2_OP, NULL};
	hashmap->Entries[23] = (AML_HashmapEntry){AML_LOCAL3_OP, NULL};
	hashmap->Entries[24] = (AML_HashmapEntry){AML_LOCAL4_OP, NULL};
	hashmap->Entries[25] = (AML_HashmapEntry){AML_LOCAL5_OP, NULL};
	hashmap->Entries[26] = (AML_HashmapEntry){AML_LOCAL6_OP, NULL};
	hashmap->Entries[27] = (AML_HashmapEntry){AML_LOCAL7_OP, NULL};
	hashmap->Entries[28] = (AML_HashmapEntry){AML_ARG0_OP, NULL};
	hashmap->Entries[29] = (AML_HashmapEntry){AML_ARG1_OP, NULL};
	hashmap->Entries[30] = (AML_HashmapEntry){AML_ARG2_OP, NULL};
	hashmap->Entries[31] = (AML_HashmapEntry){AML_ARG3_OP, NULL};
	hashmap->Entries[32] = (AML_HashmapEntry){AML_ARG4_OP, NULL};
	hashmap->Entries[33] = (AML_HashmapEntry){AML_ARG5_OP, NULL};
	hashmap->Entries[34] = (AML_HashmapEntry){AML_ARG6_OP, NULL};
	hashmap->Entries[35] = (AML_HashmapEntry){AML_STORE_OP, NULL};
	hashmap->Entries[36] = (AML_HashmapEntry){AML_REFOF_OP, NULL};
	hashmap->Entries[37] = (AML_HashmapEntry){AML_ADD_OP, NULL};
	hashmap->Entries[38] = (AML_HashmapEntry){AML_CONCAT_OP, NULL};
	hashmap->Entries[39] = (AML_HashmapEntry){AML_SUBTRACT_OP, NULL};
	hashmap->Entries[40] = (AML_HashmapEntry){AML_INCREMENT_OP, NULL};
	hashmap->Entries[41] = (AML_HashmapEntry){AML_DECREMENT_OP, NULL};
	hashmap->Entries[42] = (AML_HashmapEntry){AML_MULTIPLY_OP, NULL};
	hashmap->Entries[43] = (AML_HashmapEntry){AML_DIVIDE_OP, NULL};
	hashmap->Entries[44] = (AML_HashmapEntry){AML_SHL_OP, NULL};
	hashmap->Entries[45] = (AML_HashmapEntry){AML_SHR_OP, NULL};
	hashmap->Entries[46] = (AML_HashmapEntry){AML_AND_OP, NULL};
	hashmap->Entries[47] = (AML_HashmapEntry){AML_OR_OP, NULL};
	hashmap->Entries[48] = (AML_HashmapEntry){AML_XOR_OP, NULL};
	hashmap->Entries[49] = (AML_HashmapEntry){AML_NAND_OP, NULL};
	hashmap->Entries[50] = (AML_HashmapEntry){AML_NOR_OP, NULL};
	hashmap->Entries[51] = (AML_HashmapEntry){AML_NOT_OP, NULL};
	hashmap->Entries[52] = (AML_HashmapEntry){AML_FINDSETLEFTBIT_OP, NULL};
	hashmap->Entries[53] = (AML_HashmapEntry){AML_FINDSETRIGHTBIT_OP, NULL};
	hashmap->Entries[54] = (AML_HashmapEntry){AML_DEREF_OP, NULL};
	hashmap->Entries[55] = (AML_HashmapEntry){AML_CONCATRES_OP, NULL};
	hashmap->Entries[56] = (AML_HashmapEntry){AML_MOD_OP, NULL};
	hashmap->Entries[57] = (AML_HashmapEntry){AML_NOTIFY_OP, NULL};
	hashmap->Entries[58] = (AML_HashmapEntry){AML_SIZEOF_OP, NULL};
	hashmap->Entries[59] = (AML_HashmapEntry){AML_INDEX_OP, NULL};
	hashmap->Entries[60] = (AML_HashmapEntry){AML_MATCH_OP, NULL};
	hashmap->Entries[61] = (AML_HashmapEntry){AML_DWORDFIELD_OP, NULL};
	hashmap->Entries[62] = (AML_HashmapEntry){AML_WORDFIELD_OP, NULL};
	hashmap->Entries[63] = (AML_HashmapEntry){AML_BYTEFIELD_OP, NULL};
	hashmap->Entries[64] = (AML_HashmapEntry){AML_BITFIELD_OP, NULL};
	hashmap->Entries[65] = (AML_HashmapEntry){AML_OBJECTTYPE_OP, NULL};
	hashmap->Entries[66] = (AML_HashmapEntry){AML_QWORDFIELD_OP, NULL};
	hashmap->Entries[67] = (AML_HashmapEntry){AML_LAND_OP, NULL};
	hashmap->Entries[68] = (AML_HashmapEntry){AML_LOR_OP, NULL};
	hashmap->Entries[69] = (AML_HashmapEntry){AML_LNOT_OP, NULL};
	hashmap->Entries[70] = (AML_HashmapEntry){AML_LEQUAL_OP, NULL};
	hashmap->Entries[71] = (AML_HashmapEntry){AML_LGREATER_OP, NULL};
	hashmap->Entries[72] = (AML_HashmapEntry){AML_LLESS_OP, NULL};
	hashmap->Entries[73] = (AML_HashmapEntry){AML_TOBUFFER_OP, NULL};
	hashmap->Entries[74] = (AML_HashmapEntry){AML_TODECIMALSTRING_OP, NULL};
	hashmap->Entries[75] = (AML_HashmapEntry){AML_TOHEXSTRING_OP, NULL};
	hashmap->Entries[76] = (AML_HashmapEntry){AML_TOINTEGER_OP, NULL};
	hashmap->Entries[77] = (AML_HashmapEntry){AML_TOSTRING_OP, NULL};
	hashmap->Entries[78] = (AML_HashmapEntry){AML_MID_OP, NULL};
	hashmap->Entries[79] = (AML_HashmapEntry){AML_COPYOBJECT_OP, NULL};
	hashmap->Entries[80] = (AML_HashmapEntry){AML_CONTINUE_OP, NULL};
	hashmap->Entries[81] = (AML_HashmapEntry){AML_IF_OP, NULL};
	hashmap->Entries[82] = (AML_HashmapEntry){AML_ELSE_OP, NULL};
	hashmap->Entries[83] = (AML_HashmapEntry){AML_WHILE_OP, NULL};
	hashmap->Entries[84] = (AML_HashmapEntry){AML_NOP_OP, NULL};
	hashmap->Entries[85] = (AML_HashmapEntry){AML_RETURN_OP, NULL};
	hashmap->Entries[86] = (AML_HashmapEntry){AML_BREAK_OP, NULL};
	hashmap->Entries[87] = (AML_HashmapEntry){AML_BREAKPOINT_OP, NULL};
	hashmap->Entries[88] = (AML_HashmapEntry){AML_ONES_OP, NULL};

	return hashmap;
}

// Find the handler function from the hashmap based on the opcode
AML_OpcodeHandler FindHandler(AML_Hashmap *hashmap, uint8_t opcode) {
	for (size_t i = 0; i < hashmap->Size; ++i) {
		if (hashmap->Entries[i].OPCode == opcode) {
			return hashmap->Entries[i].Handler;
		}
	}

	return NULL; // Handler not found
}

void DeleteHashmap(AML_Hashmap *hashmap) {
	delete[] hashmap->Entries;
	delete hashmap;
}
