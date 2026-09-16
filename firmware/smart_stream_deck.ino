/************ GENERIC MACRO PAD : OLED + ENCODER + 9 KEYS ************/
/*
   V3 — bigger auto-sized label font, improved Spotify/ChatGPT icons,
   and an idle screen that shows the PC's time + CPU/RAM usage instead
   of just turning the OLED off, once it's been idle a while.

   The idle info comes from your PC over the SAME USB cable, using the
   Arduino's separate USB-Serial (CDC) interface — this runs alongside
   HID-Project's keyboard/media features without conflict, it's just a
   second "channel" on the same USB connection.

   Format expected over Serial (9600 baud), one line, ending in '\n':
       HH:MM|CPU|RAM
   e.g.  14:23|37|61
   Sent automatically every ~2s by the updated macropad_launcher.ahk.

   If no data has ever arrived (script not running / not detected the
   COM port yet), it falls back to the old behaviour: OLED turns off
   after 30s idle, same as before. If data stops arriving for a while,
   (script closed), it also falls back to turning off, instead of
   showing a frozen, stale clock forever.

   Everything else (Ctrl+Alt+1..9 direct launch, encoder volume, mute,
   debouncing) is unchanged from the previous version.

   ------------------------------------------------------------
   Designed and Developed by Ankit
   ------------------------------------------------------------
*/

#include <HID-Project.h>              // HID Keyboard + Consumer (volume/media)
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* -------------------- OLED -------------------- */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
const uint8_t OLED_ADDR = 0x3C; // Adresse I2C (souvent 0x3C ou 0x3D)

/* -------------------- PINS -------------------- */
const uint8_t ENC_A  = A1;
const uint8_t ENC_B  = A0;
const uint8_t ENC_SW = A2;

const uint8_t KEY_PINS[9] = { 4, 14, 16, 5, 6, 7, 8, 9, 10 };

/* ---------------- Identifiants touches ---------------- */
enum KeyIds { K1, K2, K3, K4, K5, K6, K7, K8, K9 };
bool lastKeyState[9];

const char* KEY_LABELS[9] = {
  "ChatGPT",
  "Bambu Studio",
  "Spotify",
  "YouTube",
  "Chrome",
  "Claude",
  "Arduino",
  "File Explorer",
  "Onshape"
};

/* ---------- Encodeur ---------- */
int8_t transTable[16] = {0,-1,+1,0, +1,0,0,-1, -1,0,0,+1, 0,+1,-1,0};
uint8_t lastAB=0; int8_t accum=0;
const uint8_t STEPS_PER_DETENT=4;
unsigned long lastStepMs=0, lastClickMs=0;
const unsigned long STEP_COOLDOWN_MS=2, CLICK_DEBOUNCE_MS=200;

/* ---------- OLED veille ---------- */
unsigned long lastActionTime=0;
const unsigned long SLEEP_DELAY=30000; // 30 s d'inactivité avant veille/idle screen
bool isSleeping=false;

/* ---------- Volume simulé ---------- */
int currentVol=50; bool muted=false;

/* ---------- Throttle du redraw OLED (fix encodeur) ---------- */
unsigned long lastDisplayRefresh = 0;
const unsigned long DISPLAY_REFRESH_MS = 60;
bool volPending = false;

/* ---------- Animation d'icône (non-bloquante) ---------- */
const int ICON_W = 16;
const int ICON_H = 16;
const int ICON_X = 3;
const int ICON_Y_REST = (SCREEN_HEIGHT - ICON_H) / 2;   // 8
const int ICON_Y_START = -ICON_H - 2;
const int TEXT_X = ICON_X + ICON_W + 6;

bool animActive = false;
uint8_t animIconId = 0;
int animY = ICON_Y_START;
unsigned long animLastFrame = 0;
const unsigned long ANIM_FRAME_MS = 16;

