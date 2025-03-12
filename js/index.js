// decode.js
const msgpack = require('@msgpack/msgpack'); // Install via npm: npm install @msgpack/msgpack

// Load WebAssembly module
const wasmPath = require('path').join(__dirname, '../cpp/dist/uqpack_wasm.js');
const UQPack = require(wasmPath);

// UQDecode: Reverse the UQEncode process.
// Expected format: "PX" + <compression flags (4 bits)> + <encoding/cipher flags (4 bits)> + ":" + <encoded> + ":" + <checksum>
async function UQDecode(urlSafeStr) {
  // Initialize WebAssembly module
  const module = await UQPack();
  // Decode using WebAssembly
  const decodedBuffer = module.decode(urlSafeStr);

  // Convert Uint8Array to Buffer and decode msgpack
  const buffer = Buffer.from(decodedBuffer);
  return msgpack.decode(buffer);
}

async function main() {
  // Pass the URL-safe string as a command-line argument.
  const urlSafeStr = process.argv[2];
  if (!urlSafeStr) {
    console.error("Usage: node index.js <urlsafe_string>");
    process.exit(1);
  }

  try {
    // Decode using WebAssembly module
    const result = await UQDecode(urlSafeStr);
    
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
  } catch (error) {
    console.error("Error decoding data:", error);
    process.exit(1);
  }
}

// Run main and handle any errors
main().catch(error => {
  console.error('Error:', error);
  process.exit(1);
});
