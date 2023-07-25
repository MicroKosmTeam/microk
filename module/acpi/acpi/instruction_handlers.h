#pragma once
#include <stdint.h>
#include <stddef.h>

struct AML_Hashmap;
struct TokenList;

void HandleUnknowOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandleZeroOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandleOneOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandleAliasOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandleNameOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandleBytePrefix(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandleWordPrefix(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandleDWordPrefix(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandleStringPrefix(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandleQWordPrefix(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandleScopeOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandleBufferOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
void HandlePackageOp(AML_Hashmap *hashmap, TokenList *list, uint8_t *data, size_t *idx);
