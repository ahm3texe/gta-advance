# Proje çalışma sistemi

Bu belge projenin süreç sözleşmesidir. Güncel rakamlar [STATUS.md](STATUS.md),
aktif ve sıradaki işler `data/work_queue.csv`, fonksiyon çalışma tekniği
[WORKFLOW.md](WORKFLOW.md), derleyici davranışı [COMPILER.md](COMPILER.md)
içindedir. Aynı bilgi ikinci bir yerde elle tutulmaz.

## 1. Bilgi sahipliği

| Bilgi | Tek doğruluk kaynağı | Türemiş görünüm |
|---|---|---|
| Fonksiyon adresi, boyutu, durum ve modül | `data/functions.csv` | Dashboard, `make status` |
| İnsan tarafından verilen ad/durum | `data/function_overrides.csv` | `functions.csv` |
| C kaynağı ve eşleşme sonucu | `src/**/*.c` + `data/c_sources.csv` | Dashboard |
| Doğrulanmış kaynak bölgeleri | `data/matching_regions.csv` | Hibrit ROM, dashboard |
| RAM/ROM veri sembolleri | `data/ram_map.csv` | Link betikleri |
| Açık teknik işler | `data/work_queue.csv` | `docs/STATUS.md`, dashboard |
| Kabul edilmiş sınır borcu | `data/boundary_baseline.json` | `make boundary-check` |
| Reddedilmiş sahte girişler | `data/non_function_entries.csv` | `make consistency` |
| ARM overlay sınırları | `data/arm_boundary_review.csv` | `make consistency` |
| Araç zinciri kimliği | `config/toolchain.lock.json` | `make toolchain-check` |
| Güncel proje özeti | Yukarıdaki veriler | Üretilmiş `docs/STATUS.md` |

`README.md`, `PLAN.md` ve `WORKLOG.md` güncel sayaç kaynağı değildir. README
giriş noktası, PLAN karar/yol haritası, WORKLOG tarihsel kayıttır.

## 2. Her çalışma oturumunun protokolü

Başlangıç:

1. `git status --short` ile başkasının değişikliğini ayır.
2. `make status` ile gerçek durumu ve tek aktif işi gör.
3. `data/work_queue.csv` içinde en fazla bir işi `in_progress` tut.
4. O işin kabul ölçütü bu oturumun sınırıdır; yan işler yeni kayıt olur.

Bitiş:

1. İşin kanıtını üret; yalnızca yorum veya ajan raporu kanıt değildir.
2. Kuyruk durumunu ve `evidence` alanını güncelle.
3. `make status-update` çalıştır.
4. Günlük değişiklikte `make check`, kilometre taşında `make check-full` çalıştır.
5. Kontroller geçmeden `done`, `matching` veya “tamamlandı” denmez.

## 3. Doğrulama kademeleri

### `make check` — her commit

- Kurulu toolchain artifact kimliğini kontrol eder.
- Bütün kayıtlı matching bölgeleri ROM'a karşı doğrular.
- Hibrit ROM içinde kaynak bölgelerinin doğru konuma oturduğunu denetler.
- CSV/kaynak tutarlılığını ve iş kuyruğu şemasını kontrol eder.
- Sınır denetimini baseline'a karşı çalıştırır; yeni veya değişen borcu reddeder.
- Bütün C kaynaklarını tarar; tek derleme hatasında başarısız olur ve eski
  `c_sources.csv` dosyasını korur.
- Yasaklı inline assembly/register sabitlemelerini denetler.
- Dashboard verisini üretir ve `docs/STATUS.md` güncelliğini kontrol eder.

### `make check-full` — kilometre taşı ve birleştirme

`make check` içindeki her şeyi build cache kullanmadan yeniden üretir. Ayrıca
23 fonksiyonluk sabit temsil corpus'unun parmak izini, dashboard ürün lint'ini ve production
build'i doğrular.

### Borç baseline'ı kuralı

Baseline bir “sorun yok” belgesi değildir. Kısa sınır listesi 2026-09-04'te
53 kaydın ayrı ayrı incelenmesiyle sıfıra indirildi; yeni bulgu, değişen
erişilebilir boyut veya kapanmış bulgunun geri gelmesi `make check`i kırar.
İnceleme kararları `data/boundary_review.csv` içinde saklanır.

