#include <postgres.h>
#include <fmgr.h>
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif
#include <funcapi.h>
#include <utils/array.h>
#include <catalog/pg_type_d.h>
#include <utils/builtins.h>
#include <access/htup_details.h>

#define PGENCODE7_ARGCOUNT 2
#define PGENCODE7_ARGLEN 0
#define PGENCODE7_ARGDATA 1
PG_FUNCTION_INFO_V1(pgencode7);
Datum pgencode7(PG_FUNCTION_ARGS);

#define PGDECODE7_ARGCOUNT 2
#define PGDECODE7_ARGLEN 0
#define PGDECODE7_ARGTEXT 1
PG_FUNCTION_INFO_V1(pgdecode7);
Datum pgdecode7(PG_FUNCTION_ARGS);

const char PGCODEC7_MAP[] = {
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,
	0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
	0x21, 0x22,	
	0x24, 0x25, 0x26, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E,
	0x2F, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x5B, 0x5D, 0x5E, 0x5F, 0x60, 0x80, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8E,
	0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9E, 0x9F, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC,
	0xAE, 0xAF,	
	0x00 };

const unsigned char PGCODEC7_TAKEMASK[] = {
	0B00000000,
	0B10000000,
	0B11000000,
	0B11100000,
	0B11110000,
	0B11111000,
	0B11111100,
	0B11111110
};	

const unsigned char PGCODEC7_CARRYMASK[] = {
	0B00000000,
	0B00000001,
	0B00000011,
	0B00000111,
	0B00001111,
	0B00011111,
	0B00111111,
	0B01111111
};	

const unsigned char PGCODEC7_BALANCEMASK7[] = {
	0B00000000,
	0B01000000,
	0B01100000,
	0B01110000,
	0B01111000,
	0B01111100,
	0B01111110,
	0B11111111
};

const unsigned char PGCODEC7_BALANCEMASK6[] = {
	0B00000000,
	0B00100000,
	0B00110000,
	0B00111000,
	0B00111100,
	0B00111110,
	0B00111111,
	0B11111111
};

#define PGCODEC7_BUFFSIZE 1024

typedef struct {
	unsigned char* pData;
	int32 len;
	int32 maxEncodeStepByteCount;
	int32 encodeLength;
} Pgcodec7EncodeData;
		
Datum 
pgencode7(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	Datum result;
	bytea* bytes;
	int32 compressedByteCount, byteCount, i;
	Pgcodec7EncodeData* ped;
	unsigned char carry, carryByte, take, mask, shift, byte;
	char *encodes, *pRun;
	
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;
		
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
			
		if(PG_NARGS()!=PGENCODE7_ARGCOUNT)
			elog(ERROR, "argument count must be %d", PGENCODE7_ARGCOUNT);			

		if(PG_ARGISNULL(PGENCODE7_ARGLEN))
			elog(ERROR, "encoding length must be defined");

		ped = (Pgcodec7EncodeData*)palloc(1*sizeof(Pgcodec7EncodeData));
		funcctx->user_fctx = (void*)ped;
		
		ped->encodeLength = 	PG_GETARG_INT32(PGENCODE7_ARGLEN);
		if( ped->encodeLength!=6 && ped->encodeLength!=7 )
			elog(ERROR, "encode length must be 6 or 7");
					
		if(PG_ARGISNULL(PGENCODE7_ARGDATA))
			elog(ERROR, "data must be defined");
						
		bytes = PG_GETARG_BYTEA_PP(PGENCODE7_ARGDATA);
		ped->pData = (unsigned char*)VARDATA_ANY(bytes);
		ped->len = VARSIZE_ANY_EXHDR(bytes);
		
		compressedByteCount = ((ped->len * 8) + (ped->encodeLength-1) ) / ped->encodeLength;
		funcctx->max_calls = (compressedByteCount + (PGCODEC7_BUFFSIZE - 1))/PGCODEC7_BUFFSIZE;
		ped->maxEncodeStepByteCount = ((ped->encodeLength * PGCODEC7_BUFFSIZE) - (ped->encodeLength-1)) / 8;
				
		MemoryContextSwitchTo(oldcontext);
	}//if (SRF_IS_FIRSTCALL())
	
	funcctx = SRF_PERCALL_SETUP();
	
	if (funcctx->call_cntr < funcctx->max_calls) {
		ped = (Pgcodec7EncodeData*)funcctx->user_fctx;
		encodes = (char*)palloc(PGCODEC7_BUFFSIZE+1);	
		pRun = encodes;
		byteCount = ped->len < ped->maxEncodeStepByteCount ? ped->len : ped->maxEncodeStepByteCount;
		carry = 0;
		carryByte = 0B00000000;	
		for(i=0; i<byteCount; i++) {
			take = ped->encodeLength - carry;
			mask = PGCODEC7_TAKEMASK[take];
			shift = 8 - take;
			byte = carryByte | ( (*(ped->pData) & mask) >> shift );
			*(pRun++) = PGCODEC7_MAP[byte];			
			
			carry = 8 - take;
			mask = PGCODEC7_CARRYMASK[carry];
			shift = ped->encodeLength - carry;
			carryByte = (*(ped->pData) & mask) << shift;			
			
			if(carry==ped->encodeLength || (carry>0 && i==byteCount-1)) {
				*(pRun++) = PGCODEC7_MAP[carryByte];
				carry = 0;
				carryByte = 0B00000000;
			}
						
			ped->pData++;		
		}
		*pRun = 0;	
		ped->len -= byteCount;
		result = CStringGetTextDatum(encodes);
		pfree(encodes);
		SRF_RETURN_NEXT(funcctx, result);
	} else {
		SRF_RETURN_DONE(funcctx);
	}

}

