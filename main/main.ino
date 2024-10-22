// Include libraries
#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

// PIN Variables
#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

#define MENU_BTN      33

#define JOYSTICK_X 34    // Joystick X-axis
#define JOYSTICK_Y 35    // Joystick Y-axis
#define JOYSTICK_BTN 32  // Joystick button pin (digital)
#define JOYSTICK_THRESHOLD 1850 // Midpoint for joystick reading (center on ESP32)

// Display attributes
#define SCREEN_WIDTH 128 // Display width
#define SCREEN_HEIGHT 64 // Display height
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Audio attributes
Audio audio;
int volume_level = 20;

String ssid = "waruhouse";
String password = "purnomo1";
// String ssid = "POCO M4 Pro";
// String password = "pakaiaja";

// Main program attributes
int current_selection = 0; // Track the current menu selection
unsigned long last_move = 0; // Debounce the joystick
const int move_delay = 200; // Delay between movements to avoid over-scrolling
bool button_pressed = false; // Indicate the button is pressed or not
bool menu_active = false; // Track if the menu is active
bool option_chosen = false; // Track if an option has been chosen;
String chosen_option = ""; // Store the chosen option to be generated in Google TTS

int first_visible_index = 0;// Scrolling variables

// Define menu states
enum MenuState 
{
  OUTPUT_AUDIO,
  MAIN_MENU,
  SENTENCE,
  WORD,
  LETTER, 
};
MenuState menu_state = OUTPUT_AUDIO; // Set startting menu

// Main menu items
const char* main_menu_items[] = {"Sentence", "Word", "Letter"};
const int main_menu_size = 3;

// Sentence items
const char* sentence_items[] = { "Saya harus melakukan pemanasan sebelum mulai.", "Jangan lupa bawa air minum saat berolahraga.", "Saya akan lari di taman sore ini.", "Ayo lakukan beberapa gerakan peregangan dulu.", "Berapa lama kamu biasanya berolahraga?", 
                                  "Saya perlu menyelesaikan laporan ini.", "Ada rapat jam 10 pagi nanti.", "Bisa kirim email itu ke saya?", "Tolong cek kembali dokumen ini.", "Saya akan bekerja dari rumah hari ini.", 
                                  "Apakah ini ada diskon?", "Berapa harga barang ini?", "Saya butuh ukuran yang lebih besar.", "Apakah Anda menerima pembayaran dengan kartu?", "Di mana saya bisa menemukan produk ini?", 
                                  "Saya akan memasak makan malam.", "Apakah kamu sudah mencuci pakaian?", "Jangan lupa matikan lampu sebelum tidur.", "Saya perlu membersihkan rumah hari ini.", "Ayo kita nonton film bersama-sama.", 
                                  "Selamat datang, senang bertemu denganmu!", "Kapan kita bisa ketemu lagi?", "Apa kabar? Lama tak jumpa.", "Mari kita makan bersama nanti.", "Terima kasih sudah datang."};
const int sentence_items_size = 30;

// Word items
const char* word_items[] = { "Ya", "Tidak", "Terima kasih", "Sama-sama", "Tolong", "Maaf", "Selamat", "Hati-hati", "Ayo", "Baik", 
                              "Kapan?", "Di mana?", "Mengapa?", "Apa?", "Siapa?", "Bisa", "Sudah", "Belum", "Senang", "Sedih", 
                              "Lama", "Cepat", "Bagus", "Sangat", "Pasti", "Mungkin", "Tentu", "Sekarang", "Nanti", "Sini", 
                              "Situ", "Kemana?", "Ke sini", "Ke sana", "Di sini", "Di situ", "Tunggu", "Datang", "Pergi", "Lihat", 
                              "Dengar", "Pikir", "Berjalan", "Bicara", "Makan", "Minum", "Tidur", "Bekerja", "Belajar", "Bermain"};
const int word_items_size = 50;

