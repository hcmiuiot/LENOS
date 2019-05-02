#include "Configs.h"
#include <Adafruit_MLX90614.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//    #define DEBUG

#define PROBE A1

String string;

bool before = 0;
bool now = 0;

//################ OTHERS #################
//char* string2char(String command) {
//  return const_cast<char*>(command.c_str());
//}

int smsSentCount = 0;
long str2long(String str) {
  return atol(str.c_str());
}
void reinitial() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    EEPROMWritelong(i, -1);
  }
  EEPROMWritelong(EEP_POS_PSW, DEFAULT_PSW);
}

void statusBlink(uint64_t count, uint64_t onTime = 200, uint64_t offTime = 200) {
  for (int i = 0; i < count; i++) {
    digitalWrite(PIN_STATUS, HIGH);
    delay(onTime);
    digitalWrite(PIN_STATUS, LOW);
    delay(offTime);
  }
}
void statusInfBlink(uint64_t onTime = 200, uint64_t offTime = 200) {
  while (true)
    statusBlink(1, onTime, offTime);
}

bool loopCheck12V() {
  now = digitalRead(PROBE);
  if (now != before) {
      if (now == HIGH) {
        broadcastSms2Clients(NOTICE_LOST_ELECTRICITY);
      } else
        broadcastSms2Clients(NOTICE_RETURN_ELECTRICITY);     
  }
  before = now;
}
//################ OTHERS #################

//################ MLX90614 ###############
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
float tempThreshold;
float temparound;

void beginMLX() {
  mlx.begin();
  delay(1000);
#ifdef DEBUG
  Serial.println(F("MLX90614 BEGIN"));
#endif
  statusBlink(2, 400, 400);
}

bool checkSendBefore = false;
bool checkOverHeat = false;
void loopCheckTemp() {
  temparound = mlx.readAmbientTempC();
  //    String temparoundinString = String(temparound);
#ifdef DEBUG
  Serial.print(F("TEMPERATURE > "));
  Serial.print(temparound);
  Serial.println(F("*C"));
#endif
  if (temparound >= tempThreshold) {
    checkOverHeat = 1;
#ifdef DEBUG
    Serial.println(F("OVERHEAT"));
#endif
  } else {
    checkOverHeat = 0;
  }
  if ((checkOverHeat != checkSendBefore) && (checkOverHeat == 1)) {
      statusBlink(5, 150, 150);
      broadcastSms2Clients(NOTICE_OVER_HEAT);
  }
  checkSendBefore = checkOverHeat;
}
//################ MLX90614 ###############

//################ EEPROM ###############
long sysPsw; // System password
long clientPhoneNums[MAX_CLIENTS];

void EEPROMWritelong(int index, long value) {
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  index *= 4;
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(index, four);
  EEPROM.write(index + 1, three);
  EEPROM.write(index + 2, two);
  EEPROM.write(index + 3, one);
}
long EEPROMReadlong(long index) {
  index *= 4;
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(index);
  long three = EEPROM.read(index + 1);
  long two = EEPROM.read(index + 2);
  long one = EEPROM.read(index + 3);

  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
void savePsw2Eeprom(String psw) {
  EEPROMWritelong(EEP_POS_PSW, str2long(psw));
}
void loadEeprom() {
  // load clients phone numbers
  for (int i = 0; i < MAX_CLIENTS; i++) {
    clientPhoneNums[i] = EEPROMReadlong(i);
        #ifdef DEBUG
        Serial.print(F("Phone #"));
        Serial.print(i);
        Serial.print(F(": "));
        Serial.println(clientPhoneNums[i]);
        #endif
  }
  // load system password
  sysPsw = EEPROMReadlong(EEP_POS_PSW);
    #ifdef DEBUG
    Serial.print(F("System password: "));
    Serial.println(sysPsw);
    #endif
  // load temperature threshold
  tempThreshold = EEPROMReadlong(EEP_POS_TEMP_THRESHOLD);
    #ifdef DEBUG
    Serial.print(F("Temperature threshold: "));
    Serial.println(tempThreshold);
    #endif
}
int checkSaveAble(String phoneNum) {
#ifdef  DEBUG
  Serial.print(F("PhoneNum: "));
  Serial.print(phoneNum);
#endif
  long phoneInLong = str2long(phoneNum);
  int res = -1 ;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (phoneInLong == clientPhoneNums[i]) {
      return -2; //DUPLICATED
    }
    if (clientPhoneNums[i] == -1) {
      res = i; //UPDATED RES TO I
    }
  }
  EEPROMWritelong(res, phoneInLong);
  clientPhoneNums[res] = phoneInLong;
  return res;
}
//################ EEPROM ###############

