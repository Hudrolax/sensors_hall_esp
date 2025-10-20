#include "melodies.h"
#include "pins.h"

int frequency(char note) {
  const int numNotes = 8;
  const char names[numNotes] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  const int  freqs[numNotes] = { 262, 294, 330, 349, 392, 440, 494, 523 };
  for (int i = 0; i < numNotes; i++) if (names[i] == note) return freqs[i];
  return 0;
}

static void playSequence(const char* notes, const int* beats, int songLength, int tempo) {
  for (int i = 0; i < songLength; i++) {
    int duration = beats[i] * tempo;
    if (notes[i] == ' ') {
      delay(duration);
    } else {
      tone(BUZZER_PIN, frequency(notes[i]), duration);
      delay(duration);
    }
    delay(tempo/10);
  }
}

void change_guard_mode_song(){
  const int songLength = 5;
  const char notes[] = "cdfda";
  const int beats[]   = {1,1,1,1,1,1,4,4,2,1,1,1,1,1,1,4,4,2}; // оставлено как в исходнике
  const int tempo = 150;
  playSequence(notes, beats, songLength, tempo);
}

void change_guard_mode_song2(){
  const int songLength = 5;
  const char notes[] = "cdfdg";
  const int beats[]   = {1,1,1,1,1,1,4,4,2,1,1,1,1,1,1,4,4,2};
  const int tempo = 150;
  playSequence(notes, beats, songLength, tempo);
}

void play_song(){
  const int songLength = 18;
  const char notes[] = "cdfda ag cdfdg gf ";
  const int beats[]   = {1,1,1,1,1,1,4,4,2,1,1,1,1,1,1,4,4,2};
  const int tempo = 150;
  playSequence(notes, beats, songLength, tempo);
}
