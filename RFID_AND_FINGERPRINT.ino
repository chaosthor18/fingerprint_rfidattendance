#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#define SS_PIN D8  
#define RST_PIN D0 
WiFiClient wificlient;
#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);


const String ip="http://192.168.1.8";
const char *ssid = "MITSI-Admin"; //WIFI NAME OR HOTSPOT
const char *password = "M@ssiv3its_2017"; //WIFI PASSWORD POR MOBILE HOTSPOT PASSWORD
#define Finger_Rx 2 //D4 black wire 2
#define Finger_Tx 0 //D3 yellow wire 0
SoftwareSerial mySerial(Finger_Rx, Finger_Tx);

/////////////////RFID AND FINGERPRINT////////////////////////
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
MFRC522 mfrc522(SS_PIN, RST_PIN);

int level_fingerprintregister=0;// not registered/registered
//GET ID IN DATABASE;
int fingerp_id = 0;
//IF SUCCESS DELETE 1 IF NOT = 0
int delete_status=0;
//if finger detected and matched 1 else 0
int finger_detected=0;
//get fingerprint id related to the rfid
int finger_rfid = 0;

void setup() {
   delay(1000);
   Serial.begin(9600);
   finger.begin(57600);//start fingerprint
   WiFi.mode(WIFI_OFF);    
   delay(1000);
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);
   Serial.println("");
   Serial.print("Connecting");
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
   }
//   lcd.setCursor(0, 0);
//   lcd.print("WIFI CONNECTED");
//   delay(500);
//   lcd.clear();
   Serial.println("");
   Serial.print("Connected to ");
   Serial.println(ssid);
   Serial.print("IP address: ");
   Serial.println(WiFi.localIP()); 
   if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
   } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
    }
   lcd.begin(16, 2);
   lcd.init();
   lcd.backlight();
   SPI.begin();
   mfrc522.PCD_Init();
}

void getfingerId(String cardid) {
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Wifi Connected");
    HTTPClient http;
    String postData = "uid=" + String(cardid) + "&action=getfingerId";
    http.begin(wificlient,ip+"/BIOMETRICS/rfid-actions/process.php");              
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  
    int httpCode = http.POST(postData);
    String payload = http.getString();
    finger_rfid = payload.toInt();
    Serial.println(httpCode);
    Serial.println(payload);
    attendance_fingerprint(finger_rfid);
    http.end();
  }
}
void sendRFID(String cardid) {
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Wifi Connected");
    HTTPClient http;
    String postData = "uid=" + String(cardid) + "&action=getmitsi_Id";
    http.begin(wificlient,ip+"/BIOMETRICS/rfid-actions/process.php");              
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  
    int httpCode = http.POST(postData);
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
    if(payload=="Please contact the administrator to register RFID"){return;}
    getfingerId(payload);
    http.end();
  }
}

//////////////////REGISTER FINGERPRINT YES/NO///////////////////
int register_fingerprint() {//ready=1 standby=0
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Wifi Connected");
    HTTPClient http;
    String postData = "action=register_fingerprint";
    http.begin(wificlient,ip+"/BIOMETRICS/fingerprint-actions/read.php");              
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  
    int httpCode = http.POST(postData);
    String payload = http.getString();
    if(httpCode > 0){
      if(payload=="ready"){
        return 1;
      }
      return 0;
    }
    http.end();
  }
  return 0;
}
///////////know if successfully registered the finger////////////
void successfully_registeredf(){
  level_fingerprintregister++;
}
void reset_counterlevel(){
  level_fingerprintregister=level_fingerprintregister*0;
  delete_status=delete_status*0;
  finger_detected=finger_detected*0;
  finger_rfid=finger_rfid*0;
}
///////////////////////////////////////////
///////////////Insert FingerprintDB/////////////////
void insert_fingerprint() {
  if(WiFi.status() == WL_CONNECTED) {
    while(level_fingerprintregister==0){
      getFingerprintEnroll(fingerp_id);
    }
    Serial.println("Successfully enrolled the finger");
    reset_counterlevel();
  }
}
////////////////////////Identify FINGER ID////////////////////
void verify_fingerid(){
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Wifi Connected");
    HTTPClient http;
    String postData = "action=get_fingerprintid";
    http.begin(wificlient,ip+"/BIOMETRICS/fingerprint-actions/read.php");              
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  
    int httpCode = http.POST(postData);
    String payload = http.getString();
    if(httpCode > 0){
      Serial.println("Finger ID Selected: "+payload);
      fingerp_id = payload.toInt();
    }
    http.end();
  }
}
////////////////////delete fingerprint/////////////
uint8_t deleteFingerprint(int id) {
  uint8_t p = -1;
  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
    delete_status = delete_status+1;
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }   
}
////////////////signal off/////////////////////////
void finger_registeroff(){
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Wifi Connected");
    HTTPClient http;
    String postData = "action=register_off";
    http.begin(wificlient,ip+"/BIOMETRICS/fingerprint-actions/read.php");              
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  
    int httpCode = http.POST(postData);
    String payload = http.getString();
    if(httpCode <= 0){
      Serial.println("ERROR");
    }
    http.end();
  }
}
//////////////////////////////////////////