//################ 800L #################
SoftwareSerial sim(PIN_SIM_TX, PIN_SIM_RX);
String balance;
void resetSIM() {
  pinMode(PIN_SIM_RESET, OUTPUT); // Set pin mode to OUTPUT
  digitalWrite(PIN_SIM_RESET, LOW);  // Pull-down
  delayMicroseconds(1000);
  digitalWrite(PIN_SIM_RESET, HIGH); // Pull-up
}
void beginSIM() {
  sim.begin(BAUD_SIM);
  sim.readString();
#ifdef DEBUG
  Serial.println(F("GSM BEGIN"));
#endif
  statusBlink(3, 400, 400);
  delay(1000);
}

uint64_t timeOld;
String waitResponse(uint64_t timeout = TIMEOUT_WAIT_SIM_RESPONSE) {
  timeOld = millis();
  while (!(millis() > timeOld + timeout)) {
    delay(13);
    if (sim.available()) {
      string = sim.readString();
#ifdef DEBUG
      Serial.print(F("[waitResponse-Return]: "));
      Serial.println(string);
#endif
      return string;
    }
  }
  return F("TIMEOUT");
}
String simPrintAndWaitYoo(char* msg, uint64_t timeout = TIMEOUT_WAIT_SIM_RESPONSE) {
#ifdef DEBUG
  Serial.print(F("[simPrintAndWait2-PRINT]: "));
  Serial.println(msg);
#endif
  sim.print(msg);
  sim.print(RETURN);
  delay(500);
  return waitResponse(20000);
}
String simPrintAndWaitOld(String msg, uint64_t timeout = TIMEOUT_WAIT_SIM_RESPONSE) {
  return simPrintAndWaitYoo(msg.c_str(), timeout);
}

String getBalance();
void sendSmsEx(char* phoneNum, char* content) {
  sim.print(F("AT+CMGS=\""));
  delay(100);
  sim.print(phoneNum);
  delay(100);
  if (simPrintAndWaitYoo("\"\r").indexOf(SMS_REQ) != -1) {
    delay(200);
    sim.print(content);
    #ifdef DEBUG 
    Serial.print(content);
    #endif
    delay(500);
    sim.print(char(26));
    delay(1500);
    if (waitResponse(20000).indexOf(OK) != -1) { // TRY CATCH ERROR
      statusBlink(3, 200, 200);
      smsSentCount++;
      if (smsSentCount > SMS_COUNT_CHECK_BALANCE) {
        smsSentCount = 0;
        getBalance();
      }
    } else {
      statusBlink(5, 500, 200);
    }
  }
  delay(1000);
}

void sendSmsExAdd0(char* phoneNum, char* content) {
  sendSmsEx(String("0"+String(phoneNum)).c_str(), content);
}

void broadcastSms2Clients(char* content) {
  for (int i = 0; i < MAX_CLIENTS; i++)
    if (clientPhoneNums[i] != -1) {
      sendSmsEx(String("0"+String(clientPhoneNums[i])).c_str(), content);
    }  
}

String getBalance() {
  if (simPrintAndWaitOld(F("AT+CUSD=1,\"*101#\",15")).indexOf(OK) != -1) { //ATD OK
    string = waitResponse(15000);
    if (string.indexOf(F("+CUSD:")) != -1) {
       #ifdef DEBUG
      Serial.println(F("Check balance OK"));
      #endif
      if (string.indexOf(F("47491466")) != -1) {            // *101#
        balance = string.substring(string.indexOf(F("c:")) + 2, string.indexOf(F("d")));
          //#ifdef DEBUG
//        Serial.println(F("BALANCE: "));
//        Serial.println(balance);
          //#endif
        int balanceInt = balance.toInt();
        bool broke = balanceInt <= THRESHOLD_BALANCE;
        if (broke) {
          broadcastSms2Clients(NOTICE_BROKE);
          #ifdef DEBUG
          Serial.println(F("Broke!!!!"));
          #endif
        }
        else {
#ifdef DEBUG
          Serial.println(F("Not Broke"));
#endif
        }
        return balance;
      }
      return F("CAN NOT FIND STB:");
    }
  }
  return F("FAIL TO REQUEST CUSD");
}
void initSim() {
  if (simPrintAndWaitOld(F("AT+CNMI=0,0,0,0,0")).indexOf(OK) == -1  || //DONT USE SMS INDICATOR
      simPrintAndWaitOld(F("AT+CMGD=1,4")).indexOf(OK) == -1)  // CLEAR OLD MSG
    statusInfBlink(2000, 1000); // **** HANG THE HARDWARE AND BLINK THE LED ****
  #ifdef DEBUG 
  Serial.print(F("TRY TO GET BALANCE: "));
  Serial.println(getBalance());
  #endif
  statusBlink(5, 400, 400);
  delay(1000);
}
void waitSimReady() {
  while (simPrintAndWaitOld(ATCMGF1).indexOf(OK) == -1) { // Set text mode for SMS incoming
    statusBlink(3);
    delay(TIMEOUT_WAIT_SIM);
  }
  delay(1000);
  statusBlink(4, 400, 400);
}

