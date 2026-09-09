#ifndef RAM_SYMBOLS_H
#define RAM_SYMBOLS_H

#include "gba_types.h"

/*
 * RAM storage whose types have not yet been fully established and which is
 * accessed through different views by different subsystems. Declare only raw
 * storage here; each translation unit explicitly casts it to its local struct
 * view. This avoids conflicting extern C declarations for one linker symbol.
 */
extern u8 gRam02000F10[];
extern u8 gRam02001450[];
extern u8 gRam02001440[];
extern u8 gRam020303C4[];
extern u8 gRam02001140[];
extern u8 gRam02025810[];

#endif /* RAM_SYMBOLS_H */