/* ---------- Infos PC reçues en Serial (pour l'écran de veille) ---------- */
char serialBuffer[40];
uint8_t serialBufIdx = 0;
char idleTimeStr[8] = "";   // "HH:MM"
int idleCpu = -1, idleRam = -1;
bool hasIdleData = false;
unsigned long lastIdleDataMs = 0;
unsigned long lastIdleRefresh = 0;
const unsigned long IDLE_REFRESH_MS = 1000;        // rafraîchit l'écran d'idle 1x/s
const unsigned long IDLE_DATA_TIMEOUT_MS = 6000;   // si pas de data depuis 6s -> considère le PC-script arrêté

/* ---------------- PROTOTYPES ---------------- */
void showMessage(const char* msg);
void showVolume();
void maybeShowVolume();
void wakeUp();
void fireMacro(uint8_t id);
void sendAppLaunch(uint8_t id);
void startAppAnimation(uint8_t id);
void updateAppAnimation();
void renderAppFrame(uint8_t id, int iconY);
void drawAppIcon(uint8_t id, int x, int y);
void drawArc(int cx, int cy, int r, float startDeg, float endDeg);
uint8_t chooseLabelSize(const char* label);
int chooseLabelY(uint8_t size);
void readSerialIdleData();
void parseIdleLine(char* line);
void showIdleScreen();

/* ==================== SETUP ==================== */
void setup() {
  for (int i=0;i<9;i++){ pinMode(KEY_PINS[i], INPUT_PULLUP); lastKeyState[i]=digitalRead(KEY_PINS[i]); }
  pinMode(ENC_A,INPUT_PULLUP); pinMode(ENC_B,INPUT_PULLUP); pinMode(ENC_SW,INPUT_PULLUP);
  lastAB = (digitalRead(ENC_A)<<1) | digitalRead(ENC_B);

  Keyboard.begin();
  Consumer.begin();
  Serial.begin(9600); // canal séparé pour recevoir l'heure / CPU / RAM du PC

  Wire.begin();
  Wire.setClock(400000);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)){ for(;;); }
  display.clearDisplay(); display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0); display.println(F("Macropad Ready")); display.display();

  lastActionTime = millis();
  showVolume();
}

/* ==================== LOOP ==================== */
void loop() {
  readSerialIdleData();

  // --- 9 touches ---
  for (uint8_t i=0;i<9;i++){
    bool pressed = (digitalRead(KEY_PINS[i])==LOW);
    if (pressed && !lastKeyState[i]) { wakeUp(); fireMacro(i); }
    lastKeyState[i]=pressed;
  }

  updateAppAnimation();

  // --- Rotation encodeur -> Volume ---
  uint8_t nowAB = (digitalRead(ENC_A)<<1)|digitalRead(ENC_B);
  int8_t dir = transTable[(lastAB<<2)|nowAB];
  if (dir!=0){
    accum += dir; lastAB = nowAB;
    if (millis()-lastStepMs > STEP_COOLDOWN_MS){
      if (accum >= STEPS_PER_DETENT){
        wakeUp(); currentVol=min(100,currentVol+2); Consumer.write(MEDIA_VOLUME_UP);
        if (!animActive) maybeShowVolume();
        accum -= STEPS_PER_DETENT; lastStepMs=millis();
      } else if (accum <= -STEPS_PER_DETENT){
        wakeUp(); currentVol=max(0,currentVol-2); Consumer.write(MEDIA_VOLUME_DOWN);
        if (!animActive) maybeShowVolume();
        accum += STEPS_PER_DETENT; lastStepMs=millis();
      }
    }
  } else lastAB=nowAB;

  if (!animActive && volPending && (millis()-lastStepMs > DISPLAY_REFRESH_MS)){
    showVolume(); lastDisplayRefresh=millis(); volPending=false;
  }

  // --- Clic bouton encodeur -> Mute ---
  if (digitalRead(ENC_SW)==LOW && (millis()-lastClickMs>CLICK_DEBOUNCE_MS)){
    lastClickMs=millis(); wakeUp(); muted=!muted; Consumer.write(MEDIA_VOLUME_MUTE);
    animActive=false;
    showMessage(muted?"MUTE":"UNMUTE"); showVolume();
  }

  // --- Veille / écran d'idle ---
  if (!isSleeping && !animActive && (millis()-lastActionTime>SLEEP_DELAY)){
    bool dataFresh = hasIdleData && (millis()-lastIdleDataMs < IDLE_DATA_TIMEOUT_MS);
    if (dataFresh){
      if (millis()-lastIdleRefresh >= IDLE_REFRESH_MS){
        showIdleScreen();
        lastIdleRefresh = millis();
      }
    } else {
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      isSleeping=true;
    }
  }
}

