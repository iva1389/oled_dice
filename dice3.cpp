#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>

using namespace std;

// OLED I2C адрес
const int OLED_ADDR = 0x3C;
int oled_fd;

// Позиции на светодиодите:
// [2]   [5]   [7]
//       [3]
// [1]   [4]   [6]
const int ledPins[7] = {3, 4, 5, 6, 21, 22, 27}; // WiringPi номера съответстващи на: GPIO 22, 23, 24, 25, 5, 6, 16

// Бутони
const int buttonRoll = 0;   // GPIO 17 -> WiringPi 0
const int buttonClear = 2;  // GPIO 27 -> WiringPi 2

// Мапване на зар спрямо позиции
const vector<int> diceMap[7] = {
    {},                // 0 – нищо
    {2},               // 1 – център (LED 3)
    {1, 5},            // 2 – диагонал (LED 2 и 6)
    {1, 2, 5},         // 3 – диагонал + център
    {0, 1, 5, 6},      // 4 – четири ъгъла (LED 1,2,6,7)
    {0, 1, 2, 5, 6},   // 5 – + център
    {0, 1, 3, 4, 5, 6} // 6 – всички без център (LED 1,2,4,5,6,7)
};

// Bitmap цифри 16x8 (по вертикали, 8x2 байта на ред):
const uint8_t digits[6][16] = {
    {0x00,0x00,0x84,0x00,0xFE,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, //1
    {0xC2,0x00,0xA1,0x00,0x91,0x00,0x89,0x00,0x89,0x00,0x89,0x00,0x86,0x00,0x00,0x00}, //2
    {0x42,0x00,0x81,0x00,0x89,0x00,0x89,0x00,0x89,0x00,0x89,0x00,0x76,0x00,0x00,0x00}, //3
    {0x30,0x00,0x28,0x00,0x24,0x00,0x22,0x00,0xFE,0x00,0x20,0x00,0x20,0x00,0x00,0x00}, //4
    {0x4F,0x00,0x89,0x00,0x89,0x00,0x89,0x00,0x89,0x00,0x89,0x00,0x71,0x00,0x00,0x00}, //5
    {0x7E,0x00,0x89,0x00,0x89,0x00,0x89,0x00,0x89,0x00,0x89,0x00,0x72,0x00,0x00,0x00}  //6
};

atomic<bool> rolling(false);
atomic<bool> running(true);

void oledCommand(uint8_t cmd) {
    wiringPiI2CWriteReg8(oled_fd, 0x00, cmd);
}

void oledData(uint8_t data) {
    wiringPiI2CWriteReg8(oled_fd, 0x40, data);
}

void oledInit() {
    oled_fd = wiringPiI2CSetup(OLED_ADDR);
    oledCommand(0xAE);
    oledCommand(0x20); oledCommand(0x00);
    oledCommand(0xB0);
    oledCommand(0xC8);
    oledCommand(0x00);
    oledCommand(0x10);
    oledCommand(0x40);
    oledCommand(0x81); oledCommand(0x7F);
    oledCommand(0xA1);
    oledCommand(0xA6);
    oledCommand(0xA8); oledCommand(0x3F);
    oledCommand(0xA4);
    oledCommand(0xD3); oledCommand(0x00);
    oledCommand(0xD5); oledCommand(0x80);
    oledCommand(0xD9); oledCommand(0xF1);
    oledCommand(0xDA); oledCommand(0x12);
    oledCommand(0xDB); oledCommand(0x40);
    oledCommand(0x8D); oledCommand(0x14);
    oledCommand(0xAF);
}

void oledClear() {
    for (int page = 0; page < 8; page++) {
        oledCommand(0xB0 + page);
        oledCommand(0x00);
        oledCommand(0x10);
        for (int i = 0; i < 128; i++) oledData(0x00);
    }
}

void oledShowDigit(int n) {
    if (n < 1 || n > 6) return;
    oledClear();

    int startCol = (128 - 16) / 2; // центриране по хоризонтала, числото е 16 пиксела широко

    oledCommand(0xB2);                     // Page 2
    oledCommand(0x00 + (startCol & 0x0F)); // Lower column
    oledCommand(0x10 + ((startCol >> 4) & 0x0F)); // Higher column

    for (int i = 0; i < 16; i++) {
        oledData(digits[n - 1][i]);
    }
}


void clearLEDs() {
    for (int pin : ledPins) digitalWrite(pin, LOW);
}

void showDice(int number) {
    clearLEDs();
    for (int i : diceMap[number]) digitalWrite(ledPins[i], HIGH);
    oledShowDigit(number);
}

void rollThread() {
    while (running) {
        if (digitalRead(buttonRoll) == HIGH && !rolling) {
            rolling = true;
            int number = rand() % 6 + 1;
            cout << "Хвърляне: " << number << endl;
            showDice(number);
            this_thread::sleep_for(chrono::milliseconds(500));
            rolling = false;
        }
        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

void clearThread() {
    while (running) {
        if (digitalRead(buttonClear) == HIGH) {
            cout << "Изчистване." << endl;
            clearLEDs();
            oledClear();
            this_thread::sleep_for(chrono::milliseconds(300));
        }
        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

int main() {
    wiringPiSetup();

    for (int pin : ledPins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    pinMode(buttonRoll, INPUT);
    pullUpDnControl(buttonRoll, PUD_DOWN);
    pinMode(buttonClear, INPUT);
    pullUpDnControl(buttonClear, PUD_DOWN);

    oledInit();
    oledClear();
    srand(time(0));

    cout << "Стартирано!" << endl;

    thread t1(rollThread);
    thread t2(clearThread);

    cin.get();
    running = false;
    t1.join();
    t2.join();

    clearLEDs();
    oledClear();
    cout << "Прекратено." << endl;
    return 0;
}
