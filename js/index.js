// decode.js
const LZ4 = require('lz4');
const msgpack = require('@msgpack/msgpack'); // Install via npm: npm install @msgpack/msgpack

// UQDecode: Reverse the UQEncode process.
// Expected format: "PX" + <compression flags (4 bits)> + <encoding/cipher flags (4 bits)> + ":" + <encoded> + ":" + <checksum>
function UQDecode(urlSafeStr) {
  const parts = urlSafeStr.split(':');
  if (parts.length !== 3) {
    throw new Error('Invalid encoded format');
  }
  const header = parts[0];
  const encoded = parts[1];
  const checksumStr = parts[2];

  let compressionFlags = 0;
  let encodingFlags = 0;
  let baseIndex;
  
  if (header.startsWith("PX")) {
    // Parse the flags (hex digits representing 4 bits each)
    if (header.length < 4) {
      throw new Error('Invalid header format: too short');
    }
    
    // First hex digit is for compression
    const compressionHex = header[2];
    compressionFlags = parseInt(compressionHex, 16);
    
    // Second hex digit is for encoding and cipher steps
    const encodingHex = header[3];
    encodingFlags = parseInt(encodingHex, 16);
    
    // Determine baseIndex from the encoding flag's first bit
    // 0 = Base64, 1 = Base70
    baseIndex = (encodingFlags & 0x1) ? 2 : 1;
  } else if (header.startsWith("UQ")) {
    console.warn("Warning: Using legacy 'UQ' header format");
    // For legacy format, compression flag is a simple boolean
    const isCompressed = header[2] === '1';
    compressionFlags = isCompressed ? 0x1 : 0x0; // Set bit 0 if compressed
    encodingFlags = 0x0; // Default to Base64 and Reverse bytes only
    baseIndex = parseInt(header[3], 10);
  } else {
    throw new Error('Invalid header');
  }

  // Extract compression flags
  const useLZ4 = !!(compressionFlags & 0x1);       // Bit 0: LZ4 compression
  const useMessagePack = !!(compressionFlags & 0x2); // Bit 1: MessagePack
  
  // Extract encoding and cipher flags
  // First bit (0x1) - Encoding step:
  //   0x0 - Base64
  //   0x1 - Base70
  // Next 3 bits - Cipher steps:
  //   0x0 - Reverse bytes only
  //   0x1 - Caesar cipher only
  //   0x2 - Reverse bytes + Caesar cipher
  //   0x3 - Caesar cipher + Reverse bytes
  //   0x4-0x8 - Reserved for future use
  const useBase70 = !!(encodingFlags & 0x1);       // Bit 0: Base70 if set, Base64 if not
  const cipherSteps = (encodingFlags >> 1) & 0x7;  // Bits 1-3: Cipher steps
  
  // Define the character sets.
  const bases = [
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_",         // Base64 URL-safe
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_.~"         // Base70
  ];
  
  // Select charset based on the encoding bit
  const charsetIndex = useBase70 ? 1 : 0;
  const charset = bases[charsetIndex];
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

  // Process the data according to the cipher steps
  let processedBuffer = buffer;
  
  // Apply the appropriate cipher steps based on the flag
  switch (cipherSteps) {
    case 0x0: // Reverse bytes only
      processedBuffer = reverseBytes(processedBuffer);
      break;
    case 0x1: // Caesar cipher only
      processedBuffer = applyCaesarCipher(processedBuffer);
      break;
    case 0x2: // Reverse bytes + Caesar cipher
      processedBuffer = reverseBytes(processedBuffer);
      processedBuffer = applyCaesarCipher(processedBuffer);
      break;
    case 0x3: // Caesar cipher + Reverse bytes
      processedBuffer = applyCaesarCipher(processedBuffer);
      processedBuffer = reverseBytes(processedBuffer);
      break;
    // 0x4-0x8: Reserved for future use
    default:
      console.warn(`Unimplemented cipher step: 0x${cipherSteps.toString(16)}`);
      break;
  }
  
  // Process the data according to the compression flags
  
  // Step 1: Decompress with LZ4 if used
  if (useLZ4) {
    // IMPORTANT: In a robust solution you should embed the uncompressed size.
    // For this example, we assume a maximum expected size.
    const uncompressedSize = 1024; // adjust as needed
    const decompressedBuffer = Buffer.alloc(uncompressedSize);
    const decodedSize = LZ4.decodeBlock(processedBuffer, decompressedBuffer);
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
}

// Helper function to reverse bytes in a buffer
function reverseBytes(buffer) {
  // This is a placeholder - actual implementation would depend on requirements
  // For now, we'll just return the original buffer
  console.log("Reverse bytes operation would be applied here");
  return buffer;
}

// Helper function to apply Caesar cipher
function applyCaesarCipher(buffer) {
  // This is a placeholder - actual implementation would depend on requirements
  // For now, we'll just return the original buffer
  console.log("Caesar cipher operation would be applied here");
  return buffer;
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