`make boundary-baseline` yalnızca bütün farklar tek tek incelendikten sonra
çalıştırılır. Sayıyı susturmak için baseline yenilenmez.

## 4. İş kuyruğu

Her işin kalıcı ID, P0–P3 önceliği, durum, teslimat, ölçülebilir kabul ölçütü
ve tamamlandıysa kanıtı olmak zorundadır. Geçerli durumlar `todo`,
`in_progress`, `blocked` ve `done`dur.

Aynı anda yalnızca bir iş `in_progress` olabilir. Bir çalışma sırasında yeni
bir sorun bulunursa mevcut hedef genişletilmez; kuyruğa yeni kayıt eklenir.

- **P0:** doğruluğu veya yeniden üretilebilirliği koruyan kapı
- **P1:** sıradaki teknik ilerlemeyi engelleyen iş
- **P2:** ölçek büyüyünce maliyeti artacak mimari/kalite borcu
- **P3:** kozmetik veya isteğe bağlı iyileştirme

## 5. Durumlar ve kanıt

| Durum | Asgari kanıt |
|---|---|
| `candidate` | Otomatik harita kaydı; doğruluk iddiası yok |
| `discovered` | Giriş adresi için BL hedefi, işaretçi, prolog veya bölme kanıtı; gövde incelenmiş sayılmaz |
| `documented` | Disassembly okunmuş, davranış/çağrılar yazılmış, isim kanıtlı |
| `decompiled` | Okunabilir doğal C var; henüz byte eşleşmiyor |
| `matching` | Kayıtlı kaynak bölgesi temiz build'de ROM ile birebir |

“İncelenmiş” metriği yalnızca `documented + decompiled + matching` toplamıdır.
Fonksiyon sayısı yardımcı metriktir; ana ilerleme metriği matching kod baytıdır.

Kaynak türü durumdan bağımsızdır: `c`, `asm` veya `none`. Eşleşmeyen C yine
C'dir; kaynaksız aday assembly sayılmaz.

## 6. Dürüstlük kuralları

- ROM içinde özgün C adları ve yorumları yoktur; kanıtsız isim uydurulmaz.
- Kısmi veya davranışsal eşdeğerlik byte-matching diye sunulmaz.
- Hibrit ROM, tam ROM'un kaynaktan üretildiği anlamına gelmez: bilinmeyen
  baytlar `baserom.gba`dan kopyalanır.
- Bir iddia sayı, hash, disassembly veya temiz build ile desteklenir.
- Denenip tutmayan yollar kaynak başında veya WORKLOG'da kaydedilir.
- Bayat yorum ve sayaç aynı değişiklikte düzeltilir.
- `register ... asm(...)` ve inline assembly ile C eşleşmesi zorlanmaz.
- Bir fonksiyon direniyorsa matching komşuları bekletmez; ayrı translation
  unit'e alınır.

## 7. Sınıflandırma stratejisi

1.988 fonksiyon önceden topluca isimlendirilmez. Ana döngü, input, UI, entity,
dünya/collision, rasterizer, görev/script, ses ve save kümeleri çağrı grafiğiyle
açıldıkça sınıflandırılır. Kanıt yoksa `FUN_...` adı korunur.

Öncelik sırası bitişik yaprak blokları, çağrıları bilinen kümeler, kesintisiz
doğrulanmış alanı büyüten bloklar ve bir alt sistemi açan büyük fonksiyonlardır.

## 8. Çoklu ajan ve yayın

- Her ajan aktif iş ID'sini bilir; `data/*.csv` aynı anda tek yerden yazılır.
- Ortak `build/` dizini silinmez; bölge/baseline kararı ana süreçte kalır.
- Ajan raporu doğrulama yerine geçmez; ana süreç ROM'a karşı yeniden ölçer.
- ROM, save, Ghidra projesi, decompiler dökümleri ve üretilmiş ROM Git'e girmez.
- Uzak depo varsayılan olarak özeldir; kullanıcı hedef seçmeden dışarı gönderim yapılmaz.
