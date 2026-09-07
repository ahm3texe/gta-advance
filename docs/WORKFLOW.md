# Fonksiyon çalışma tekniği

Projenin bağlayıcı süreç sözleşmesi [PROJECT_SYSTEM.md](PROJECT_SYSTEM.md),
güncel durum [STATUS.md](STATUS.md), derleyici davranışı
[COMPILER.md](COMPILER.md) içindedir. Bu belge tek fonksiyon/blok üzerindeki
tersine mühendislik tekniğini açıklar. Çelişkide PROJECT_SYSTEM geçerlidir.

## 1. Doğrulama komutları

Commit öncesi tek komut:

```sh
make check
```

Bu günlük kapıdır: matching kaynakları, toolchain kimliğini, veri/kaynak
tutarlılığını, sınır baseline'ını, bütün C kaynaklarını, iş kuyruğunu ve
üretilmiş durum belgesini denetler. Kilometre taşı öncesinde build cache'i
yok sayan corpus ve dashboard doğrulaması için `make check-full` kullanılır.

**Kural:** `make check` geçmeden commit atılmaz. Hibrit ROM hash'i yalnızca
doğrulanmış kaynak bölgelerinin doğru konuma oturduğunu kanıtlar; tam ROM'un
kaynaktan üretildiğini kanıtlamaz.

## 2. Hedef seçimi

```sh
python3 tools/find_leaf_candidates.py --limit=20 --max-size=200
```

Yaprak (`bl` içermeyen) fonksiyonlar en ucuzudur. Ama **bir C dosyası bitişik
bir ROM bölgesi üretir** — bu yüzden hedef tek fonksiyon değil, *bitişik
adayların oluşturduğu blok*tur.

Öncelik sırası:

1. Bitişik yaprak blokları — en öngörülebilir
2. Çağırdığı her şey artık bilinen fonksiyonlar — ağaç yukarı açılır
3. Kesintisiz aralığı büyüten bloklar — kapsam yüzdesinden daha iyi bir
   sağlık göstergesidir
4. Büyük fonksiyonlar — bayt yüzdesini asıl hareket ettiren bunlar

### Aday seçimi ELLE YAPILMAZ, araçla yapılır

Hedef listesi üreten iki araç var; ikisi de salt okunurdur ve çıktısı
`--out` ile CSV'ye yazılabilir:

- `tools/find_neighbour_dense.py` — komşuluğu eşleşmiş, henüz C'si
  yazılmamış fonksiyonlar (bölgenin tip sözlüğü hazır demektir)
- `tools/find_twins.py` — yapısal olarak aynı fonksiyon çiftleri/kümeleri
  (bkz. §10)

**Bir sayı raporlanacaksa, onu üreten komut da yazılır.** Tek seferlik
python parçacığıyla seçim yapmak yasak: 2026-09-07'de plana "123 taze
komşu-yoğun aday" diye bir sayı girdi ve depoda onu yeniden üretecek
hiçbir şey yoktu. Sayılar ayrıca ANLIK FOTOĞRAFTIR — her yeni eşleşme
komşularının sayacını yukarı ittiği için aday sayısı *artabilir*. Bir
sayıyı doğrulamak için ölçüldüğü commit'te koşturun:

```
git archive <commit> data/functions.csv src | tar -x -C /tmp/at
cp tools/find_neighbour_dense.py /tmp/at/tools/ && cd /tmp/at
python3 tools/find_neighbour_dense.py
```

## 3. Fonksiyon döngüsü

1. `tools/disasm_function.py <ad>` ile ROM'u oku, davranışı **anla**
2. Temiz C yaz — Ghidra çıktısı kopyalanmaz
3. `make c-match FILE=...` ile ölç
4. Eşleşmiyorsa `make diff FILE=... FUNC=...` ile nerede saptığını gör
5. **Assembly'yi değil C'yi** değiştir; [COMPILER.md](COMPILER.md) kurallarına bak
6. Eşleşince bölgeyi kaydet, `make check` çalıştır, commit at

## 4. Dosya ve sembol düzeni

| Ne | Nereye |
|---|---|
| Tipler (`u8`, `s16`, `vu32`) | `include/gba_types.h` |
| Donanım yazmaçları, `DmaChannel`, bellek tabanları | `include/gba_io.h` |
| RAM/ROM veri sembolleri | `data/ram_map.csv` |
| Fonksiyon adı, durum, modül | `data/function_overrides.csv` |
| Doğrulanmış kaynak bölgeleri | `data/matching_regions.csv` (araçla) |
| libc bölgeleri | `data/libc_regions.csv` |

**Kaynak dosyalarında `typedef` veya `#define REG_...` tanımlanmaz.** Yeni bir
yazmaç gerekiyorsa `gba_io.h`'ye eklenir.

`data/*.csv` dosyaları elle değil araçlarla değiştirilir:
`add_c_region.py`, `retire_asm.py`, `audit_boundaries.py` ve
`discover_functions.py`. Eski `split_at_calls.py` doğrusal taraması 52 sahte
sınır ürettiği için yazma kipinde kalıcı olarak devre dışıdır.

**Birincil kaynak `data/functions.csv`'nin kendisidir.**
`sync_function_map.py` haritayı bayat Ghidra dökümünden *yeniden kurar* —
bu oturumda kazara çalıştı ve 479 kaydı, 148 adı, 133 `matching` durumunu
sildi. Artık kayıp kapısı var ve reddediyor; yine de yalnızca sıfırdan
yeniden kurmak istendiğinde kullanılır.

