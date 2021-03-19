#ifndef _HW_DEFS_H_
#define _HW_DEFS_H_
//---------------------------------------------------------------------------//
//                                  Defines                                  //
//---------------------------------------------------------------------------//
#define ERASABLE_SIZE 2048
#define FIXED_SIZE   65536

//---------------------------------------------------------------------------//
//                             Type Definitions                              //
//---------------------------------------------------------------------------//
typedef enum {
    RUPT_GOJAM,
    RUPT_T6RUPT,
    RUPT_T5RUPT,
    RUPT_T3RUPT,
    RUPT_T4RUPT,
    RUPT_KEYRUPT1,
    RUPT_KEYRUPT2,
    RUPT_UPRUPT,
    RUPT_DOWNRUPT,
    RUPT_RADARUPT,
    RUPT_RUPT10,
} rupt_t;

typedef enum {
    COUNTER_TIME2,
    COUNTER_TIME1,
    COUNTER_TIME3,
    COUNTER_TIME4,
    COUNTER_TIME5,
    COUNTER_TIME6,
    NUM_COUNTERS
} counter_t;

#endif//_HW_DEFS_H_
