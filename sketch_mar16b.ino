#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
// For the breakout, you can use any 2 or 3 pins
// These pins will also work for the 1.8" TFT shield
#define TFT_CS     5
#define TFT_RST    16  // you can also connect this to the Arduino reset
                       // in which case, set this #define pin to -1!
#define TFT_DC     4

// Option 1 (recommended): must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)

// For 1.44" and 1.8" TFT with ST7735 use
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

// For 1.54" TFT with ST7789
//Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS,  TFT_DC, TFT_RST);

// Option 2: use any pins but a little slower!
//#define TFT_SCLK    // set these to be whatever pins you like!
//#define TFT_MOSI D1   // set these to be whatever pins you like!
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

WiFiClient client;

//Week Days
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// Your Domain name with URL path or IP address with path
String openWeatherMapApiKey = "a31375e54127c9fb873bd81642cbb2b5";
// Example:
//String openWeatherMapApiKey = "bd939aa3d23ff33d3c8f5dd1dd435";

String jsonBuffer;
String quotationJson;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(2, 3);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextWrap(true);
  tft.print("Hello! Waiting for connection...");

  WiFi.begin("TP-LINK_B4DE", "24398251");

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(10800);
  tft.fillScreen(ST77XX_BLACK);

}

void loop() {
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();
  tft.setCursor(2, 3);
  tft.setTextSize(3);
  tft.print(formattedTime);
  tft.setCursor(2, 30);
  tft.setTextSize(1);
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;
  tft.print(weekDays[timeClient.getDay()] + ", " + ptm->tm_mday + " " + months[currentMonth-1]);
  tft.setCursor(2, 42);
  tft.print(currentYear);
  tft.setCursor(2, 52);
  char str[64];
  sprintf(str, "Temp: %s\176C", get_temp(0));
  tft.print(str);
  tft.setCursor(2, 62);
  sprintf(str, "Wind Speed: %smps", get_temp(2));
  tft.print(str);
  tft.setCursor(2, 72);
  sprintf(str, "Weather: %s(%s)", get_temp(3), get_temp(4));
  tft.print(str);
  tft.setCursor(2, 82);
  tft.print(get_temp(5));
  delay(100);
  
}

String quotationRequest(const char* serverName) {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    // Ignore SSL certificate validation
    client->setInsecure();
    
    //create an HTTPClient instance
    HTTPClient https;
    
    //Initializing an HTTPS communication using the secure client
    Serial.print("[HTTPS] begin...\n");
    https.begin(*client, serverName);
    int httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String payload = https.getString();
      Serial.println(payload);
      return payload;
    } else {
      return "None";
    }
  } else {
      return "None";
  }
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

static String get_temp(int i) {
  static String temp;
  static String humidity;
  static String wind_speed;
  static String main;
  static String description;
  static String quotation;
  static String quotationAuthor;
  static unsigned long prev = -300007;
  unsigned int now = millis();
  if (now - prev > 5 * 60000) {
    prev = now;
    //JSONVar ipJSON = JSON.parse(quotationRequest("https://ipapi.co/json"));
    //Serial.println(ipJSON);
    //static String city = JSON.stringify(ipJSON["city"]).substring(1, JSON.stringify(ipJSON["city"]).length() - 1);
    //static String countryCode = JSON.stringify(ipJSON["country_code"]).substring(1, JSON.stringify(ipJSON["country_code"]).length() - 1);
    static String city = "Ramenskoye";
    static String countryCode = "RU";
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      
    jsonBuffer = httpGETRequest(serverPath.c_str());
    Serial.println(jsonBuffer);
    JSONVar myObject = JSON.parse(jsonBuffer);
    JSONVar quotationJSON = JSON.parse(quotationRequest("https://api.quotable.io/random?maxLength=50"));

    // JSON.typeof(jsonVar) can be used to get the type of the var
    if (JSON.typeof(myObject) == "undefined") {
      Serial.println("Parsing input failed!");
    }
  
    Serial.print("JSON object = ");
    Serial.println(myObject);
    Serial.print("Temperature: ");
    Serial.println(strtof(JSON.stringify(myObject["main"]["temp"]).c_str(), 0) - 273.15f);
    Serial.print("Pressure: ");
    Serial.println(myObject["main"]["pressure"]);
    Serial.print("Humidity: ");
    Serial.println(myObject["main"]["humidity"]);
    Serial.print("Wind Speed: ");
    Serial.println(myObject["wind"]["speed"]);
    humidity = JSON.stringify(myObject["main"]["humidity"]).c_str();
    if (strtof(JSON.stringify(myObject["main"]["temp"]).c_str(), 0) - 273.15f != 0.00) {
      temp = String(strtof(JSON.stringify(myObject["main"]["temp"]).c_str(), 0) - 273.15f);
    } else {
      temp = "0";
    }
    wind_speed = JSON.stringify(myObject["wind"]["speed"]).c_str();
    main = JSON.stringify(myObject["weather"][0]["main"]).substring(1, JSON.stringify(myObject["weather"][0]["main"]).length() - 1);
    description = JSON.stringify(myObject["weather"][0]["description"]).substring(1, JSON.stringify(myObject["weather"][0]["description"]).length() - 1).c_str();
    Serial.println(description);
    quotation = JSON.stringify(quotationJSON["content"]).substring(1, JSON.stringify(quotationJSON["content"]).length() - 1);
    quotationAuthor = JSON.stringify(quotationJSON["author"]).substring(1, JSON.stringify(quotationJSON["author"]).length() - 1);
    Serial.println(quotation);
    Serial.println(quotationAuthor);

    tft.fillScreen(ST77XX_BLACK);
  }
  if (i == 0) {
    return temp;
  } else if (i == 1) {
    return humidity;
  } else if (i == 2) {
    return wind_speed;
  } else if (i == 3) {
    return main;
  } else if (i == 4) {
    return description.c_str();
  } else if (i == 5) {
    char str[64];
    sprintf(str, "%s - %s", quotation.c_str(), quotationAuthor.c_str());
    return str;
  } else {
    return "0.00";
  }
}

/* Recode russian fonts from UTF-8 to Windows-1251 
String utf8rus(String source)
{
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length(); i = 0;

  while (i < k) {
    n = source[i]; i++;

    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break; }
          if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB8; break; }
          if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
return target;
}
*/