#include <LiquidCrystal.h>

const char string_0[] PROGMEM = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: xxx\r\n\r\n<!DOCTYPE html><html><head><title>Will's PowerWall</title></head>";
const char string_1[] PROGMEM = "<body background = \"https://cdn.wallpapersafari.com/18/9/3sTDBS.jpg\">";
const char string_2[] PROGMEM = "<h1><center><b>Will's PowerWall Status</b></center></h1><table border = \"0\" width = 100%>";
const char string_3[] PROGMEM = "<tr><td width = 33%><h2><center>Commanded </center></h2></td><td width = 34%><h2><center>Inverter Status</center></h2></td><td width = 33%><h2><center>Pack Voltage</center></h2></td></tr><tr><td><h3><center>";
const char string_4[] PROGMEM = "</center></h3></td><td><h3><center>";
const char string_5[] PROGMEM = " V</center></h3></td></tr><tr><td><h3></br></h3></td></tr><tr><td><h2><center>Lowest Cell</center></h2></td><td><h2><center>Why Off?</center></h2></td><td><h2><center>Charge Time</center></h2></td></tr><tr><td><h3><center>";
const char string_6[] PROGMEM = "v </center></h3></td><td><h3><center> ";
const char string_7[] PROGMEM = " </center></h3></td><td><h3><center> ";
const char string_8[] PROGMEM = " Mins</center></h3></td></tr></table></body></html>\r\n";

const char* const string_table[] PROGMEM = 
{
      string_0,
      string_1,
      string_2,
      string_3,
      string_4,
      string_5,
      string_6,
      string_7,
      string_8,
};

char longstring[1000];char str1[11];
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int lowloaddisc = 1; //0 means disabled, 1 means enabled, 2, while running, means currently low-load powered-down
int RemoteEnable = 0;
int RemoteInputPin = 9;
int InverterControlPin = 6;
int InverterStatusPin = 8;
int ledpin = 13;
float TurnOnVoltage = 15.3; 
float ShutdownVoltage = 11.80;
float ShutdownCellVoltage = 2.70;
unsigned long PowerDownMillis = 0;
unsigned long PowerUpMillis = 0;
unsigned long OnTime = 0;
unsigned long PUDelay = 300000; //300000 for 5 minutes-ish
int i;float myArray[5];
int CurrentInverterStatus = 2;
int UserDesiredInverterStatus;
float PackVoltage; float CellVoltage0; float CellVoltage1; float CellVoltage2; float CellVoltage3;
float LowestCellVoltage; float HighestCellVoltage; float cellArray[4];
int connectionId;
String cipSend;
void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  pinMode(InverterControlPin, OUTPUT);
  pinMode(ledpin, OUTPUT);
  pinMode(InverterStatusPin, INPUT);
  pinMode(RemoteInputPin, INPUT);
  delay(2000);
  sendData("AT+RST\r\n",6000); // reset module
  sendData("AT+CIPMUX=1\r\n",200); // configure for multiple connections
  sendData("AT+CIPSERVER=1,80\r\n",500); // turn on server on port 80
  digitalWrite(ledpin,HIGH);
}

