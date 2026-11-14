#ifndef BEAM_FAKE_DOS_H
#define BEAM_FAKE_DOS_H
#include <exec/types.h>
#define OFFSET_BEGINNING -1
#define MODE_OLDFILE 1005
#define MODE_NEWFILE 1006
#define ACCESS_READ (-2)
#define DOSTRUE (-1)
struct DateStamp { LONG ds_Days,ds_Minute,ds_Tick; };
#endif
