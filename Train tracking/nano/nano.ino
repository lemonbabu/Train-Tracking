#include<SoftwareSerial.h>
#include<string.h>
SoftwareSerial gsm(10, 11);

String phnNum = "";
String sendedMsg = "";
String buffer = "";
unsigned long int currentTime, container;
int locationError = 0, serverError = 0;
void setup() {
  gsm.begin(9600);
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  device_on();
  sendATCommand("AT+CMGF=1", "OK", 1110);
  delay(500);
 // sendATCommand("AT+CMGDA=\"DEL ALL\"", "OK", 25000);
  //delay(500);
}

void loop() {
  int msgTigger = 0;
  char mystr[19];
  String location = "", timeStamp = "", lat = "", lng = "";
  gsm.println("AT+CGNSINF");
  bufferReadStr(5000, "+CGNSINF:");
  buffer = spiltString(buffer, "GNSINF: ", 1);
  if (buffer.indexOf("1,1,20") != -1) {
    locationError = 0;
    location = spiltString(buffer, ",", 3) + "," + spiltString(buffer, ",", 4);
    location.toCharArray(mystr,19);
    Serial.write(mystr,19);
    delay(1000);
    
    timeStamp = spiltString(buffer, ",", 2) + "," + spiltString(buffer, ",", 6) + "," + spiltString(buffer, ",", 7);
  }
  else {
    locationError++;
    gsm.println("AT+CIPGSMLOC=1,1");
    bufferReadStr(10000, "+CIPGSMLOC:");
   // Serial.println("GSM LOC : " + buffer);
    if (buffer.indexOf("+CIPGSMLOC: 0") != -1) {
      locationError = 1;
      buffer = spiltString(buffer, "+CIPGSMLOC: ", 1);
      location = spiltString(buffer, ",", 2) + "," + spiltString(buffer, ",", 1);
      buffer = spiltString(buffer, ",", 3) + spiltString(buffer, ",", 4);
      for (int i = 0; i < buffer.length(); i++) {
        if (buffer[i] <= 57 && buffer[i] >= 48)
          timeStamp += buffer[i];
      }
    }
    else {
      Serial.println("Location Error");
      led(1, 0, 0);
      if (locationError == 2 || locationError == 6) {
        locationError++;
        delay(2000);
        sendATCommand("AT+CGNSPWR=1", "OK", 1000);
        sendATCommand("AT+CGNSSEQ=\"RMC\"", "OK", 1000);
        httpSetup();
      }
      else if (locationError == 10) {
        locationError = 0;
        sendATCommand("AT+HTTPTERM", "OK", 2000);
        delay(500);
        sendATCommand("AT+SAPBR=0,1", "OK", 2000);
        delay(500);
        device_on();
      }
      return;
    }
  }
  //readMassage();
  gsm.print("AT+HTTPPARA=\"URL\",\"");
  gsm.print("http://tracker.duetbd.org/device.php?data=");
  gsm.print("D_24,");//Device ID : D_(Identity)

  if (phnNum == "") {
    if (sendedMsg == "") {
      msgTigger = 0;
      gsm.print("0,");
    }
    else {
      gsm.print(sendedMsg.substring(0, 11) + ",");
      msgTigger = 0;
    }
  }
  else {
    msgTigger = 1;
    gsm.print("1,");
  }


  gsm.print(location + ",");
  gsm.println(timeStamp + "\"");
  timeStamp = "";
  bufferReadStr(2000, "OK");
  //Serial.println("URL : " + buffer);
  gsm.println("AT+HTTPACTION=0");
  bufferReadStr(10000, "+HTTPACTION:");
  //Serial.println("URL ACTION : " + buffer);
  if (buffer.indexOf("200") != -1) {
    led(0, 0, 1);
    serverError = 0;
    gsm.println("AT+HTTPREAD=1,1000");
    bufferReadStr(10000, "+HTTPREAD:");
    //Serial.println("HTTP Data READ : " + buffer);
    buffer = spiltString(buffer, "@", 1);
    if (msgTigger == 1) {
      led(0, 1, 1);
      while (phnNum != "") {
        if (sendATCommand("AT+CMGS=" + phnNum.substring(0, 16), ">", 10000)) {
          gsm.print(buffer);
          gsm.write(0x1A);
          timeStamp = "";
          currentTime = millis();
          container = currentTime;
          while ((millis() - container) < 5000) {
            if (gsm.available()) {
              while (gsm.available()) {
                char c = gsm.read();
                timeStamp += c;
                delay(1);
              }
              if (timeStamp.indexOf("OK") == -1) {
                currentTime = millis();
                container = currentTime;
              }
              else break;
            }
          }
          if (timeStamp.indexOf("OK") != -1) {
            sendedMsg += phnNum.substring(4, 15);
          }
          delay(500);
        }
        phnNum = phnNum.substring(16);
        if (phnNum != "")
          delay(1000);
      }
      msgTigger = 0;
    }
    else {
      if (buffer == "sp")
        sendedMsg = sendedMsg.substring(11);
    }
  }

  else {
    serverError++;
    led(0, 1, 0);
    delay(2000);
    if (buffer.indexOf("604") != -1) {
      httpSetup();
    }
    else if (serverError == 25) {
      serverError = 0;
      device_on();
    }
  }

}



