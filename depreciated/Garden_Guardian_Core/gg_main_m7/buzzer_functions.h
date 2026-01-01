//Init buzzer notes
int melody[] = {
  NOTE_E5, NOTE_E5, 0, NOTE_E5, 0, NOTE_C5, NOTE_E5,
  NOTE_G5, 0, NOTE_G4, 0, 
  NOTE_C5, NOTE_G4, 0, NOTE_E4,
 
};

int durations[] = {
  8, 8, 8, 8, 8, 8, 8,
  4, 4, 8, 4, 
 
};

void playBootSound (int BUZZER_PIN){

    int size = sizeof(durations) / sizeof(int);

  for (int note = 0; note < size; note++) {
    //to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int duration = 1000 / durations[note];
    tone(BUZZER_PIN, melody[note], duration);

    //to distinguish the notes, set a minimum time between them.
    //the note's duration + 30% seems to work well:
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    
    //stop the tone playing:
    noTone(BUZZER_PIN);
  }

}