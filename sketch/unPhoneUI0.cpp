// unPhoneUI0.cpp ////////////////////////////////////////////////////////////
// agglomerated default UI based on Adafruit's GFX and
// mostly derived from HarryEH's code
// thanks!

// UIController.cpp //////////////////////////////////////////////////////////

#if UNPHONE_UI0 == 1
#include "unPhoneUI0.h"
#include <Adafruit_ImageReader.h> // image-reading for test card scrn
#include <vector>

static unPhone *up;

// initialisation flag, not complete until parent has finished config
bool UIController::provisioned = false;

// the UI elements types (screens) /////////////////////////////////////////
const char *UIController::ui_mode_names[] = {
  "Menu",
  "Space Invaders",
  "Home",
};
uint8_t UIController::NUM_UI_ELEMENTS = 3;  // number of UI elements

// keep Arduino IDE compiler happy /////////////////////////////////////////
UIElement::UIElement(Adafruit_HX8357* tftp, XPT2046_Touchscreen* tsp, SdFat *sdp) {
  m_tft = tftp;
  m_ts = tsp;
  m_sd = sdp;
}
void UIElement::someFuncDummy() { }

// constructor for the main class ///////////////////////////////////////////
UIController::UIController(ui_modes_t start_mode) {
  m_mode = start_mode;
}

bool UIController::begin(unPhone& u) { ///////////////////////////////////////
  up = &u;
  begin(u, true);
  return true;
}
bool UIController::begin(unPhone& u, boolean doDraw) { ///////////////////////
  up = &u;
  D("UI.begin()\n")

  up->tftp->fillScreen(HX8357_GREEN);
  WAIT_MS(50)
  up->tftp->fillScreen(HX8357_BLACK);
  
  // define the menu element and the first m_element here 
  m_menu = new MenuUIElement(up->tftp, up->tsp, up->sdp);
  if(m_menu == NULL) {
    Serial.println("ERROR: no m_menu allocated");
    return false;
  }
  allocateUIElement(m_mode);

  if(doDraw)
    redraw();
  return true;
}

UIElement* UIController::allocateUIElement(ui_modes_t newMode) {
  // TODO trying to save memory here, but at the expense of possible
  // fragmentation; perhaps maintain an array of elements and never delete?
  if(m_element != 0 && m_element != m_menu) delete(m_element);

  switch(newMode) {
    case ui_menu:
      m_element = m_menu;                                               break;
    case ui_configure:
      m_element = new ConfigUIElement(up->tftp, up->tsp, up->sdp);      break;
     case ui_spaceinvaders:
       m_element = new SpaceInvadersUIElement(up->tftp, up->tsp, up->sdp); break;
    default:
      Serial.printf("invalid UI mode %d in allocateUIElement\n", newMode);
      m_element = m_menu;
  }

  return m_element;
}

// touch management code ////////////////////////////////////////////////////
TS_Point nowhere(-1, -1, -1);    // undefined coordinate
TS_Point firstTouch(0, 0, 0);    // the first touch defaults to 0,0,0
TS_Point p(-1, -1, -1);          // current point of interest (signal)
TS_Point prevSig(-1, -1, -1);    // the previous accepted touch signal
bool firstTimeThrough = true;    // first time through gotTouch() flag
uint16_t fromPrevSig = 0;        // distance from previous signal
unsigned long now = 0;           // millis
unsigned long prevSigMillis = 0; // previous signal acceptance time
unsigned long sincePrevSig = 0;  // time since previous signal acceptance
uint16_t DEFAULT_TIME_SENSITIVITY = 150; // min millis between touches
uint16_t TIME_SENSITIVITY = DEFAULT_TIME_SENSITIVITY;
uint16_t DEFAULT_DIST_SENSITIVITY = 200; // min distance between touches
uint16_t DIST_SENSITIVITY = DEFAULT_DIST_SENSITIVITY;
uint16_t TREAT_AS_NEW = 600;     // if no signal in this period treat as new
uint8_t MODE_CHANGE_TOUCHES = 1; // number of requests needed to switch mode
uint8_t modeChangeRequests = 0;  // number of current requests to switch mode
bool touchDBG = false;           // set true for diagnostics