int count = 0;
int index1;
void processSms(String phoneNum, String smsContent) {
  statusBlink(3, 100, 100);
  String inputArr[3] = {"", "", ""};
  smsContent.trim();
  count = 0;
  for (int i = 0; i < smsContent.length(); i++) {
    if (smsContent.charAt(i) == ' ') {
      count++;
    }
    else {
      if (smsContent.charAt(i) != '\n') {
        inputArr[count] += smsContent.charAt(i);
      }
    }
  }
  #ifdef DEBUG 
  Serial.print(F("INPUT 0>")); Serial.println(inputArr[0]); Serial.println(F("<0"));
  Serial.print(F("INPUT 1>")); Serial.println(inputArr[1]); Serial.println(F("<1"));
  #endif
  if (inputArr[0].equalsIgnoreCase(F("DK"))) {                                       // Register new phoneNum
    //String psw = inputArr[1];
    #ifdef DEBUG 
    Serial.println(F("DANG KY NE"));
    #endif
    if (str2long(inputArr[1]) == sysPsw) {  // MATCH passwordd
      switch (checkSaveAble(phoneNum)) {
        case -1:
          #ifdef DEBUG 
          Serial.println(F("No more slot"));
          #endif
          sendSmsExAdd0(phoneNum.c_str(), DK_EMPTY_SLOT);
          break;
        case -2:
          #ifdef DEBUG 
          Serial.println(F("Duplicated"));
          #endif
          sendSmsExAdd0(phoneNum.c_str(), NOTICE_DUPLICATE_REGISTER);
          break;
        default:
            #ifdef DEBUG 
            Serial.println(F("DK OK"));
            #endif
            sendSmsExAdd0(phoneNum.c_str(), DK_OK);
            break;
      }
    }
    else { // Wrong password
      #ifdef DEBUG 
      Serial.println(F("WRONG PASSWORD"));
      #endif
      sendSmsExAdd0(phoneNum.c_str(), DK_WRONG_PSW);
    }
    return;
  }

  if (inputArr[0].equalsIgnoreCase(F("HDK"))) { // Cancel register
    #ifdef DEBUG
    Serial.println(F("HUY DANG KY"));
    Serial.println(phoneNum);
    Serial.println(smsContent);
    #endif
    index1 = getPhoneNumIndex(phoneNum);
    if (index1 != -1 ) {  // PhoneNum of clients is available in Eeprom
      clientPhoneNums[index1] = -1;
      EEPROMWritelong(index1, -1);
      sendSmsExAdd0(phoneNum.c_str(), NOTICE_CANCEL_REGISTER_SUCCESS);
    }
    else {                                                            // PhoneNum of clients is not available in Eeprom
      sendSmsExAdd0(phoneNum.c_str(), NOTICE_CANCEL_REGISTER_FAIL);
    }
    return;
  }

  if (inputArr[0].equalsIgnoreCase(F("NT"))) {         //NAP TIEN
    processNT(phoneNum.c_str(), inputArr[1].c_str());
  }

  //    if (processNT(ntCode)) {
  //      sendSmsEx(phoneNum, "OK. Balance: " + balance);
  //    }

  ///###### DEPRECATED ############################################################
     if (inputArr[0].equalsIgnoreCase(F("ST"))) {  //RESET TEMP THRESHOLD
      if (getPhoneNumIndex(phoneNum) != -1) {
        tempThreshold = max(inputArr[1].toFloat(), 35);
        EEPROMWritelong(12, tempThreshold );
        String tempThresholdStr = (String) tempThreshold;
        sendSmsExAdd0(phoneNum.c_str(), String("Set temperature threshold to: "+ tempThresholdStr).c_str());
      }
     }

    if (inputArr[0].equalsIgnoreCase(F("LTT"))) {    // Get full info
      if (getPhoneNumIndex(phoneNum) != -1) {
        String elecStat;
        balance = getBalance();
        if (digitalRead(PROBE) == LOW)
          elecStat = "POWER OK";
        else
          elecStat = "LOST POWER";
        sendSmsExAdd0(phoneNum.c_str(), String("Balance: " + balance + "." + " Temperature: " + String(mlx.readAmbientTempC()) + "*C. " + elecStat).c_str());
  
      } else {
        sendSmsExAdd0(phoneNum.c_str(), NOTICE_CANCEL_REGISTER_FAIL);
      }
      return;
    }

    if (inputArr[0].equalsIgnoreCase(F("DMK"))) {
      if (str2long(inputArr[1]) == sysPsw ) {
        sysPsw = str2long(inputArr[2]);
        savePsw2Eeprom(inputArr[2]);
        sendSmsExAdd0(phoneNum.c_str(), String("Your new password is successfully changed to: " + inputArr[2]).c_str() );
      }
      else {
        sendSmsExAdd0(phoneNum.c_str(), NOTICE_CHANGE_PASSWORD_FAIL);
      }
    }
  ///###### DEPRECATED ############################################################
}

