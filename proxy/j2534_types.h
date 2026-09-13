#ifndef J2534_TYPES_H
#define J2534_TYPES_H

typedef struct {
    unsigned long ProtocolID;
    unsigned long RxStatus;
    unsigned long TxFlags;
    unsigned long Timestamp;
    unsigned long DataSize;
    unsigned long ExtraDataIndex;
    unsigned char  Data[4128];
} PASSTHRU_MSG;

typedef struct {
    unsigned long Parameter;   // parameter ID being set/read
    unsigned long Value;        // value for that parameter
} SCONFIG;

typedef struct {
    unsigned long NumOfParams; // count of SCONFIG entries
    SCONFIG*       ConfigPtr;  // pointer to array of entries
} SCONFIG_LIST;

// The spec defines input/output variants as the same shape; J2534 APIs
// declare SCONFIG_INPUT/SCONFIG_OUTPUT as void* that you cast to SCONFIG_LIST.

#endif