void setTimeSensitivity(uint16_t s = DEFAULT_TIME_SENSITIVITY) { ////////////
  TIME_SENSITIVITY = s;
}
void setDistSensitivity(uint16_t d = DEFAULT_DIST_SENSITIVITY) { ////////////
  DIST_SENSITIVITY = d;
}
uint16_t distanceBetween(TS_Point a, TS_Point b) { // coord distance ////////
  uint32_t xpart = b.x - a.x, ypart = b.y - a.y;
  xpart *= xpart; ypart *= ypart;
  return sqrt(xpart + ypart);
}
void dbgTouch() { // print current state of touch model /////////////////////
  if(touchDBG) {
    D("p(x:%04d,y:%04d,z:%03d)", p.x, p.y, p.z)
    D(", now=%05lu, sincePrevSig=%05lu, prevSig=", now, sincePrevSig)
    D("p(x:%04d,y:%04d,z:%03d)", prevSig.x, prevSig.y, prevSig.z)
    D(", prevSigMillis=%05lu, fromPrevSig=%05u", prevSigMillis, fromPrevSig)
  }
}
const char *UIController::modeName(ui_modes_t m) {
  switch(m) {
    case ui_menu:               return "ui_menu";          break;
    case ui_configure:          return "ui_configure";     break;
    case ui_spaceinvaders:      return "ui_spaceinvaders";   break;
    default:
      D("invalid UI mode %d in allocateUIElement\n", m)
      return "invalid UI mode";
  }
}

// accept or reject touch signals ///////////////////////////////////////////
bool UIController::gotTouch() { 
  if(!up->tsp->touched()) {
    return false; // no touches
  }
    
  // set up timings
  now = millis();
  if(firstTimeThrough) {
    sincePrevSig = TIME_SENSITIVITY + 1;
  } else {
    sincePrevSig = now - prevSigMillis;
  }

  // retrieve a point
  p = up->tsp->getPoint();
  // add the following if want to dump the rest of the buffer:
  // while (! up->tsp->bufferEmpty()) {
  //   uint16_t x, y;
  //   uint8_t z;
  //   up->tsp->readData(&x, &y, &z);
  // }
  // delay(300);
  if(touchDBG)
    D("\n\np(x:%04d,y:%04d,z:%03d)\n\n", p.x, p.y, p.z)

  // if it is at 0,0,0 and we've just started then ignore it
  if(p == firstTouch && firstTimeThrough) {
    dbgTouch();
    if(touchDBG) D(", rejecting (0)\n\n")
    return false;
  }
  firstTimeThrough = false;
  
  // calculate distance from previous signal
  fromPrevSig = distanceBetween(p, prevSig);
  dbgTouch();

  if(touchDBG)
    D(", sincePrevSig<TIME_SENS.: %d...  ", sincePrevSig<TIME_SENSITIVITY)
  if(sincePrevSig < TIME_SENSITIVITY) { // ignore touches too recent
    if(touchDBG) D("rejecting (2)\n")
  } else if(
    fromPrevSig < DIST_SENSITIVITY && sincePrevSig < TREAT_AS_NEW
  ) {
    if(touchDBG) D("rejecting (3)\n")
#if UNPHONE_SPIN >= 9
  } else if(p.z < 400) { // ghost touches in 9 (on USB power) are ~300 pressure
    // or ignore: x > 1200 && x < 1700 && y > 2000 && y < 3000 && z < 450 ?
    if(touchDBG) D("rejecting (4)\n") // e.g. p(x:1703,y:2411,z:320)
#endif
  } else {
    prevSig = p;
    prevSigMillis = now;
    if(false) // delete this line to debug touch debounce
      D("decided this is a new touch: p(x:%04d,y:%04d,z:%03d)\n", p.x, p.y, p.z)
    return true;
  }
  return false;
}

/////////////////////////////////////////////////////////////////////////////
void UIController::changeMode() {
  D("changing mode from %d (%s) to...", m_mode, modeName(m_mode))
  up->tftp->fillScreen(HX8357_BLACK);
  setTimeSensitivity();         // set TIME_SENS to the default
  nextMode = (ui_modes_t) ((MenuUIElement *)m_menu)->getMenuItemSelected();
  if(nextMode == -1) nextMode = ui_menu;

  // allocate an element according to nextMode and 
  if(m_mode == ui_menu) {       // coming OUT of menu
    // if(nextMode == ui_touchpaint)
    //   setTimeSensitivity(25);   // TODO make class member and move to TPUIE
    m_mode =    nextMode;
    m_element = allocateUIElement(nextMode);
  } else {                      // going INTO menu
    m_mode =    ui_menu;
    m_element = m_menu;
  }
  D("...%d (%s)\n", m_mode, modeName(m_mode))

  redraw();
  return;
}

/////////////////////////////////////////////////////////////////////////////
void UIController::handleTouch() {
  int temp = p.x;
  p.x = map(p.y, up->TS_MAXX, up->TS_MINX, 0, up->tftp->width());
  p.y = map(temp, up->TS_MAXY, up->TS_MINY, 0, up->tftp->height());
  // Serial.print("dbgTouch from handleTouch: "); dbgTouch(); Serial.flush();
  
  if(m_element->handleTouch(p.x, p.y)) {
    if(++modeChangeRequests >= MODE_CHANGE_TOUCHES) {
      changeMode();
      modeChangeRequests = 0;
    }
  } 
}

