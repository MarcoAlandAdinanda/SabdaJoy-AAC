#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Joystick pins
#define JOYSTICK_X 34    // Joystick X-axis
#define JOYSTICK_Y 35    // Joystick Y-axis
#define JOYSTICK_BTN 32  // Joystick button pin (digital)
#define JOYSTICK_THRESHOLD 2048 // Midpoint for joystick reading (center on ESP32)

// Push button to start displaying the menu
#define PUSH_BTN 33  // Push button pin (digital)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int currentSelection = 0;  // Keep track of the current menu selection
unsigned long lastMove = 0; // To debounce the joystick
const int moveDelay = 200;  // Delay between movements to avoid over-scrolling
bool buttonPressed = false;
bool menuActive = false;  // To track if the menu is currently active
bool optionChosen = false;  // Track if an option has been chosen
String chosenOption = "";  // Store the chosen option for the "Output audio"

// Define different menu states
enum MenuState {
  OUTPUT_AUDIO,
  MAIN_MENU,
  QUICK_CHAT,
  PICK_WORD,
  TYPING
};
MenuState menuState = OUTPUT_AUDIO;  // Start with OUTPUT_AUDIO
x
// Main menu items
const char* mainMenuItems[] = {"Quick chat", "Pick word", "Typing"};
const int mainMenuSize = 3;

// Quick chat items
const char* quickChatItems[] = {
  "Tolong bantu saya", "Terima kasih banyak", "Siapa namamu", "Mohon kerja samanya"
};
const int quickChatSize = 4;

// Pick word items
const char* pickWordItems[] = {"Halo", "Pergi", "Makan", "Siapa"};
const int pickWordSize = 4;

// Typing items (A-Z and SPACE)
const char* typingItems[] = {
  "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
  "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "SPACE"
};
const int typingSize = 27; // Updated to include SPACE

// Scrolling variables
int firstVisibleIndex = 0;  // Index of the first visible item

void setup() {
  Serial.begin(115200);

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // Initialize joystick and push button
  pinMode(JOYSTICK_BTN, INPUT_PULLUP);  // Joystick button pin with pull-up
  pinMode(PUSH_BTN, INPUT_PULLUP);      // Push button pin with pull-up

  display.clearDisplay();
  showOutputAudioScreen();  // Show initial output audio screen
}

void loop() {
  // Check if push button is pressed to start the menu
  if (digitalRead(PUSH_BTN) == LOW && !menuActive) {
    delay(200);  // Simple debounce
    menuActive = true;
    menuState = MAIN_MENU;
    currentSelection = 0;  // Reset selection to the top of the main menu
    optionChosen = false;  // Reset the option chosen flag
    firstVisibleIndex = 0; // Reset the first visible index
  }

  if (menuActive) {
    handleJoystick();  // Handle joystick input to move through menu
    handleButton();    // Handle button press to select an option
    displayMenu();     // Display the appropriate menu based on state
  } else if (optionChosen) {
    // Display the selected option under "Output audio" after an option is chosen
    handleOutputAudioJoystick();  // Handle left joystick movement for deletion
    showOutputAudioScreen();
  }
}

void handleJoystick() {
  int yValue = analogRead(JOYSTICK_Y);

  if (millis() - lastMove > moveDelay) {
    if (yValue < JOYSTICK_THRESHOLD - 500) {  // Move up
      if (currentSelection > 0) {
        currentSelection--;
        // Update the first visible index if necessary
        if (currentSelection < firstVisibleIndex) {
          firstVisibleIndex--;
        }
      }
      lastMove = millis();
    }
    if (yValue > JOYSTICK_THRESHOLD + 500) {  // Move down
      if (menuState == MAIN_MENU && currentSelection < mainMenuSize - 1) {
        currentSelection++;
        if (currentSelection >= firstVisibleIndex + SCREEN_HEIGHT / 8) {
          firstVisibleIndex++;
        }
      }
      else if (menuState == QUICK_CHAT && currentSelection < quickChatSize - 1) {
        currentSelection++;
        if (currentSelection >= firstVisibleIndex + SCREEN_HEIGHT / 8) {
          firstVisibleIndex++;
        }
      }
      else if (menuState == PICK_WORD && currentSelection < pickWordSize - 1) {
        currentSelection++;
        if (currentSelection >= firstVisibleIndex + SCREEN_HEIGHT / 8) {
          firstVisibleIndex++;
        }
      }
      else if (menuState == TYPING && currentSelection < typingSize - 1) {
        currentSelection++;
        if (currentSelection >= firstVisibleIndex + SCREEN_HEIGHT / 8) {
          firstVisibleIndex++;
        }
      }
      lastMove = millis();
    }
  }
}

