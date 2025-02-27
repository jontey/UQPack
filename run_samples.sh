#!/bin/bash

# Sample JSON strings
SAMPLES=(
  '{"reference":"35d93b66cf574076e14b36ce9eecf29b2f94b6a3","basket":{"numberOfProducts":1,"content":[{"productQuantity":1,"price":339,"productName":"Frappuccino"}]},"urlToken":"d471974fe310c6d63000dbcffb5d8d203c209e08","terminal":{"manufacturerId":"<MANUFACTURER_ID>"},"state":"INCOMPLETE","transaction":{"displayTime":"2025-02-27T11:04:48","localTime":"2025-02-27T11:04:48","amount":"339","isoTime":"2025-02-27T11:04:48Z","time":"2025-02-27T11:04:48Z","currencyCode":"USD"},"state2":"INCOMPLETE","state3":"INCOMPLETE","state4":"INCOMPLETE","state5":"INCOMPLETE"}'
  
  '{"reference":"42a87c55df982165f25a47de8bbcf38a1e83c7b4","basket":{"numberOfProducts":2,"content":[{"productQuantity":1,"price":499,"productName":"Latte"},{"productQuantity":2,"price":250,"productName":"Croissant"},{"productQuantity":2,"price":250,"productName":"Bagel"},{"productQuantity":2,"price":250,"productName":"Cake"}]},"urlToken":"e582085ff421d7e74111ecdffc6e9e314d318f19","terminal":{"manufacturerId":"<MANUFACTURER_ID>"},"state":"COMPLETE","transaction":{"amount":"999","currencyCode":"USD"}}'
  
  '{"reference":"59c64d44ef063254a36b58ed7ccdf49c0f72d5b5","basket":{"numberOfProducts":3,"content":[{"productQuantity":1,"price":599,"productName":"Mocha"},{"productQuantity":1,"price":350,"productName":"Sandwich"},{"productQuantity":1,"price":350,"productName":"Sandwich"},{"productQuantity":1,"price":350,"productName":"Sandwich"},{"productQuantity":1,"price":350,"productName":"Sandwich"},{"productQuantity":1,"price":350,"productName":"Sandwich"},{"productQuantity":1,"price":199,"productName":"Cookie"}]},"urlToken":"f693196ff532e8f85222fdeffb7f0f425e427f2a","terminal":{"manufacturerId":"<MANUFACTURER_ID>"},"state":"PENDING","transaction":{"displayTime":"2025-02-27T11:15:37","localTime":"2025-02-27T11:15:37","amount":"1148","isoTime":"2025-02-27T11:15:37Z","time":"2025-02-27T11:15:37Z","currencyCode":"USD"}}'
)

echo "Running sample JSON strings through URL safe encoder and decoder..."
echo "=================================================================="

for i in "${!SAMPLES[@]}"; do
  echo -e "\nSample $((i+1)):"
  echo "----------------------------------------------------------------"
  
  # Encode the JSON string using the C++ encoder and capture full output
  FULL_OUTPUT=$(./cpp/build/url_safe_encoder "${SAMPLES[$i]}")
  
  # Print the full output from url_safe_encoder
  echo "Output from url_safe_encoder:"
  echo "$FULL_OUTPUT"
  echo ""
  
  # Extract only the encoded string from the output (the part after "Encoded string: ")
  ENCODED=$(echo "$FULL_OUTPUT" | grep "Encoded string:" | sed 's/Encoded string: //')
  
  # Decode the encoded string using the JavaScript decoder
  node js/index.js "$ENCODED"
  
  echo "----------------------------------------------------------------"
done

echo -e "\nAll samples processed successfully!"
