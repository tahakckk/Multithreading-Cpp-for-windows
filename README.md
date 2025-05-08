# Multithreaded File Compression Utility

A high-performance file compression and decompression utility that supports both single-threaded and multi-threaded operations.

## Features

- Multi-threaded compression and decompression
- Single-threaded mode for comparison
- Progress bar display
- File integrity verification
- Configurable chunk size
- Automatic output directory management
- Comprehensive error handling
- Performance benchmarking

## Requirements

- C++17 or later
- Windows OS (uses Windows-specific APIs)
- G++ compiler

## Building

```bash
g++ -std=c++17 main.cpp compressor.cpp -o compressor.exe
```
-----------

```bash
g++ -std=c++17 generate_files.cpp -o genarete_files.exe
```

## Usage

You must run this code exactly, otherwise the modes will not work:
```bash
./generate_files.exe
```

### Basic Usage

```bash
./compressor.exe <compress|decompress> <single|multi> <input_file> <output_file> <chunk_size_in_bytes>
```

Example:
```bash
./compressor.exe compress multi input/bigfile.txt output/bigfile.compressed 1048576
```

### Test Mode

Run all tests with:
```bash
./compressor.exe test
```

## Performance Results

### Small File (1KB)
```
Test Results (for smallfile.txt):
┌─────────────────┬───────────────┬───────────────┬───────────────┐
│ Mode           │ Compression   │ Decompression │ Comparison    │
├─────────────────┼───────────────┼───────────────┼───────────────┤
│ Multi-thread   │    0.133404s │    0.153865s │ ✅ MATCH │
│ Single-thread  │    0.002848s │    0.004176s │ ✅ MATCH │
└─────────────────┴───────────────┴───────────────┴───────────────┘

Performance Comparison:
├─ Single-thread compression is 46.84x faster than multi-thread
└─ Single-thread decompression is 36.86x faster than multi-thread
```

### Large File (100MB)
```
Test Results (for bigfile.txt):
┌─────────────────┬───────────────┬───────────────┬───────────────┐
│ Mode           │ Compression   │ Decompression │ Comparison    │
├─────────────────┼───────────────┼───────────────┼───────────────┤
│ Multi-thread   │    2.181153s │    5.217046s │ ✅ MATCH │
│ Single-thread  │    7.831049s │   15.992304s │ ✅ MATCH │
└─────────────────┴───────────────┴───────────────┴───────────────┘

Performance Comparison:
├─ Multi-thread compression is 3.59x faster than single-thread
└─ Multi-thread decompression is 3.07x faster than single-thread
```

## Notes

- For small files (< 1MB), single-threaded mode may be faster due to thread management overhead
- For large files (> 1MB), multi-threaded mode provides significant performance benefits
- Chunk size must be between 1KB and 1GB
- Output files are automatically organized in `output/` and `output/decompress/` directories

## Error Handling

The utility includes comprehensive error handling for:
- Invalid operations
- Invalid modes
- Invalid chunk sizes
- File access permissions
- Memory allocation failures
- File existence checks 