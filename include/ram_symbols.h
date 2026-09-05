#ifndef RAM_SYMBOLS_H
#define RAM_SYMBOLS_H

#include "gba_types.h"

/*
 * Türü henüz kesinleşmemiş, farklı sistemlerce farklı görünümlerle kullanılan
 * RAM depoları. Semboller burada yalnız ham depolama olarak bildirilir; her
 * translation unit kendi yerel struct görünümüne açıkça cast eder. Böylece
 * linker'daki tek nesne için çelişkili extern C türleri oluşmaz.
 */
extern u8 gRam02000F10[];
extern u8 gRam02001450[];
extern u8 gRam02001440[];
extern u8 gRam020303C4[];
extern u8 gRam02001140[];
extern u8 gRam02025810[];

#endif /* RAM_SYMBOLS_H */
