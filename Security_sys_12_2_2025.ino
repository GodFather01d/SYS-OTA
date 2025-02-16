  #include <WiFiManager.h>  
  #include <EEPROM.h>
  #include <FirebaseESP8266.h>
  #include <addons/TokenHelper.h>
  #include <addons/RTDBHelper.h>
  #include <Ticker.h>
  #include <WiFiUdp.h>
  #include <NTPClient.h>
  #include <TimeLib.h>
  #include <WiFiClientSecure.h>
  #include <ESP8266httpUpdate.h>

  #define wifiLed 2            //  D4   wifi led
  #define TRIGGER_PIN 14       //  D5   wifi setup pin access point

  #define API_KEY "AIzaSyDM4F9Ca8XhdNJTDtQxeKV4uKlaTvhBDoA"
  #define DATABASE_URL "security-system-6ec5c-default-rtdb.firebaseio.com/"

 // volatile char* GITHUB_BIN_URL = "https://raw.githubusercontent.com/GodFather01d/SYS-OTA/main/SYS_26_10_2024.ino.bin";

  WiFiClientSecure client;  
  #define EEPROM_SIZE 512

  #define sys_on_time 20
  #define sys_off_time 15
  #define SYS_ID_ADDR 0
      // access point start pin 
  #define EEPROM_UPDATE 30
  #define UPDATE_LINK_ADDER 35

  char sys_id[32]; 
  float on_time;
  float off_time;
  volatile bool buttonPressed = false;


  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;
  bool signupOK = false;
 
  const long utcOffsetInSeconds = 19800;
  char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

  Ticker timer,timer1;
  unsigned int sys_ON = 0,sys_OFF = 0, sys_PANIC_OFF = 0 ,sys_PANIC_ON = 0 ,hours_ontime = 0,mint_ontime = 0,hours_offtime = 0,notify = 0,DOOR_OPEN = 0;;
  unsigned int mint_offtime = 0, ontime_flag = 0, offtime_flag = 0, time_hours = 0, time_mint = 0, panic = 0, SYS_STATE,RESET = 0, panic_counter = 0;
  unsigned int restart = 0,ADD_BUTTON = 0,DELETE_BUTTON = 0,CLEAR_BUTTON = 0,panic_flag = 0 ,currentYear = 0,currentMonth = 0,currentDay = 0,PANIC_DELAY = 5;
  unsigned int online_flag = 0, offline_flag = 0, online_flag2 = 0, offline_flag2 = 0;
  unsigned int bit_value = 0, bit_set_flag = 0;
  unsigned int reset_time = 60*240;
  unsigned int reset_counter = 0;
  unsigned char JT_PWRON_Flag = 0,JT_PWROFF_Flag = 0,JT_SYSON_Flag = 0,JT_SYSOFF_Flag = 0,JT_PANIC_Flag = 0,JT_BATTERY_OK_Flag = 0,JT_BATTERY_LOW_Flag = 0,JT_DOOR_OPEN_Flag =0;
  unsigned char JT_OK_Flag = 0,send_hystory_flag = 0;
  unsigned int doorNumber = 0;
  WiFiManager wm;
  int update_ota = 0;
  void IRAM_ATTR handleButtonPress() {
    // Set flag to indicate button press
    buttonPressed = true;
  }
  void setup()
  {
    wm.setConfigPortalTimeout(1);
    wm.autoConnect("JT_WIFI_SETUP","Jyoti@2000");
    pinMode(wifiLed ,OUTPUT);
    pinMode(TRIGGER_PIN ,INPUT_PULLUP);
    EEPROM.begin(EEPROM_SIZE);
    update_ota =  EEPROM.read(EEPROM_UPDATE);
    readSysIdFromEEPROM();
    Serial.begin(9600);
    delay(1000);
    Serial.print("update : ");
    Serial.println(update_ota);

    if(WiFi.status() == WL_CONNECTED ) 
    {
       Serial.println("connected");

      if(update_ota == 1) 
      {
        String link = readUpdateLinkFromEEPROM();
        EEPROM.put(EEPROM_UPDATE, 0);
        EEPROM.commit();
        checkForUpdates(link);
      }
      else
      {
         EEPROM.put(EEPROM_UPDATE, 0);
         EEPROM.commit();
 
      }
      
    }
    if(WiFi.status() == WL_CONNECTED ) 
    { 
      timeClient.begin();
      config.api_key = API_KEY;
      config.database_url = DATABASE_URL;
      if (Firebase.signUp(&config, &auth, "", ""))
      {
        signupOK = true;
        config.token_status_callback = tokenStatusCallback; 
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
        delay(1000);
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BUTTONS/ADD_BUTTON", 0);
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BUTTONS/DELETE_BUTTON", 0);
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BUTTONS/CLEAR_BUTTON", 0);
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BUTTONS/PANIC_FROM_APP", 0);
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/SYSTEM_ON_OFF_TIME/RESET", 0);
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/SIREN_ON", 0);



        if (Firebase.getFloat(fbdo,"SYSTEM/" + String(sys_id) + "/SYSTEM_ON_OFF_TIME/ON_TIME"))
          {
          float  value = fbdo.floatData();
          on_time = value;
          }
        if (Firebase.getFloat(fbdo,"SYSTEM/" + String(sys_id) + "/SYSTEM_ON_OFF_TIME/OFF_TIME")) 
          {
          float  value = fbdo.floatData();
          off_time = value;
          }
        delay(1000);
      }
      else 
      {
        //Serial.printf("%s\n", config.signer.signupError.message.c_str());
      }
      digitalWrite(wifiLed, LOW);  
    }
    else
    {
      digitalWrite(wifiLed, HIGH); 

    }
    hours_ontime = on_time;
    mint_ontime = (on_time - hours_ontime)*100 ;
    hours_offtime = off_time ;
    mint_offtime = (off_time - hours_offtime)*100 ;
    timer.attach(1, task);
   // timer1.attach(1, panic_off);
     attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), handleButtonPress, CHANGE);



  }

  void task()
  {
    if(reset_counter++ >= reset_time)
    {
        ESP.restart();
    }
    
  }
  void loop()
  {
    if (buttonPressed || digitalRead(TRIGGER_PIN) == LOW)
     {
     // delay(500); 
        buttonPressed = false;

        setupConfigPortal();
      
     }
    delay(500);
    settime();
    delay(500);
    readata();
    delay(500);
    handledata();
    if(RESET == 1)
      {
         ESP.restart();
      }
   
     
  }
  void settime()
  {
    if(WiFi.status() == WL_CONNECTED) 
    {
       if (buttonPressed) return;
      online_flag2 = 1;
      timeClient.update();
      time_hours = timeClient.getHours();
      time_mint = timeClient.getMinutes();
      unsigned long epochTime = timeClient.getEpochTime();
      currentYear = year(epochTime);
      currentMonth = month(epochTime);
      currentDay = day(epochTime);
      

 
      if (time_hours == hours_ontime && time_mint == mint_ontime) 
      {
        if(ontime_flag < 2)
        {
          Serial.println("JT_SYS_ON");
          delay(500);
          ontime_flag++; 
        }
      }
      else
      {
        ontime_flag = 0;
      }
      if (time_hours == hours_offtime && time_mint == mint_offtime) 
      {
        if (offtime_flag < 2)
        {
          Serial.println("JT_SYS_OFF");
          delay(500);
          offtime_flag++;
        }
      }
      else
      {
        offtime_flag = 0;
      }
    }
  }
   void sendnotification(String sys_path)
  {
     Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/"+ sys_path +"/HR", time_hours);
     Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/" + sys_path + "/MIN", time_mint);
     Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/"+ sys_path + "/" + sys_path, 1);
  }
  void sendhistory(String sys_path)
  {
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/"+ sys_path +"/MONTH", currentMonth);  
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/"+ sys_path +"/DAY", currentDay);  
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/"+ sys_path +"/YEAR", currentYear); 
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/"+ sys_path +"/HR", time_hours); 
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/"+ sys_path +"/MIN", time_mint);  
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/SIREN_ON", 1);
      if(sys_path == "DOOR_HISTORY")
      {
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/DOOR_HISTORY/DOOR_OPEN", doorNumber); 
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/DOOR_OPEN", doorNumber);
      }
      send_hystory_flag = 0;
  }
  void handledata()
   {
    if(WiFi.status() == WL_CONNECTED) 
    {
       if (buttonPressed) return;
      if(panic == 1)
      {
          Serial.println();
          Serial.println("JT_PANIC");
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BUTTONS/PANIC_FROM_APP", 0);
      }
     if(ADD_BUTTON == 1)
      {
        Serial.println();
        Serial.println("JT_ADD");
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BUTTONS/ADD_BUTTON", 0);
      }
     if(DELETE_BUTTON == 1)
      {
        Serial.println();
        Serial.println("JT_DEL");
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BUTTONS/DELETE_BUTTON", 0);
      }
     if(CLEAR_BUTTON == 1)
      {
        Serial.println();
        Serial.println("JT_CLR");
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BUTTONS/CLEAR_BUTTON", 0);
      }
     if(sys_ON == 1)
      {
        Serial.println();
        Serial.println("JT_SYS_ON");
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BUTTONS/ON_BUTTON", 0);
      }
     if(sys_OFF == 1)
      {
        Serial.println();
        Serial.println("JT_SYS_OFF ");
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BUTTONS/OFF_BUTTON", 0);
      }
     if(Serial.available()) 
      {
        String s =  Serial.readStringUntil('\n');
        char inputCharArray[s.length() + 1];
        s.toCharArray(inputCharArray, s.length() + 1);
        if (strcmp(inputCharArray, "JT_SYS_ON") == 0) 
        {
          JT_SYSON_Flag = 1;
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ON_OFF_STATE", 1);
        }  
        if (strcmp(inputCharArray, "JT_SYS_OFF") == 0) 
        {
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/PANIC", 0);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BUTTONS/PANIC_FROM_APP", 0);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/PANIC_FROM_SYS", 0);
          JT_SYSOFF_Flag = 1;
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/DOOR_OPEN", 0);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ON_OFF_STATE", 0);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/SIREN_ON", 0);

        }
        if (strcmp(inputCharArray, "JT_PANIC") == 0) 
        {
      

          JT_PANIC_Flag = 1;
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/PANIC", 1);
          send_hystory_flag = 1;


        }
         if (strncmp(inputCharArray, "JT_DOOR", 7) == 0) {
        // Extract the door number (assuming it's always in the form "XXX")
         doorNumber = atoi(inputCharArray + 8); // Extract digits starting from position 8

        // Check if the number is between 1 and 25
        if (doorNumber >= 1 && doorNumber <= 99) {
            
            JT_DOOR_OPEN_Flag = 1;
            send_hystory_flag = 2;
        }
       }
       if (strcmp(inputCharArray, "JT_STS") == 0) 
        {
          Serial.println("JT_ONLINE");
        }
       if (strcmp(inputCharArray, "JT_SIREN_OFF") == 0) 
        {
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/DOOR_OPEN", 0);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/SIREN_ON", 0);
        }
        if (strcmp(inputCharArray, "JT_PWRON") == 0) 
        {
           JT_PWRON_Flag = 1;
        }
        if (strcmp(inputCharArray, "JT_PWROFF") == 0) 
        {
           JT_PWROFF_Flag = 1;
        }
        if (strcmp(inputCharArray, "JT_OK") == 0) 
        {
           JT_OK_Flag = 1;
        }
        if (strcmp(inputCharArray, "JT_BATOK") == 0) 
        {
            JT_BATTERY_OK_Flag = 1;
        }
         if (strcmp(inputCharArray, "JT_LOWBAT") == 0) 
        {
            JT_BATTERY_LOW_Flag = 1;
        }
      }
    }
    else
    {
      if(Serial.available()) 
      {
        String s =  Serial.readStringUntil('\n');
        char inputCharArray[s.length() + 1];
        s.toCharArray(inputCharArray, s.length() + 1);
        if (strcmp(inputCharArray, "JT_STS") == 0) 
        {
          delay(3000);
          Serial.println("JT_OFFLINE");
        }
      }
    }
  }

  void readata()
  {
    if (WiFi.status() == WL_CONNECTED) 
      {
         if (buttonPressed) return;

        digitalWrite(wifiLed, LOW);
        if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/BUTTONS/ON_BUTTON")) 
        {
          sys_ON = fbdo.intData();
        }
          if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/BUTTONS/OFF_BUTTON")) 
        {
          sys_OFF = fbdo.intData();
        }
         if (buttonPressed) return;
         if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/BUTTONS/ADD_BUTTON")) 
        {
          ADD_BUTTON = fbdo.intData();
        }
         if (buttonPressed) return;
         if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/BUTTONS/DELETE_BUTTON")) 
        {
          DELETE_BUTTON = fbdo.intData();
        }
         if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/BUTTONS/CLEAR_BUTTON")) 
        {
          CLEAR_BUTTON = fbdo.intData();
        }
         if (buttonPressed) return;
        if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/BUTTONS/PANIC_FROM_APP"))
        {
          panic = fbdo.intData();
        } 
         if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/SYSTEM_ON_OFF_TIME/RESET"))
        {
          RESET = fbdo.intData();
        }
         if (buttonPressed) return;
        if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/UPDATE")) 
          {
            int value = fbdo.intData();
            EEPROM.put(EEPROM_UPDATE, value);
          //  Serial.print("update : " + value);

            if(value == 1)
            {
              String path = "/UPDATE_LINK";
             if (Firebase.getString(fbdo, path)) {
              if (fbdo.dataType() == "string") {
                String link = fbdo.stringData(); // Store the string from Firebase
                Serial.println("Link fetched successfully: " + link);
            
                // Convert the String to a char array
                char linkArray[link.length() + 1]; // Ensure enough space for null terminator
                link.toCharArray(linkArray, sizeof(linkArray));
            
                // Write the char array to EEPROM
                for (int i = 0; i < link.length() && i < (EEPROM_SIZE - UPDATE_LINK_ADDER); i++) {
                  EEPROM.write(UPDATE_LINK_ADDER + i, linkArray[i]);
                }
                EEPROM.write(UPDATE_LINK_ADDER + link.length(), '\0'); // Null-terminate the stored string
              } else {
                Serial.println("The data type is not a string!");
              }
            } else {
              Serial.println("Failed to fetch update link: " + fbdo.errorReason());
            }

              String update_time = (String(time_hours)+" : "+String(time_mint)+ " d : " );
              String update_date = (String(currentDay)+" / "+String(currentMonth)+" / "+String(currentYear));
              Firebase.setString(fbdo, "SYSTEM/" + String(sys_id) + "/LAST_UPDATE_TIME/",update_time + update_date);
              Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/UPDATE", 0);
              EEPROM.commit();
              delay(1000);
              ESP.restart();
            }
    
          }
         if (buttonPressed) return;
        if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/BIT_SET/BIT_SET_BUTTON"))
        {
          bit_set_flag = fbdo.intData();

          if(bit_set_flag == 1)
          {
            if(Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/BIT_SET/BIT_VALUE"))
            {
              bit_value = fbdo.intData();
              Serial.println("JT_BIT" + String(bit_value));
              Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/BIT_SET/BIT_SET_BUTTON", 0);
              delay(500);

            }
          } 
        } 
         if (buttonPressed) return;
        bool status = Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ONLINE_STATUS", 1);
        if (status == 0)
        {
          restart++;
        }
        else
        {
          restart = 0;
        }
        if(restart >= 3)
        {
          ESP.restart();
        }
        if(currentYear > 2000)
        {
          if(JT_SYSON_Flag == 1)
          {
            sendnotification("ON_NOTI");
            JT_SYSON_Flag = 0;
          }
          if(JT_SYSOFF_Flag == 1)
          {
            sendnotification("OFF_NOTI");
            JT_SYSOFF_Flag = 0;
          }
          if(JT_OK_Flag == 1)
          {
            sendnotification("JT_OK");
            JT_OK_Flag = 0;
          } 
          if(JT_PANIC_Flag == 1)
          {
            sendnotification("PANIC_NOTI");
            JT_PANIC_Flag = 0;
          }
          if(JT_BATTERY_LOW_Flag == 1)
          {
            sendnotification("BATTERY_LOW");
            JT_BATTERY_LOW_Flag = 0;
          }
          if(JT_BATTERY_OK_Flag == 1)
          {
            sendnotification("BATTERY_OK");
            JT_BATTERY_OK_Flag = 0;
          }
          if(JT_PWRON_Flag == 1)
          {
            sendnotification("POWER_ON");
            JT_PWRON_Flag = 0;
          }
          if(JT_PWROFF_Flag == 1)
          {
            sendnotification("POWER_OFF");
            JT_PWROFF_Flag = 0;
          }
          if(JT_DOOR_OPEN_Flag == 1)
          {
            sendnotification("DOOR_NOTI");
            JT_DOOR_OPEN_Flag = 0;
          }
          if(send_hystory_flag == 1)
          {
             sendhistory("PANIC_HISTORY");
          }
          if(send_hystory_flag == 2)
          {
             sendhistory("DOOR_HISTORY");
          }
            
          
        }
      }
      else
      {
          digitalWrite(wifiLed, HIGH);  

      }
  }
  String readUpdateLinkFromEEPROM() {
    char linkBuffer[EEPROM_SIZE - UPDATE_LINK_ADDER];
    int i = 0;
      for (; i < (EEPROM_SIZE - UPDATE_LINK_ADDER); i++) {
        linkBuffer[i] = EEPROM.read(UPDATE_LINK_ADDER + i);
        if (linkBuffer[i] == '\0') break;
      }
   
      linkBuffer[i] = '\0';
      return String(linkBuffer);
  }

 void setupConfigPortal() 
 {
  for(int i = 0;i<= 5;i++)
  {
    digitalWrite(wifiLed,HIGH);
    delay(500);
    digitalWrite(wifiLed,LOW);
    delay(500);
  }
  WiFiManagerParameter sys_id_param("sys_id", "System ID", sys_id, 32);
  wm.addParameter(&sys_id_param);
  wm.setConfigPortalTimeout(180);

  if (!wm.startConfigPortal("JT_WIFI_SETUP","Jyoti@2000") )
  {
   Serial.println("Failed to connect through Config Portal");
  }
  strcpy(sys_id, sys_id_param.getValue());
  for (int i = 0; i < sizeof(sys_id); i++) 
  {
    EEPROM.write(SYS_ID_ADDR + i, sys_id[i]);
  }
  EEPROM.commit();
  for(int i = 0;i<= 5;i++)
  {
    digitalWrite(wifiLed,HIGH);
    delay(500);
    digitalWrite(wifiLed,LOW);
    delay(500);
  }
  ESP.restart();
 }
  unsigned long startTime;
  const unsigned long timeout = 20000; // Increase timeout to 10 seconds
  void checkForUpdates(String GITHUB_BIN_URL) {
    Serial.println("Checking for updates...");
  
    client.setInsecure();
  
  
    // Start the update process and check for timeout
    startTime = millis();
    t_httpUpdate_return result = ESPhttpUpdate.update(client, GITHUB_BIN_URL);
  
  
    if (millis() - startTime >= timeout) {
      Serial.println("Update timeout occurred");
    }
  
    switch (result) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("Update failed. Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;
  
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("No updates available.");
        break;
  
      case HTTP_UPDATE_OK:
        Serial.println("Update successful. Rebooting...");
        break;
    
    }
  }
 void readSysIdFromEEPROM() 
  {
    for (int i = 0; i < sizeof(sys_id); i++) 
    {
      sys_id[i] = EEPROM.read(SYS_ID_ADDR + i);
    }          
    sys_id[sizeof(sys_id) - 1] = '\0'; 
  }
