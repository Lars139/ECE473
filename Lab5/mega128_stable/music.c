//Credit: Kellen_music.c -- from class website under Student Work
#define C0 0x1DDC
#define Db0 0x1C30
#define D0 0x1A9A
#define Eb0 0x1919
#define E0 0x17B2
#define F0 0x165D
#define Gb0 0x151D
#define G0 0x13ED
#define Ab0 0x12CE
#define A0 0x11C0
#define Bb0 0x10C0
#define B0 0x0FD0
#define C1 0x0EED
#define Db1 0x0E16
#define D1 0x0D4C
#define Eb1 0x0C8D
#define E1 0x0BD8
#define F1 0x0B2E
#define Gb1 0x0A8D
#define G1 0x09F6
#define Ab1 0x0967
#define A1 0x08DF
#define Bb1 0x0860
#define B1 0x07E7
#define C2 0x0776
#define Db2 0x070A
#define D2 0x06A5
#define Eb2 0x0646
#define E2 0x05EB
#define F2 0x0596
#define Gb2 0x0546
#define G2 0x04FA
#define Ab2 0x04B2
#define A2 0x046F
#define Bb2 0x042F
#define B2 0x03F3
#define C3 0x03BA
#define Db3 0x0384
#define D3 0x0352
#define Eb3 0x0322
#define E3 0x02F5
#define F3 0x02CA
#define Gb3 0x02A2
#define G3 0x027C
#define Ab3 0x0258
#define A3 0x0237
#define Bb3 0x0217
#define B3 0x01F9
#define C4 0x01DC
#define Db4 0x01C1
#define D4 0x01A8
#define Eb4 0x0190
#define E4 0x017A
#define F4 0x0164
#define Gb4 0x0150
#define G4 0x013D
#define Ab4 0x012B
#define A4 0x011B
#define Bb4 0x010B
#define B4 0x00FC
#define C5 0x00ED
#define Db5 0x00E0
#define D5 0x00D3
#define Eb5 0x00C7
#define E5 0x00BC
#define F5 0x00B1
#define Gb5 0x00A7
#define G5 0x009E
#define Ab5 0x0095
#define A5 0x008D
#define Bb5 0x0085
#define B5 0x007D
#define C6 0x0076
#define Db6 0x006F
#define D6 0x0069
#define Eb6 0x0063
#define E6 0x005D
#define F6 0x0058
#define Gb6 0x0053
#define G6 0x004E
#define Ab6 0x004A
#define A6 0x0046
#define Bb6 0x0042
#define B6 0x003E
#define C7 0x003A
#define Db7 0x0037
#define D7 0x0034
#define Eb7 0x0031
#define E7 0x002E
#define F7 0x002B
#define Gb7 0x0029
#define G7 0x0026
#define Ab7 0x0024
#define A7 0x0022
#define Bb7 0x0020
#define B7 0x001E
#define C8 0x001C
#define Db8 0x001B
#define D8 0x0019
#define Eb8 0x0018
#define E8 0x0015
#define F8 0x0012
#define Gb8 0x0010
#define G8 0x000D
#define Ab8 0x000B
#define A8 0x0009
#define Bb8 0x0007
#define B8 0x0005

static volatile uint16_t beat;
static volatile uint16_t max_beat;
static volatile uint8_t  notes;

void music_init(void) {
   beat = 0;
   max_beat = 0;
   notes = 0;
}