bool processNT(char* phoneNum, char* ntCode) {
  sim.print(F("AT+CUSD=1"));
  sim.print(RETURN);
  delay(3000);
  sim.readString();
  char* msg = "ATD*100*910978264770674111#";
  sprintf(msg, "ATD*100*%s#", ntCode);
  #ifdef DEBUG 
  Serial.print(F("MA NT: "));
  #endif
  if (simPrintAndWaitYoo(msg, 20000).indexOf(OK) != -1) {
     String res = waitResponse(20000);
     #ifdef DEBUG 
     Serial.print(F("processNT RESPONSE:"));
     Serial.println(res);
     #endif
     if (res.indexOf("dong") != -1) {
      sendSmsExAdd0(phoneNum, "CHARGE THE BALANCE OK!");
     }
  }
}
int getPhoneNumIndex(String phoneNum) {
  for (int i = 0; i < MAX_CLIENTS; i++) 
    if (clientPhoneNums[i] == str2long(phoneNum))
      return i;
  return -1;
}

bool need2Process;
String subStr;
char c;
String phoneNum;
void loopCheckSMS() {
  sim.readString();
  string = simPrintAndWaitOld(F("AT+CMGL=\"REC UNREAD\""));
  if (string.indexOf(OK) != -1) { // OK GET UNREAD SMS
    subStr = "";
    need2Process = false;
    for (int i = 0; i < string.length(); i++) {
      if (string.charAt(i) != 10) {
        subStr += string.charAt(i);
      } else {
        if (need2Process) {
          #ifdef DEBUG
          Serial.print(F("("));
          Serial.print(phoneNum);
          Serial.print(F("):"));
          Serial.println(subStr);
          #endif
          processSms(phoneNum, subStr);
          need2Process = false;
        }
        if (subStr.indexOf(F("+CMGL:")) != -1) {
          need2Process = true;
          phoneNum = subStr.substring(subStr.indexOf(F(",\"+")) + 5, subStr.indexOf(F("\",\"\",\"")));
        }
        subStr = "";
      }
    }
  }
}
//################ 800L #################

void setup() {
  // RESET SIM
  resetSIM(); //-> Commented by Tuan Lee (BLAME HIM WHEN NEED)

  // SET PINS PROBE MODE
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);

  before = digitalRead(PROBE);
  now = digitalRead(PROBE);

  // SET LED PINS MODE
  pinMode(PIN_STATUS, OUTPUT);
  digitalWrite(PIN_STATUS, HIGH);

  // BEGIN SERIAL MODULE
  #ifdef DEBUG 
  Serial.begin(BAUD_SERIAL);
  Serial.println(VERSION);
  #endif

  // LOAD DATA FROM EEPROM
  loadEeprom();

  // BEGIN MLX90614 SENSOR
  beginMLX();

  // BEGIN SIM MODULE
  beginSIM();

  // CHECK SIM STATUS ~~~~~ IF ERROR ~~INFINITE~~ BLINK THE STATUS LED ~~~~~
  waitSimReady();

  // INIT SOME CONFIGS ~~~~~ IF ERROR ~~INFINITE~~ BLINK THE STATUS LED ~~~~~
  initSim();

  // SHOULD REACH HERE
  #ifdef DEBUG
  Serial.println(F("*** READY TO WORK ***"));
  #endif
  statusBlink(5, 500, 500);
}

void loop() {
  digitalWrite(PIN_STATUS, HIGH);
  sim.flush();
  loopCheckSMS(); // NEED TO CHECK PROCESSINGSTRING
  sim.flush();
  loopCheckTemp();
  loopCheck12V();
  digitalWrite(PIN_STATUS, LOW);
  delay(5000);
}
