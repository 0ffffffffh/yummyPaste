#pragma once

typedef struct _BINARY_DATA
{
	BYTE *binary;
	size_t size;
	size_t index;
	BOOL invalid;
}BINARY_DATA;


void *Malloc(size_t size);

void _Free(void **p);

#define Free(p) _Free((void **)&(p))



void ResetBinaryObject();

BOOL InitBinaryObject(size_t initial);

void DestroyBinaryObject();

BOOL ParseBytes(LPSTR data, size_t length);

BINARY_DATA *GetBinaryData();