/* ==================== FONCTIONS ==================== */
void fireMacro(uint8_t id){
  startAppAnimation(id);
  sendAppLaunch(id);
}

void sendAppLaunch(uint8_t id){
  KeyboardKeycode digitKey = (KeyboardKeycode)(KEY_1 + id);
  Keyboard.press(KEY_LEFT_CTRL);
  Keyboard.press(KEY_LEFT_ALT);
  Keyboard.press(digitKey);
  delay(30);
  Keyboard.releaseAll();
}

/* -------- Lecture non-bloquante du Serial (heure/CPU/RAM du PC) -------- */
void readSerialIdleData(){
  while (Serial.available()){
    char c = Serial.read();
    if (c == '\n'){
      serialBuffer[serialBufIdx] = '\0';
      parseIdleLine(serialBuffer);
      serialBufIdx = 0;
    } else if (c != '\r'){
      if (serialBufIdx < sizeof(serialBuffer)-1){
        serialBuffer[serialBufIdx++] = c;
      } else {
        serialBufIdx = 0; // ligne trop longue / corrompue -> on l'ignore
      }
    }
  }
}

// Format attendu : "HH:MM|CPU|RAM"  (ex: "14:23|37|61")
void parseIdleLine(char* line){
  char* p1 = strchr(line, '|');
  if (!p1) return;
  *p1 = '\0';
  char* p2 = strchr(p1+1, '|');
  if (!p2) return;
  *p2 = '\0';

  strncpy(idleTimeStr, line, sizeof(idleTimeStr)-1);
  idleTimeStr[sizeof(idleTimeStr)-1] = '\0';
  idleCpu = atoi(p1+1);
  idleRam = atoi(p2+1);
  hasIdleData = true;
  lastIdleDataMs = millis();
}

void showIdleScreen(){
  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(28, 0);
  display.print(idleTimeStr);

  display.setTextSize(1);
  display.setCursor(4, 22);
  display.print(F("CPU "));
  display.print(idleCpu);
  display.print(F("%   RAM "));
  display.print(idleRam);
  display.print(F("%"));

  display.display();
  // Ne touche PAS lastActionTime/isSleeping ici : c'est un affichage
  // "en arrière-plan" pendant l'idle, pas une action utilisateur.
}

/* -------- Animation : l'icône tombe du haut et se pose -------- */
void startAppAnimation(uint8_t id){
  animActive = true;
  animIconId = id;
  animY = ICON_Y_START;
  animLastFrame = millis();
  renderAppFrame(id, animY);
}

void updateAppAnimation(){
  if (!animActive) return;
  if (millis() - animLastFrame < ANIM_FRAME_MS) return;
  animLastFrame = millis();

  int remaining = ICON_Y_REST - animY;
  if (abs(remaining) <= 1) {
    animY = ICON_Y_REST;
    animActive = false;
  } else {
    int step = remaining / 3;
    if (step == 0) step = (remaining > 0) ? 1 : -1;
    animY += step;
  }
  renderAppFrame(animIconId, animY);
}

// Choisit une police plus grande (taille 2) quand le nom tient dans la
// largeur restante, sinon retombe sur la taille 1 pour ne pas déborder.
uint8_t chooseLabelSize(const char* label){
  return (strlen(label) <= 8) ? 2 : 1;
}