/////////////////////////////////////////////////////////////////////////////
void UIController::run() {
  if(gotTouch())
    handleTouch();
  m_element->runEachTurn();
}

////////////////////////////////////////////////////////////////////////////
void UIController::redraw() {
  up->tftp->fillScreen(HX8357_BLACK);
  m_element->draw();
}

////////////////////////////////////////////////////////////////////////////
void UIController::message(char *s) {
  up->tftp->setCursor(0, 465);
  up->tftp->setTextSize(2);
  up->tftp->setTextColor(HX8357_CYAN, HX8357_BLACK);
  up->tftp->print("                          ");
  up->tftp->setCursor(0, 465);
  up->tftp->print(s);
}

////////////////////////////////////////////////////////////////////////////
void UIElement::drawSwitcher(uint16_t xOrigin, uint16_t yOrigin) {
  uint16_t leftX = xOrigin;
  if(leftX == 0)
    leftX = (SWITCHER * BOXSIZE) + 8; // default is on right hand side
    m_tft->fillRect(leftX, 15 + yOrigin, BOXSIZE - 15, HALFBOX - 10, WHITE);
    m_tft->fillTriangle(
      leftX + 15, 35 + yOrigin,
      leftX + 15,  5 + yOrigin,
      leftX + 30, 20 + yOrigin,
      WHITE
    );
}

////////////////////////////////////////////////////////////////////////////
void UIElement::showLine(const char *buf, uint16_t *yCursor) {
  *yCursor += 20;
  m_tft->setCursor(0, *yCursor);
  m_tft->print(buf);
}


//////////////////////////////////////////////////////////////////////////////
// ConfigUIElement.cpp ///////////////////////////////////////////////////////

/**
 * Handle touches on this page
 * 
 * @param x - the x coordinate of the touch 
 * @param y - the y coordinate of the touch 
 * @returns bool - true if the touch is on the switcher
 */
bool ConfigUIElement::handleTouch(long x, long y) {
  return y < BOXSIZE && x > (BOXSIZE * SWITCHER);
}

// writes various things including mac address and wifi ssid ///////////////
void ConfigUIElement::draw() {
  // say hello
  m_tft->setTextColor(GREEN);
  m_tft->setTextSize(2);
  uint16_t yCursor = 0;
  m_tft->setCursor(0, yCursor);
  m_tft->print("Welcome to unPhone!");
  m_tft->setTextColor(YELLOW);

  // note about switcher
  yCursor += 20;
  if(UIController::provisioned) {
    showLine("(where you see the arrow,", &yCursor);
    showLine("  press for menu)", &yCursor);
    drawSwitcher();
  } else {
    yCursor += 20;
  }

  // are we connected?
  yCursor += 40;
  m_tft->setCursor(0, yCursor);

  // display the mac address
  char mac_buf[13];
  yCursor += 40;
  m_tft->setCursor(0, yCursor);
  m_tft->print("MAC addr: ");
  m_tft->print(up->getMAC());

  // firmware version
  showLine("Firmware date:", &yCursor);
  showLine("  ", &yCursor);
  m_tft->print(up->buildTime);

  // (used to be) AP details, now just unPhone.name
  showLine("Firmware name: ", &yCursor);
  showLine("  ", &yCursor);
  m_tft->print(up->appName);
  //m_tft->print("-");
  //m_tft->print(up->getMAC());

  // battery voltage
  showLine("VBAT: ", &yCursor);
  m_tft->print(up->batteryVoltage());

  // battery voltage
  showLine("Hardware version: ", &yCursor);
  m_tft->print(UNPHONE_SPIN);

  // display the on-board temperature
  char buf[256];
  float onBoardTemp = temperatureRead();
  sprintf(buf, "MCU temp: %.2f C", onBoardTemp);
  showLine(buf, &yCursor);

  // web link
  yCursor += 60;
  m_tft->setTextColor(BLUE);
  showLine("An ", &yCursor);
  m_tft->setTextColor(MAGENTA);
  m_tft->print("IoT platform");
  m_tft->setTextColor(BLUE);
  m_tft->print(" from the");
  m_tft->setTextColor(MAGENTA);
  showLine("  University of Sheffield", &yCursor);
  m_tft->setTextColor(BLUE);
  showLine("Find out more at", &yCursor);
  m_tft->setTextColor(GREEN);
  showLine("              unphone.net", &yCursor);
}

//////////////////////////////////////////////////////////////////////////////
void ConfigUIElement::runEachTurn() {
  
}


//////////////////////////////////////////////////////////////////////////////
// MenuUIElement.cpp /////////////////////////////////////////////////////////

/**
 * Process touches.
 * @returns bool - true if the touch is a menu item
 */
