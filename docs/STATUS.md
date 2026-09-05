# Güncel proje durumu

Bu dosya elle düzenlenmez. `make status-update` ile `data/*.csv`, sınır
baseline'ı ve toolchain kilidinden üretilir. Canlı terminal özeti: `make status`.

## Ölçümler

| Ölçüm | Değer |
|---|---:|
| Fonksiyon haritası | 1933 fonksiyon / 454068 bayt |
| İnsan incelemesi (`documented+`) | 409 / 1933 |
| Byte-matching | 365 fonksiyon / 23092 bayt (%5.09) |
| C kaynağı | 375 toplam / 353 matching |
| Kaynaktan doğrulanan ROM | 15064 bayt |
| libc doğrulaması | 448 bayt |
| Toplam doğrulanmış ROM alanı | 15512 bayt |
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

## Araç zinciri kilidi

- Uyumlu pret/agbcc revizyonu: `da598c1d918402c42c0c0d7128ba14567f3175e9`
- Sabit temsil C-corpus parmak izi: `9cd640a2a570228f958ec9cdd5574c4d267778f68938484158d2b4515bf76b3f`
- ROM çıktısı hibrit bütünleştirme sınamasıdır; bilinmeyen baytlar baserom'dan kopyalanır.
