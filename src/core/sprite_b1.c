/* FUN_08012cd8 @ 0x08012CD8, 320 bayt -- RLE (run-length) cozucu.
 *
 * TESHIS -- DOSYA ADI YANILTICI: "sprite_b1" kardes atamasindan geldi, govde
 * sprite ile ILGISIZ. src[0] & 0xF0 == 0x30 kontrolu GBA sikistirma
 * basligidir (0x10 LZ77, 0x20 Huffman, 0x30 RL). Cozulmus boyut src[1..3]'ten
 * 24 bit little-endian okunur, veri src[4]'ten baslar.
 *
 * Cikis 16 bit hizali yazilir -- hedef VRAM oldugundan bayt yazisi yasak.
 * Bu yuzden tek adrese denk gelen bayt bir sonraki halfword yazmasina
 * "carry" olarak tasinir; kosunun sonunda artan tek bayt da bir sonraki
 * kosuya carry olur. carry u8'dir: her atamada lsls#24/lsrs#24 cikar.
 *
 * Bayrak baytinin bit7'si kosuyu secer:
 *   set   -> uzunluk (bayrak & 0x7F) + 3, tek veri bayti tekrarlanir
 *   clear -> uzunluk (bayrak & 0x7F) + 1, o kadar bayt aynen kopyalanir
 * Kopyalama kolunda KAYNAK adresinin tekligi ayrica dallanir: cift ise
 * dogrudan halfword okunur, tek ise iki bayttan elle birlestirilir. ROM
 * ni/nj hesabini iki kolun ICINDE de tekrarliyor -- oyle birakildi.
 *
 * DURUM: 320/320 bayt boyut TUTUYOR, 290/320 bayt AYNI. Komut dizisi
 * (govde, dallar, dongu bicimleri, operand sirasi) ROM ile BIREBIR.
 * Kalan 30 bayt TEK BIR YAZMAC TAKASI: ROM `src`'i yigina koyup sl'yi
 * kopyalama kolunun `tail` gecicisine verir; agbcc bizde tersini yapar
 * (src -> sl, tail -> sp+8). Ikisi de ayni komut sayisi, ayni boyut.
 *
 * OLCULEN NEDEN (agbcc -dg dokumu, global.c onceligi
 * = floor_log2(refs) * refs / live_length):
 *     src  : refs 11, live_length 122  -> 0.2705   (yazmac alir)
 *     tail : refs  6, live_length  46  -> 0.2609   (yigina duser)
 * Sira %3.7 farkla src lehine. Cevirmek icin src >= 127 live_length
 * ya da refs <= 10 ya da tail <= 45 live_length gerekiyor.
 *
 * DENENENLER / ELENENLER -- TEKRAR DENEME, yeni deneyeni buraya EKLE:
 *  1. ni/nj/tail/n/s/d/b kapsam (dal ici / dongu govdesi / fonksiyon)
 *     512 kombinasyon tarandi. TEK kazanan: `ni` dongu govdesine tasinir
 *     (67 -> 30 bayt). Digerlerinin hepsi >= 30.
 *  2. Yerel bildirim SIRASI (6! permutasyon) -- agbcc'yi hic etkilemiyor.
 *  3. `nj` dongu/fonksiyon kapsamina -- 308 bayt, boyut bozuluyor.
 *  4. `ni` ya da `nj`'yi atip `i += len` / `j++` yazmak -- 304..306 bayt.
 *  5. ni/nj sirasini cevirmek (compressed kolda) -- 322 bayt.
 *  6. `carry = tail` yerine `carry = (n != 0) ? ... : carry` ucluk
 *     islec -- phi yapisini kaybediyor, 296 bayt.
 *  7. `carry` u32 + `& 0xFF` -- 78 bayt fark (lsls/lsrs yerine ands).
 *  8. `len`i dallarin icinde hesaplamak -- 330 bayt (agbcc hoisting
 *     yapmiyor, iki kolda da ayri `ands` cikiyor).
 *  9. `u8 *volatile src` ile src'i yigina zorlamak -- 288 bayt, her
 *     ifade icinde yeniden yukluyor.
 * 10. Adres hesabini bolmek (`s = src; s += j;`) reload oncesi RTL'ye
 *     komut EKLIYOR, cikti degismiyor: src live_length 122 -> 124.
 *     Gerekli 127'ye ulasmiyor; compressed koldaki bolme +0 katkili
 *     (src'in son kullanimindan SONRA kaliyor).
 * 11. Baslik okumalarini bolmek (`t = src[1]; size |= t;`) -- combine
 *     geri birlestiriyor, live_length degismiyor.
 * 12. `tail`i odd-adres yazmasinda kullanip referans eklemek -- kopya
 *     yayilimi carry'ye geri katliyor, sayilar degismiyor.
 * 13. Bayt kopyasi ile allocno bolme (`b2 = b;`) -- CLAUDE.md kural:
 *     her zaman eleniyor; burada da elendi (live_length 122'de kaldi).
 * 14. decomp-permuter: 28. yinelemeden sonra her mutasyon derleme
 *     hatasi veriyor, bu govdede kullanilamadi.
 *
 * SONRAKI ADIM ICIN NOT: kalan fark KAYNAK YAPISINDA degil, tek bir
 * oncelik esiginde. Aranacak sey, cikti komutlarini degistirmeden
 * kopyalama kolunun ON EKINE (`s = src` satirindan ONCE) 3 komut daha
 * ekleyen ya da `tail = carry` ile `carry = tail` ARASINDAN 1 komut
 * cikaran bir yeniden yazim.
 *
 * Dogrulama: make c-match FILE=src/core/sprite_b1.c
 */

