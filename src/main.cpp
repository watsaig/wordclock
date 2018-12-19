#include <Arduino.h>
#include <Wire.h>
#include <NeoPixelBus.h>
#include <NTPTimeESP.h>

#define DEBUG_OUTPUT

NTPtime NTPch("ch.pool.ntp.org");
char* ssid = "wifi name";
char* password = "wifi password";

strDateTime ntpDateTime;

//typedef RowMajorAlternatingLayout panelLayout;
const uint8_t panelWidth = 11;
const uint8_t panelHeight = 11;
const uint16_t nPixels = panelWidth * panelHeight;

NeoTopology<ColumnMajorAlternatingLayout> topo(panelWidth, panelHeight); // I actually have RowMajorAlternatingLayout but I swapped the coordinates (used row, column instead of x, y) so setting this cancels out the mistake.
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(nPixels); // must be connected to pin 3 on ESP8266

RgbColor red(128, 0, 0);
RgbColor green(0, 128, 0);
RgbColor blue(0, 0, 128);
RgbColor white(128);
RgbColor black(0);


unsigned long currentTimeEstimate; // in seconds (UNIX epoch). Based on last NTP update + millis()/1000
strDateTime lastNtpDateTime;
strDateTime lastDisplayUpdateTime; // the time at which the display was last updated, rounded to the closest 5 minutes
strDateTime currentTimeRounded; // the current time, rounded to the closest 5 minutes
//unsigned long lastNtpUpdateTimeInS; // in seconds (UNIX epoch)
//strDateTime startTime;

// Test functions
void testPhotoresistor();
void testClockAccuracy();

// utility functions
strDateTime roundDateTime(strDateTime t);

// Display functions
void updateDisplay();

void itIs(RgbColor& color);
void a(RgbColor& color);

void fiveMin(RgbColor& color);
void tenMin(RgbColor& color);
void quarter(RgbColor& color);
void twenty(RgbColor& color);
void half(RgbColor& color);

void way(RgbColor& color);
void past(RgbColor& color);
void to(RgbColor& color);

void one(RgbColor& color);
void two(RgbColor& color);
void three(RgbColor& color);
void four(RgbColor& color);
void five(RgbColor& color);
void six(RgbColor& color);
void seven(RgbColor& color);
void eight(RgbColor& color);
void nine(RgbColor& color);
void ten(RgbColor& color);
void eleven(RgbColor& color);
void noon(RgbColor& color);
void midnight(RgbColor& color);

void oclock(RgbColor& color);

void happyBirthday();


unsigned long displayUpdateTimer;
unsigned long ntpUpdateTimer;
unsigned long brightnessTimer = 0;

bool shouldUpdateNtpTime;
float brightness = 1.0;

void setup() {
    Serial.begin(115200);

    strip.Begin();
    strip.Show();

    #ifdef DEBUG_OUTPUT
    Serial.println("Connecting to WiFi");
    #endif

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        #ifdef DEBUG_OUTPUT
        Serial.print(".");
        #endif
        delay(500);
    }

    #ifdef DEBUG_OUTPUT
    Serial.println("WiFi connected");
    #endif


    do {
        ntpDateTime = NTPch.getNTPtime(1.0, 1); //1.0: GMT+1; 1: European daylight savings time
    }
    while (!ntpDateTime.valid);

    lastNtpDateTime = ntpDateTime;

    #ifdef DEBUG_OUTPUT
    Serial.print("Start time: ");
    NTPch.printDateTime(lastNtpDateTime);
    #endif

    currentTimeEstimate = lastNtpDateTime.epochTime;
    currentTimeRounded = roundDateTime(lastNtpDateTime);

    updateDisplay();
    displayUpdateTimer = millis();
    ntpUpdateTimer = millis();

}


