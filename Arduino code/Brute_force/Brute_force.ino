int rx_pin = 13;  // Pin used for transmitting messages

// Main message patterns for "on" and "off" states
byte message[] = { 0x7D, 0x14, 0x7D, 0x14 };

// Array of ending bytes used to terminate messages
byte ending_bytes[] = { 0x69, 0x78, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A };
int curent_end_byte = 0;  // Index to track the current ending byte in the sequence

int message_repeats = 1;  // Number of times the message is repeated during transmission

void setup() {
  pinMode(rx_pin, OUTPUT);
  Serial.begin(9600);  // Initialize serial communication at 9600 baud rate
}

void loop() {
  for (int byte1 = 0; byte1 <= 0xFF; byte1++) {    // First byte brute-force loop
    for (int byte2 = 0; byte2 <= 0xFF; byte2++) {  // Second byte brute-force loop
      // Skip the known working message
      if (byte1 == 0x7D && byte2 == 0x14) {  // Do not try the known working one
        continue;
      }

      message[0] = byte1;  // Update the first byte
      message[1] = byte2;  // Update the second byte

      // Print the current byte values being tested with zero padding
      Serial.print("Testing bytes: 0x");
      if (byte1 < 0x10) Serial.print("0");  // Add leading zero for single-digit hex
      Serial.print(byte1, HEX);

      Serial.print(", 0x");
      if (byte2 < 0x10) Serial.print("0");  // Add leading zero for single-digit hex
      Serial.println(byte2, HEX);

      // Send the modified message
      send_message(rx_pin, message, sizeof(message), ending_bytes, sizeof(ending_bytes), curent_end_byte);
      delay(10);  // Optional delay to avoid overwhelming the receiver
    }
  }
}

// Sends a warmup signal to prepare the receiver, perhaps this is the preamble, not sure
void warmup(int pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(9000);
  digitalWrite(pin, LOW);
  delayMicroseconds(4580);
}

// Generates a short pulse signal
void pulse(int pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(560);
  digitalWrite(pin, LOW);
}

// Sends a "low bit" signal, aka 0
void low_bit(int pin) {
  pulse(pin);
  delayMicroseconds(1760);
}

// Sends a "high bit" signal, aka 1
void high_bit(int pin) {
  pulse(pin);
  delayMicroseconds(608);
}

// Sends an 8-bit byte, bit by bit
void send_byte(int pin, byte data) {
  for (int bit = 7; bit >= 0; bit--) {  // Loop through all 8 bits of the byte
    if (data & (1 << bit)) {            // Check if the bit is 1
      high_bit(pin);
    } else {  // Otherwise, the bit is 0
      low_bit(pin);
    }
  }
}

// Sends a complete message, consisting the warmup, payload, ending bytes and "warmdown"
void send_message(int pin, byte* mainMessage, size_t mainSize, byte* endBytes, size_t endSize, int& currentEndIndex) {
  for (int i =0; i< sizeof(ending_bytes; i++){ // Must try all ending bytes, as we do not know which one is correct
    for (int i = 0; i < message_repeats; i++) {
      warmup(pin);  // Send warmup signal

      // Send main message bytes
      for (size_t i = 0; i < mainSize; i++) {
        send_byte(pin, mainMessage[i]);
      }

      // Send the current ending byte
      send_byte(pin, endBytes[currentEndIndex]);

      // Update the current end byte index
      currentEndIndex++;
      if (currentEndIndex >= endSize) {
        currentEndIndex = 0;
      }
      pulse(pin);  // Send warmdown signal
      delayMicroseconds(11200);
    }
  }
}
