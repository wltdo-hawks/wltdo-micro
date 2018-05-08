/**
 * @file mainApp.ino
 * @details The main application control file
 */

// -------------------------
// --- INCLUDES ---
// -------------------------
#include "stdio.h"
#include "time.h"
#include "Particle.h"
#include <math.h>
#include "SparkJson/firmware/SparkJson.h"
#include "locale.h"

void getOpenRequest(const char *topic, const char *data);
void getLockStatus(const char *topic, const char *data);

const char *CHECK_EVENT_NAME = "Users/eHbg7Z5H5lPt4z96qvSSul2CW4A2/Flags/DoorStatus";

const char *PUBLISH_EVENT_NAME = "Logs";

const char *PUBLISH_OPENREQUEST = "SetOpenRequest";

const char *PUBLISH_DOORSTATE = "SetDoorState";


const unsigned long CHECK_PERIOD_MS = 10000; //10 sec
const unsigned long FIRST_CHECK_MS = 5000;

unsigned long lastPublish1 = FIRST_CHECK_MS - CHECK_PERIOD_MS;
unsigned long lastPublish = FIRST_CHECK_MS - CHECK_PERIOD_MS;
//----------
int trigPinDW = D5;
int echoPinDW = D6;

int trigPinIO = A0;
int echoPinIO = A1;

long duration, cm, inches;

const int contactA = A3;
const int contactB = A2;

const int goUp = D0;
const int goDown = D2;
const int enableMotor = D3;

volatile bool topFlag = false;
volatile bool botFlag = false;
volatile bool openFlag = false;
volatile bool closeFlag = false;

volatile bool dogFlag = false;
volatile bool dogInDoorFlag = false;

volatile bool requestOpenFlag = false;
volatile bool isClosed = true;

const int bTPin = A4;
volatile bool bTFlag = true;
volatile bool bTChange = false;

volatile bool lockStatusFlag = false;

/**
 * @brief setup function for the application
 */
void setup() {
   Serial.begin(9600);

   pinMode(trigPinDW, OUTPUT);
   pinMode(echoPinDW, INPUT);
   pinMode(trigPinIO, OUTPUT);
   pinMode(echoPinIO, INPUT);
   //------
   pinMode(contactA, INPUT);
   pinMode(contactB, INPUT);

   attachInterrupt(contactA, atTop, FALLING);
   attachInterrupt(contactB, atBot, FALLING);
   //------
   pinMode(goUp, OUTPUT);
   pinMode(goDown, OUTPUT);
   pinMode(enableMotor, OUTPUT);
   digitalWrite(enableMotor, LOW);

   pinMode(bTPin, INPUT);
   attachInterrupt(bTPin, bTFlagOn, CHANGE);

   Particle.subscribe("hook-response/GetOpenRequest", getOpenRequest, MY_DEVICES);
   Particle.subscribe("hook-response/GetLockStatus", getLockStatus, MY_DEVICES);
}

void atTop() {
  digitalWrite(enableMotor, LOW);
  topFlag = true;
}
void atBot() {
  digitalWrite(enableMotor, LOW);
  botFlag = true;
}
void open() {
  openFlag = true;
  isClosed = false;
}
void close() {
  closeFlag = true;
}
void dog() {
  dogFlag = true;
}
void dogInDoor() {
  dogInDoorFlag = true;
}
void openRequest() {
  requestOpenFlag = true;
}
void itIsClosed() {
  isClosed = true;
}
void bTFlagOn() {
  bTChange = true;
}
void newOpenDoor() {
   Particle.publish("Door is opening", 12);
   digitalWrite(enableMotor, HIGH);
   digitalWrite(goUp, HIGH);
   digitalWrite(goDown, LOW);
 }
void newCloseDoor() {
    Particle.publish("Door is closing", 12);
    digitalWrite(enableMotor, HIGH);
    digitalWrite(goUp, LOW);
    digitalWrite(goDown, HIGH);
}
/**
 * @brief loop function for the application
 */