// Letter items
const char* letter_items[] = {"SPACE",
                              "A", "B", "C", "D", "E", 
                              "F", "G", "H", "I", "J", 
                              "K", "L", "M", "N", "O", 
                              "P", "Q", "R", "S", "T", 
                              "U", "V", "W", "X", "Y", 
                              "Z"
                            };
const int letter_items_size = 27;

///////////////////////////////////////////////////////////////////////////////////////////////////

// WiFi setup 
void setup_wifi()
{
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    Serial.println("Connecting...");

    while (WiFi.status () != WL_CONNECTED) // Wait until connected
        delay(1500); 
    
    Serial.println("//////////////");
    Serial.println("WiFi connected");
    Serial.println("//////////////");
}

// I2S audio setup 
void setup_i2s_audio(int volume_level)
{
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(volume_level);
}

// OLED display setup
void setup_display() {
  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  String message = "SabdaJoy"; 
  int16_t x1, y1;
  uint16_t width, height;

  display.getTextBounds(message, 0, 0, &x1, &y1, &width, &height);

  int x = (SCREEN_WIDTH - width) / 2;
  int y = (SCREEN_HEIGHT - height) / 2;

  display.setCursor(x, y);
  display.println(message);
  display.display(); 
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void output_menu()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Output audio");
  display.println("_______________");

  if (chosen_option != "")
  {
    display.println(chosen_option);
  }

  display.display();
}


void display_menu()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  if (menu_state == MAIN_MENU)
  {
    display.setCursor(0, 0);
    display.println("Main Menu");

    for (int i = first_visible_index; i < main_menu_size && i < first_visible_index + SCREEN_HEIGHT / 8; i++)
    {
      if (i == current_selection)
      {
        display.setTextColor(BLACK, WHITE);
        display.print("> ");
      }
      else
      {
        display.setTextColor(WHITE);
        display.print("  ");
      }
      display.println(main_menu_items[i]);
    }
  }

  else if (menu_state == SENTENCE)
  {
    display.setCursor(0, 0);
    display.println("Sentence");

    for (int i = first_visible_index; i < sentence_items_size && i < first_visible_index + SCREEN_HEIGHT / 8; i++)
    {
      if (i == current_selection)
      {
        display.setTextColor(BLACK, WHITE);
        display.print("> ");
      }
      else
      {
        display.setTextColor(WHITE);
        display.print("  ");
      }
      display.println(sentence_items[i]);
    }
  }

  else if (menu_state == WORD)
  {
    display.setCursor(0, 0);
    display.println("Word");

    for (int i = first_visible_index; i < word_items_size && i < first_visible_index + SCREEN_HEIGHT / 8; i++)
    {
      if (i == current_selection)
      {
        display.setTextColor(BLACK, WHITE);
        display.print("> ");
      }
      else
      {
        display.setTextColor(WHITE);
        display.print("  ");
      }
      display.println(word_items[i]);
    }
  }

  else if (menu_state == LETTER)
  {
    display.setCursor(0, 0);
    display.println("Letter");

    for (int i = first_visible_index; i < letter_items_size && i < first_visible_index + SCREEN_HEIGHT / 8; i++)
    {
      if (i == current_selection)
      {
        display.setTextColor(BLACK, WHITE);
        display.print("> ");
      }
      else
      {
        display.setTextColor(WHITE);
        display.print("  ");
      }
      display.println(letter_items[i]);
    }
  }

  display.display();
}