void play_note(char note, uint8_t flat, uint8_t octave, uint8_t duration) {
   //pass in the note, it's key, the octave they want, and a duration
   //function sets the value of OCR1A and the timer
   //note must be A-G
   //flat must be 1 (for flat) or 0 (for natural) (N/A on C or F)
   //octave must be 0-8 (0 is the lowest, 8 doesn't sound very good)
   //duration is in 64th notes at 120bpm
   //e.g. play_note('D', 1, 0, 16)
   //this would play a Db, octave 0 for 1 quarter note
   //120 bpm (every 32ms inc beat)
   beat = 0;             //reset the beat counter
   max_beat = duration;  //set the max beat
   switch (octave) {
      case 0: switch (note) {
		 case 'A': if(flat){OCR1A=Ab0;}
			      else {OCR1A=A0;}
			      break;
		 case 'B': if(flat){OCR1A=Bb0;}
			      else {OCR1A=B0;}
			      break;
		 case 'C': OCR1A=C0;
			   break;
		 case 'D': if(flat){OCR1A=Db0;}
			      else {OCR1A=D0;}
			      break;
		 case 'E': if(flat){OCR1A=Eb0;}
			      else {OCR1A=E0;}
			      break;
		 case 'F': OCR1A=F0;
			   break;
		 case 'G': if(flat){OCR1A=Gb0;}
			      else {OCR1A=G0;}
			      break;
	      } 
	      break;
      case 1: switch (note) {
		 case 'A': if(flat){OCR1A=Ab1;}
			      else {OCR1A=A1;}
			      break;
		 case 'B': if(flat){OCR1A=Bb1;}
			      else {OCR1A=B1;}
			      break;
		 case 'C': OCR1A=C1;
			   break;
		 case 'D': if(flat){OCR1A=Db1;}
			      else {OCR1A=D1;}
			      break;
		 case 'E': if(flat){OCR1A=Eb1;}
			      else {OCR1A=E1;}
			      break;
		 case 'F': OCR1A=F1;
			   break;
		 case 'G': if(flat){OCR1A=Gb1;}
			      else {OCR1A=G1;}
			      break;
	      } 
	      break;
      case 2: switch (note) {
		 case 'A': if(flat){OCR1A=Ab2;}
			      else {OCR1A=A2;}
			      break;
		 case 'B': if(flat){OCR1A=Bb2;}
			      else {OCR1A=B2;}
			      break;
		 case 'C': OCR1A=C2;
			   break;
		 case 'D': if(flat){OCR1A=Db2;}
			      else {OCR1A=D2;}
			      break;
		 case 'E': if(flat){OCR1A=Eb2;}
			      else {OCR1A=E2;}
			      break;
		 case 'F': OCR1A=F2;
			   break;
		 case 'G': if(flat){OCR1A=Gb2;}
			      else {OCR1A=G2;}
			      break;
	      } 
	      break;
      case 3: switch (note) {
		 case 'A': if(flat){OCR1A=Ab3;}
			      else {OCR1A=A3;}
			      break;
		 case 'B': if(flat){OCR1A=Bb3;}
			      else {OCR1A=B3;}
			      break;
		 case 'C': OCR1A=C3;
			   break;
		 case 'D': if(flat){OCR1A=Db3;}
			      else {OCR1A=D3;}
			      break;
		 case 'E': if(flat){OCR1A=Eb3;}
			      else {OCR1A=E3;}
			      break;
		 case 'F': OCR1A=F3;
			   break;
		 case 'G': if(flat){OCR1A=Gb3;}
			      else {OCR1A=G3;}
			      break;
	      } 
	      break;
      case 4: switch (note) {
		 case 'A': if(flat){OCR1A=Ab4;}
			      else {OCR1A=A4;}
			      break;
		 case 'B': if(flat){OCR1A=Bb4;}
			      else {OCR1A=B4;}
			      break;
		 case 'C': OCR1A=C4;
			   break;
		 case 'D': if(flat){OCR1A=Db4;}
			      else {OCR1A=D4;}
			      break;
		 case 'E': if(flat){OCR1A=Eb4;}
			      else {OCR1A=E4;}
			      break;
		 case 'F': OCR1A=F4;
			   break;
		 case 'G': if(flat){OCR1A=Gb4;}
			      else {OCR1A=G4;}
			      break;
	      } 
	      break;
      case 5: switch (note) {
		 case 'A': if(flat){OCR1A=Ab5;}
			      else {OCR1A=A5;}
			      break;
		 case 'B': if(flat){OCR1A=Bb5;}
			      else {OCR1A=B5;}
			      break;
		 case 'C': OCR1A=C5;
			   break;
		 case 'D': if(flat){OCR1A=Db5;}
			      else {OCR1A=D5;}
			      break;
		 case 'E': if(flat){OCR1A=Eb5;}
			      else {OCR1A=E5;}
			      break;
		 case 'F': OCR1A=F5;
			   break;
		 case 'G': if(flat){OCR1A=Gb5;}
			      else {OCR1A=G5;}
			      break;
	      } 
	      break;
      case 6: switch (note) {
		 case 'A': if(flat){OCR1A=Ab6;}
			      else {OCR1A=A6;}
			      break;
		 case 'B': if(flat){OCR1A=Bb6;}
			      else {OCR1A=B6;}
			      break;
		 case 'C': OCR1A=C6;
			   break;
		 case 'D': if(flat){OCR1A=Db6;}
			      else {OCR1A=D6;}
			      break;
		 case 'E': if(flat){OCR1A=Eb6;}
			      else {OCR1A=E6;}
			      break;
		 case 'F': OCR1A=F6;
			   break;
		 case 'G': if(flat){OCR1A=Gb6;}
			      else {OCR1A=G6;}
			      break;
	      } 
	      break;
      case 7: switch (note) {
		 case 'A': if(flat){OCR1A=Ab7;}
			      else {OCR1A=A7;}
			      break;
		 case 'B': if(flat){OCR1A=Bb7;}
			      else {OCR1A=B7;}
			      break;
		 case 'C': OCR1A=C7;
			   break;
		 case 'D': if(flat){OCR1A=Db7;}
			      else {OCR1A=D7;}
			      break;
		 case 'E': if(flat){OCR1A=Eb7;}
			      else {OCR1A=E7;}
			      break;
		 case 'F': OCR1A=F7;
			   break;
		 case 'G': if(flat){OCR1A=Gb7;}
			      else {OCR1A=G7;}
			      break;
	      } 
	      break;
      case 8: switch (note) {
		 case 'A': if(flat){OCR1A=Ab8;}
			      else {OCR1A=A8;}
			      break;
		 case 'B': if(flat){OCR1A=Bb8;}
			      else {OCR1A=B8;}
			      break;
		 case 'C': OCR1A=C8;
			   break;
		 case 'D': if(flat){OCR1A=Db8;}
			      else {OCR1A=D8;}
			      break;
		 case 'E': if(flat){OCR1A=Eb8;}
			      else {OCR1A=E8;}
			      break;
		 case 'F': OCR1A=F8;
			   break;
		 case 'G': if(flat){OCR1A=Gb8;}
			      else {OCR1A=G8;}
			      break;
	      } 
	      break;
      default: OCR1A=0x0000;
   }
}