void loop() {
  long petInDoor = distanceDoorway();
  long petApproach = distanceInOut();

  if (millis() - lastPublish1 >= CHECK_PERIOD_MS){
  		lastPublish1 = millis();
      // Get some data
      String faker = String(10);
      // Trigger the integration
      Particle.publish("GetOpenRequest-1", faker, PRIVATE);
  }
  if (millis() - lastPublish >= CHECK_PERIOD_MS){
  		lastPublish = millis();
      // Get some data
      String faker1 = String(10);
      // Trigger the integration
      Particle.publish("GetLockStatus-1", faker1, PRIVATE);
  }
  if(requestOpenFlag && itIsClosed && !lockStatusFlag){
    dog();
    open();
    delay(1000);

    requestOpenFlag = false;
    endRequest(0);
  }
  if (bTChange){
    switch(bTPin){
      case 0:{
        bTFlag = true;
        break;
      }
      case 1:{
        bTFlag = false;
        break;
      }
      default:{
        break;
      }
    }
  }
  if ((petApproach <= 4.0) && (!dogFlag) && (bTFlag) && (!lockStatusFlag)){
    Particle.publish("Your pet is approaching the door.", 12);

    open();
    dog();
  }
  if (topFlag){
    Particle.publish("Contact A is pressed. The door has opened.", 12);

    digitalWrite(enableMotor, LOW);
    delay(3000);

    if (!dogInDoorFlag){
      close();
      topFlag = false;
    }
  }
  if (botFlag){
    Particle.publish("Contact B is pressed. The door has closed.", 12);

    updateLog(0);
    updateDoorState(0);
    itIsClosed();
    digitalWrite(enableMotor, LOW);

    openFlag = false;
    botFlag = false;
    dogFlag = false;
  }
  if (openFlag){
    updateLog(1);
    updateDoorState(1);
    newOpenDoor();
    openFlag = false;
  }
  if (closeFlag){
    newCloseDoor();
    closeFlag = false;
  }
  if ((petInDoor <= 6.0) && (!dogInDoorFlag) && (dogFlag)){
    Particle.publish("THERE IS A DOG IN THE DOORWAY!!!", 12);
    newOpenDoor();
    dogInDoor();
  }
  if ((petInDoor > 6.0) && (dogInDoorFlag)){
    dogInDoorFlag = false;
  }
}

void updateLog(int l) {
  const char dog[] = "Lucy";
  const char statusOpen[] = "Opened by ";
  const char statusClosed[] = "Closed by ";

  char buf[256];

   switch (l){
     case 0:{
      snprintf(buf, sizeof(buf), "{\"dog\":\"%s\",\"status\":\"%s\"}", dog, statusClosed);
      break;
    }
    case 1:{
      snprintf(buf, sizeof(buf), "{\"dog\":\"%s\",\"status\":\"%s\"}", dog, statusOpen);
      break;
    }
    default:{
      break;
    }
  }

	Particle.publish(PUBLISH_EVENT_NAME, buf, PRIVATE);
}

void updateDoorState(int l) {
  const char doorOpen[] = "Open";
  const char doorClose[] = "Close";

  char buf3[256];

   switch (l){
     case 0:{
      snprintf(buf3, sizeof(buf3), "{\"DoorState\":\"%s\"}", doorClose);
      break;
    }
    case 1:{
      snprintf(buf3, sizeof(buf3), "{\"DoorState\":\"%s\"}", doorOpen);
      break;
    }
    default:{
      break;
    }
  }
  Particle.publish(PUBLISH_DOORSTATE, buf3);
}

void endRequest(int l) {
  const char endRequest[] = "0";

  char buf2[256];

  switch (l){
    case 0:{
      snprintf(buf2, sizeof(buf2), "{\"OpenRequest\":\"%s\"}", endRequest);
      break;
    }
    default:{
      break;
    }
  }
  Particle.publish(PUBLISH_OPENREQUEST, buf2);
}

void getOpenRequest(const char *topic, const char *data) {
	// This isn't particularly efficient; there are too many copies of the data floating
	// around here, but the data is not very large and it's not kept around long so it
	// should be fine.
	StaticJsonBuffer<256> jsonBuffer;
	char *mutableCopy = strdup(data);
	JsonObject& root = jsonBuffer.parseObject(mutableCopy);
	free(mutableCopy);
  int oReq = atoi(root["OpenRequest"]);
	Serial.printlnf("data: %s", data);
  Serial.printlnf("OpenR: %d", oReq);

  if(oReq == 1){
     openRequest();
  }
}

void getLockStatus(const char *topic, const char *data) {
	// This isn't particularly efficient; there are too many copies of the data floating
	// around here, but the data is not very large and it's not kept around long so it
	// should be fine.
	StaticJsonBuffer<256> jsonBuffer;
	char *mutableCopy = strdup(data);
	JsonObject& root = jsonBuffer.parseObject(mutableCopy);
	free(mutableCopy);

  int lStat = atoi(root["LockStatus"]);
	Serial.printlnf("data: %s", data);
  Serial.printlnf("LockS: %d", lStat);

  if(lStat){

     lockStatusFlag = true;
  }
  else{
    lockStatusFlag = false;
  }
}

static double distanceDoorway() {
  int retVal1;
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPinDW, LOW);
  delayMicroseconds(10);
  digitalWrite(trigPinDW, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinDW, LOW);

  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPinDW, INPUT);
  duration = pulseIn(echoPinDW, HIGH);

  // convert the time into a distance
  cm = (duration/2) / 29.1;
  inches = (duration/2) / 74;

  return inches;
}

static double distanceInOut() {
  int retVal1;
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPinIO, LOW);
  delayMicroseconds(10);
  digitalWrite(trigPinIO, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinIO, LOW);

  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPinIO, INPUT);
  duration = pulseIn(echoPinIO, HIGH);

  // convert the time into a distance
  cm = (duration/2) / 29.1;
  inches = (duration/2) / 74;

  return inches;
}