#include "gba_types.h"

/* 0x08012CD8 */
void FUN_08012cd8(u8 *src, u8 *dst)
{
    s32 size;
    s32 i;
    s32 j;
    u8 carry;
    u32 flag;
    s32 len;

    /* Yalnizca RL basligi (0x3n) kabul edilir. */
    if ((src[0] & 0xF0) != 0x30)
        return;

    size = src[2] << 8;
    size |= src[1];
    size |= src[3] << 16;

    carry = 0;
    j = 4;
    i = 0;
    while (i < size) {
        s32 ni;
        flag = src[j];
        len = flag & 0x7F;
        if ((flag & 0x80) != 0) {
            u8 *s;
            u8 *d;
            u32 b;
            s32 n;
            u32 tail;
            s32 nj;

            /* Tekrar kosusu: tek bayt (len+3) kez yazilir. */
            len += 3;
            j++;
            s = &src[j];
            d = &dst[i];
            b = *s;
            n = len;
            tail = carry;
            if (((u32)d & 1) != 0) {
                /* Tek adres: onceki bayti da yanina alip halfword yaz. */
                *(u16 *)(d - 1) = carry | (b << 8);
                d++;
                n--;
            }
            nj = j + 1;
            ni = i + len;
            while (n > 1) {
                *(u16 *)d = (b << 8) | b;
                d += 2;
                n -= 2;
            }
            if (n != 0)
                tail = b;
            carry = tail;
            j = nj;
            i = ni;
        } else {
            u8 *s;
            u8 *d;
            s32 n;
            u32 tail;
            s32 nj;

            /* Duz kopya kosusu: (len+1) bayt aynen aktarilir. */
            len += 1;
            j++;
            d = &dst[i];
            s = &src[j];
            n = len;
            tail = carry;
            if (((u32)d & 1) != 0) {
                *(u16 *)(d - 1) = carry | (*s << 8);
                d++;
                n--;
                s++;
            }
            if (((u32)s & 1) != 0) {
                /* Kaynak tek adreste: halfword okunamaz, elle birlestir. */
                ni = i + len;
                nj = j + len;
                while (n > 1) {
                    *(u16 *)d = (s[1] << 8) | s[0];
                    d += 2;
                    n -= 2;
                    s += 2;
                }
            } else {
                ni = i + len;
                nj = j + len;
                while (n > 1) {
                    *(u16 *)d = *(u16 *)s;
                    d += 2;
                    n -= 2;
                    s += 2;
                }
            }
            if (n != 0)
                tail = *s;
            carry = tail;
            j = nj;
            i = ni;
        }
    }
}
