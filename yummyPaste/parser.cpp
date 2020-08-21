#include "plugin.h"
#include "parser.h"

BINARY_DATA gBinary = { 0 };


void *Malloc(size_t size)
{
	BYTE *mem = (BYTE *)malloc(size);

	if (!mem)
		return NULL;

	memset(mem, 0, size);

	return mem;
}

BOOL _ReAlloc(void **ptr, size_t oldSize, size_t newSize)
{
	BYTE *mem = NULL;
	int expansion = newSize - oldSize;

	if (ptr == NULL)
		return FALSE;

	mem = (BYTE *)realloc(*ptr, newSize);

	if (!mem)
		return FALSE;

	if (expansion > 0)
		memset(mem + oldSize, 0, expansion);

	*ptr = mem;

	return TRUE;
}

void _Free(void **p)
{
	if (p == NULL)
		return;

	if (*p == NULL)
		return;

	free(*p);

	*p = NULL;
}

#define ReAlloc(p,old,news) _ReAlloc( (void **)&(p), (old), (news) )



BOOL PutByteArray(BYTE *ba, size_t size)
{
	if (size > gBinary.size - gBinary.index)
	{
		if (!ReAlloc(gBinary.binary, gBinary.size, gBinary.size + size + 100))
		{
			return FALSE;
		}

		gBinary.size += size + 100;
	}

	memcpy(gBinary.binary + gBinary.index, ba, size);
	gBinary.index += size;

	return TRUE;
}

BOOL PutByte(BYTE b)
{
	return PutByteArray(&b, 1);
}

void ResetBinaryObject()
{
	if (!gBinary.binary)
	{
		InitBinaryObject(100);
		return;
	}

	memset(gBinary.binary, 0, gBinary.size);
	gBinary.index = 0;
	gBinary.invalid = FALSE;
}

BOOL InitBinaryObject(size_t initial)
{
	gBinary.binary = (BYTE *)Malloc(initial);

	if (!gBinary.binary)
		return FALSE;

	gBinary.index = 0;
	gBinary.size = initial;

	return TRUE;
}

void DestroyBinaryObject()
{
	Free(gBinary.binary);

	memset(&gBinary, 0, sizeof(BINARY_DATA));
}


BOOL IsDelimiter(CHAR  c)
{
	switch (c)
	{
	case '\r':
	case '\n':
	case '\b':
	case '\t':
	case ' ':
	case ',':
	case '{':
	case '}':
		return TRUE;
	}

	return FALSE;
}

BOOL IsHexChar(CHAR c)
{
	c = tolower(c);

	if (c >= '0' && c <= '9')
		return TRUE;

	if (c >= 'a' && c <= 'f')
		return TRUE;

	return FALSE;
}

#define MAX_HEX_BLOCK 2

size_t GetNum(CHAR hex)
{
	hex = tolower(hex);

	if (hex >= '0' && hex <= '9')
		return hex - 0x30;

	if (hex >= 'a' && hex <= 'f')
		return hex - 0x57;

	return 0;
}

size_t Hex2Val(LPSTR sval, size_t len)
{
	size_t val = 0, num = 0;
	LPSTR p = sval;

	for (int i = 0; i < len; i++)
	{
		num = GetNum(*p++);
		val += num;

		if (i != len - 1)
			val <<= 4;
	}


	return val;
}

BYTE GetByteValue(LPSTR data, size_t length)
{
	if (length > 2)
	{
		if (!strncmp(data, "0x", 2))
		{
			if (strchr(data + 2, 'x') || strchr(data + 2,'-'))
			{
				gBinary.invalid = TRUE;
				return 0;
			}

			return (BYTE)Hex2Val(data + 2, length - 2);
		}
	}

	if (strchr(data, 'x'))
	{
		gBinary.invalid = TRUE;
		return 0;
	}

	if (*data == '-')
	{
		if (length == 1)
		{
			gBinary.invalid = TRUE;
			return 0;
		}

		if (strchr(data + 1, '-'))
		{
			gBinary.invalid = TRUE;
			return 0;
		}
	}

	return (BYTE)atoi(data);
}

#define PutValueAndResetBuffer() PutByte((BYTE)Hex2Val(buf, bufi)); \
									written++; \
									bufi = 0; \
									memset(buf, 0, sizeof(buf))





