#include <TonyS_X1.h>
#include <TonyS_X1_ExternalModule.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
                                                                                
#define FIREBASE_HOST "Your FIREBASE HOST"                
#define FIREBASE_AUTH "Your FIREBASE AUTH"                

#define tempDir "temp"                 //<==========แก้ให้ไม่เหมือนกัน        
#define humiDir "humi"                 //<==========แก้ให้ไม่เหมือนกัน        
#define controlDir "control/"          //<==========แก้ให้ไม่เหมือนกัน        

const char* ssid = "Your SSID";               //<==========ชื่อ Wi-Fi
const char* password = "Your Password";       //<==========รหัสผ่าน Wi-Fi

TonySHT31 sht31;
TonyTFT tft = TonyTFT(SLOT1, TFT_240_320);

FirebaseData firebaseData;
FirebaseData stream;

unsigned long lastMeasure = 0;
unsigned long interval = 1000;
unsigned long now = 0;

lv_obj_t * label_Temp;
lv_obj_t * label_Humi;
lv_obj_t * lmeter_Temp;
lv_obj_t * lmeter_Humi;
lv_obj_t * chart;
lv_chart_series_t * ser_Temp;
lv_chart_series_t * ser_Humi;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  Tony.begin();
  sht31.begin(0x44);

  tft.init(240, 320);
  tft.setRotation(2);
  tft.setCursor(0, 0);
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  tft.setTextColor(GREEN);
  tft.print("Reconnecting to ");
  tft.println(ssid);

  Serial.print("Reconnecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(250);
    Serial.print(".");
    tft.print(".");
  }
  Serial.println("");
  Serial.println("Wi-Fi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  tft.println("");
  tft.println("Wi-Fi conected");

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  Firebase.setStreamCallback(stream, streamCallback, streamTimeoutCallback);
  if (!Firebase.beginStream(stream, controlDir)) {
      Serial.println("Error : " + firebaseData.errorReason());
  }

  tft.fillScreen(BLACK);

  lvglSetup();

  Tony.pinMode(Relay_1, OUTPUT);
  Tony.pinMode(Relay_2, OUTPUT);
}

void loop() {
  now = millis();

  if (now - lastMeasure > interval){
    lastMeasure = now;
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();

    String temp = String(t, 2);
    String humi = String(h, 2);

    if(!isnan(t) && !isnan(h)){
      Serial.print("Temperature = ");
      Serial.print(t);
      Serial.println(" *c");

      Serial.print("Humidity = ");
      Serial.print(h);
      Serial.println(" %");

      lv_label_set_text(label_Temp, temp.c_str());
      lv_lmeter_set_value(lmeter_Temp, t);
      lv_chart_set_next(chart, ser_Temp, t);

      lv_label_set_text(label_Humi, humi.c_str());
      lv_lmeter_set_value(lmeter_Humi, h);
      lv_chart_set_next(chart, ser_Humi, h);

      Firebase.setFloat(firebaseData, tempDir, t);
      Serial.print(tempDir);
      Serial.print(" = ");
      Serial.println(t);

      Firebase.setFloat(firebaseData, humiDir, h);
      Serial.print(humiDir);
      Serial.print(" = ");
      Serial.println(h);
    }

    else{
      Serial.println("Failed to read data from SHT31");
      return;
    }
  
    Serial.println("");
  }

  if(WiFi.status() != WL_CONNECTED){
    reconnectToWiFi();
    tft.fillScreen(BLACK);
    lvglSetup();
  }
  //delay(1000);
}

