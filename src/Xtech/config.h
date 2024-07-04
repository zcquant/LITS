#ifndef __CONFIG__
#define __CONFIG__

#include "mode_internal.h"

#if MODE == 1
#define FILE_PLH "../data/stocks/plhx.txt"
#define FILE_DS "../data/stocks/filenamex.txt"
#define FILE_IDX "../data/stocks/FilenameIdx.txt"
#elif MODE == 2
#define FILE_PLH "../data/stocks/plhx.txt"
#elif MODE == 3
#define FILE_PLH "../data/stocks/PlhGenus.txt"
#define FILE_DS "../data/stocks/FilenameGenus.txt"
#define FILE_IDX "../data/stocks/FilenameIdx.txt"
#endif

#define FREQ_TICK 3000
#define FREQ_ATOMIC_CALC_VW 100
#define FREQ_OQ_CHECK_ALIVE 15000
#define FREQ_GENUS_DATA 500

#define MAX_FIELD_ALST 8
#define MAX_FIELD_QUOTA 10
#define MAX_FIELD_MACD 5

#define MAX_SYMBOLS 8192

#define MAX_LENGTH_PLH (MAX_FIELD_ALST * MAX_SYMBOLS)

#define MAX_QUEUE_SIZE_STRUCTED 64
#define MAX_QUEUE_SIZE_UNSTRUCTED 1024
#define MAX_QUEUE_SIZE_MACDS 16

#define RATE_LOB_CUT_VW 0.05
#define FREQ_VW_ATOMIC 0.1

#define GENUS_DATA_ROWS 28441

#define NOW_FS 360
#define NOW_XS 353
#define NOW_IS 20

#endif