Datum
pgdecode7(PG_FUNCTION_ARGS)
{
	ArrayType *arr;
	Datum *dimdatums;
	int32 ndim, i, decodeLength;
	char *s;
	unsigned char carry, carryByte, balance, byte, available, take, mask, shift;
	const unsigned char *pBalanceMask;
	
	bytea* byteout;
	StringInfoData bytedata;
	
	if(PG_NARGS()!=PGDECODE7_ARGCOUNT)
		elog(ERROR, "argument count must be %d", PGDECODE7_ARGCOUNT);			

	if(PG_ARGISNULL(PGDECODE7_ARGLEN))
		elog(ERROR, "decoding length must be defined");

	decodeLength = 	PG_GETARG_INT32(PGDECODE7_ARGLEN);
	if( decodeLength!=6 && decodeLength!=7 )
		elog(ERROR, "decode length must be 6 or 7");		
						
	if(PG_ARGISNULL(PGDECODE7_ARGTEXT))
		elog(ERROR, "text must be defined");
	
	pBalanceMask = decodeLength==7 ? PGCODEC7_BALANCEMASK7 : PGCODEC7_BALANCEMASK6;
	initStringInfo(&bytedata);	
	arr = PG_GETARG_ARRAYTYPE_P(PGDECODE7_ARGTEXT);
	deconstruct_array(arr, TEXTOID, -1, false, 'i', &dimdatums, NULL, &ndim);
	for(i=0; i<ndim; i++) {
		s = TextDatumGetCString(dimdatums[i]);
		carry = 0;
		carryByte = 0B00000000;
		balance = 8;	
		while( (byte = *(s++)) ) {
			byte = strchr(PGCODEC7_MAP, byte) - PGCODEC7_MAP;												
			available = decodeLength;
			if( carry>0 ) {
				take = 8 - carry;
				mask = pBalanceMask[take];
				shift = decodeLength - take;
				carryByte |= ((byte & mask) >> shift);
				appendBinaryStringInfo(&bytedata, (const char*)&carryByte, 1);
				balance = 8;
				carryByte = 0B00000000;
				available -= take;
			}
			take = balance > available ? available : balance;
			mask = PGCODEC7_CARRYMASK[take];
			shift = 8 - take;
			carry = take;
			carryByte |= ((byte & mask) << shift);
			balance -= take;		
		}
	}
	
	byteout=(bytea*)palloc(bytedata.len+VARHDRSZ);
	SET_VARSIZE(byteout, bytedata.len+VARHDRSZ);
	memcpy(VARDATA(byteout), bytedata.data, bytedata.len);
	pfree(bytedata.data);	
	PG_RETURN_BYTEA_P(byteout);
}