void lvglSetup(){
  lvglInit(&tft);
  startLvglHandle();

  lv_theme_t *themes = lv_theme_alien_init(20, NULL);
  lv_theme_set_current(themes);

  lv_obj_t * cont_Temp = lv_cont_create(lv_disp_get_scr_act(NULL), NULL);
  lv_obj_set_size(cont_Temp, 95, 110);
  lv_obj_align(cont_Temp, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 30);

  lv_obj_t * cont_Humi = lv_cont_create(lv_disp_get_scr_act(NULL), NULL);
  lv_obj_set_size(cont_Humi, 95, 110);
  lv_obj_align(cont_Humi, NULL, LV_ALIGN_IN_TOP_RIGHT, -20, 30);

  lmeter_Temp = lv_lmeter_create(cont_Temp, NULL);
  lv_lmeter_set_range(lmeter_Temp, 0, 100);                   /*Set the range*/
  lv_lmeter_set_value(lmeter_Temp, 80);                       /*Set the current value*/
  lv_lmeter_set_scale(lmeter_Temp, 240, 21);                  /*Set the angle and number of lines*/
  lv_obj_set_size(lmeter_Temp, 80, 80);
  lv_obj_align(lmeter_Temp, NULL, LV_ALIGN_CENTER, 0, 0);

  lmeter_Humi = lv_lmeter_create(cont_Humi, NULL);
  lv_lmeter_set_range(lmeter_Humi, 0, 100);                   /*Set the range*/
  lv_lmeter_set_value(lmeter_Humi, 20);                       /*Set the current value*/
  lv_lmeter_set_scale(lmeter_Humi, 240, 21);                  /*Set the angle and number of lines*/
  lv_obj_set_size(lmeter_Humi, 80, 80);
  lv_obj_align(lmeter_Humi, NULL, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t * symbol_Temp = lv_label_create(cont_Temp, NULL);
  lv_label_set_text(symbol_Temp, "T");
  lv_obj_align(symbol_Temp, NULL, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t * symbol_Humi = lv_label_create(cont_Humi, NULL);
  lv_label_set_text(symbol_Humi, "H");
  lv_obj_align(symbol_Humi, NULL, LV_ALIGN_CENTER, 0, 0);
    
  label_Temp = lv_label_create(cont_Temp, NULL);
  lv_label_set_text(label_Temp, "00.00");
  lv_obj_align(label_Temp, NULL, LV_ALIGN_CENTER, 0, 40);

  label_Humi = lv_label_create(cont_Humi, NULL);
  lv_label_set_text(label_Humi, "00.00");
  lv_obj_align(label_Humi, NULL, LV_ALIGN_CENTER, 0, 40);

  /*Create a chart*/
  chart = lv_chart_create(lv_scr_act(), NULL);
  lv_obj_set_size(chart, 200, 150);
  lv_obj_align(chart, NULL, LV_ALIGN_CENTER, 0, 60);
  lv_chart_set_type(chart, LV_CHART_TYPE_POINT | LV_CHART_TYPE_LINE);            /*Show lines and points too*/
  lv_chart_set_series_width(chart, 4);                                          /*Line width and point radious*/
  lv_chart_set_range(chart, 0, 100);
  
  /*Add two data series*/
  ser_Temp = lv_chart_add_series(chart, LV_COLOR_RED);
  ser_Humi = lv_chart_add_series(chart, LV_COLOR_CYAN);
}

void streamCallback(StreamData data) {
  Serial.printf("stream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
  if(data.dataPath() == "/relay1"){
     Tony.digitalWrite(Relay_1, data.boolData());
  }
  if(data.dataPath() == "/relay2"){
     Tony.digitalWrite(Relay_2, data.boolData());
  }
}

void streamTimeoutCallback(bool timeout) {
    if (timeout){
        Serial.println("Stream timed out, resume streaming...\n");
    }
}

void reconnectToWiFi(){
  tft.setCursor(0, 0);
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  tft.setTextColor(GREEN);
  tft.print("Reconnecting to ");
  tft.println(ssid);

  Serial.print("Reconnecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(250);
    Serial.print(".");
    tft.print(".");
  }
  Serial.println("");
  Serial.println("Wi-Fi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  tft.println("");
  tft.println("Wi-Fi conected");
}