bool MenuUIElement::handleTouch(long x, long y) {
  // D("text mode: responding to touch @ %d/%d/%d: ", x, y,-1)
  m_tft->setTextColor(WHITE, BLACK);
  int8_t menuItem = mapTextTouch(x, y);
  D("menuItem=%d, ", menuItem)
  if(menuItem == -1) D("ignoring\n")

  if(menuItem > 0 && menuItem < UIController::NUM_UI_ELEMENTS) {
    menuItemSelected = menuItem;
    return true;
  }
  return false;
}

// returns menu item number //////////////////////////////////////////////////
int8_t MenuUIElement::mapTextTouch(long xInput, long yInput) {
  for(
    int y = 30, i = 1;
    i < UIController::NUM_UI_ELEMENTS && y < 480;
    y += 48, i++
  ) {
    if(xInput > 270 && yInput > y && yInput < y + 48)
      return i;
  }
  return -1;
}

// draw a textual menu ///////////////////////////////////////////////////////
void MenuUIElement::draw(){
  m_tft->setTextSize(2);
  m_tft->setTextColor(BLUE);

  m_tft->setCursor(230, 0);
  m_tft->print("MENU");

  uint16_t yCursor = 30;
  m_tft->drawFastHLine(0, yCursor, 320, MAGENTA);
  yCursor += 16;

  for(int i = 1; i < UIController::NUM_UI_ELEMENTS; i++) {
    m_tft->setCursor(0, yCursor);
    m_tft->print(UIController::ui_mode_names[i]);
    drawSwitcher(288, yCursor - 12);
    yCursor += 32;
    m_tft->drawFastHLine(0, yCursor, 320, MAGENTA);
    yCursor += 16;
  }
}

//////////////////////////////////////////////////////////////////////////////
void MenuUIElement::runEachTurn(){ // text page UI, run each turn
  // do nothing
}

// //////////////////////////////////////////////////////////////////////////////
// // Space Invaders //////////////////////////////////////////////////

/// Screen edges
#define X_MIN 5
#define X_MAX 290
#define Y_MIN 5
#define Y_MAX 450

#define NUM_STARS 100
#define STAR_SPEED 3
#define STAR_COLOR HX8357_WHITE

/// Enemy
#define MAX_ENEMIES 13
#define ENEMY_PROJECTILE_SPEED 7
#define ENEMY_PROJECTILE_SIZE 4
#define ENEMY_SIZE 35
unsigned long movementInterval = 1000;
const int MAX_POSITIONS = 10;
unsigned int enemyRows = 1;
unsigned int waveCounter = 1;

/// formations for enemies
const int formation1[MAX_POSITIONS] = {0, 0, 1, 1, 1, 1, 1, 1, 0, 0};
const int formation2[MAX_POSITIONS] = {0, 0, 1, 0, 1, 1, 0, 1, 0, 0};
const int formation3[MAX_POSITIONS] = {0, 0, 1, 0, 1, 0, 1, 0, 1, 0};
const int formation4[MAX_POSITIONS] = {0, 1, 1, 1, 0, 0, 1, 1, 1, 0};
const int formation5[MAX_POSITIONS] = {0, 1, 1, 0, 1, 1, 0, 1, 1, 0};
const int formation6[MAX_POSITIONS] = {0, 0, 0, 0, 1, 1, 1, 0, 0, 0};
const int formation7[MAX_POSITIONS] = {0, 0, 1, 0, 1, 0, 1, 0, 1, 0};


/// Array of pointers to formations
const int* formations[] = {formation1, formation2, formation3, formation4, formation5, formation6, formation7};
const int TOTAL_FORMATIONS = sizeof(formations) / sizeof(formations[0]);

/// Define player colors
#define PLAYER_COLOR1 HX8357_BLUE 
#define PLAYER_COLOR2 HX8357_RED 
#define PLAYER_COLOR3 HX8357_WHITE 

// Player coords and score
#define XMID 160
#define YMID 400
unsigned int score = 0;

// Player projectile attributes
#define PROJECTILE_SPEED 7
#define PROJECTILE_SIZE 4
#define MAX_PROJECTILES 1
#define ENEMY_MAX_PROJECTILES 10

/// Structure to represent a star
struct Star {
    int x;
    int y;
    int speed;
    bool active;
};

Star stars[NUM_STARS];

/// Structure to represent the player
struct Player { 
    int x;
    int y;
    int width;
    int height;
};

Player player;

/// Structure to represent an enemy
struct Enemy {
    int x;
    int y;
    bool active;
    int xDirection;
};
std::vector<Enemy> enemies; // Declare enemies as a vector

/// Structure to represent an enemy projectile
struct EnemyProjectile {
    int x;
    int y;
    bool active;
};

EnemyProjectile enemyProjectiles[ENEMY_MAX_PROJECTILES];

/// Define projectile struct
struct Projectile {
  int x;
  int y;
  bool active;
};

Projectile projectiles[MAX_PROJECTILES];

/**
 * @brief Draws the score on the screen.
 */