void handle_joystick()
{
  int y_value = analogRead(JOYSTICK_Y);

  if (millis() - last_move > move_delay)
  {
    if (y_value < JOYSTICK_THRESHOLD - 500) // Move up
    {
      if (current_selection > 0)
      {
        current_selection--;
        // update first visible index if necessary
        if (current_selection < first_visible_index)
        {
          first_visible_index--;
        }
      }
      last_move = millis();
    }
    if (y_value > JOYSTICK_THRESHOLD + 500) // Move down
    {
      if (menu_state == MAIN_MENU && current_selection < main_menu_size - 1)
      {
        current_selection++;
        if (current_selection >= first_visible_index + SCREEN_HEIGHT / 8)
        {
          first_visible_index++;
        }
      }
      else if (menu_state == SENTENCE && current_selection < sentence_items_size - 1)
      {
        current_selection++;
        if (current_selection >= first_visible_index + SCREEN_HEIGHT / 8)
        {
          first_visible_index++;
        }
      }
      else if (menu_state == WORD && current_selection < word_items_size - 1)
      {
        current_selection++;
        if (current_selection >= first_visible_index + SCREEN_HEIGHT / 8)
        {
          first_visible_index++;
        }
      }
      else if (menu_state == LETTER && current_selection < letter_items_size - 1)
      {
        current_selection++;
        if (current_selection >= first_visible_index + SCREEN_HEIGHT / 8)
        {
          first_visible_index++;
        } 
      }
      last_move = millis();
    }
  }
}

void handle_selection() 
{
  if (digitalRead(JOYSTICK_BTN) == LOW && !button_pressed)
  {
    button_pressed = true;

    if (menu_state == MAIN_MENU)
    {
      if (current_selection == 0) menu_state = SENTENCE;
      else if (current_selection == 1) menu_state = WORD;
      else if (current_selection == 2) menu_state = LETTER;
      current_selection = 0;
      first_visible_index = 0;
    }
    else
    {
      if (menu_state == SENTENCE)
      {
        if (chosen_option != "") chosen_option += " ";
        chosen_option += sentence_items[current_selection];
      }
      if (menu_state == WORD)
      {
        if (chosen_option != "") chosen_option += " ";
        chosen_option += word_items[current_selection];
      }
      if (menu_state == LETTER)
      {
        if (current_selection == letter_items_size - 1)
        {
          chosen_option += " ";
        }
        else
        {
          chosen_option += letter_items[current_selection];
        }
      }

      // Return to OUTPUT AUDIO after choosing option
      menu_active = false;
      option_chosen = true;
    }

    delay(200); // debounce delay
  }
  else if (digitalRead(JOYSTICK_BTN) == HIGH) button_pressed = false;
}

void handle_output_joystick() // delete output (word by word)
{
  int x_value = analogRead(JOYSTICK_X);

  if (x_value < JOYSTICK_THRESHOLD - 500 && millis() - last_move > move_delay)
  {
    int last_space = chosen_option.lastIndexOf(' ');
    if (last_space != -1)
    {
      chosen_option = chosen_option.substring(0, last_space);
    }
    else
    {
      chosen_option = "";
    }

    last_move = millis();
  }
}

void speak()
{
  if(digitalRead(JOYSTICK_BTN) == LOW && !menu_active)
  {
    audio.connecttospeech(chosen_option.c_str(), "id");
    delay(200); // debounce delay
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200);

  pinMode(MENU_BTN, INPUT_PULLUP);

  pinMode(JOYSTICK_BTN, INPUT_PULLUP); // TESTING

  setup_wifi();

  setup_i2s_audio(volume_level);

  setup_display();

  audio.connecttospeech("The device is ready", "en"); // Google TTS

  delay(5000);

  output_menu();
}

void loop()
{
  audio.loop();

  if (digitalRead(MENU_BTN) == LOW && !menu_active)
  {
    delay(200); // Simple debounce
    Serial.println("TOMBOL MENU DITEKAN");
    menu_active = true;
    menu_state = MAIN_MENU;
    current_selection = 0;
    option_chosen = false;
    first_visible_index = 0;
  }

  if (menu_active)
  {
    handle_joystick();
    handle_selection();
    display_menu();
  }
  else if (option_chosen)
  {
    output_menu();
    handle_output_joystick();
    speak();
  }
}

// Deubg audio I2S
// void audio_info(const char *info) {
//   Serial.print("audio_info: "); Serial.println(info);
// }