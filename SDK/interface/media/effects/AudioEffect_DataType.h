#ifndef _AUDIOEFFECT_DATATYPE_
#define _AUDIOEFFECT_DATATYPE_

enum PCMDataType {
    DATA_INT_16BIT = 0,
    DATA_INT_32BIT,
    DATA_FLOAT_32BIT
};

typedef struct _af_DataType_ {
    unsigned char IndataBit; // enum PCMDataType
    unsigned char OutdataBit; // enum PCMDataType
    char IndataInc;
    char OutdataInc;
} af_DataType;


enum {
    af_DATABIT_NOTSUPPORT = 0x404
};

#endif