int chooseLabelY(uint8_t size){
  return (size == 2) ? (SCREEN_HEIGHT - 16) / 2 : (SCREEN_HEIGHT - 8) / 2 + 1;
}

void renderAppFrame(uint8_t id, int iconY){
  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();

  const char* label = KEY_LABELS[id];
  uint8_t sz = chooseLabelSize(label);
  display.setTextSize(sz);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(TEXT_X, chooseLabelY(sz));
  display.println(label);

  drawAppIcon(id, ICON_X, iconY);
  display.display();
  lastActionTime = millis();
  isSleeping = false;
}

// Petit utilitaire pour dessiner un arc de cercle (Adafruit_GFX n'en a pas
// nativement) — utilisé pour l'icône Spotify.
void drawArc(int cx, int cy, int r, float startDeg, float endDeg){
  const int STEPS = 8;
  float startRad = startDeg * PI / 180.0;
  float endRad   = endDeg   * PI / 180.0;
  int px = cx + (int)(r * cos(startRad));
  int py = cy + (int)(r * sin(startRad));
  for (int i = 1; i <= STEPS; i++){
    float t = startRad + (endRad - startRad) * i / STEPS;
    int nx = cx + (int)(r * cos(t));
    int ny = cy + (int)(r * sin(t));
    display.drawLine(px, py, nx, ny, SSD1306_WHITE);
    px = nx; py = ny;
  }
}

