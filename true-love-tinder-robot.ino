/* This is the code for V1 of the True Love Tinder Robot.
 * It's built with an Arduino with the Emic 2 Text-to-Speech Module, some LEDs, a simple GSR sensor, and a servo.
 * I use the Emic 2 Arduino library: https://github.com/pAIgn10/EMIC2
 * This finite state machine example was helpful for me buiding my own: http://hacking.majenko.co.uk/finite-state-machine
 * This is the first prototype of this project.
 */

#include <SPI.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <EMIC2.h>
#include <Servo.h>

Servo leftRightServo;
Servo forwardBackServo;

//keep track of which phrases are said
byte iBegin = 0;
byte iLeft = 0;
byte iRight = 0;

//keep track of timers
int timePassed;
int totalTimePassed;

//booleans for counting phrases
boolean hasSwiped = false;
boolean hasBegun = false;
boolean hasSwipedLeft = false;
boolean hasSwipedRight = false;

//variables for measuring the GSR readings
int readingStart;
int readingEnd;
int readingDiff;

//leds
#define swipeRightGreenLed 8
#define swipeLeftRedLed 9
#define gotReadingLed 7

//emic
#define rxPin 12   //Serial input (connects to Emic 2 SOUT)
#define txPin 11   //Serial output (connects to Emic 2 SIN)

//state machine
#define waiting 1
#define checkIfNewUser 2
#define newUser 3
#define giveIntro 4
#define beginSwiping 5
#define calculate 6
#define changeHasSwipedLeft 7
#define changeHasSwipedRight 8
#define swipeLeft 9
#define swipeRight 10
#define getNoSensorTime 11
#define ending 12

EMIC2 emic;  // Creates an instance of the EMIC2 library

void setup() {

  emic.begin(rxPin, txPin);
  emic.setVoice(2);  // Sets the voice (9 choices: 0 - 8)
  emic += 10; //increases volume of emic by 10 decibels

  leftRightServo.attach(3);
  leftRightServo.write(90);

  forwardBackServo.attach(5);
  forwardBackServo.write(90);

  //pinMode(emicLedPin, OUTPUT);
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);

  pinMode(swipeLeftRedLed, OUTPUT);
  pinMode(swipeRightGreenLed, OUTPUT);
  pinMode(gotReadingLed, OUTPUT);

  digitalWrite(gotReadingLed, LOW);
}

