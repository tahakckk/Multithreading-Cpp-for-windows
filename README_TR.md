# Çok İş Parçacıklı Dosya Sıkıştırma Aracı

Hem tek iş parçacıklı hem de çok iş parçacıklı işlemleri destekleyen yüksek performanslı bir dosya sıkıştırma ve açma aracı.

## Özellikler

- Çok iş parçacıklı sıkıştırma ve açma
- Karşılaştırma için tek iş parçacıklı mod
- İlerleme çubuğu gösterimi
- Dosya bütünlüğü doğrulama
- Yapılandırılabilir parça boyutu
- Otomatik çıktı dizini yönetimi
- Kapsamlı hata yönetimi
- Performans ölçümü

## Gereksinimler

- C++17 veya daha yeni
- Windows İşletim Sistemi (Windows'a özgü API'ler kullanır)
- G++ derleyici

## Derleme

```bash
g++ -std=c++17 main.cpp compressor.cpp -o compressor.exe
```
-----------

```bash
g++ -std=c++17 generate_files.cpp -o genarete_files.exe
```

## Kullanım

Kesin olarak bu kodu çalıştırmalısınız aksi takdirde mode'lar çalışmaz:
```bash
./generate_files.exe
```

### Temel Kullanım

```bash
./compressor.exe <compress|decompress> <single|multi> <input_file> <output_file> <chunk_size_in_bytes>
```

Örnek:
```bash
./compressor.exe compress multi input/bigfile.txt output/bigfile.compressed 1048576
```

### Test Modu

Tüm testleri çalıştırmak için:
```bash
./compressor.exe test
```

## Performans Sonuçları

### Küçük Dosya (1KB)
```
Test Sonuçları (smallfile.txt için):
┌─────────────────┬───────────────┬───────────────┬───────────────┐
│ Mod            │ Sıkıştırma    │ Açma         │ Karşılaştırma │
├─────────────────┼───────────────┼───────────────┼───────────────┤
│ Multi-thread   │    0.133404s │    0.153865s │ ✅ MATCH │
│ Single-thread  │    0.002848s │    0.004176s │ ✅ MATCH │
└─────────────────┴───────────────┴───────────────┴───────────────┘

Performans Karşılaştırması:
├─ Single-thread sıkıştırma, multi-thread'den 46.84x daha hızlı
└─ Single-thread açma, multi-thread'den 36.86x daha hızlı
```

### Büyük Dosya (100MB)
```
Test Sonuçları (bigfile.txt için):
┌─────────────────┬───────────────┬───────────────┬───────────────┐
│ Mod            │ Sıkıştırma    │ Açma         │ Karşılaştırma │
├─────────────────┼───────────────┼───────────────┼───────────────┤
│ Multi-thread   │    2.181153s │    5.217046s │ ✅ MATCH │
│ Single-thread  │    7.831049s │   15.992304s │ ✅ MATCH │
└─────────────────┴───────────────┴───────────────┴───────────────┘

Performans Karşılaştırması:
├─ Multi-thread sıkıştırma, single-thread'den 3.59x daha hızlı
└─ Multi-thread açma, single-thread'den 3.07x daha hızlı
```

## Notlar

- Küçük dosyalar için (< 1MB), iş parçacığı yönetimi ek yükü nedeniyle tek iş parçacıklı mod daha hızlı olabilir
- Büyük dosyalar için (> 1MB), çok iş parçacıklı mod önemli performans avantajları sağlar
- Parça boyutu 1KB ile 1GB arasında olmalıdır
- Çıktı dosyaları otomatik olarak `output/` ve `output/decompress/` dizinlerinde düzenlenir

## Hata Yönetimi

Araç şu durumlar için kapsamlı hata yönetimi içerir:
- Geçersiz işlemler
- Geçersiz modlar
- Geçersiz parça boyutları
- Dosya erişim izinleri
- Bellek ayırma hataları
- Dosya varlık kontrolleri