void SpaceInvadersUIElement::drawScore() {
    m_tft->fillRect(20, 10, 140, 20, BLACK);

    m_tft->setTextSize(2);
    m_tft->setTextColor(WHITE);
    m_tft->setCursor(20, 10);
    m_tft->print("Score: ");
    m_tft->print(score);
}

/**
 * @brief Resets the score to zero.
 */
void SpaceInvadersUIElement::resetScore() {
    score = 0;
}

/**
 * @brief Increments the score by one.
 */
void SpaceInvadersUIElement::updateScore() {
    score++;
}

/**
 * @brief Draws borders around the screen.
 */
void SpaceInvadersUIElement::drawBorders(){
  m_tft->drawLine(0, 0, 319, 0, BLUE);
  m_tft->drawLine(319, 0, 319, 479, BLUE);
  m_tft->drawLine(319, 479, 0, 479, BLUE);
  m_tft->drawLine(0, 479, 0, 0, BLUE);
}

/**
 * @brief Initializes the stars in the game.
 */
void SpaceInvadersUIElement::initialiseStars() {
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = random(0, m_tft->width());
        stars[i].y = random(0, m_tft->height());
        stars[i].speed = STAR_SPEED;
        stars[i].active = true;

    }
}

/**
 * @brief Draws the stars on the screen.
 */
void SpaceInvadersUIElement::drawStars() {
    for (int i = 0; i < NUM_STARS; i++) {
        m_tft->drawPixel(stars[i].x, stars[i].y, STAR_COLOR);
    }
}

/**
 * @brief Moves the stars downward and wraps them around when they go off-screen.
 */
void SpaceInvadersUIElement::moveStars() {
    for (int i = 0; i < NUM_STARS; i++) {
        // Clear the previous position of the star
        m_tft->drawPixel(stars[i].x, stars[i].y, BLACK);
        
        // Move the star downward
        stars[i].y += STAR_SPEED;

        // Wrap around if star goes below the screen
        if (stars[i].y >= 500) {
            stars[i].y = random(-20, 0);
            stars[i].x = random(0, m_tft->width());
        }
        
        // Draw the star at its new position
        m_tft->drawPixel(stars[i].x, stars[i].y, STAR_COLOR);
    }
}

/**
 * @brief Initializes the player in the game.
 */
void SpaceInvadersUIElement::initialisePlayer() {
    player.x = XMID - 10;
    player.y = YMID - 10;
    player.width = 20;
    player.height = 20;
}

/**
 * @brief Draws the player on the screen.
 */
void SpaceInvadersUIElement::drawPlayer() {
    // Draw main body of the spaceship
    m_tft->fillRect(player.x + player.width / 4, player.y, player.width / 2, player.height / 2, PLAYER_COLOR1);
    
    // Draw wings
    m_tft->fillRect(player.x, player.y + player.height / 4, player.width / 4, player.height / 2, PLAYER_COLOR2);
    m_tft->fillRect(player.x + player.width - player.width / 4, player.y + player.height / 4, player.width / 4, player.height / 2, PLAYER_COLOR2);
    
    // Draw cockpit
    m_tft->fillRect(player.x + player.width / 4, player.y - player.height / 4, player.width / 2, player.height / 4, PLAYER_COLOR3);
}

/**
 * @brief Initializes the projectile array.
 */
void SpaceInvadersUIElement::initialiseProjectiles() {
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    projectiles[i].active = false;
  }
}

/**
 * @brief Shoots a projectile from the player's current position.
 */
void SpaceInvadersUIElement::shootProjectile() {
  // Find an available slot in the projectiles array
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    if (!projectiles[i].active) {
      // Create a new projectile
      projectiles[i].x = player.x;
      projectiles[i].y = player.y-5;
      projectiles[i].active = true;
      break;
    }
  }
}

/**
 * @brief Updates the position of active projectiles and deactivates those that move off-screen.
 */
void SpaceInvadersUIElement::updateProjectiles() {
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    if (projectiles[i].active) {
      // Move projectile upward
      m_tft->fillCircle(projectiles[i].x, projectiles[i].y, PROJECTILE_SIZE, BLACK);
      projectiles[i].y -= PROJECTILE_SPEED;
      // Check if projectile is off screen
      if (projectiles[i].y < 0) {
        // Deactivate the projectile
        projectiles[i].active = false;
      } else {
        // Draw the projectile
        m_tft->fillCircle(projectiles[i].x, projectiles[i].y, PROJECTILE_SIZE, YELLOW);      
      }
    }
  }
}

/**
 * @brief Initializes the enemies in the game.
 * Clears the current enemies and sets up new ones based on random formations.
 */