void loop() {
    // If there has been a considerable change in brightness (20%), update the display
    if (millis() - brightnessTimer > 100) {
        float b = analogRead(A0)/1024.0;
        if (abs(b-brightness) > 5 && (b/brightness > 1.2 || b/brightness < 0.8)) {
            #ifdef DEBUG_OUTPUT
            Serial.print("brightness:");
            Serial.println(b);
            #endif
            brightness = b;
            updateDisplay();
            brightnessTimer = millis();
        }
    }


    // every ten seconds, update the display (if the time has changed)
    if (millis() - displayUpdateTimer > 10*1000) {

        unsigned long elapsed = millis() - ntpUpdateTimer;
        currentTimeEstimate = lastNtpDateTime.epochTime + elapsed/1000;
        strDateTime currentTime = NTPch.ConvertUnixTimestamp(currentTimeEstimate);

        #ifdef DEBUG_OUTPUT
        Serial.print("Current time estimate: ");
        NTPch.printDateTime(currentTime);
        #endif

        // Calculate current time, rounded to closest five minutes
        currentTimeRounded = roundDateTime(currentTime);

        // Skip the actual updating if the time hasn't changed
        if (currentTimeRounded.hour != lastDisplayUpdateTime.hour ||
            currentTimeRounded.minute != lastDisplayUpdateTime.minute)
        {
            #ifdef DEBUG_OUTPUT
            Serial.println("Rounded time has changed; updating display");
            #endif
            lastDisplayUpdateTime = currentTimeRounded;
            updateDisplay();
        }

        displayUpdateTimer = millis();
    }

    // every 20 minutes or so, get NTP time
    if (millis() - ntpUpdateTimer > 20*60*1000) {
        ntpDateTime = NTPch.getNTPtime(1.0, 1);
        if (ntpDateTime.valid) {
            lastNtpDateTime = ntpDateTime;
            ntpUpdateTimer = millis();
            #ifdef DEBUG_OUTPUT
            Serial.println("NTP time updated");
            #endif
        }
    }
}

/////// TESTS

void testPhotoresistor()
{
    delay(100);
    //Serial.println(analogRead(A0));
    int brightness = analogRead(A0);
    strip.ClearTo(black);

    for (int i = 0; i < (11*brightness)/1024; i++) {
      strip.SetPixelColor(i, green);
    }

    strip.Show();
}


strDateTime roundDateTime(strDateTime t)
{
    // Round to the closest 5 minutes; round up at 2'31''.

    // How many seconds do we need to add or subtract ?
    // Calculate new epoch time
    // Initialize new strDateTime based on that epoch time, return it.

    // if the seconds are below 30, remove them.
    int deltaS = 0;
    int deltaM = 0;

    if (t.second > 30) {
        t.minute++;
        deltaS = 60 - t.second;
    }
    else {
        deltaS = -t.second;
        t.second = 0;
    }

    byte rest = t.minute % 5;

    if (rest > 2) {
        t.minute += (5-rest);
        deltaM = 5-rest;
    }
    else {
        t.minute -= rest;
        deltaM = -rest;
    }

    unsigned long roundedEpochTime = t.epochTime + deltaS + 60*deltaM;

    return NTPch.ConvertUnixTimestamp(roundedEpochTime);
}

void updateDisplay()
{
    #ifdef DEBUG_OUTPUT
    Serial.println("Updating display");
    Serial.print("Rounded time: ");
    currentTimeRounded.valid = true;
    NTPch.printDateTime(currentTimeRounded);
    #endif
    RgbColor c = (255 * brightness);

    strip.ClearTo(black);

    // check for birthdays (April 27th and January 18th)
    /*
    // Force an arbitrary date, for testing
    currentTimeRounded.month = 1;
    currentTimeRounded.day = 18;
    */
    if ((currentTimeRounded.month == 4 && currentTimeRounded.day == 27) ||
        (currentTimeRounded.month == 1 && currentTimeRounded.day == 18))
    {
        happyBirthday();
    }



    /*
    // Force an arbitrary time, for testing
    currentTimeRounded.hour = 23;
    currentTimeRounded.minute = 45;
    */

    // Display time
    itIs(c);

    switch (currentTimeRounded.minute) {
        case 0:
            if (currentTimeRounded.hour % 12) // not noon or midnight
                oclock(c);
            break;
        case 5:
            fiveMin(c);
            past(c);
            break;
        case 10:
            tenMin(c);
            past(c);
            break;
        case 15:
            a(c);
            quarter(c);
            past(c);
            break;
        case 20:
            twenty(c);
            past(c);
            break;
        case 25:
            twenty(c);
            fiveMin(c);
            past(c);
            break;
        case 30:
            half(c);
            past(c);
            break;
        case 35:
            twenty(c);
            fiveMin(c);
            to(c);
            break;
        case 40:
            twenty(c);
            to(c);
            break;
        case 45:
            a(c);
            quarter(c);
            to(c);
            break;
        case 50:
            tenMin(c);
            to(c);
            break;
        case 55:
            fiveMin(c);
            to(c);
            break;
    }

    byte hour = currentTimeRounded.hour;

    // If it is e.g. 9:45, it should be quarter to ten => add one to the hour when using "to"
    if (currentTimeRounded.minute > 30) {
       hour += 1;
   }

    if (hour == 0 || hour == 24)
        midnight(c);
    else if (hour == 12)
        noon(c);
    else {
        hour = hour % 12;
        switch (hour) {
            case 1:
                one(c);
                break;
            case 2:
                two(c);
                break;
            case 3:
                three(c);
                break;
            case 4:
                four(c);
                break;
            case 5:
                five(c);
                break;
            case 6:
                six(c);
                break;
            case 7:
                seven(c);
                break;
            case 8:
                eight(c);
                break;
            case 9:
                nine(c);
                break;
            case 10:
                ten(c);
                break;
            case 11:
                eleven(c);
                break;
        }
    }

    strip.Show();
    #ifdef DEBUG_OUTPUT
    Serial.println("");
    #endif
}