void loop() { 
  OnTime = millis() / 1000; // In Seconds
  if(CurrentInverterStatus == 1) PowerDownMillis = millis();
  getInvStatus();
  int AnalogInput0 = 0;
  for (i=0;i<10;i=i+1) {
    AnalogInput0 = AnalogInput0 + analogRead(A0);
    delay(25);  
  }
  AnalogInput0 = AnalogInput0 / 10;
  int AnalogInput1 = analogRead(A1);
  int AnalogInput2 = analogRead(A2);
  int AnalogInput3 = analogRead(A3);
  delay(1000);
  PackVoltage = AnalogInput0 * .02087;
  CellVoltage0 = PackVoltage - (AnalogInput1 * .0209);
  CellVoltage1 = PackVoltage - CellVoltage0 - (AnalogInput2 * .0210);
  CellVoltage3 = AnalogInput3 * .0210;
  CellVoltage2 = (AnalogInput2 * .02085) - CellVoltage3;
  MinMax();
  
  if(bitRead(OnTime,2) == 0)
    DisplayStatus();
  if(bitRead(OnTime,2) == 1)
    DisplayCells();
 
  lcd.setCursor(0,1);  lcd.print("Total "); lcd.print(PackVoltage);
  lcd.setCursor(15,1);
  if(UserDesiredInverterStatus == 1)lcd.print(".");
  if(UserDesiredInverterStatus == 0)lcd.print(" ");
  for (i=0;i<4;i=i+1) {myArray[i] = myArray[i+1];}
  myArray[4] = PackVoltage;
  if(PackVoltage > TurnOnVoltage && UserDesiredInverterStatus == 1  && lowloaddisc != 2 && CurrentInverterStatus == 0 && (PowerDownMillis + PUDelay) < millis()) PowerUp();
  if((PackVoltage < ShutdownVoltage || LowestCellVoltage < ShutdownCellVoltage) && CurrentInverterStatus == 1) PowerDown();
  if(lowloaddisc == 1 && myArray[0] != 0 && myArray[4] - 0.70 > myArray[0]) {lowloaddisc = 2; PowerDown();}
  if (digitalRead(RemoteInputPin) == 1 && CurrentInverterStatus == 0 && RemoteEnable == 1) PowerUp();
  if (digitalRead(RemoteInputPin) == 0 && CurrentInverterStatus == 1 && RemoteEnable == 1) PowerDown();
//  SerStatus();
  if(Serial.available())
    {
    if(Serial.find("+IPD,"))
      {
      delay(300);
      connectionId = Serial.read()-48;

      sendWebpage();

      cipSend = "AT+CIPCLOSE=";
      cipSend+=connectionId;
      cipSend+="\r\n";
      sendData(cipSend,300);
      }
    }
  
}


int DisplayStatus() {
  lcd.setCursor(0,0); lcd.print("                ");
  lcd.setCursor(0,0); lcd.print("LCM: ");lcd.print((PowerDownMillis - PowerUpMillis)/60000);
  lcd.print("   ");
  if(lowloaddisc == 2)lcd.print("LLD");
  if(lowloaddisc != 2 && CurrentInverterStatus == 0 && UserDesiredInverterStatus == 1) lcd.print("LVD");
}


int DisplayCells() {
  lcd.setCursor(0,0); lcd.print("                ");
  lcd.setCursor(0,0); lcd.print(CellVoltage3,1);
  lcd.print(" ");     lcd.print(CellVoltage2,1);
  lcd.print(" ");     lcd.print(CellVoltage1,1);
  lcd.print(" ");     lcd.print(CellVoltage0,1);
}

int PowerDown() {
  while(digitalRead(InverterStatusPin) == 1) {
//    digitalWrite(ledpin,HIGH);
    digitalWrite(InverterControlPin,HIGH);
    delay(1000);
//    digitalWrite(ledpin,LOW);
    digitalWrite(InverterControlPin,LOW);
    delay(3000);
  }
  for (i=0;i<5;i=i+1) {myArray[i] = 0;}
  lcd.setCursor(12,1); lcd.print("OFF"); CurrentInverterStatus = 0;
  PowerDownMillis = millis();
}

int PowerUp() {
  while(digitalRead(InverterStatusPin) == 0) {
//    digitalWrite(ledpin,HIGH);
    digitalWrite(InverterControlPin,HIGH);
    delay(1000);
//    digitalWrite(ledpin,LOW);
    digitalWrite(InverterControlPin,LOW);
    delay(3000);
  }
  for (i=0;i<5;i=i+1) {myArray[i] = 0;}
  lcd.setCursor(12,1); lcd.print(" ON"); CurrentInverterStatus = 1;
  PowerUpMillis = millis();
}