void loop() {

  static int state = waiting;
  static unsigned long ts; //time saved
  static unsigned long noSensorTS; //time when there's no reading from the sensor, saved

  int changeUserDuration = 5; //number of seconds with no sensor reading to determine if it's a new user
  int GSR;

  int FBpos;
  int LRpos;

  GSR = analogRead(1); //get the incoming GSR sensor reading

  timePassed = (millis() / 1000) - ts; //the amount of time passed is the current time minus the last time saved
  totalTimePassed = (millis() / 1000) - noSensorTS;

  char* startingWords[] = {
    "Start by looking at this person",
    "Look carefully",
    "Judge this person",
    "I can read your feelings",
    "Can you see yourself spending your life with this person?",
    "Determine if this person has any value",
    "I can tell that you are desperate for love",
    "Honestly, I feel sad for you",
    "Why do humans want love anyway?",
    "Love only gives you pain",
    "Ok. This is your last shot."
  };

  char* swipeLeftWords[] = {
    "Swipe Left",
    "Swipe left",
    "No match here",
    "Swipe left",
    "You're picky",
    "Not a match",
    "No way",
    "This person is worthless",
    "Swipe left",
    "Nope"
  };

  char* swipeRightWords[] = {
    "Swipe right",
    "Swipe right",
    "I can tell that you like this one",
    "Swipe right",
    "You like this person",
    "Take this one home",
    "Swipe right",
    "Hubba hubba",
    "Swipe right",
    "Swipe right"
  };

  switch (state)
  {
    case waiting:
      state = newUser;
      break;

    case checkIfNewUser:
      if (GSR < 150) { //if there's no reading....
        if (totalTimePassed > changeUserDuration) { //...check to see if enough time has passed to determine a new user
          state = newUser;
        }
      }  else { //...otherwise go back to swiping
        state = beginSwiping;
      }
      break;

    case newUser:
      leftRightServo.write(90);
      forwardBackServo.write(90);
      if (GSR > 150) { //if there's a reading, start everything over and give the intro
        digitalWrite(gotReadingLed, HIGH);
        leftRightServo.write(90);
        forwardBackServo.write(90);
        iBegin = 0;
        iLeft = 0;
        iRight = 0;
        state = giveIntro;
      } else {
        digitalWrite(gotReadingLed, LOW);
      }
      break;

    case giveIntro:
      emic.speak(F("hello human. i am the true love tender robot. i'm going to help you find love."));
      //emic.speak(F("as you look carefully at each tinder profile,"));
      //emic.speak(F("i will read your heart's desire and then i will tell you whether or not you are a good match with that person."));
      // emic.speak(F("you can trust me because i am a robot. let's begin."));
      state = beginSwiping;
      break;

    case beginSwiping:
      if (iBegin < sizeof(startingWords) / 2) { //if we haven't gone through all the starting words, keep going.
        hasBegun = false;
        digitalWrite(swipeLeftRedLed, LOW);
        digitalWrite(swipeRightGreenLed, LOW);
        if (GSR > 150) {
          ts = int(millis() / 1000);
          state = calculate;
        } else {
          state = getNoSensorTime;
        }
      } else { //if we did all the starting words, go to the ending
        state = ending;
      }
      break;

    case calculate:
      if (GSR > 150) { //calculate only if there's a reading.
        if (timePassed >= 1 && hasBegun == false) { //after one second since beginning...
          emic.speak(startingWords[iBegin]); //speak the starting words...
          // leftRightServo.write(servoCenter); //move the servo back to center...
          // forwardBackServo.write(servoBack);
          readingStart = GSR; //get the initial reading...
          iBegin++; //get ready to move on to the next starting words next time.
          hasBegun = true;
          hasSwiped = false;
          digitalWrite(swipeRightGreenLed, LOW);
          digitalWrite(swipeLeftRedLed, LOW);
        }
        if (timePassed > 4) { //after four seconds, get the ending reading.
          readingEnd = GSR;
        }
        if (timePassed >= 5 && readingStart > 0 && hasSwiped == false) { //after five seconds and only if there was an initial reading...
          readingDiff = readingEnd - readingStart; //get the reading difference.
          if (readingDiff > 0 || readingDiff < -100) { //if the reading was positive (of dramatically negative)...
            state = changeHasSwipedRight; //swipe right...
          } else {
            state = changeHasSwipedLeft; //otherwise swipe left.
          }
        } else {
          readingDiff = 0;
        }
      }
      else {
        state = getNoSensorTime; //if there's no reading, go to the no sensor state.
      }
      break;

    case changeHasSwipedLeft:
      hasSwipedLeft = false;
      state = swipeLeft;
      break;

    case changeHasSwipedRight:
      hasSwipedRight = false;
      state = swipeRight;
      break;

    case swipeLeft:
      hasSwiped = true;
      if (timePassed >= 6 && hasSwipedLeft == false) {
        emic.speak(swipeLeftWords[iLeft]);
        iLeft++;
        hasSwipedLeft = true;
        digitalWrite(swipeLeftRedLed, HIGH);
        digitalWrite(swipeRightGreenLed, LOW);
        //forwardBackServo.write(servoForward);
        //leftRightServo.write(servoLeft);


        //forward
        for (FBpos = 90; FBpos >= 45; FBpos -= 1)
        {
          forwardBackServo.write(FBpos);  // Move to next position
          delay(20);               // Short pause to allow it to move
        }

        //left
        for (LRpos = 90; LRpos <= 150; LRpos += 2)
        {
          leftRightServo.write(LRpos);  // Move to next position
          delay(20);               // Short pause to allow it to move
        }

        //back
        for (FBpos = 45; FBpos <= 90; FBpos += 1)
        {
          forwardBackServo.write(FBpos);  // Move to next position
          delay(20);               // Short pause to allow it to move
        }

        //right back to center
        for (LRpos = 150; LRpos >= 90; LRpos -= 1)
        {
          leftRightServo.write(LRpos);  // Move to next position
          delay(20);               // Short pause to allow it to move
        }

        state = beginSwiping;
      }

      break;

    case swipeRight:
      hasSwiped = true;
      if (timePassed >=  6 && hasSwipedRight == false) {
        emic.speak(swipeRightWords[iRight]);
        iRight++;
        hasSwipedRight = true;
        digitalWrite(swipeRightGreenLed, HIGH);
        digitalWrite(swipeLeftRedLed, LOW);
        //forwardBackServo.write(servoForward);
        //leftRightServo.write(servoRight);

        //forward
        for (FBpos = 90; FBpos >= 45; FBpos -= 1)
        {
          forwardBackServo.write(FBpos);  // Move to next position
          delay(20);               // Short pause to allow it to move
        }

        //swipe right
        for (LRpos = 90; LRpos >= 30; LRpos -= 1)
        {
          leftRightServo.write(LRpos);  // Move to next position
          delay(20);               // Short pause to allow it to move
        }

        //back
        for (FBpos = 45; FBpos <= 90; FBpos += 1)
        {
          forwardBackServo.write(FBpos);  // Move to next position
          delay(20);               // Short pause to allow it to move
        }


        //left back to center
        for (LRpos = 30; LRpos <= 90; LRpos += 2)
        {
          leftRightServo.write(LRpos);  // Move to next position
          delay(20);               // Short pause to allow it to move
        }




        state = beginSwiping;
      }

      break;

    case getNoSensorTime:
      noSensorTS = int(millis() / 1000); //get the time when there was no sensor reading
      state = checkIfNewUser;
      break;

    case ending:
      digitalWrite(gotReadingLed, LOW); //lights off...
      digitalWrite(swipeRightGreenLed, LOW);
      digitalWrite(swipeLeftRedLed, LOW);
      emic.speak(F("go away now. please take your hands off the sensors. goodbye and good luck with your love life."));
      if (GSR < 150) { //go back to the beginning.
        state = waiting;
      } else {
      }
      break;

    default:
      state = waiting;
      break;
  }
}

