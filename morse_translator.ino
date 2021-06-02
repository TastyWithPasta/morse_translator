// A tiny string-to-morse translator which uses the Arduino's integrated LED.
// Made with love by @TastyWithPasta


// Write your message here!
#define MESSAGE "A nice cup of tea!"


#define MASK(x) ((unsigned char) (1<<x))

#define SHORT_RUNE_LENGTH_MS 400
#define LONG_RUNE_LENGTH_MS (SHORT_RUNE_LENGTH_MS * 3)

#define RUNE_SPACING_MS SHORT_RUNE_LENGTH_MS
#define LETTER_SPACING_MS ((SHORT_RUNE_LENGTH_MS * 3) - RUNE_SPACING_MS) // Compensated by 1x since we include the spacing from the previous rune.
#define WORD_SPACING_MS ((SHORT_RUNE_LENGTH_MS * 7) - LETTER_SPACING_MS)   // Same reasoning as above 
#define SENTENCE_SPACING_MS ((SHORT_RUNE_LENGTH_MS * 20) - WORD_SPACING_MS)


/* DENOMINATION
-  . or _ : Rune
-  Group of runes: Letter
-  Morse Character not defined within the Morse Alphabet, such as full-stops or spaces: Special Character
-  Space-separated group of letters: Word
*/

typedef union {
    struct {
    uint8_t R1 : 1;                     // Runes 1 to 5 (big endian). 0 is short (.), 1 is long (_). Amount of runes used is Data.LEN
    uint8_t R2 : 1;
    uint8_t R3 : 1;
    uint8_t R4 : 1;
    uint8_t R5 : 1;
  } Letter;
   struct {
    uint8_t BITS : 5;
    uint8_t LEN : 3;                    // LEN is between 1 and 5, as symbol length is implicit in morse. 0 means we are reading special runes that do not translate into short or long signals.
  } Data;
  struct {
    uint8_t : UNRECOGNIZED : 1;
    uint8_t SPACE_RUNE : 1;
    uint8_t SPACE_LETTER : 1;
    uint8_t SPACE_WORD : 1;
    uint8_t SPACE_SENTENCE : 1;      
    uint8_t IS_LETTER : 3;          // If zero, it means LEN=0, we are dealing with a special symbol and not a letter.
  } Flags;
  uint8_t Value;
} MorseChar;

const uint8_t LEN_MC_TABLE = 26;
const MorseChar MC_TABLE[LEN_MC_TABLE] = {
  { .Data = { 0b00010, 02 } }, // A
  { .Data = { 0b00001, 04 } }, // B
  { .Data = { 0b00101, 04 } }, // C
  { .Data = { 0b00001, 03 } }, // D
  { .Data = { 0b00000, 01 } }, // E
  { .Data = { 0b00100, 04 } }, // F
  { .Data = { 0b00011, 04 } }, // G
  { .Data = { 0b00000, 04 } }, // H
  { .Data = { 0b00000, 02 } }, // I
  { .Data = { 0b01110, 04 } }, // J
  { .Data = { 0b00101, 03 } }, // K
  { .Data = { 0b00010, 04 } }, // L
  { .Data = { 0b00011, 02 } }, // M
  { .Data = { 0b00010, 02 } }, // N
  { .Data = { 0b00111, 03 } }, // O
  { .Data = { 0b00110, 04 } }, // P
  { .Data = { 0b01011, 04 } }, // Q
  { .Data = { 0b00010, 03 } }, // R
  { .Data = { 0b00000, 03 } }, // S
  { .Data = { 0b00001, 01 } }, // T
  { .Data = { 0b00100, 03 } }, // U
  { .Data = { 0b01000, 04 } }, // V
  { .Data = { 0b00110, 03 } }, // W
  { .Data = { 0b01001, 04 } }, // X
  { .Data = { 0b01101, 04 } }, // Y
  { .Data = { 0b00011, 04 } }, // Z
};


void inline flip() {
  PORTB ^= MASK(5);
}
void inline on(){
  PORTB |= MASK(5);
}
void inline off(){
  PORTB &= ~MASK(5);
}
void inline PrintSpace(int16_t duration) {
  off();
  delay(duration);
}
void inline PrintRuneShort() {
  on();
  delay(SHORT_RUNE_LENGTH_MS);
}
void inline PrintRuneLong() {
  on();
  delay(LONG_RUNE_LENGTH_MS);
}

/* 
 *  Print a single MorseChar.
 */
void PrintMorseChar(MorseChar mc) {
  if(mc.Flags.IS_LETTER) {
    // We compare each rune to its corresponding bit, print long if 1, short if 0
    for(uint8_t i = 0; i < mc.Data.LEN; i++) {
        mc.Data.BITS & MASK(i) ? PrintRuneLong() : PrintRuneShort();
        PrintSpace(RUNE_SPACING_MS); // Implicit space after a rune
    }
    PrintSpace(LETTER_SPACING_MS); // Implicit space after a letter
  }
  else {
    // If the MorseChar is a special character. Do nothing if character is UNRECOGNIZED
    if(mc.Flags.SPACE_WORD) PrintSpace(WORD_SPACING_MS);
    else if(mc.Flags.SPACE_SENTENCE) PrintSpace(SENTENCE_SPACING_MS);
    else if(mc.Flags.SPACE_RUNE) PrintSpace(RUNE_SPACING_MS);
    else if (mc.Flags.SPACE_LETTER) PrintSpace(LETTER_SPACING_MS);
  }
}

/* 
 *  Print a string of morse characters.
 */
void PrintMorseString(const MorseChar* mstr) {
  for(int i = 0; i < strlen((const char*)mstr); i++) {
    PrintMorseChar(mstr[i]);
  }
  PrintSpace(SENTENCE_SPACING_MS);
}

/* 
 *  Convert an 8-bit char into an 8-bit MorseChar.
 */
MorseChar CharToMorse(char c) {
  // Special cases:
  switch (c) {
    case ' ': 
      return (MorseChar){ .Data = { 0b01000, 00 } }; // WORD SPACE
    case '.': 
    case '!':
    case '?':
      return (MorseChar){ .Data = { 0b10000, 00 } }; // SENTENCE SPACE
  }
  
  c &= ~MASK(5);      // With how ASCII is arranged, setting the 5th bit to 0 gives us uppercase.
  c -= (uint8_t)65;   // Subtract 65 to rebase it on the ascii 'A' character.
  if(c < LEN_MC_TABLE)
    return MC_TABLE[c];
  else
    return (MorseChar){ .Data = { 0b00001, 00 } }; // UNKNOWN CHARACTER (skipped)
}

/* 
 *  Convert an string into a string of MorseChar, keeping \0.
 */
MorseChar* StringToMorseString(char* str) {
  for(int i = 0; i < strlen((const char*)str); i++) {
    str[i] = (char)CharToMorse(str[i]).Value;
  }
  return (MorseChar*)str;
}


MorseChar* morseString;

void setup() {
  DDRB |= MASK(5); // Direct the output to the Arduino's LED (pin 5)
  morseString = StringToMorseString(MESSAGE);
}


void loop() {
  PrintMorseString(morseString);
}
