#include <LiquidCrystal.h>
#include <EEPROM.h>

// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Pins
int contrastPin = 6;
int buttonPinNext = 7; // Down
int buttonPinPrev = 8; // Up
int statusPin = 9; // status led

// Button debounce
unsigned long lastDebounceTimeNext = 0;
unsigned long lastDebounceTimePrev = 0;
const unsigned long debounceDelay = 50;

int lastButtonStateNext = HIGH;
int buttonStateNext = HIGH;
int lastButtonStatePrev = HIGH;
int buttonStatePrev = HIGH;

int score = 0;
int e_score = 0;

float multiplier = 1.01; // lower = faster the game gets

unsigned long lastSpikeMoveTime = 0;
const unsigned long spikeMoveInterval = 300;  // Slower for fairness

// Custom characters
uint8_t playerMap[] = { 
  B01100,
  B11000,
  B11010,
  B01111,
  B01111,
  B11010,
  B11000,
  B01100
};

uint8_t spikeMap[] = { 
  B00000,
  B00100,
  B01110,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000
};

uint8_t catMap[] = {
  B00101,
  B00010,
  B00111,
  B00110,
  B01110,
  B01110,
  B01110,
  B11110
};

uint8_t pp[] = {
  B00100,
  B01010,
  B01110,
  B01010,
  B01010,
  B01010,
  B10101,
  B01010
};

// Structures
struct Player {
  int x;
  int y;
};

struct Spike {
  int x;
  int y;
};

// Entities
Player player;
Spike spike1, spike2, spike3, spike4;

void setup() {
  EEPROM.get(0, e_score);
  Serial.begin(9600);
  Serial.println(String("Saved score: ") + String(e_score));

  analogWrite(contrastPin, 75);
  pinMode(buttonPinNext, INPUT_PULLUP);
  pinMode(buttonPinPrev, INPUT_PULLUP);
  pinMode(statusPin, OUTPUT);

  lcd.begin(16, 2);

  // Custom characters
  lcd.createChar(0, playerMap);
  lcd.createChar(1, spikeMap);
  lcd.createChar(2, catMap);
  lcd.createChar(3, pp);

  // Intro
  typewriterPrint("Rocket!", 0, 0, 100);
  delay(500);
  typewriterPrint("By: Kale\x02", 0, 1, 100);
  delay(2250);
  typewriterPrint("Your best: ", 0, 0, 100);
  delay(500);
  lcd.clear();
  typewriterPrint(String(e_score), 0, 0, 100);
  delay(1500);
  lcd.clear();
  lcd.print("Let's go!");
  delay(1500);
  lcd.clear();

  // Initial positions
  player.x = 0;
  player.y = 0;

  spike1 = {20, 0};
  spike2 = {10, 1};
  spike3 = {5, 0};
  spike4 = {25, 1};

  delay(500);
}

void loop() {
  handleNextButton(buttonPinNext);
  handlePrevButton(buttonPinPrev);

  // Check for collision
  if (checkCollision(player, spike1) ||
      checkCollision(player, spike2) ||
      checkCollision(player, spike3) ||
      checkCollision(player, spike4)) {
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game over!");
    delay(300);
    lcd.setCursor(0, 1);
    if (score > e_score) {
      lcd.print("Saving score...");
      digitalWrite(statusPin, HIGH);
      EEPROM.put(0, score);
      e_score = score;  // <-- Update the in-memory best score
      digitalWrite(statusPin, LOW);
      lcd.clear();
      delay(300);
      lcd.setCursor(0, 0);
      lcd.print("Saved!");
    }

    lcd.setCursor(0, 1);
    lcd.print(String("Best: ") + String(e_score));

    delay(2000);

    softwareReset();  // software reset instead of hardware pin
  }

  // Move spikes
  unsigned long currentTime = millis();
  if (currentTime - lastSpikeMoveTime >= spikeMoveInterval * multiplier) {
    moveSpike(spike1, spike2, spike3, spike4);
    moveSpike(spike2, spike1, spike3, spike4);
    moveSpike(spike3, spike1, spike2, spike4);
    moveSpike(spike4, spike1, spike2, spike3);
    lastSpikeMoveTime = currentTime;
  }

  score++;

  render();
  delay(25);
}

void moveSpike(Spike& s, Spike& o1, Spike& o2, Spike& o3) {
  if (s.x < -1) {
    int minSpacing = 4;
    int newX;
    bool tooClose;
    int attempts = 0;
    const int maxAttempts = 20;  // More attempts, but capped

    do {
      newX = 16 + random(2, 8); // widen range a bit
      tooClose = abs(newX - o1.x) < minSpacing ||
                 abs(newX - o2.x) < minSpacing ||
                 abs(newX - o3.x) < minSpacing;
      attempts++;
      if (attempts >= maxAttempts) {
        // Give up on spacing constraint to avoid freeze
        tooClose = false;
      }
    } while (tooClose);

    s.x = newX;
    s.y = random(0, 2);
  }

  s.x--;
}


bool checkCollision(Player p, Spike s) {
  return (p.x == s.x && p.y == s.y);
}

void render() {
  lcd.clear();

  // Draw spikes
  drawSpike(spike1);
  drawSpike(spike2);
  drawSpike(spike3);
  drawSpike(spike4);

  // Draw player
  lcd.setCursor(player.x, player.y);
  lcd.write(byte(0));
}

void drawSpike(Spike s) {
  if (s.x >= 0 && s.x < 16) {
    lcd.setCursor(s.x, s.y);
    lcd.write(byte(1));
  }
}

void handleNextButton(int pin) {
  int reading = digitalRead(pin);
  if (reading != lastButtonStateNext) {
    lastDebounceTimeNext = millis();
  }
  if ((millis() - lastDebounceTimeNext) > debounceDelay) {
    if (reading != buttonStateNext) {
      buttonStateNext = reading;
      if (buttonStateNext == LOW && player.y == 0) {
        player.y++;
      }
    }
  }
  lastButtonStateNext = reading;
}

void handlePrevButton(int pin) {
  int reading = digitalRead(pin);
  if (reading != lastButtonStatePrev) {
    lastDebounceTimePrev = millis();
  }
  if ((millis() - lastDebounceTimePrev) > debounceDelay) {
    if (reading != buttonStatePrev) {
      buttonStatePrev = reading;
      if (buttonStatePrev == LOW && player.y == 1) {
        player.y--;
      }
    }
  }
  lastButtonStatePrev = reading;
}

void typewriterPrint(String message, int col, int row, int delayTime) {
  lcd.setCursor(col, row);
  for (int i = 0; message[i] != '\0'; i++) {
    lcd.print(message[i]);
    delay(delayTime);
  }
}

void scoreTracker() {
  score++;
  EEPROM.put(0, score);
}

// Software reset function
void softwareReset() {
  asm volatile ("jmp 0");
}