size_t TryExtractShellcodeStyle(LPSTR data, size_t length)
{
	size_t i = 0, bufi = 0, written = 0;
	CHAR buf[MAX_HEX_BLOCK + 1] = { 0 };
	BOOL fail = FALSE;

	while (data[i])
	{

		if (gBinary.invalid)
			return 0;

		if (i + 2 >= length)
			break;

		if (data[i] != '\\' && data[i + 1] != 'x')
		{
			fail = TRUE;
			break;
		}

		i += 2;

		while (i < length)
		{
			if (!IsHexChar(data[i]))
			{
				if (data[i] == '\\' || data[i] == 0)
				{
					PutValueAndResetBuffer();

					break;
				}

				fail = TRUE;
				break;
			}

			if (!IsHexChar(data[i]))
			{
				fail = TRUE;
				break;
			}

			buf[bufi++] = data[i++];

			if (bufi == MAX_HEX_BLOCK)
			{
				PutValueAndResetBuffer();

				break;
			}

		}

		if (fail)
			break;
	}

	if (fail)
	{
		gBinary.invalid = TRUE;
		written = 0;
	}
	else
	{
		if (bufi > 0)
		{
			PutValueAndResetBuffer();
		}
	}


	return written;
}




BOOL ParseBytes(LPSTR data, size_t length)
{
	size_t bufi = 0;
	LPSTR p = data, sp, supply = NULL;
	BOOL quoteOn = FALSE,curly=FALSE;

	supply = (LPSTR)Malloc(length + 1);

	if (!supply)
		return FALSE;

	sp = supply;

	while (*p)
	{
		if (gBinary.invalid)
		{
			bufi = 0;
			break;
		}

		
		if (*p == '"' || *p == '\'')
		{
			quoteOn = !quoteOn;

			if (quoteOn)
			{
				if (bufi > 0)
					PutByte(GetByteValue(supply, bufi));

			}
			else
			{
				if (bufi > 0)
					TryExtractShellcodeStyle(supply, bufi);

			}

			if (bufi > 0)
			{
				memset(supply, 0, bufi);
				bufi = 0;
				sp = supply;
			}

			p++;
			continue;
		}
		else
		{
			
			if (*p == '{' || *p == '}')
			{
				curly = *p == '{' ? TRUE : FALSE;

				p++;
				continue;
			}
			
		}


		if (!quoteOn)
		{
			if (IsDelimiter(*p))
			{
				if (bufi > 0)
				{
					PutByte(GetByteValue(supply, bufi));
					memset(supply, 0, bufi);
					bufi = 0;
					sp = supply;
				}

				p++;
				continue;
			}
			else
			{
				//maybe I can try to parse shellcode style without quotation.
			}
		}

		if (IsHexChar(*p) || *p == '-' || *p == 'x' || (quoteOn && *p == '\\'))
		{
			*sp++ = *p++;
			bufi++;
		}
		else
		{
			if (*p == ';' && !curly && !quoteOn)
			{
				p++;
				continue;
			}

			gBinary.invalid = TRUE;
			bufi = 0;
			break;
		}

	}

	if (quoteOn)
		gBinary.invalid = TRUE;
	else
	{
		if (bufi > 0)
			PutByte(GetByteValue(supply, bufi));
	}

	Free(supply);

	return !gBinary.invalid;
}


BINARY_DATA *GetBinaryData()
{
	return &gBinary;
}

struct
{
	LPSTR input;
	BYTE expect[7];
	BOOL willFail;
}Tests[] = {
	{ "{ 0xFA, 0xDE, 0x24, 255 } '\\xde\\xBF\\xf'", {0xfa,0xde,0x24,255,0xde,0xbf,0xf}, FALSE },
	{"\r\n   124, 65 0x95 21,0x44 '\\xaa\\xee'" , {124,65,0x95,21,0x44,0xaa,0xee}, FALSE },
	{"'\\xde\\xad\\xbe\\xef\\x41\\x41'", {0xde,0xad,0xbe,0xef,0x41,0x41},FALSE},
	{ "{ 0xFA, 0xDEx, 0x24, 255 } '\\xde\\xBF\\xf'", {0xfa,0xde,0x24,255,0xde,0xbf,0xf}, TRUE},
	{"\r\n   124, 65 0x95 21,0x44 '\\xaax\\xee'" , {124,65,0x95,21,0x44,0xaa,0xee}, TRUE},
};
void TestParser()
{
	InitBinaryObject(2);

	for (int i = 0; i < sizeof(Tests) / sizeof(Tests[0]); i++)
	{

		ParseBytes(Tests[i].input, strlen(Tests[i].input));

		if (Tests[i].willFail)
		{
			if (gBinary.invalid)
			{
				printf("Test %d is PASSED\n", i + 1);
			}
			else
			{
				printf("Test %d is FAILED\n", i + 1);
			}
		}
		else
		{
			if (gBinary.invalid)
			{
				printf("Test %d is FAILED\n", i + 1);
			}
			else
			{
				if (!memcmp(Tests[i].expect, gBinary.binary, 7))
				{
					printf("Test %d is PASSED\n", i + 1);
				}
				else
				{
					printf("Test %d is FAILED\n", i + 1);
				}
			}
		}

		ResetBinaryObject();
	}

	DestroyBinaryObject();
}