# Sozluk

Bu projede tekrar eden terimlerin karsiligi.

## Surec terimleri

**Park / park etmek**
Cozulemeyen bir fonksiyonu silmeden, elimizdeki en iyi C kaynagi + kac bayt
tuttugu + denenip ELENEN yollar ve eleme sebepleri dosya basina yorum olarak
yazilarak birakmak. "Rafa kaldirdim ama notlariyla." Amaci ayni cikmaz yolun
aylar sonra bastan denenmemesi. Kural: takilinca bir odakli deneme, sonra park.

**Aday**
Uzerinde calisilmak icin secilen, henuz eslesmemis fonksiyon.

**Hasat**
Sirayla aday secip eslestirmeye calisma turu. "Hasat turu" = bir oturumda
birkac fonksiyon deneme.

**Bant**
Fonksiyonlari boyutuna gore gruplama. "60-127 bant" = 60 ile 127 bayt
arasindaki fonksiyonlar. Kucuk bantlar kolay ve hizli, buyukler zor.

## Eslestirme terimleri

**Bayt eslesmesi (byte-matching)**
Yazdigimiz C kodunun derlendiginde orijinal ROM'daki baytlarin BIREBIR
AYNISINI uretmesi. Benzer degil, ayni. Projenin olcutu bu.

**"farkli: 97/120"**
Bizim derledigimiz 120 bayt cikti, bunun 97 bayti ROM'dakinden farkli.
0 olursa eslesme tamam.

**baserom.gba**
Senin kendi ROM kopyan. Depoya girmiyor, sadece yerelde karsilastirma
icin kullaniliyor.

**Hibrit ROM**
`make rom` ciktisi: dogruladigimiz bolgeler BIZIM kaynagimizdan derleniyor,
geri kalani baserom'dan kopyalaniyor. SHA-1'i orijinalle ayni cikmasi,
bizim urettigimiz baytlarin dogru oldugunu kanitliyor.

**SHA-1**
Dosyanin parmak izi. Tek bayt degisse tamamen baska cikar. Bizimki
06230842626da504f92396074f7c655e100f5d44 olmali.

## Derleyici / assembly terimleri

**agbcc**
Oyunun 2004'te derlendigi eski derleyici. Modern derleyici ayni baytlari
uretmiyor, o yuzden aynisini kullanmak zorundayiz.

**Yazmac (register)**
Islemcinin icindeki cok hizli, cok az sayidaki (Thumb'da pratikte 8) depo.
r0-r7 gibi.

**Dagitim (register allocation)**
Derleyicinin hangi degiskeni hangi yazmaca koyacagina karar vermesi.
Eslesmeme sebeplerimizin cogu burada: kod mantiksal olarak dogru ama
derleyici yazmaclari baska sirayla secmis.

**Spill (tasma)**
Yazmac yetmeyince bir degeri gecici olarak yigina (RAM'e) yazmak.
Yavas; derleyici mecbur kalmadikca yapmaz.

**Havuz (literal pool)**
Fonksiyonun sonuna derleyicinin koydugu sabit degerler tablosu. Buyuk
sabitler komutun icine sigmadigi icin oradan okunuyor. Bunlar KOD DEGIL,
VERI -- disassembler'a komut diye okutursan sacmalar (bu tuzaga dustuk).

**Prolog / epilog**
Fonksiyonun basindaki (`push`) ve sonundaki (`pop`) standart kaliplar.

**Dongu degismezi (loop invariant)**
Dongu icinde her turda ayni kalan deger. Iyi derleyici bunu dongu ONCESINE
tasiyip bir yazmacta tutar. Bizim eslesmeme sebeplerimizden biri.

**MMIO**
Donanimi kontrol eden ozel bellek adresleri. Ornegin 0x04000208'e yazmak
kesmeleri aciyor/kapatiyor; normal bir degisken degil.

## Arac terimleri

**CFG (kontrol akis grafigi)**
Fonksiyonun "blok" haritasi: nereden nereye dallaniyor. `tools/dump_cfg.py`
cikariyor. Kodu yazmadan once seklini gormemizi sagliyor.

**Blok**
Dallanmasiz duz komut dizisi. Ilk dallanmada biter.

**Birlesme noktasi (join point)**
Birden fazla yerden gelinen blok. `if/else` sonrasi ya da dongu basi.
Derleyicinin yazmac kararlarini en cok etkileyen yer.

**Extern**
"Bu isim baska bir dosyada tanimli" bildirimi. Yanlis yazilirsa ya da
yeniden adlandirmada guncellenmezse baglanti kirilir.

**Tutarlilik denetleyicisi**
`tools/check_consistency.py`. Uydurma isim, celisen tip, kirik extern gibi
hatalari yakaliyor. Bu oturumda 3 gercek hatami yakaladi.

**check-full**
Tam dogrulama: tum kaynaklari derle, ROM'u kur, SHA-1'i karsilastir,
tutarliligi denetle. Cikis kodu 0 = her sey temiz.