void handleButton() {
  if (digitalRead(JOYSTICK_BTN) == LOW && !buttonPressed) {
    buttonPressed = true;  // Mark the button as pressed

    if (menuState == MAIN_MENU) {
      if (currentSelection == 0) menuState = QUICK_CHAT;
      else if (currentSelection == 1) menuState = PICK_WORD;
      else if (currentSelection == 2) menuState = TYPING;
      currentSelection = 0;  // Reset selection for the submenus
      firstVisibleIndex = 0;  // Reset first visible index
    } else {
      // Handle selection in the submenus
      if (menuState == QUICK_CHAT) {
        if (chosenOption != "") chosenOption += " ";  // Add space between selections
        chosenOption += quickChatItems[currentSelection];
      }
      if (menuState == PICK_WORD) {
        if (chosenOption != "") chosenOption += " ";  // Add space between selections
        chosenOption += pickWordItems[currentSelection];
      }
      if (menuState == TYPING) {
        if (currentSelection == typingSize - 1) {  // If SPACE is selected
          chosenOption += " ";  // Add space without any other character
        } else {
          // Add the typing option directly without space
          chosenOption += typingItems[currentSelection];
        }
      }

      // Return to "Output audio" after choosing an option
      menuActive = false;
      optionChosen = true;
    }

    delay(200);  // Simple debounce delay
  } else if (digitalRead(JOYSTICK_BTN) == HIGH) {
    buttonPressed = false;  // Reset button press state when button is released
  }
}

void handleOutputAudioJoystick() {
  int xValue = analogRead(JOYSTICK_X);

  // Detect left movement on the joystick to delete the last chosen option
  if (xValue < JOYSTICK_THRESHOLD - 500 && millis() - lastMove > moveDelay) {
    // Remove the last selected word/phrase
    int lastSpace = chosenOption.lastIndexOf(' ');
    if (lastSpace != -1) {
      chosenOption = chosenOption.substring(0, lastSpace);  // Keep the text before the last space
    } else {
      chosenOption = "";  // If there are no spaces, clear the entire string
    }

    lastMove = millis();  // Update the debounce timer
  }
}

void displayMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  if (menuState == MAIN_MENU) {
    display.setCursor(0, 0);
    display.println("Mode Selection");
    for (int i = firstVisibleIndex; i < mainMenuSize && i < firstVisibleIndex + SCREEN_HEIGHT / 8; i++) {
      if (i == currentSelection) {
        display.setTextColor(BLACK, WHITE);  // Highlight the current selection
        display.print("> ");
      } else {
        display.setTextColor(WHITE);  // Normal text for other items
        display.print("  ");
      }
      display.println(mainMenuItems[i]);
    }
  } else if (menuState == QUICK_CHAT) {
    display.setCursor(0, 0);
    display.println("Quick Chat");
    for (int i = firstVisibleIndex; i < quickChatSize && i < firstVisibleIndex + SCREEN_HEIGHT / 8; i++) {
      if (i == currentSelection) {
        display.setTextColor(BLACK, WHITE);  // Highlight the current selection
        display.print("> ");
      } else {
        display.setTextColor(WHITE);
        display.print("  ");
      }
      display.println(quickChatItems[i]);
    }
  } else if (menuState == PICK_WORD) {
    display.setCursor(0, 0);
    display.println("Pick Word");
    for (int i = firstVisibleIndex; i < pickWordSize && i < firstVisibleIndex + SCREEN_HEIGHT / 8; i++) {
      if (i == currentSelection) {
        display.setTextColor(BLACK, WHITE);  // Highlight the current selection
        display.print("> ");
      } else {
        display.setTextColor(WHITE);
        display.print("  ");
      }
      display.println(pickWordItems[i]);
    }
  } else if (menuState == TYPING) {
    display.setCursor(0, 0);
    display.println("Typing");
    for (int i = firstVisibleIndex; i < typingSize && i < firstVisibleIndex + SCREEN_HEIGHT / 8; i++) {
      if (i == currentSelection) {
        display.setTextColor(BLACK, WHITE);  // Highlight the current selection
        display.print("> ");
      } else {
        display.setTextColor(WHITE);
        display.print("  ");
      }
      display.println(typingItems[i]);
    }
  }

  display.display();  // Render the display
}

void showOutputAudioScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Output audio");
  display.println("_______________");

  // Display the selected phrase(s) if chosen
  if (chosenOption != "") {
    display.println(chosenOption);
  }

  display.display();
}