void itIs(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("It is ");
    #endif

    strip.SetPixelColor(topo.Map(0, 0), color);
    strip.SetPixelColor(topo.Map(0, 1), color);
    strip.SetPixelColor(topo.Map(0, 3), color);
    strip.SetPixelColor(topo.Map(0, 4), color);
}
void a(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("a ");
    #endif

    strip.SetPixelColor(topo.Map(0, 10), color);
}
void fiveMin(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("five ");
    #endif

    strip.SetPixelColor(topo.Map(3, 0), color);
    strip.SetPixelColor(topo.Map(3, 1), color);
    strip.SetPixelColor(topo.Map(3, 2), color);
    strip.SetPixelColor(topo.Map(3, 3), color);
}
void tenMin(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("ten ");
    #endif

    strip.SetPixelColor(topo.Map(1, 1), color);
    strip.SetPixelColor(topo.Map(1, 2), color);
    strip.SetPixelColor(topo.Map(1, 3), color);
}
void quarter(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("quarter ");
    #endif

    strip.SetPixelColor(topo.Map(1, 4), color);
    strip.SetPixelColor(topo.Map(1, 5), color);
    strip.SetPixelColor(topo.Map(1, 6), color);
    strip.SetPixelColor(topo.Map(1, 7), color);
    strip.SetPixelColor(topo.Map(1, 8), color);
    strip.SetPixelColor(topo.Map(1, 9), color);
    strip.SetPixelColor(topo.Map(1, 10), color);
}
void twenty(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("twenty ");
    #endif

    strip.SetPixelColor(topo.Map(2, 0), color);
    strip.SetPixelColor(topo.Map(2, 1), color);
    strip.SetPixelColor(topo.Map(2, 2), color);
    strip.SetPixelColor(topo.Map(2, 3), color);
    strip.SetPixelColor(topo.Map(2, 4), color);
    strip.SetPixelColor(topo.Map(2, 5), color);
}
void half(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("half ");
    #endif

    strip.SetPixelColor(topo.Map(0, 6), color);
    strip.SetPixelColor(topo.Map(0, 7), color);
    strip.SetPixelColor(topo.Map(0, 8), color);
    strip.SetPixelColor(topo.Map(0, 9), color);
}
void past(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("past ");
    #endif

    strip.SetPixelColor(topo.Map(3, 6), color);
    strip.SetPixelColor(topo.Map(3, 7), color);
    strip.SetPixelColor(topo.Map(3, 8), color);
    strip.SetPixelColor(topo.Map(3, 9), color);
}
void to(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("to ");
    #endif

    strip.SetPixelColor(topo.Map(4, 0), color);
    strip.SetPixelColor(topo.Map(4, 1), color);
}
void one(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("one ");
    #endif

    strip.SetPixelColor(topo.Map(10, 0), color);
    strip.SetPixelColor(topo.Map(10, 1), color);
    strip.SetPixelColor(topo.Map(10, 2), color);
}
void two(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("two ");
    #endif

    strip.SetPixelColor(topo.Map(5, 8), color);
    strip.SetPixelColor(topo.Map(5, 9), color);
    strip.SetPixelColor(topo.Map(5, 10), color);
}
void three(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("three ");
    #endif

    strip.SetPixelColor(topo.Map(6, 4), color);
    strip.SetPixelColor(topo.Map(6, 5), color);
    strip.SetPixelColor(topo.Map(6, 6), color);
    strip.SetPixelColor(topo.Map(6, 7), color);
    strip.SetPixelColor(topo.Map(6, 8), color);
}
void four(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("four ");
    #endif

    strip.SetPixelColor(topo.Map(5, 1), color);
    strip.SetPixelColor(topo.Map(5, 2), color);
    strip.SetPixelColor(topo.Map(5, 3), color);
    strip.SetPixelColor(topo.Map(5, 4), color);
}
void five(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("five ");
    #endif

    strip.SetPixelColor(topo.Map(8, 7), color);
    strip.SetPixelColor(topo.Map(8, 8), color);
    strip.SetPixelColor(topo.Map(8, 9), color);
    strip.SetPixelColor(topo.Map(8, 10), color);
}
void six(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("six ");
    #endif

    strip.SetPixelColor(topo.Map(4, 3), color);
    strip.SetPixelColor(topo.Map(4, 4), color);
    strip.SetPixelColor(topo.Map(4, 5), color);
}
void seven(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("seven ");
    #endif

    strip.SetPixelColor(topo.Map(5, 0), color);
    strip.SetPixelColor(topo.Map(6, 0), color);
    strip.SetPixelColor(topo.Map(7, 0), color);
    strip.SetPixelColor(topo.Map(8, 0), color);
    strip.SetPixelColor(topo.Map(9, 0), color);
}
void eight(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("eight ");
    #endif

    strip.SetPixelColor(topo.Map(6, 0), color);
    strip.SetPixelColor(topo.Map(6, 1), color);
    strip.SetPixelColor(topo.Map(6, 2), color);
    strip.SetPixelColor(topo.Map(6, 3), color);
    strip.SetPixelColor(topo.Map(6, 4), color);
}
void nine(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("nine ");
    #endif

    strip.SetPixelColor(topo.Map(9, 0), color);
    strip.SetPixelColor(topo.Map(9, 1), color);
    strip.SetPixelColor(topo.Map(9, 2), color);
    strip.SetPixelColor(topo.Map(9, 3), color);
}
void ten(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("ten ");
    #endif

    strip.SetPixelColor(topo.Map(8, 1), color);
    strip.SetPixelColor(topo.Map(8, 2), color);
    strip.SetPixelColor(topo.Map(8, 3), color);
}
void eleven(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("eleven ");
    #endif

    strip.SetPixelColor(topo.Map(9, 3), color);
    strip.SetPixelColor(topo.Map(9, 4), color);
    strip.SetPixelColor(topo.Map(9, 5), color);
    strip.SetPixelColor(topo.Map(9, 6), color);
    strip.SetPixelColor(topo.Map(9, 7), color);
    strip.SetPixelColor(topo.Map(9, 8), color);
}
void noon(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("noon ");
    #endif

    strip.SetPixelColor(topo.Map(4, 10), color);
    strip.SetPixelColor(topo.Map(5, 10), color);
    strip.SetPixelColor(topo.Map(6, 10), color);
    strip.SetPixelColor(topo.Map(7, 10), color);
}
void midnight(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("midnight ");
    #endif

    strip.SetPixelColor(topo.Map(7, 1), color);
    strip.SetPixelColor(topo.Map(7, 2), color);
    strip.SetPixelColor(topo.Map(7, 3), color);
    strip.SetPixelColor(topo.Map(7, 5), color);
    strip.SetPixelColor(topo.Map(7, 6), color);
    strip.SetPixelColor(topo.Map(7, 7), color);
    strip.SetPixelColor(topo.Map(7, 8), color);
    strip.SetPixelColor(topo.Map(7, 9), color);
}