void SpaceInvadersUIElement::initialiseEnemies() {
    enemies.clear();

    // Randomly select the number of rows (up to enemyRows) and select random formations for these rows
    int numRows = enemyRows;
    const int* selectedFormations[numRows];

    for (int i = 0; i < numRows; i++) {
        selectedFormations[i] = formations[random(0, TOTAL_FORMATIONS)];
    }

    // Calculate row height
    int rowHeight = ENEMY_SIZE + 5; // Adjust as needed
    int wallPadding = 20; // Padding from the wall

    for (int row = 0; row < numRows; row++) {
        const int* currentFormation = selectedFormations[row];

        // Calculate total horizontal space required by enemies
        int enemiesInThisRow = MAX_POSITIONS;
        int totalEnemyWidth = enemiesInThisRow * ENEMY_SIZE;
        int totalSpacing = std::min(8, (m_tft->width() - totalEnemyWidth) / (enemiesInThisRow - 1));

        for (int i = 0; i < enemiesInThisRow; i++) {
            if (currentFormation[i] == 1) { // Only create enemy if formation indicates
                Enemy newEnemy;
                newEnemy.x = wallPadding + i * (ENEMY_SIZE + totalSpacing);
                newEnemy.y = (row * rowHeight + rowHeight / 2) + 20;
                newEnemy.active = true;
                newEnemy.xDirection = 1;
                enemies.push_back(newEnemy);
            }
        }
    }
}

/**
 * @brief Draws the enemies on the screen.
 */
void SpaceInvadersUIElement::drawEnemies() {
    for (const auto& enemy : enemies) {
        if (enemy.active) {
            // Draw alien body
            m_tft->fillRect(enemy.x - 15, enemy.y - 5, 30, 10, RED);
            m_tft->fillRect(enemy.x - 10, enemy.y + 5, 20, 10, RED);

            // Draw eyes
            m_tft->fillCircle(enemy.x - 5, enemy.y - 3, 2, WHITE);
            m_tft->fillCircle(enemy.x + 5, enemy.y - 3, 2, WHITE);

            // Draw mouth
            m_tft->drawLine(enemy.x - 3, enemy.y + 4, enemy.x + 3, enemy.y + 4, WHITE);
        }
    }
}

/**
 * @brief Reduces the movement interval for enemies, making them move faster.
 */
void reduceMovementInterval() {
    movementInterval -= 10;
    
    // Ensure the movement interval is never less than 400
    if (movementInterval < 400) {
        movementInterval = 400;
    }
}

/**
 * @brief Moves the enemies on the screen. Changes direction and moves downward if they hit the screen edges.
 */