int device_on() {
  led(1, 1, 1);
  while (!sendATCommand("AT", "OK", 1000))
    delay(500);
  delay(500);
  led(1, 1, 0);

  while (!sendATCommand("AT+CGNSPWR=1", "OK", 1000))
    delay(500);
  delay(500);

  sendATCommand("AT+CGNSSEQ=\"RMC\"", "OK", 1000);
  delay(500);

  currentTime = millis();
  container = currentTime;
  while ((millis() - container) < 60000) {
    if (sendATCommand("AT+CREG?", "+CREG: 0,1", 1000)) {
      break;
    }
    delay(1000);
  }
  delay(500);

  currentTime = millis();
  container = currentTime;
  while ((millis() - container) < 60000) {
    if (sendATCommand("AT+CGATT=1", "OK", 2000)) {
      break;
    }
    delay(1000);
  }
  delay(500);

  sendATCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", 2000);
  delay(500);

  sendATCommand("AT+SAPBR=3,1,\"APN\",\"gpinternet\"", "OK", 2000);
  delay(500);

  if (!sendATCommand("AT+SAPBR=1,1", "OK", 20000))
    if (!sendATCommand("AT+SAPBR=1,1", "OK", 20000))
      if (!sendATCommand("AT+SAPBR=1,1", "OK", 20000))
        if (!sendATCommand("AT+SAPBR=1,1", "OK", 20000))
          if (!sendATCommand("AT+SAPBR=1,1", "OK", 20000));
  delay(2000);

  sendATCommand("AT+HTTPINIT", "OK", 10000);
  delay(500);

  return true;
}

String spiltString(String str, String spliter, int x) {
  unsigned int i = 0, ptr = 0, counter = 0;
  unsigned int strLen = str.length();
  unsigned int splLen = spliter.length();
  if (splLen >= strLen)
    return "";
  if (str.indexOf(spliter) == -1)
    return "";

  while (i + splLen <= strLen) {
    if (str.substring(i, i + splLen).equals(spliter)) {
      if (i - ptr > 0) {
        if (x == counter) {
          return str.substring(ptr, i);
        }
        else {
          counter++;
          i += splLen;
          ptr = i;
        }
      }
      else {
        i += splLen;
        ptr = i;
      }
    }

    else if ((i + splLen) == strLen) {
      return str.substring(ptr, strLen);
      i++;
    }
    else i++;
  }
}



int sendATCommand(String ATCmd, String expectedResult, int timeOut) {
  String funBuffer = "";
  if (ATCmd != "") {
    gsm.println(ATCmd);
  }
  currentTime = millis();
  container = currentTime;
  while ((millis() - container) < timeOut) {
    if (gsm.available()) {
      while (gsm.available()) {
        char c = gsm.read();
        if (c != 10)
          funBuffer += c;
        delay(1);
      }
      break;
    }
  }
  //Serial.print("AT function : ");
  //Serial.println(funBuffer);
  if (funBuffer.indexOf(expectedResult) != -1)
    return true;
  else
    return false;
}


void bufferReadStr(long int x, String str) {
  buffer = "";
  currentTime = millis();
  container = currentTime;
  while ((millis() - container) < x) {
    if (gsm.available()) {
      while (gsm.available()) {
        char c = gsm.read();
        //if (c != 10)
        buffer += c;
        delay(1);
      }
      if (buffer.indexOf(str) == -1) {
        currentTime = millis();
        container = currentTime;
      }
      else break;
    }
  }
  
  if (buffer.indexOf(">") != -1)
    gsm.write(0x1A);
}
void led(int r, int y, int g) {
  digitalWrite(3, r);
  digitalWrite(2, g);
}
void httpSetup() {
  sendATCommand("AT+HTTPTERM", "OK", 2000);
  delay(100);
  sendATCommand("AT+SAPBR=0,1", "OK", 2000);
  delay(100);
  sendATCommand("AT+CGATT=0", "OK", 2000);
  delay(100);
  if (!sendATCommand("AT+CGATT=1", "OK", 20000))
    if (!sendATCommand("AT+CGATT=1", "OK", 20000))
      if (!sendATCommand("AT+CGATT=1", "OK", 20000));
  delay(100);
  sendATCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", 2000);
  delay(100);
  sendATCommand("AT+SAPBR=3,1,\"APN\",\"gpinternet\"", "OK", 2000);
  delay(100);
  if (!sendATCommand("AT+SAPBR=1,1", "OK", 20000))
    if (!sendATCommand("AT+SAPBR=1,1", "OK", 20000))
      if (!sendATCommand("AT+SAPBR=1,1", "OK", 20000));
  delay(100);
  sendATCommand("AT+HTTPINIT", "OK", 20000);
}
void readMassage() {
  sendATCommand("AT+CMGF=1", "OK", 1110);
  gsm.println("AT+CMGL=\"REC UNREAD\"");
  currentTime = millis();
  container = currentTime;
  buffer = "";
  while ((millis() - container) < 5000) {
    if (gsm.available()) {
      while (gsm.available()) {
        char c = gsm.read();
        if (c == 10) {
          Serial.println("Read Msg : " + buffer);
          int y = buffer.indexOf("+880") - 1;
          if (y != -2) {
            buffer = buffer.substring(y, (y + 16));
            if (buffer[0] == '\"' && buffer[15] == '\"') {
              Serial.println("New Num : " + buffer);
              phnNum += buffer;
            }
            buffer = "";
          }
        }
        else {
          buffer += c;
        }
        delay(1);
      }
      if (buffer.indexOf("OK") == -1) {
        currentTime = millis();
        container = currentTime;
      }
      else {
        break;
      }
    }
  }


  gsm.println("AT+CMGDA=\"DEL READ\"");
  currentTime = millis();
  container = currentTime;
  buffer = "";
  while ((millis() - container) < 5000) {
    if (gsm.available()) {
      while (gsm.available()) {
        char c = gsm.read();
        buffer += c;
        delay(1);
      }
      if (buffer.indexOf("OK") == -1) {
        currentTime = millis();
        container = currentTime;
      }
      else {
        break;
      }
    }
  }

  Serial.println("Deleted Msg : " + buffer);
}