void oclock(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("o'clock ");
    #endif

    strip.SetPixelColor(topo.Map(10, 4), color);
    strip.SetPixelColor(topo.Map(10, 5), color);
    strip.SetPixelColor(topo.Map(10, 6), color);
    strip.SetPixelColor(topo.Map(10, 7), color);
    strip.SetPixelColor(topo.Map(10, 8), color);
    strip.SetPixelColor(topo.Map(10, 9), color);
    strip.SetPixelColor(topo.Map(10, 10), color);
}

void way(RgbColor& color)
{
    #ifdef DEBUG_OUTPUT
    Serial.print("way ");
    #endif

    strip.SetPixelColor(topo.Map(2, 8), color);
    strip.SetPixelColor(topo.Map(2, 9), color);
    strip.SetPixelColor(topo.Map(2, 10), color);
}
void happyBirthday()
{
    #ifdef DEBUG_OUTPUT
    Serial.println("Happy Birthday! ");
    #endif
    int pixels [13][2] = {{0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {3, 4}, {4, 4}, {5, 4}, {6, 4}, {7, 4}, {8, 4}, {8, 5}, {8, 6}};

    #ifdef DEBUG_OUTPUT
    Serial.print("Switching LEDs: ");
    #endif
    for (int i(0); i < 13; ++i) {
        RgbColor c(rand()%256 * brightness, rand()%256 * brightness,rand()%256 * brightness);
        strip.SetPixelColor(topo.Map(pixels[i][0], pixels[i][1]), c);
        delay(50);
        strip.Show();

        #ifdef DEBUG_OUTPUT
        Serial.printf("%d; %d\n", pixels[i][0], pixels[i][1]);
        #endif
    }
}
