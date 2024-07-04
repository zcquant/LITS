#ifndef __MODE__
#define __MODE__

///////////////////////////////////////////////////////////////////////////////////////////////////

// #define MODE 1

// #if MODE == 1
// /**
//  * @brief Offline
//  *
//  */
// #define OFFLINE
// #define SAVE_FACTOR

// // #define MODE_TEN_FIELDS
// // #define USING_CALC_TRANSACTION

// // #define USING_VW
// // #define USING_DICE
// // #define USING_TWIN
// // #define USING_INDEX

// // #define SENDX_ORIGIN
// // #define SENDX_INDEX

// // #define USING_DICE_NAIVESIX

// #define USING_SUPERBIA
// #define SHIYU
// #define GENUS

// // #define BACKTEST_LIMITORDER

// #elif MODE == 2
// /**
//  * @brief Online
//  *
//  */
// #define ONLINE
// // #define USING_MSGTOOL_T2

// // #define MODE_FAKE_FILES
// // #define MODE_TEN_FIELDS
// // #define USING_OQ_CHECK_ALIVE

// // #define SENDX_ORIGIN
// // #define SENDX_INDEX

// #define USING_SUPERBIA
// #define SHIYU
// #define GENUS

// #elif MODE == 3
// /**
//  * @brief GENUS DATA MODE
//  *
//  */
// #define OFFLINE
// #define SAVE_FACTOR
// #define SAVE_LOBOFL

// #endif

///////////////////////////////////////////////////////////////////////////////////////////////////

#define OFFLINE
// #define ONLINETEST
// #define ONLINE

#if defined(OFFLINE) || defined(ONLINETEST)
#define IOFILES
#endif

#if defined(ONLINE) || defined(ONLINETEST)
#define INFILE_STATIC
#endif

#if defined(ONLINE)
#define USING_MSGTOOL_T2
#endif

#endif