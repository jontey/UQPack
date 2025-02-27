// decode.js
const LZ4 = require('lz4');
const msgpack = require('@msgpack/msgpack'); // Install via npm: npm install @msgpack/msgpack

// UQDecode: Reverse the UQEncode process.
// Expected format: "UQ" + <encoding flags (4 bits)> + <base index> + ":" + <encoded> + ":" + <checksum>
function UQDecode(urlSafeStr) {
  const parts = urlSafeStr.split(':');
  if (parts.length !== 3) {
    throw new Error('Invalid encoded format');
  }
  const header = parts[0];
  const encoded = parts[1];
  const checksumStr = parts[2];

  let encodingFlags = 0;
  let baseIndex;
  
  if (header.startsWith("UQ")) {
    // Parse the encoding flags (hex digit representing 4 bits)
    const flagHex = header[2];
    encodingFlags = parseInt(flagHex, 16);
    baseIndex = parseInt(header[3], 10);
  } else {
    throw new Error('Invalid header');
  }

  // Extract specific encoding flags
  const useLZ4 = !!(encodingFlags & 0x1);       // Bit 0: LZ4 compression
  const useMessagePack = !!(encodingFlags & 0x2); // Bit 1: MessagePack
  
  // Define the same three charsets.
  const bases = [
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",         // Base62
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_",         // Base64 URL-safe
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_.~"         // Base70
  ];
  const charset = bases[baseIndex];
  const base = charset.length;

  // Decode the big integer back into a byte array.
  // We'll build the number in base 256.
  let digits = [0];
  for (let i = 0; i < encoded.length; i++) {
    const char = encoded[i];
    const value = charset.indexOf(char);
    if (value === -1) {
      throw new Error('Invalid character in encoded string');
    }
    let carry = value;
    for (let j = digits.length - 1; j >= 0; j--) {
      const product = digits[j] * base + carry;
      digits[j] = product & 0xFF;
      carry = product >> 8;
    }
    while (carry > 0) {
      digits.unshift(carry & 0xFF);
      carry >>= 8;
    }
  }

  const buffer = Buffer.from(digits);

  // Verify checksum.
  let sum = 0;
  for (let i = 0; i < buffer.length; i++) {
    sum += buffer[i];
  }
  sum = sum % 256;
  const computedChecksum = sum.toString(16).padStart(2, '0').toUpperCase();
  if (computedChecksum !== checksumStr) {
    throw new Error('Checksum mismatch!');
  }

  // Process the data according to the encoding flags
  let processedBuffer = buffer;
  
  // Step 1: Decompress with LZ4 if used
  if (useLZ4) {
    // IMPORTANT: In a robust solution you should embed the uncompressed size.
    // For this example, we assume a maximum expected size.
    const uncompressedSize = 1024; // adjust as needed
    const decompressedBuffer = Buffer.alloc(uncompressedSize);
    const decodedSize = LZ4.decodeBlock(buffer, decompressedBuffer);
    processedBuffer = decompressedBuffer.slice(0, decodedSize);
  }
  
  // Step 2: Decode MessagePack if used
  if (useMessagePack) {
    try {
      return msgpack.decode(processedBuffer);
    } catch (err) {
      console.error("Error decoding MessagePack:", err);
      throw new Error("Failed to decode MessagePack data");
    }
  } else {
    // treat as a string
    return processedBuffer.toString();
  }
  
  return processedBuffer;
}

function main() {
  // Pass the URL-safe string as a command-line argument.
  const urlSafeStr = process.argv[2];
  if (!urlSafeStr) {
    console.error("Usage: node index.js <urlsafe_string>");
    process.exit(1);
  }

  try {
    // console.log("Received URL-safe string:", urlSafeStr);
    let result = UQDecode(urlSafeStr);

    if (typeof result === 'string') {
      result = JSON.parse(result);
    }
    
    if (typeof result === 'object' && !Buffer.isBuffer(result)) {
      // If result is a JSON object (from MessagePack)
      const jsonData = result;
      const inputBytes = Buffer.byteLength(urlSafeStr, 'utf8');
      const outputBytes = Buffer.byteLength(JSON.stringify(jsonData), 'utf8');
      const compressionRatio = (1 - (inputBytes / outputBytes)) * 100;
      console.log("Decoded JSON:");
      console.log(JSON.stringify(jsonData, null, 2));
      console.log(`Compression ratio: ${compressionRatio.toFixed(2)}%`);
    } else {
      // If result is a Buffer
      console.log("Decoded buffer:", result);
    }
  } catch (err) {
    console.error("Error decoding data:", err);
  }
}

main();
