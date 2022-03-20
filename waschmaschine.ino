#include <WiFi.h>
#include <WebServer.h>

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h> 

#include <HTTPClient.h>

#include "credential.h"

Adafruit_ADS1115 ads1115; // Construct an ads1115

//Netzspannung 230 V ±10 %

// Laut Hersteller 30[A/V]
// Kalibrierung erfolgte mit Haushaltsgeräten
const float factor = 0.030616891; // [A/mv]
int sample_rate = 2000; // [ms] zwei Sekunden lang messen
int strompreis = 28; // [cent/kWh]

String hostname = "Waschmaschine";

String text= "";
double current;
float power;
int p_max = 0;
float E = 0;
float kosten = 0;
unsigned long currentMillis = 0;
int stunde, minuten, sekunde;
int counter = 0;


WebServer server(80);

// database
HTTPClient http;
  
void WiFiConnect() {
  WiFi.mode(WIFI_STA); // prevent sleeping
  WiFi.setHostname(hostname.c_str()); //define hostname
  WiFi.begin(ssid, password);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi..");
    delay(1000);
    // schould take longer than 10 seconds
    count = count + 1;
    if (count > 10){
      // wait 2 minutes before try again
      delay(2*60*1000);
      ESP.restart();
    }
  }
  // Print ESP32 Local IP Address
  Serial.println("Connected");
  Serial.println(WiFi.localIP());
}

// Replace dots with commas.
String replace_dot(float number){
  String text = String(number, 2);
  text.replace(".", ",");
  return(text);
}

// 1000 Watt to 1.000 Watt
String add_dot(int number) {
	
	if (number >= 1000) {   
		String text = String(number/1000); 
		text += "."; 
    
		if ( ((number%1000) >= 10) & ((number%1000) < 100)) {
			text += "0";
		}
    
		if ((number%1000) < 10) {
			text += "00";
		}
    
		text += String(number%1000);
		
		return(text);
	}
	else {
		return(String(number));
	}

}

void sendMessage() {
  WiFiClientSecure client;
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  UniversalTelegramBot bot(BOTtoken, client);
  String mes = "Übersicht:\n";
  mes += "Maximale Leistung: " + add_dot(p_max) + " Watt";
  mes += "\n";
  mes += "Energie: " + replace_dot(E/1000) + " kWh";
  mes += "\n";
  mes += "Kosten: " + replace_dot(kosten) + " €";
  mes += "\n";
  mes += "Dauer: ";
  if (stunde == 1) {
	mes += String(stunde) + " Stunde und ";
  }
  else {
	mes += String(stunde) + " Stunden und ";
  }
  if (minuten == 1){
	mes += String(minuten) + " Minuten.";
  } else {
	mes += String(minuten) + " Minute.";
  }
  mes += "\n";
  mes += WiFi.localIP().toString();
  bot.sendMessage(CHAT_ID, mes, ""); // https://core.telegram.org/bots/api#formatting-options
}

float getcurrent() {
  int bits;
  float voltage;
  float current;
  float current_sum = 0;
  int i = 0;
  unsigned long time_check = millis();

  while(millis() - time_check < sample_rate){
    
    bits = ads1115.readADC_Differential_0_1();
    // threshold for measurement
    if (abs(bits) <= 6) {
      bits = 0;
    }
    // 16 bit -> 2^(16-1) = 32768 bits  4x gain: 1.024V -> 1.024V/32768bits
    voltage = bits * (1.024*1000)/32768; // [mv]
    current = voltage * factor; // [A]
    current_sum += sq(current); // [A^2]
    i = i + 1;
 
  }  
  
  current = sqrt(current_sum/i); // [A]
  return(current);
  
}

