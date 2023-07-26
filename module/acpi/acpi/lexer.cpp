#include "aml_executive.h"

#include <mkmi_log.h>
#include <mkmi_memory.h>

void ParseByte(TokenList *tokens, AML_Hashmap *hashmap, uint8_t *data, size_t *idx) {
	uint8_t byte = data[*idx];
	*idx += 1;

	AML_OpcodeHandler handler = FindHandler(hashmap, byte);

	if (handler) return handler(hashmap, tokens, data, idx);
	return AddToken(tokens, UNKNOWN, byte);	
}

AMLExecutive::AMLExecutive() {
	/* Initialize the hashmap and the root token list */
	Hashmap = CreateHashmap();
	RootTokenList = CreateTokenList(); 
}

AMLExecutive::~AMLExecutive() {
	/* Free the memory occupied by the token list and the hashmap */
	DeleteHashmap(Hashmap);
	FreeTokenList(RootTokenList);
}

inline void PrintName(char *nameSegments, size_t count, bool isRoot) {
	char segs[count * 4 + 1];
	if (nameSegments != NULL || nameSegments != -1) {
		Memcpy(segs, nameSegments, count * 4);
	}
	segs[count * 4] = '\0';
	MKMI_Printf(" - Name segments: %d\r\n"
		    " - Is root: %s\r\n"
		    " - Segments: %s\r\n",
		    count, isRoot ? "Yes" : "No", segs);
}

int AMLExecutive::Parse(uint8_t *data, size_t size) {
	size_t idx = 0;
	while (idx < size) {
		ParseByte(RootTokenList, Hashmap, data, &idx);
	}

	MKMI_Printf("Done parsing %d bytes of AML code.\r\n", size);
//	return 0;

	Token *current = RootTokenList->Head;

	bool error = false;
	while (current && !error) {
//	for (int i = 0; i < 250 && current && !error; ++i) {
		MKMI_Printf("Token Type: ");
		switch (current->Type) {
			case ZERO:
				MKMI_Printf("ZERO\r\n");
				break;
			case ONE:
				MKMI_Printf("ONE\r\n");
				break;
			case ALIAS:
				MKMI_Printf("ALIAS\r\n");
				break;
			case NAME:
				MKMI_Printf("NAME:\r\n");
				PrintName(current->Name.NameSegments, current->Name.SegmentNumber, current->Name.IsRoot);
				break;
			case INTEGER:
				MKMI_Printf("INTEGER:\r\n"
					    " - Size: %d\r\n"
					    " - Data: %d\r\n", current->Int.Size, current->Int.Data);
				break;
			case STRING:
				MKMI_Printf("STRING\r\n");
				break;
			case SCOPE:
				MKMI_Printf("SCOPE:\r\n"
				            " - PkgLength: %d\r\n", current->Scope.PkgLength);
				PrintName(current->Scope.Name.NameSegments, current->Scope.Name.SegmentNumber, current->Scope.Name.IsRoot);
				break;
			case BUFFER: {
				MKMI_Printf("BUFFER:\r\n"
					    " - PkgLength: %d\r\n"
					    " - Buffer size: %d\r\n"
					    " - Byte list:", current->Buffer.PkgLength, current->Buffer.BufferSize.Data);
				for (int i = 0; i < current->Buffer.BufferSize.Data; ++i) {
					MKMI_Printf(" 0x%x ", current->Buffer.ByteList[i]);
				}
				MKMI_Printf("\r\n");
				}
				break;
			case PACKAGE:
				MKMI_Printf("PACKAGE\r\n");
				break;
			case REGION:
				MKMI_Printf("REGION:\r\n"
					    " - Region space: %d\r\n"
					    " - Region offset: %d\r\n"
					    " - Region length: %d\r\n", current->Region.RegionSpace, current->Region.RegionOffset.Data, current->Region.RegionLen.Data);
				PrintName(current->Region.Name.NameSegments, current->Region.Name.SegmentNumber, current->Region.Name.IsRoot);
				break;
			case FIELD:
				MKMI_Printf("FIELD:\r\n"
					    " - PkgLength: %d\r\n"
					    " - Field flags: %d\r\n", current->Field.PkgLength, current->Field.FieldFlags);
				PrintName(current->Field.Name.NameSegments, current->Field.Name.SegmentNumber, current->Field.Name.IsRoot);
				break;
			case DEVICE:
				MKMI_Printf("DEVICE:\r\n"
					    " - PkgLength: %d\r\n", current->Device.PkgLength);
				PrintName(current->Device.Name.NameSegments, current->Device.Name.SegmentNumber, current->Device.Name.IsRoot);
				break;
			case UNKNOWN:
				MKMI_Printf("UNKNOWN, Opcode: 0x%x\r\n", current->UnknownOpcode);
				break;
			default:
				MKMI_Printf("UNKNOWN\r\n");
				error = true;
				break;
		}

		current = current->Next;
	}

	return 0;
}
	
int AMLExecutive::Execute() {
	return 0;
}
