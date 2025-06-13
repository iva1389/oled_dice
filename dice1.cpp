#include <wiringPi.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

using namespace std;

const int ledPins[7] = {3, 4, 5, 6, 21, 22, 27}; // LED 1 до 7

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

atomic<bool> rolling(false);
atomic<bool> running(true);

// Изчистване на светодиодите
void clearLEDs() {
    for (int pin : ledPins) {
        digitalWrite(pin, LOW);
    }
}

// Показване на число на зар
void showDice(int number) {
    clearLEDs();
    for (int index : diceMap[number]) {
        digitalWrite(ledPins[index], HIGH);
    }
}

// Нишка за хвърляне на зар
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

// Нишка за изчистване
void clearThread() {
    while (running) {
        if (digitalRead(buttonClear) == HIGH) {
            cout << "Изчистване." << endl;
            clearLEDs();
            this_thread::sleep_for(chrono::milliseconds(300));
        }
        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

int main() {
    wiringPiSetup();

    // Инициализация на светодиодите
    for (int pin : ledPins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    // Инициализация на бутоните
    pinMode(buttonRoll, INPUT);
    pullUpDnControl(buttonRoll, PUD_DOWN);
    pinMode(buttonClear, INPUT);
    pullUpDnControl(buttonClear, PUD_DOWN);

    srand(time(0));
    cout << "Стартирано. Натисни бутон за хвърляне или за изчистване." << endl;

    // Стартиране на нишки
    thread t1(rollThread);
    thread t2(clearThread);

    // Изчакване на ENTER
    cin.get();
    running = false;

    t1.join();
    t2.join();

    clearLEDs();
    cout << "Прекратено." << endl;
    return 0;
}