### Yeniden adlandırma

Bir fonksiyonu `functions.csv`'de yeniden adlandırmak, o adı `extern` ile
kullanan **her kaynağı kırar**. Bu projede yedi kez oldu ve her seferinde
ancak `make rom` zincirinin sonunda fark edildi.

**Kural:** yeniden adlandırdıktan sonra `make consistency` çalıştır — hangi
dosyanın kırıldığını anında söyler. Referansları güncellemeden commit atma.

## 5. Dürüstlük kuralları

Bunlar üslup değil, doğruluk meselesi. Her biri bu projede en az bir kez
yanlış yola sapmamıza yol açtı.

- **İsim uydurma.** Bir fonksiyonun ne yaptığını kanıtlayamıyorsan `FUN_...`
  adında bırak. Assembly kaynaklarındaki `.equ` etiketleri önceki çalışmanın
  *tahminleriydi* ve birçoğu yanlış çıktı.
- **Belirsizliği belirsiz işaretle.** İki sembolün gövdesi aynıysa hangisinin
  nerede olduğunu uydurma; `discovered` yaz ve alternatifi nota geç.
- **Provisional olan provisional kalır.** `ram_map.csv`'de doğrulanmamış her
  şey `provisional` statüsündedir.
- **Eşleşmeyeni "eşleşti" sayma.** Kısmi sonuç değerlidir; uydurma değildir.
- **Denenip tutmayanları yaz.** Kaynak dosyanın başına. Aynı yolu iki kez
  yürümek pahalıdır.
- **Bayat notu düzelt.** Bir yorum "çözülemedi" diyorsa ve artık çözüldüyse,
  o yorum yanlış bilgidir.

## 6. Register sabitleme yasak

`register T *p asm("r4")` gibi acik register baglamalari, inline assembly ile
ayni kategoridedir: byte'lari tutturur ama **neden** tuttugunu gizler.

Bu bir cekic: her register uyusmazligi boyle "cozulebilir". Kabul edilirse
kural setinin (docs/COMPILER.md) kesfi anlamsizlasir ve proje byte-matching
tiyatrosuna doner. Ozgun 2004 kaynaginin register sabitledigine dair hicbir
kanit da yok.

Bir fonksiyon ancak DOGAL C ile eslesirse eslesmis sayilir.
`tools/review_c_source.py` bunu yakalar ve `make check` basarisiz olur.

Ayni sey inline assembly icin de gecerli.

## 7. Yarım işi ayır

Bir blokta bir fonksiyon direniyorsa, eşleşen kısmı ayrı dosyaya alıp bölge
olarak kaydet. Yarım iş tamamı bekletmez. Direnen fonksiyon kendi dosyasında,
denenenler yorumda.

## 8. Negatif sonuçlar da kayıttır

Bir hipotez tükendiğinde belgeye yazılır. Ama **tek başına etkisiz çıkan bir
değişiklik, başkasıyla birleştiğinde belirleyici olabilir** — bu projede tam
olarak böyle oldu. "Denendi, tutmadı" kaydını mutlak kabul etme.

## 9. Paralel çalışma

Birden fazla ajan çalışıyorsa:

- Ortak dizinlere (`build/`) dokunulmaz; `rm -rf build` çalıştırılmaz
- `data/*.csv` tek bir yerden yazılır
- Her ajan yalnızca kendi kaynak dosyasına yazar
- Emeklilik ve bölge kaydı kararı ana süreçte kalır
- Ajan raporu doğrulama yerine geçmez; sonuç ROM'a karşı yeniden ölçülür

## 10. Önce kardeşin ROM gövdesiyle diff'le

Bir fonksiyon eşleşmiyorsa ve ROM'da **boyutu yakın bir kardeşi zaten
eşleşiyorsa**, yazmaç dağıtımı kovalamadan önce iki ROM gövdesini
birbiriyle karşılaştır:

```
python3 tools/disasm_function.py 0x08031844 | sed -E 's/^ *[0-9a-f]+:\t[0-9a-f ]+\t//' > a
python3 tools/disasm_function.py 0x08031A1C | sed -E 's/^ *[0-9a-f]+:\t[0-9a-f ]+\t//' > b
diff a b
```

Ölçülen örnek: `0x08031844` (472 bayt) bir ajanın 341 bin jetonunu yedi ve
226/235 komutta takıldı; rapor "üç yazmaçlı döngüsel yer değiştirme, kaynak
düzeyinde kaldıraç yok" diyordu. Kardeşi `0x08031A1C` zaten eşleşiyordu.
İki gövdenin diff'i **235 komutun 235'inin aynı** olduğunu, farkın yalnızca
dal hedefleri ve sutun testinin kutbu (`blt` ↔ `bge`) olduğunu gösterdi.
Kaynakta karşılığı tek bir karakterdi: `if (col++ >= 0)` → `if (col++ < 0)`.
Eşleşen kardeşin kaynağını kopyalayıp o testi çevirmek ilk denemede tam
eşleşme verdi.

Aynı yöntem `0x080316B0`'de de ilk denemede tuttu (aile aynı, sütun kırpması
yok). Kural: **kardeş varsa diff ilk adımdır**, `dump_alloc.py` son adım.