/* -------- Icônes génériques 16x16 dessinées avec Adafruit_GFX -------- */
void drawAppIcon(uint8_t id, int x, int y){
  switch (id) {

    case K1: { // ChatGPT -> bulle de discussion + étincelle "IA"
      display.drawRoundRect(x, y, 14, 10, 3, SSD1306_WHITE);
      display.drawLine(x+3, y+9, x+1, y+14, SSD1306_WHITE);
      display.drawLine(x+4, y+9, x+3, y+13, SSD1306_WHITE);
      int scx = x+7, scy = y+5;
      display.drawLine(scx-3, scy,   scx+3, scy,   SSD1306_WHITE);
      display.drawLine(scx,   scy-2, scx,   scy+2, SSD1306_WHITE);
      display.drawLine(scx-2, scy-2, scx+2, scy+2, SSD1306_WHITE);
      display.drawLine(scx-2, scy+2, scx+2, scy-2, SSD1306_WHITE);
      break;
    }

    case K2: { // Bambu Studio -> cube filaire (impression 3D)
      display.drawLine(x+5, y+0,  x+11, y+3,  SSD1306_WHITE);
      display.drawLine(x+11,y+3,  x+5,  y+6,  SSD1306_WHITE);
      display.drawLine(x+5, y+6,  x+0,  y+3,  SSD1306_WHITE);
      display.drawLine(x+0, y+3,  x+5,  y+0,  SSD1306_WHITE);
      display.drawLine(x+5, y+6,  x+5,  y+13, SSD1306_WHITE);
      display.drawLine(x+0, y+3,  x+0,  y+10, SSD1306_WHITE);
      display.drawLine(x+11,y+3,  x+11, y+10, SSD1306_WHITE);
      display.drawLine(x+5, y+13, x+11, y+10, SSD1306_WHITE);
      display.drawLine(x+5, y+13, x+0,  y+10, SSD1306_WHITE);
      break;
    }

    case K3: { // Spotify -> cercle + 3 ondes sonores (arcs)
      display.drawCircle(x+8, y+8, 8, SSD1306_WHITE);
      display.fillCircle(x+4, y+11, 1, SSD1306_WHITE);
      drawArc(x+4, y+11, 4, -70, 10);
      drawArc(x+4, y+11, 6, -70, 10);
      drawArc(x+4, y+11, 8, -70, 10);
      break;
    }

    case K4: { // YouTube -> rectangle arrondi + triangle play
      display.drawRoundRect(x, y+2, 16, 11, 3, SSD1306_WHITE);
      display.fillTriangle(x+5, y+4, x+5, y+11, x+12, y+7, SSD1306_WHITE);
      break;
    }

    case K5: { // Chrome -> cercle type "globe/navigateur"
      display.drawCircle(x+7, y+7, 7, SSD1306_WHITE);
      display.drawFastHLine(x+0, y+7, 15, SSD1306_WHITE);
      display.fillCircle(x+7, y+7, 2, SSD1306_WHITE);
      break;
    }

    case K6: { // Claude -> star/sunburst (generic "AI assistant" icon)
      int scx = x+7, scy = y+7;
      display.drawLine(scx, scy-7, scx, scy+7, SSD1306_WHITE); // vertical
      display.drawLine(scx-7, scy, scx+7, scy, SSD1306_WHITE); // horizontal
      display.drawLine(scx-5, scy-5, scx+5, scy+5, SSD1306_WHITE); // diagonal
      display.drawLine(scx-5, scy+5, scx+5, scy-5, SSD1306_WHITE); // diagonal
      display.fillCircle(scx, scy, 2, SSD1306_WHITE); // centre
      break;
    }

    case K7: { // Arduino IDE -> puce électronique
      display.drawRect(x+3, y+3, 10, 10, SSD1306_WHITE);
      display.drawFastVLine(x+5,  y+0, 3, SSD1306_WHITE);
      display.drawFastVLine(x+8,  y+0, 3, SSD1306_WHITE);
      display.drawFastVLine(x+11, y+0, 3, SSD1306_WHITE);
      display.drawFastVLine(x+5,  y+13, 3, SSD1306_WHITE);
      display.drawFastVLine(x+8,  y+13, 3, SSD1306_WHITE);
      display.drawFastVLine(x+11, y+13, 3, SSD1306_WHITE);
      break;
    }

    case K8: { // File Explorer -> dossier
      display.drawRect(x+1, y+5, 14, 9, SSD1306_WHITE);
      display.drawRect(x+1, y+3, 6, 3, SSD1306_WHITE);
      break;
    }

    case K9: { // Onshape -> repère d'axes 3D (icône CAO)
      int cx = x+8, cy = y+14;
      display.drawLine(cx, cy, cx,   cy-13, SSD1306_WHITE);
      display.drawLine(cx, cy, cx+6, cy-3,  SSD1306_WHITE);
      display.drawLine(cx, cy, cx-6, cy-3,  SSD1306_WHITE);
      display.drawLine(cx, cy-13, cx-2, cy-11, SSD1306_WHITE);
      display.drawLine(cx, cy-13, cx+2, cy-11, SSD1306_WHITE);
      break;
    }
  }
}

void showMessage(const char* msg){
  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay(); display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,8); display.println(msg); display.display();
  lastActionTime=millis(); isSleeping=false;
}

void maybeShowVolume(){
  volPending = true;
  if (millis()-lastDisplayRefresh >= DISPLAY_REFRESH_MS){
    showVolume();
    lastDisplayRefresh = millis();
    volPending = false;
  }
}

void showVolume(){
  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay(); display.setTextSize(1); display.setTextColor(SSD1306_WHITE); display.setCursor(0,0);
  if (muted) display.println(F("MUTE"));
  else { display.print(F("Volume: ")); display.print(currentVol); display.println('%'); }
  int barW = map(currentVol, 0, 100, 0, SCREEN_WIDTH-10);
  display.drawRect(5,16,SCREEN_WIDTH-10,10,SSD1306_WHITE);
  if (!muted && barW>0) display.fillRect(5,16,barW,10,SSD1306_WHITE);
  display.display(); lastActionTime=millis(); isSleeping=false;
}

void wakeUp(){ if(isSleeping){ display.ssd1306_command(SSD1306_DISPLAYON); isSleeping=false; } lastActionTime=millis(); }
