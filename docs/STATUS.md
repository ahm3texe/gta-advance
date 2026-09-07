# Güncel proje durumu

Bu dosya elle düzenlenmez. `make status-update` ile `data/*.csv`, sınır
baseline'ı ve toolchain kilidinden üretilir. Canlı terminal özeti: `make status`.

## Ölçümler

| Ölçüm | Değer |
|---|---:|
| Fonksiyon haritası | 1935 fonksiyon / 454258 bayt |
| İnsan incelemesi (`documented+`) | 560 / 1935 |
| Byte-matching | 511 fonksiyon / 48590 bayt (%10.70) |
| C kaynağı | 526 toplam / 498 matching |
| Kaynaktan doğrulanan ROM | 30116 bayt |
| libc doğrulaması | 448 bayt |
| Toplam doğrulanmış ROM alanı | 30564 bayt |
| Açık sınır borcu | 0 kısa sınır + 0 ARM incelemesi + 0 aşırı büyüme |

## Şu anki tek aktif iş

Aktif iş yok.

## Açık iş kuyruğu

| ID | Öncelik | Durum | İş | Bitti sayılma koşulu |
|---|---|---|---|---|
| BACKUP-001 | P1 | blocked | Özel uzak yedek oluştur | Bütün commit geçmişi kullanıcının seçtiği özel remote'a gönderildi |
| TOOL-010 | P1 | todo | Yeni harita girisi icin yapisal kapi yaz | Bir aday ancak (a) push prologuyla basliyorsa VE (b) oncesinde fonksiyon bitiren komut (pop{..,pc} / bx lr / kosulsuz b) varsa haritaya eklenebilir; bu sinama split_at_calls'in urettigi 57 sahte girisin 57 sinide yakaladi |
| MAP-010 | P2 | todo | Bosluk analizinin YALNIZ sifir-riskli 16 ARM girisini uygula | ARM bolgesi bagimsiz olculdu (cond!=0xF orani tam 1.0000); bu 16 giris TOOL-010 kapisindan gecirilerek eklenir. 123 prologsuz Thumb yapragi BILEREK DISARIDA birakilir |
| ARM-001 | P1 | todo | ARM bolgesindeki 18 fonksiyonu C ile eslestir | ARM kipi derleme zinciri calisir durumda; her aday ROM ile olculur, eslesirse bolge kaydedilir |
| MATCH-019 | P1 | todo | SIO sürücüsünün kalan komut farklarını çöz | FUN_080657d8 doğal C ile make c-match kapısından geçer; RX adres ilişkisi ve pencere/çıkış blokları güncel ROM diff ile ayrı ayrı incelenir |

## Araç zinciri kilidi

- Uyumlu pret/agbcc revizyonu: `da598c1d918402c42c0c0d7128ba14567f3175e9`
- Sabit temsil C-corpus parmak izi: `9cd640a2a570228f958ec9cdd5574c4d267778f68938484158d2b4515bf76b3f`
- ROM çıktısı hibrit bütünleştirme sınamasıdır; bilinmeyen baytlar baserom'dan kopyalanır.