int getInvStatus() {
  if (digitalRead(InverterStatusPin) == 1) {
    lcd.setCursor(12,1); lcd.print(" ON"); 
    if (CurrentInverterStatus != 1) {
      UserDesiredInverterStatus = 1; 
      if (lowloaddisc == 2) lowloaddisc = 1;
      for (i=0;i<5;i=i+1) {myArray[i] = 0;}
      PowerUpMillis = millis();
      CurrentInverterStatus = 1;
    }
  }
  else {
    lcd.setCursor(12,1); lcd.print("OFF"); 
    if (CurrentInverterStatus != 0) {
      UserDesiredInverterStatus = 0;
      for (i=0;i<5;i=i+1) {myArray[i] = 0;}
      CurrentInverterStatus = 0;
    }

  }
}

int MinMax() {
  float cellArray[4] = {CellVoltage3, CellVoltage2, CellVoltage1, CellVoltage0};
  LowestCellVoltage = 10; //set to unreasonable values
  HighestCellVoltage = 0; //ditto
  for(i=0;i<4;i=i+1) {
    LowestCellVoltage = min(LowestCellVoltage,cellArray[i]);
    HighestCellVoltage = max(HighestCellVoltage,cellArray[i]);
  }
}

void sendData(String command, const int timeout)
            {
//                String response = "";
                Serial.print(command);
                long int time = millis();
                while( (time+timeout) > millis())
                  {
                    while(Serial.available())
                      {
                         char c = Serial.read(); // read the next character.
//                         response+=c;
                      }  
                  }
            }


void sendWebpage()
          {

            longstring[0] = '\0';

            strcat_P(longstring, (char*)pgm_read_word(&(string_table[0])));
            strcat_P(longstring, (char*)pgm_read_word(&(string_table[1])));
            strcat_P(longstring, (char*)pgm_read_word(&(string_table[2])));
            strcat_P(longstring, (char*)pgm_read_word(&(string_table[3])));
              if(UserDesiredInverterStatus == 1){strcpy(str1,"On");}else{strcpy(str1,"Off");}strcat(longstring, str1);
            strcat_P(longstring, (char*)pgm_read_word(&(string_table[4])));
              if(CurrentInverterStatus == 1){strcpy(str1,"On");}else{strcpy(str1,"Off");}strcat(longstring,str1);
            strcat_P(longstring, (char*)pgm_read_word(&(string_table[4])));
              dtostrf(PackVoltage,5,2,str1);strcat(longstring,str1);
            strcat_P(longstring, (char*)pgm_read_word(&(string_table[5])));
              
              dtostrf(LowestCellVoltage,3,1,str1);strcat(longstring,str1);
            strcat_P(longstring, (char*)pgm_read_word(&(string_table[6])));
              if(CurrentInverterStatus == 0 && UserDesiredInverterStatus == 1 && lowloaddisc != 2){strcpy(str1,"Batt Dead ");}else{strcpy(str1,"Too Soon ");}
              if(lowloaddisc == 2){strcpy(str1,"Bike Full ");}strcat(longstring,str1);
            strcat_P(longstring, (char*)pgm_read_word(&(string_table[7])));
              dtostrf((PowerDownMillis - PowerUpMillis)/60000,3,0,str1);strcat(longstring,str1);
            strcat_P(longstring, (char*)pgm_read_word(&(string_table[8])));
           
            int len = strlen(longstring);

             String cipSend = "AT+CIPSEND=";
             cipSend += connectionId; 
             cipSend += ",";
             cipSend +=len;
             cipSend +="\r\n";
             sendData(cipSend,200); // was 100
            
            dtostrf(len - 67,3,0,str1);

            for(int i = 58;i<61;i++){longstring[i] = str1[i-58];}

             for(int i = 0;i<len;i++){
               Serial.print(longstring[i]);
             }
          }
