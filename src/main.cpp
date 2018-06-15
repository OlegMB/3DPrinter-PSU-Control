#include <Arduino.h>
/* PSU Control
 * Управление питанием 3D Printer
 * D0/PWM0/AREF/MOSI/SDA ----------Вход с SBASE (подтянут к 0 через R 10kOm)
 * D1/PWM1/MISO--------------------(LED на плате) Реле включения питания AC-DC (0-выключено, 1- включено)
 * D2/SCK/SCL----------------------Вход кнопки. (Замыкает на 0)
 * D3/A3/USB+(poollup + 1,5kOm)----
 * D4/PWM4/A2(USB-)----------------Hotend Cooler
 * D5/A0---------------------------LED
 * 
 * Принцип работы. 
 * Если принтер выключен.
 *      при нажатии кнопки питания включить принтер.
 * 
 * Если пинтре включен.
 *      1. При нажатии кнопки питания - дождаться охлаждения хотенда - выключить питание (автовыключение после окончания печати)
 *      2. Команда с сбасе - дождаться охлаждения хотенда - выключить питание 
 * 
 *      ИЛИ при долгом нажатии (4сек) кнопки выключени питания выключить принтер игнорируя вентилятор охлаждения хотенда
 * 
 * Планы 
 * Добавить ожидание автоматического выключения - пока не охладиться хотенд. 
 * Светодид состояния PSU (на D3 ?)
 * Покашен - выключено
 * Включен - выключено
 * Мигает с интервалом 5 сек (1 сек.) - ожидает выключение вентилятора хотенда
 * 
 * 
 * При нажатии ресет возможно выключение питание т.к. пропадает управляющее напряжениее на выходе sbase - 
 * сделать задержку выключение по команде с контроллера в 5 секунд. 
 */

#define SBASE_POWER_PIN 0
#define RELAY_PIN 1
#define BUTTON_PIN 2
#define HOTENDCOOLER_PIN 4
#define LED_PIN 5

// PSUState
#define OFF 0
#define ON 1
#define WAIT_COOLING 2

uint8_t SBASE_connected = OFF;                                  // обнаружение влюченного SBASE 
uint8_t PSUState = OFF;                                         // Состояние PSU

// Кнока POWER включение выключение питания.
uint8_t buttonState = 0;                                        // Состояние кнопки POWER
uint8_t lastButtonState = 0;                                    // Предыдущее состояние кнопи POWER
unsigned long lastDebounceTime = 0;                             // the debounce time
unsigned long debounceDelay = 100;                              // the debounce duration

//unsigned long changePSUStateTime = 0;                           // Время изменения состояния принтера
//unsigned long changePSUStateDelay = 3000;                       // задержка оценки состояния принтера


// LED
uint8_t ledState = LOW;             // состояние светодиода
long ledPreviousMillis = 0;        // храним время последнего переключения светодиода
long ledBlinkInterval = 5000;      // интервал между включение/выключением светодиода (5 сек)
long ledBlinkDuration = 1000;      // длительность "горения"


void LedTrigger()
{
    if (PSUState==WAIT_COOLING)
    {
        long seekmillis=millis();
        if (ledState) // led is on
        {
            if (seekmillis - ledPreviousMillis>ledBlinkDuration)
            {
                ledPreviousMillis=seekmillis;
                ledState=0;
                digitalWrite(LED_PIN,0);
            }
        } else // led is off
        {
            if (seekmillis - ledPreviousMillis>ledBlinkInterval)
            {
                ledPreviousMillis=seekmillis;
                ledState=1;
                digitalWrite(LED_PIN,1);
            }
        }
    }

}


// the setup routine runs once when you press reset:
void setup()
{
    // initialize the digital pin as an output.
    pinMode(RELAY_PIN, OUTPUT);        
    pinMode(BUTTON_PIN, INPUT_PULLUP); 
    pinMode(SBASE_POWER_PIN, INPUT);   
    pinMode(HOTENDCOOLER_PIN, INPUT);   
    pinMode(LED_PIN, OUTPUT);   
    digitalWrite(LED_PIN,0);
    digitalWrite(RELAY_PIN,0);
    
}

// the loop routine runs over and over again forever:
void loop()
{
    int reading = !digitalRead(BUTTON_PIN);
    int sbasereading = digitalRead(SBASE_POWER_PIN);
    int hotendcoolerreading = digitalRead(HOTENDCOOLER_PIN );

    if (!SBASE_connected && sbasereading) SBASE_connected=1;


     switch (PSUState)
     {
         case OFF:;
         case ON:;
         case WAIT_COOLING:
                LedTrigger();
     }

    if (SBASE_connected && !sbasereading && !hotendcoolerreading)
    {
        PSUState = 0;
        SBASE_connected = 0;
        digitalWrite(RELAY_PIN,0);
    }

    if (reading != lastButtonState)
    {
        // reset the debouncing timer
        lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if (reading != buttonState)
        {
            buttonState = reading;
            // only toggle the LED if the new button state is HIGH
            if (buttonState == HIGH)
            {
                PSUState = !PSUState;
                digitalWrite(RELAY_PIN, PSUState);
                // changePSUStateTime = millis();
            }
        }
    }
    lastButtonState = reading;

}