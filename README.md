# JSON Serializer/Deserializer

A utility for encoding and decoding JSON data in a URL-safe format, supporting compression and various encoding schemes.

## Overview

This project provides a bi-directional conversion system for JSON data:

- C++ encoder that converts JSON data into a URL-safe string format
- JavaScript decoder that converts the encoded string back to the original JSON

The encoding process uses a custom algorithm that supports:

- Multiple character sets (Base62, Base64 URL-safe, Base70)
- LZ4 compression
- MessagePack serialization
- Checksum verification

## Project Structure

- `/cpp` - C++ implementation of the encoder
- `/js` - JavaScript implementation of the decoder
- `run_samples.sh` - Script to test the encoder/decoder with sample JSON data

## Requirements

### C++ Encoder

- C++11 or higher
- CMake
- nlohmann/json library
- LZ4 compression library

### JavaScript Decoder

- Node.js
- LZ4 package (`npm install lz4`)
- MessagePack package (`npm install @msgpack/msgpack`)

## Building

To build the C++ encoder:

```bash
make build
```

This will create the executable at `cpp/build/url_safe_encoder`.

## Usage

### Running the Sample Script

The project includes a sample script that demonstrates encoding and decoding with test data:

```bash
make run-samples
```

### Using the C++ Encoder

```bash
./cpp/build/url_safe_encoder '{"reference":"35d93b66","transaction":{"amount":"339","currencyCode":"USD"}}'
```

### Using the JavaScript Decoder

```bash
node js/index.js "UQ10:encoded_string_here:checksum"
```

## Encoding Format

The encoded string follows this format:

```sh
"UQ" + <encoding flags (4 bits)> + <base index> + ":" + <encoded> + ":" + <checksum>
```

Where:

- `UQ` is the header identifier
- Encoding flags indicate compression and serialization methods
- Base index selects the character set (0=Base62, 1=Base64, 2=Base70)
- Encoded is the actual encoded data
- Checksum provides data integrity verification

## License

[Add your license information here]

## Contact

[Add your contact information here]
