#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <time.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <UnixTime.h>

#include "FS.h"
#include "sntp.h"
#include "vars.h"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;  // last time messages' scan has been done
UnixTime stamp(-7);

void calculateTime(uint32_t timeInStamp, uint32_t timeOutStamp, uint32_t lunchtimeStart, uint32_t lunchTimeStop) {
  // Date format = MM/DD/YYYY
  // Time format = HH:MM:SS
  uint8_t timeWorked = timeOutStamp - timeInStamp;
  uint8_t lunchTime = lunchTimeStop - lunchtimeStart;
  uint8_t totalTime = timeWorked - lunchTime;

  Serial.println(totalTime);
}

void printClockFile() {
  File file = LittleFS.open(clockFilePath, "r");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

String clockFileToString() {
  char jsonFileBuffer[512];
  StaticJsonDocument<512> jsonDoc;
  File clockFile = LittleFS.open(clockFilePath, "r");
  deserializeJson(jsonDoc, clockFile);
  clockFile.close();
  serializeJsonPretty(jsonDoc, jsonFileBuffer);
  return (String)jsonFileBuffer;
}

StaticJsonDocument<200> getClockData() {
  File jsonClockFile = LittleFS.open(clockFilePath, "r");
  StaticJsonDocument<200> jsonDoc;
  deserializeJson(jsonDoc, jsonClockFile);
  jsonClockFile.close();

  return jsonDoc;
}

bool clockIn(tm timeinfo) {
  String dateString;
  String response;
  if (isClockedIn()) {
    bot.sendMessage(CHAT_ID, ALREADY_CLOCKED_IN_MESSAGE, "Markdown");
    return false;
  }
  // Serial.println(isClockedIn());
  File clockFile = LittleFS.open(clockFilePath, "w");
  StaticJsonDocument<200> jsonDoc;
  deserializeJson(jsonDoc, clockFile);
  dateString = String((timeinfo.tm_mon + 1)) + "/" + String(timeinfo.tm_mday) + "/" + (timeinfo.tm_year + 1900);  // Had to wrap timeinfo.tm_mday in a String() for some reason.

  jsonDoc["date"] = dateString;
  jsonDoc["time_in"] = String(toStringAddZero(timeinfo.tm_hour) + ":" + toStringAddZero(timeinfo.tm_min) + ":" + toStringAddZero(timeinfo.tm_sec));
  jsonDoc["is_clocked_in"] = true;
  serializeJson(jsonDoc, clockFile);
  clockFile.close();
  printClockFile();

  response = "Clocked in at " + toStringAddZero(timeinfo.tm_hour) + ":" + toStringAddZero(timeinfo.tm_min) + ":" + toStringAddZero(timeinfo.tm_sec) + " on \n"
             + (timeinfo.tm_mon + 1) + "/" + timeinfo.tm_mday + "/" + (timeinfo.tm_year + 1900);
  bot.sendMessage(CHAT_ID, response, "Markdown");
  return true;
}

bool clockOut(tm timeinfo) {
  String response;
  if (!isClockedIn()) {
    bot.sendMessage(CHAT_ID, ALREADY_CLOCKED_OUT_MESSAGE, "Markdown");
    return false;
  }
  StaticJsonDocument<200> oldClockData = getClockData();  // We want to re-populate the fields that we aren't updating with their previous values.
  StaticJsonDocument<200> jsonDoc;
  File clockFile = LittleFS.open(clockFilePath, "w");
  deserializeJson(jsonDoc, clockFile);

  jsonDoc["date"] = oldClockData["date"];
  jsonDoc["time_in"] = oldClockData["time_in"];
  jsonDoc["time_out"] = String(toStringAddZero(timeinfo.tm_hour) + ":" + toStringAddZero(timeinfo.tm_min) + ":" + toStringAddZero(timeinfo.tm_sec));
  jsonDoc["lunch_time_start"] = oldClockData["lunch_time_start"];
  jsonDoc["lunch_time_stop"] = oldClockData["lunch_time_stop"];
  jsonDoc["is_clocked_in"] = false;
  jsonDoc["work_notes"] = String("Notes and shit");
  jsonDoc["total_hours"] = String("a bunch of hours");

  if (!serializeJson(jsonDoc, clockFile)) return false;
  clockFile.close();
  printClockFile();
  // Serial.println(isClockedIn());
  response = "Clocked out at " + toStringAddZero(timeinfo.tm_hour) + ":" + toStringAddZero(timeinfo.tm_min) + ":" + toStringAddZero(timeinfo.tm_sec) + " on \n"
             + (timeinfo.tm_mon + 1) + "/" + timeinfo.tm_mday + "/" + (timeinfo.tm_year + 1900);
  bot.sendMessage(CHAT_ID, response, "Markdown");
  return true;
}

bool toggleLunchStatus(tm timeinfo) {
  String response;
  if (!clockFileExists()) {
    bot.sendMessage(CHAT_ID, "Clockfile not found, this is bad...");
    return false;
  }
  if (!isAtLunch()) {
    StaticJsonDocument<200> jsonDoc;
    StaticJsonDocument<200> oldJsonDoc = getClockData();  // Any fields we aren't updating will need to be re-written with it's old values so we don't loose data.
    File clockFile = LittleFS.open(clockFilePath, "w");
    deserializeJson(jsonDoc, clockFile);
    Serial.println("Lunch Status: " + String(isAtLunch()));
    // Start lunch and return
    jsonDoc["date"] = oldJsonDoc["date"];
    jsonDoc["time_in"] = oldJsonDoc["time_in"];
    jsonDoc["lunch_time_start"] = String(toStringAddZero(timeinfo.tm_hour) + ":" + toStringAddZero(timeinfo.tm_min) + ":" + toStringAddZero(timeinfo.tm_sec));
    jsonDoc["is_clocked_in"] = false;
    serializeJson(jsonDoc, clockFile);
    clockFile.close();
    printClockFile();
    response = String("Started lunch at " + String(toStringAddZero(timeinfo.tm_hour) + ":" + toStringAddZero(timeinfo.tm_min) + ":" + toStringAddZero(timeinfo.tm_sec) + ".  Enjoy your lunch!"));
    bot.sendMessage(CHAT_ID, response, "Markdown");
    return true;
  }
  // Else end lunch and return
  StaticJsonDocument<200> jsonDoc;
  StaticJsonDocument<200> oldJsonDoc = getClockData();  // Any fields we aren't updating will need to be re-written with it's old values so we don't loose data.
  File clockFile = LittleFS.open(clockFilePath, "w");
  deserializeJson(jsonDoc, clockFile);
  jsonDoc["date"] = oldJsonDoc["date"];
  jsonDoc["time_in"] = oldJsonDoc["time_in"];
  jsonDoc["lunch_time_start"] = oldJsonDoc["lunch_time_start"];
  jsonDoc["lunch_time_stop"] = String(toStringAddZero(timeinfo.tm_hour) + ":" + toStringAddZero(timeinfo.tm_min) + ":" + toStringAddZero(timeinfo.tm_sec));
  jsonDoc["is_clocked_in"] = true;
  serializeJson(jsonDoc, clockFile);
  clockFile.close();
  printClockFile();
  response = String("You ended your lunch at " + toStringAddZero(timeinfo.tm_hour) + ":" + toStringAddZero(timeinfo.tm_min) + ":" + toStringAddZero(timeinfo.tm_sec) + ".  Welcome back!");
  bot.sendMessage(CHAT_ID, response, "Markdown");
  return true;
}

bool isClockedIn() {
  File clockFile = LittleFS.open(clockFilePath, "r");
  StaticJsonDocument<200> jsonDoc;
  deserializeJson(jsonDoc, clockFile);
  clockFile.close();

  if (jsonDoc["is_clocked_in"]) {
    return true;
  }
  return false;
}

bool isAtLunch() {
  File clockFile = LittleFS.open(clockFilePath, "r");
  StaticJsonDocument<200> jsonDoc;
  deserializeJson(jsonDoc, clockFile);
  clockFile.close();
  String myThing = jsonDoc["lunch_time_start"];
  Serial.println("Lunch time start: " + myThing);
  if (jsonDoc["lunch_time_start"] != NULL && jsonDoc["is_clocked_in"] == false) {
    return true;
  }
  return false;
}

bool clockFileExists() {
  File clockFile = LittleFS.open("/clock.json", "r", false);
  if (!clockFile) {
    clockFile.close();
    return false;
  }
  clockFile.close();
  return true;
}

bool createClockFile() {
  File clockFile = LittleFS.open("/clock.json", "w");
  clockFile.print(CLOCKFILE_JSON_STRING);
  clockFile.close();

  if (!LittleFS.open(clockFilePath, "r")) {
    return false;
  }
  return true;
}

void handleMessages(int numNewMessages) {
  String response;
  // Get current time
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  for (int i = 0; i < numNewMessages; i++) {
    telegramMessage &msg = bot.messages[i];
    Serial.println("Got " + msg.text);

    if (msg.text == start) {
      // Make sure little fs is set up properly
      if (!clockFileExists()) {
        Serial.println("Clockfile not found!  Attempting to create.");
        createClockFile() ? bot.sendMessage(CHAT_ID, "Successfully generated new Clockfile! :D") : bot.sendMessage(CHAT_ID, "Failed to create clockfile... :(");
      }

      response = "You have started the time bot.";
    } else if (msg.text == help) {
      response = "You need help?  No help for you!";
    } else if (msg.text == in) {
      // response = "Clocked in at " + toStringAddZero(timeinfo.tm_hour) + ":" + toStringAddZero(timeinfo.tm_min) + ":" + toStringAddZero(timeinfo.tm_sec) + " on \n"
      //            + (timeinfo.tm_mon + 1) + "/" + timeinfo.tm_mday + "/" + (timeinfo.tm_year + 1900);
      clockIn(timeinfo);
    } else if (msg.text == out) {
      // response = "Clocked out at " + toStringAddZero(timeinfo.tm_hour) + ":" + toStringAddZero(timeinfo.tm_min) + ":" + toStringAddZero(timeinfo.tm_sec) + " on \n"
      //            + (timeinfo.tm_mon + 1) + "/" + timeinfo.tm_mday + "/" + (timeinfo.tm_year + 1900 + ".  Generating daily CSV report.");
      // Clock out
      clockOut(timeinfo);
      // Calculate time
    } else if (msg.text == lunch) {
      // response = "Started lunch at " + toStringAddZero(timeinfo.tm_hour) + ":" + toStringAddZero(timeinfo.tm_min) + ":" + toStringAddZero(timeinfo.tm_sec) + ".  Have a good lunch!";
      // Start Lunch
      toggleLunchStatus(timeinfo);
    } else if (msg.text == status) {
      // Show status
      response = clockFileToString();
    }
    bot.sendMessage(CHAT_ID, response);  // For some reason if we put "Markdown" as the third parameter it causes the user to be unable to check status while clocked in.
  }
}

String toStringAddZero(int data) {
  String st = "";
  if (data < 10) {
    st = "0" + String(data);
  } else {
    st = String(data);
  }
  return st;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  LittleFS.begin(true);

  sntp_servermode_dhcp(1);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  configTzTime(time_zone, ntpServer1, ntpServer2);  // Set
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);  // Add root certificate for api.telegram.org

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting...");
    delay(500);
  }

  /*
  Commands:
    in
    out
    lunch
    status
    help
  */
  const String COMMANDS = F("["
                            "{\"command\": \"in\", \"description\": \"Clock In\"},"
                            "{\"command\": \"out\", \"description\": \"Clock Out\"}"
                            "{\"command\": \"lunch\", \"description\": \"Start or end lunch break\"}"
                            "{\"command\": \"status\", \"description\": \"Check current clock status\"}"
                            "{\"command\": \"help\", \"description\": \"Show a help message explaining how to use this software.\"}"
                            "]");

  bot.setMyCommands(COMMANDS);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (millis() - timeSinceLastMessage > 1000) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("Got Message");
      handleMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    timeSinceLastMessage = millis();
  }
}