void SpaceInvadersUIElement::moveEnemies() {
    static unsigned long lastMovementTime = 0;
    const int enemyMovementSpeed = 5;
    const int descent_speed = 20; // Adjust as needed
    static int movementDirection = 1; // 1 for right, -1 for left

    unsigned long currentTime = millis();
    if (currentTime - lastMovementTime >= movementInterval) {
        lastMovementTime = currentTime;
        for (auto& enemy : enemies) {
            if (enemy.active) {
                m_tft->fillRect(enemy.x - 15, enemy.y - 5, 30, 20, BLACK);
                // Move enemy horizontally
                enemy.x += movementDirection * enemyMovementSpeed;
                
                // Reverse direction if hitting screen edges
                if (enemy.x <= 15 || enemy.x >= 320 - ENEMY_SIZE) {
                    movementDirection *= -1;
                    
                    // Move enemies down
                    for (auto& enemy : enemies) {
                        if (enemy.active) {
                            m_tft->fillRect(enemy.x - 15, enemy.y - 5, 30, 20, BLACK);
                            enemy.y += descent_speed;
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief Shoots projectiles from enemies at random intervals.
 */
void SpaceInvadersUIElement::shootEnemyProjectile() {
    for (const auto& enemy : enemies) {
        if (enemy.active && random(0, 100) < 3) { // Random chance to shoot
            for (int j = 0; j < ENEMY_MAX_PROJECTILES; j++) {
                if (!enemyProjectiles[j].active) {
                    enemyProjectiles[j].x = enemy.x;
                    enemyProjectiles[j].y = enemy.y;
                    enemyProjectiles[j].active = true;
                    break;
                }
            }
        }
    }
}

/**
 * @brief Updates the position of enemy projectiles and deactivates those that move off-screen.
 */
void SpaceInvadersUIElement::updateEnemyProjectiles() {
    for (int i = 0; i < ENEMY_MAX_PROJECTILES; i++) {
        if (enemyProjectiles[i].active) {
            m_tft->fillCircle(enemyProjectiles[i].x, enemyProjectiles[i].y, ENEMY_PROJECTILE_SIZE, BLACK);
            enemyProjectiles[i].y += ENEMY_PROJECTILE_SPEED;
            if (enemyProjectiles[i].y > 480) {
                enemyProjectiles[i].active = false;
            } else {
                m_tft->fillCircle(enemyProjectiles[i].x, enemyProjectiles[i].y, ENEMY_PROJECTILE_SIZE, RED);
            }
        }
    }
}

/**
 * @brief Checks if all enemies are killed.
 * @return True if all enemies are killed, false otherwise.
 */
bool SpaceInvadersUIElement::allEnemiesKilled() {
    for (const auto& enemy : enemies) {
        if (enemy.active) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Spawns the next wave of enemies, increasing the difficulty.
 */
void SpaceInvadersUIElement::spawnNextWave() {
    // After wave 3, reduce movement at each wave else reset movement
    if (waveCounter >= 3){
      reduceMovementInterval();
      enemyRows = 4;
    } else {
      enemyRows += 1;
      movementInterval = 1000;
    }

    initialiseEnemies();
    
    // Redraw the enemies with their new positions
    drawEnemies();
    waveCounter += 1;
}

/**
 * @brief Draws the initial screen elements.
 */
void SpaceInvadersUIElement::draw(){
   m_tft->fillScreen(BLACK);

    // Draw instructions
    m_tft->setTextSize(2);
    m_tft->setTextColor(GREEN);
    m_tft->setCursor(20, m_tft->height() / 2 - 80);
    m_tft->print("Welcome to Space Invaders!");

    m_tft->setTextSize(1);
    m_tft->setTextColor(WHITE);
    m_tft->setCursor(20, m_tft->height() / 2 - 40);
    m_tft->print("Use the accelerometer to move your spaceship.");

    m_tft->setCursor(20, m_tft->height() / 2 - 20);
    m_tft->print("Press button 1 or 2 to shoot.");

    m_tft->setCursor(20, m_tft->height() / 2);
    m_tft->print("Avoid enemy and their projectiles.");

    m_tft->setCursor(20, m_tft->height() / 2 + 20);
    m_tft->print("Destroy all enemies to advance to the next wave.");

    m_tft->setTextSize(2);
    m_tft->setTextColor(RED);
    m_tft->setCursor(20, m_tft->height() / 2 + 60);
    m_tft->print("Press button 1 to start!");

    // Wait for button 1 to be pressed to continue
    while (!up->button1()) {
        delay(10);
    }

    m_tft->fillScreen(BLACK);
    initialiseStars();
    initialiseEnemies();
    drawStars(); // Draw stars
    drawBorders();
    drawSwitcher();
    drawEnemies();
    initialisePlayer();
    drawPlayer();
    drawScore();
}

/**
 * @brief Checks for collisions between player projectiles, enemies, enemy projectiles, and the player.
 * Handles the appropriate responses to these collisions.
 */
void SpaceInvadersUIElement::checkCollisions() {
    bool playerHit = false;
    bool enemyReachedBottom = false;

    // Check collisions between player projectiles and enemies
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            for (auto& enemy : enemies) {
                if (enemy.active) {
                    TS_Point projectilePoint = {static_cast<int16_t>(projectiles[i].x), static_cast<int16_t>(projectiles[i].y), 0};
                    TS_Point enemyPoint = {static_cast<int16_t>(enemy.x), static_cast<int16_t>(enemy.y), 0};
                    
                    if (distanceBetween(projectilePoint, enemyPoint) < ENEMY_SIZE / 2) {
                        reduceMovementInterval();
                        projectiles[i].active = false;
                        enemy.active = false;
                        m_tft->fillCircle(projectiles[i].x, projectiles[i].y, PROJECTILE_SIZE, BLACK);
                        m_tft->fillRect(enemy.x - 15, enemy.y - 5, 30, 20, BLACK);
                        updateScore();
                        break;
                    }
                }
            }
        }
    }

    // Check collisions between enemy projectiles and player
    for (int i = 0; i < ENEMY_MAX_PROJECTILES; i++) {
        if (enemyProjectiles[i].active) {
            TS_Point enemyProjectilePoint = {static_cast<int16_t>(enemyProjectiles[i].x), static_cast<int16_t>(enemyProjectiles[i].y), 0};
            if (player.x < enemyProjectiles[i].x && player.x + player.width > enemyProjectiles[i].x &&
                player.y < enemyProjectiles[i].y && player.y + player.height > enemyProjectiles[i].y) {
                enemyProjectiles[i].active = false;
                m_tft->fillCircle(enemyProjectiles[i].x, enemyProjectiles[i].y, ENEMY_PROJECTILE_SIZE+5, BLACK);

                // Handle player being hit
                playerHit = true;
                break;
            }
        }
    }

    // Check collisions between player and enemies
    for (auto& enemy : enemies) {
        if (enemy.active) {
            if (player.x < enemy.x + ENEMY_SIZE / 2 && player.x + player.width > enemy.x - ENEMY_SIZE / 2 &&
                player.y < enemy.y + ENEMY_SIZE / 2 && player.y + player.height > enemy.y - ENEMY_SIZE / 2) {
                // Handle player being hit
                playerHit = true;
                break;
            }

            // Check if enemy reaches the bottom of the screen
            if (enemy.y + ENEMY_SIZE / 2 >= 480) {
                enemyReachedBottom = true;
                break;
            }
        }
    }

    // if player dies or enemies reach the bottom, remove all enemies and projectiles
    // reset the game.
    if (playerHit || enemyReachedBottom) {
        m_tft->fillRect(player.x, player.y-10, player.width, player.height+10, BLACK);
        
        for(int i = 0; i < 2; i++) {
            up->vibe(true);  delay(100);
            up->vibe(false); delay(100);
        }

        for (auto& enemy : enemies) {
            enemy.active = false;
            m_tft->fillRect(enemy.x - 15, enemy.y - 5, 30, 20, BLACK);
        }

        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (projectiles[i].active) {
                m_tft->fillCircle(projectiles[i].x, projectiles[i].y, PROJECTILE_SIZE, BLACK);
                projectiles[i].active = false;
            }
        }

        for (int i = 0; i < ENEMY_MAX_PROJECTILES; i++) {
            if (enemyProjectiles[i].active) {
                m_tft->fillCircle(enemyProjectiles[i].x, enemyProjectiles[i].y, ENEMY_PROJECTILE_SIZE, BLACK);
                enemyProjectiles[i].active = false;
            }
        }

        movementInterval = 1000;
        gameOver();
    }
}

/**
 * @brief Handles the game over state by displaying a "Game Over" message and the player's score.
 * Waits for the player to press button 3 to restart the game.
 */
void SpaceInvadersUIElement::gameOver() {
    // Print "Game Over" message with score
    m_tft->setTextSize(3);
    m_tft->setTextColor(RED);
    m_tft->setCursor(80, m_tft->height() / 2 - 20); // Adjust position as needed
    m_tft->print("Game Over!");
    
    m_tft->setTextSize(2);
    m_tft->setTextColor(WHITE);
    m_tft->setCursor(100, m_tft->height() / 2 + 20); // Adjust position as needed
    m_tft->print("Score: ");
    m_tft->print(score);

    m_tft->setTextColor(GREEN);
    m_tft->setCursor(10, m_tft->height() / 2 + 60); // Adjust position as needed
    m_tft->print("Press Button 3 to restart");

    delay(1000);

    // Wait for button 3 to be pressed to restart game
    while (!up->button3()) {
        delay(10);
    }

    // Clear screen and restart game
    m_tft->fillScreen(BLACK);
    resetScore(); // Reset score
    enemyRows = 1;
    draw(); // Reinitialize the game    
}

/**
 * @brief Main game loop to be called each turn. Updates player movement, handles shooting, updates enemies, 
 * draws game components, and checks for collisions.
 */
void SpaceInvadersUIElement::runEachTurn() {
  // Get a new sensor event
  sensors_event_t event;
  up->getAccelEvent(&event);

  drawBorders();
  drawSwitcher();


  // Clear only the area where the circle was previously drawn
  m_tft->fillRect(player.x, player.y-10, player.width, player.height+10, BLACK);
  float absX = abs(event.acceleration.x);
  float absY = abs(event.acceleration.y);

  // Define movement speed
  int movementSpeed = 5; // Adjust as needed

  // Move player based on accelerometer readings
  if (absX > absY) {
      // Move horizontally
      if (event.acceleration.x > 2 && player.x > X_MIN) {
          player.x -= movementSpeed;
      } else if (event.acceleration.x < -2 && player.x < X_MAX) {
          player.x += movementSpeed;
      }
  } else {
      // Move vertically
      if (event.acceleration.y > 2 && player.y < Y_MAX) {
          player.y += movementSpeed;
      } else if (event.acceleration.y < -2 && player.y > Y_MIN) {
          player.y -= movementSpeed;
      }
  }

  // Shoot projectile when button is pressed
  if (up->button1() || up->button2()) {
    shootProjectile();
  }

  updateProjectiles();
  
  // Update and draw enemies
  shootEnemyProjectile();
  updateEnemyProjectiles();
  moveEnemies();
  drawEnemies();

  // Draw the circle at the updated position
  drawPlayer(); // Draw player
  drawScore();

  drawStars();
  moveStars();

  if (allEnemiesKilled()) {
      spawnNextWave();
  }
  // Check for collisions
  checkCollisions();

  // Delay for smoothness (adjust as needed)
  delay(20);
}


/**
 * Process touches.
 * @returns bool - true if the touch is on the switcher
 */
bool SpaceInvadersUIElement::handleTouch(long x, long y) {
  return y < BOXSIZE && x > (BOXSIZE * SWITCHER);
}
#endif // UNPHONE_UI0 == 1
//////////////////////////////////////////////////////////////////////////////