////////////GET FINGERPRINT ENROLL////////////////////
uint8_t getFingerprintEnroll(int id) {
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    successfully_registeredf();
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
}

///////////////////////////////////////////////
//////////////Delete Fingerprint Y/N///////////
int delete_fingerprint(){
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Wifi Connected");
    HTTPClient http;
    String postData = "action=delete_fingerprint";
    http.begin(wificlient,ip+"/BIOMETRICS/fingerprint-actions/reset.php");              
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  
    int httpCode = http.POST(postData);
    String payload = http.getString();
    if(httpCode > 0){
      if(payload=="yes"){
        return 1;
      }
      return 0;
    }
    http.end();
  }
  return 0;
}

///////////////////////////////////////////////
////////////DELETE FINGER ID SELECTED//////////
int delete_fingerid(){
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Wifi Connected");
    HTTPClient http;
    String postData = "action=get_fingerprintid";
    http.begin(wificlient,ip+"/BIOMETRICS/fingerprint-actions/reset.php");              
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  
    int httpCode = http.POST(postData);
    String payload = http.getString();
    if(httpCode > 0){
      Serial.println("Finger ID to be deleted: "+payload);
      int fingerdelete_id = payload.toInt();
      return fingerdelete_id;
    }
    http.end();
    return 0;
  }
  return 0;
}
//////////////////////////////////////////////
/////////////Delete Fingerprint//////////////
void delete_fingerprint(int id){
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Wifi Connected");
    HTTPClient http;
    while(delete_status==0){ //1 is success
      deleteFingerprint(id);
    }
    Serial.println("Successfully Deleted");
    reset_counterlevel();
    http.end();
  }
}
////////////////////////////////////////////
/////////////Finger Search/////////////////
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  finger_detected=finger.fingerID;//MATCH AND FOUND
  return finger.fingerID;
}
////////////////////////////////////////////////
///////////////ATTENDANCE FINGERPRINT///////////
void attendance_fingerprint(int fingerid_matchrfid){
  int s=1; //seconds
  while(s<=10){
    Serial.println("Place your finger");
    getFingerprintID();
    if(finger_detected!=0 && fingerid_matchrfid==finger_detected && fingerid_matchrfid!=0){
        Serial.print("Fingerprint matchrfid: ");
        Serial.println(fingerid_matchrfid);
        fingerprint_timein(finger_detected);
        reset_counterlevel();
        break;
    }
    s++;
    delay(1000); //1sec
  }
  reset_counterlevel();
}
void fingerprint_timein(int id){
   if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Wifi Connected");
    Serial.println(id);
    HTTPClient http;
    String postData = "fingerid="+String(id)+"&action=timeintimeout_insert";
    http.begin(wificlient,ip+"/BIOMETRICS/fingerprint-actions/process.php");              
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  
    int httpCode = http.POST(postData);
    String payload = http.getString();
    if(httpCode > 0){
      Serial.println(payload);
    }
    http.end();
  }
}
////////////////////////////////////////////////////////////////////


/////////////////////MAIN LOOP PROGRAM///////////////////////////////
void loop() {
  //ATTENDANCE(RFID AND FINGERPRINT) FINGERPRINT
  lcd.setCursor(0, 0);
  lcd.print("Place your RFID:");
  if ( mfrc522.PICC_IsNewCardPresent()){
    if ( mfrc522.PICC_ReadCardSerial()){
      String uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++){
        uid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : ""));
        uid.concat(String(mfrc522.uid.uidByte[i], HEX)); 
      }
      Serial.println("UID:"+uid);
        sendRFID(uid);
      }                        
     }
  //DELETE FINGERPRINT
  if(delete_fingerprint()==1){
  int delete_id = delete_fingerid();
  finger_registeroff();
  if(delete_id!=0){
    delete_fingerprint(delete_id);
  }
 }
 //INSERT FINGERPRINT
 if(register_fingerprint()==1){//register to slot of fingerprint flash mem
  verify_fingerid();
  finger_registeroff();
  insert_fingerprint();
 }
  delay (2000);
 lcd.clear();
}