const String page PROGMEM = "<head>"
            " <title>Waschmaschine</title>"
            " <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js\"></script>"
            " <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">"
            " <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
            " <style>"
            " body {color: #eee;"
            " background: #121212;"
            " font: 100% system-ui;}"
            " a {color: #809fff;}"
            " h2 {display: inline-block; margin: .2em;}"
            " </style>"
            " </head>"
            " <body>"
            " <h1>Waschmaschine</h1>"
            " <h2>Strom:</h2><h2 id=\"current\">-,---</h2><h2>A</h2><br>\r\n"
            " <h2>Leistung:</h2><h2 id=\"power\">-,--</h2><h2>W</h2><br>\r\n" 
            " <h2>P<sub>maximal</sub>:</h2><h2 id=\"p_max\">-</h2><h2>W</h2><br>\r\n"
            " <h2>Energie:</h2><h2 id=\"energie\">-,--</h2><h2>Wh</h2><br>\r\n"
            " <h2>Kosten:</h2><h2 id=\"kosten\">-,--</h2><h2>€</h2><br>\r\n"
            " <h2>Läuft seit:</h2><h2 id=\"runtime\">--:--:--</h2><br>\r\n"
            " <h2>Status:</h2><h2 id=\"status\">-</h2><br>\r\n"
            " <p>Arbeitspreis: " + String(strompreis) + " Cent/kWh</p>"
            " <script>\r\n"
            " $(document).ready(function(){\r\n"
            " setInterval(getData,2000);\r\n"
            " function getData(){\r\n"
            " $.ajax({\r\n"
            "  type:\"GET\",\r\n"
            "  url:\"data\",\r\n"
            "  success: function(data){\r\n"
            "  var s = data.split(\'-\');\r\n"
            "  $('#current').html(s[0].replace(\'.\',\',\'));\r\n"
            "  $('#power').html(s[1].replace(\'.\',\',\'));\r\n"
            "  $('#p_max').html(s[2]);\r\n"
            "  $('#energie').html(s[3].replace(\'.\',\',\'));\r\n"
            "  $('#kosten').html(s[4].replace(\'.\',\',\'));\r\n"
            "  $('#runtime').html(s[5]);\r\n"
            "  $('#status').html(s[6]);\r\n"
            "}\r\n"
            "}).done(function() {\r\n"
            "  console.log('ok');\r\n"
            "})\r\n"
            "}\r\n"
            "});"
            "</script>\r\n"
            "</body>";
            
void setup() {
  Serial.begin(115200);

  WiFiConnect(); 
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());

  // ads.setGain(GAIN_FOUR); // 4x gain   +/- 1.024V  1 bit = 0.03125mV
  ads1115.setGain(GAIN_FOUR);
  ads1115.setDataRate(RATE_ADS1115_860SPS);
  ads1115.begin(); // Initialize ads1015 at the default address 0x48
  
  server.on("/data", [](){
    text = String(current,3);
    text += "-";
    text += (String)power;
    text += "-";
    text += (String)p_max;
    text += "-";
    text += (String)E;
    text += "-";
    text += (String)kosten;
    text += "-";
    if(stunde < 10){
      text += "0";
    }
    text += String(stunde);
    text += ":";
    if(minuten < 10){
      text += "0";
    }
    text += (String)minuten;
    text += ":";
    if(sekunde < 10){
      text += "0";
    }
    text += (String)sekunde;
    text += "-";
    if (counter >= 40){
      text += "Fertig";
    }
    else {
      text += "Läuft";
    }
    //Serial.println(text);
    server.send(200, "text/plain", text);
  });
 
  server.on("/", []() {
   server.send(200, "text/html", page);
  });
 
  server.begin();
  Serial.println("HTTP server started");

  http.setReuse(true);
 
}

void loop() {
  
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks
  if (WiFi.status() != WL_CONNECTED){
    WiFiConnect(); 
  }

  current = getcurrent();

  http.begin(webpage);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST("current=" + String(current, 3) + "&key=" + key);
  http.end();
  
  power = 230 * current; // [W]
  E = E + power * sample_rate/(1000*60*60); // [Wh]

  kosten = (E * strompreis) / 100000 ; // [€]
  
  if(power > p_max) {
    p_max = round(power);
  }

  // wenn nach 18 Minuten im Idle, fange an zu prüfen ob fertig
  if ((current < 0.05) & (currentMillis > 1080000)) {
    counter = counter + 1;
  }
  else {
    counter = 0;
  }
  
  // Nachricht nur einmal verschicken
  // Von 27 Waschvorgängen, waren 30 Messpunkte am Stück im Idle * 1,5 Sicherheitsfaktor = 45
  // 45*2 Sekunden = 1,5 Minuten. -> 1,5 Minuten nach Beenden des Waschvorgangs, sollte die Nachricht kommen
  // & messege_sent = False nicht eingebaut, falls die Berechnungen falsch sind...
  if (counter == 45) {
    sendMessage();  
  }

  currentMillis = millis();
  
  sekunde = (currentMillis/1000) % 60;
  minuten = (currentMillis/60000) % 60;
  stunde  = (currentMillis/3600000) % 24;



  /*
  Serial.print("Strom: ");
  Serial.print(current, 6);
  Serial.print(" A | ");
  Serial.print("Power: ");
  Serial.print(power,6);
  Serial.println(" W");
  */
  
}
