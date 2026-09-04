# Güncel proje durumu

Bu dosya elle düzenlenmez. `make status-update` ile `data/*.csv`, sınır
baseline'ı ve toolchain kilidinden üretilir. Canlı terminal özeti: `make status`.

## Ölçümler

| Ölçüm | Değer |
|---|---:|
| Fonksiyon haritası | 1931 fonksiyon / 442112 bayt |
| İnsan incelemesi (`documented+`) | 263 / 1931 |
| Byte-matching | 219 fonksiyon / 10614 bayt (%2.40) |
| C kaynağı | 211 toplam / 207 matching |
| Kaynaktan doğrulanan ROM | 12064 bayt |
| libc doğrulaması | 448 bayt |
| Toplam doğrulanmış ROM alanı | 12512 bayt |
| Açık sınır borcu | 0 kısa sınır + 0 ARM incelemesi + 0 aşırı büyüme |

## Şu anki tek aktif iş

Aktif iş yok.

## Açık iş kuyruğu

| ID | Öncelik | Durum | İş | Bitti sayılma koşulu |
|---|---|---|---|---|
| BACKUP-001 | P1 | blocked | Özel uzak yedek oluştur | Bütün commit geçmişi kullanıcının seçtiği özel remote'a gönderildi |

## Araç zinciri kilidi

- Uyumlu pret/agbcc revizyonu: `da598c1d918402c42c0c0d7128ba14567f3175e9`
- Sabit temsil C-corpus parmak izi: `9cd640a2a570228f958ec9cdd5574c4d267778f68938484158d2b4515bf76b3f`
- ROM çıktısı hibrit bütünleştirme sınamasıdır; bilinmeyen baytlar baserom'dan kopyalanır.
