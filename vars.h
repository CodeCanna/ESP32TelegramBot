// Wifi network station credentials
#define WIFI_SSID "ChimkenChonkJr"
#define WIFI_PASSWORD "froogal&julz4ever@home"
// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "5963101542:AAEr2qKrbuEOLvpXDNvkSD3_5GgcsUSAiqA"
#define CHAT_ID "779473861"

// Error Messages
#define ALREADY_CLOCKED_IN_MESSAGE "You are already clocked in, no changes made."
#define ALREADY_CLOCKED_OUT_MESSAGE "You aren't clocked in, no changes made."


const unsigned long BOT_MTBS = 1000;  // mean time between scan messages
unsigned long timeSinceLastMessage;

// Timezone and timeywimey stuff
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

const char* time_zone = "MST7MDT,M3.2.0,M11.1.0";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

// Commands
const String start = "/start";
const String in = "/in";
const String out = "/out";
const String lunch = "/lunch";
const String status = "/status";
const String help = "/help";

const String CLOCKFILE_JSON_STRING = R"END(
{
  "date": "",
  "time_in": "",
  "time_out": "",
  "lunch_time_start": "",
  "lunch_time_stop": "",
  "is_clocked_in": false,
  "work_notes": "",
  "total_hours": ""
}
)END";

const String clockFilePath = "/clock.json";