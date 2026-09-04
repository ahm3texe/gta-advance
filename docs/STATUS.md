# Güncel proje durumu

Bu dosya elle düzenlenmez. `make status-update` ile `data/*.csv`, sınır
baseline'ı ve toolchain kilidinden üretilir. Canlı terminal özeti: `make status`.

## Ölçümler

| Ölçüm | Değer |
|---|---:|
| Fonksiyon haritası | 1931 fonksiyon / 454044 bayt |
| İnsan incelemesi (`documented+`) | 266 / 1931 |
| Byte-matching | 222 fonksiyon / 11992 bayt (%2.64) |
| C kaynağı | 214 toplam / 210 matching |
| Kaynaktan doğrulanan ROM | 12100 bayt |
| libc doğrulaması | 448 bayt |
| Toplam doğrulanmış ROM alanı | 12548 bayt |
| Açık sınır borcu | 0 kısa sınır + 0 ARM incelemesi + 0 aşırı büyüme |

## Şu anki tek aktif iş

**MATCH-014 — world/entity kucuk fonksiyon hasadi (kural 35-36 ile)**

## Açık iş kuyruğu

| ID | Öncelik | Durum | İş | Bitti sayılma koşulu |
|---|---|---|---|---|
| BACKUP-001 | P1 | blocked | Özel uzak yedek oluştur | Bütün commit geçmişi kullanıcının seçtiği özel remote'a gönderildi |
| TOOL-010 | P1 | todo | Yeni harita girisi icin yapisal kapi yaz | Bir aday ancak (a) push prologuyla basliyorsa VE (b) oncesinde fonksiyon bitiren komut (pop{..,pc} / bx lr / kosulsuz b) varsa haritaya eklenebilir; bu sinama split_at_calls'in urettigi 57 sahte girisin 57 sinide yakaladi |
| MAP-010 | P2 | todo | Bosluk analizinin YALNIZ sifir-riskli 16 ARM girisini uygula | ARM bolgesi bagimsiz olculdu (cond!=0xF orani tam 1.0000); bu 16 giris TOOL-010 kapisindan gecirilerek eklenir. 123 prologsuz Thumb yapragi BILEREK DISARIDA birakilir |
| MATCH-014 | P2 | in_progress | world/entity kucuk fonksiyon hasadi (kural 35-36 ile) | Idiom kutuphanesinin en guclu oldugu modullerde 5 kumelik gruplar halinde ilerlenir; her kume sonunda make check-full gecmeli |

## Araç zinciri kilidi

- Uyumlu pret/agbcc revizyonu: `da598c1d918402c42c0c0d7128ba14567f3175e9`
- Sabit temsil C-corpus parmak izi: `9cd640a2a570228f958ec9cdd5574c4d267778f68938484158d2b4515bf76b3f`
- ROM çıktısı hibrit bütünleştirme sınamasıdır; bilinmeyen baytlar baserom'dan kopyalanır.
