//GRAVITY TEST

enum blinkRoles {WALL, GRAVITY, BUCKET};
byte blinkRole = WALL;

enum gravityStates {LEFT_DOWN, LEFT_UP, TOP, RIGHT_UP, RIGHT_DOWN, BOTTOM, NOTHING, IM_BUCKET};
byte gravityState[6] = {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING};

enum signalStates {BLANK, SENDING, LISTENING, RECEIVED};
byte signalState[6] = {BLANK, BLANK, BLANK, BLANK, BLANK, BLANK};

bool bChangeRole = false;
bool bLongPress = false;


//GRAVITY
Timer gravityPulseTimer;
#define GRAVITY_PULSE 2000

byte tempColor;

void setup() {
  // put your setup code here, to run once:

  gravityPulseTimer.set(GRAVITY_PULSE);

}

void loop() {

  setRole();

  if (blinkRole != BUCKET) { //if I'm not a bucket...
    FOREACH_FACE(f) {
      switch (signalState[f]) {
        case BLANK:
          blankLoop(f);
          break;
        case SENDING:
          sendingLoop(f);
          break;
        case RECEIVED:
          receivedLoop(f);
          break;
      }
    }
  }

  displayLoop();

  FOREACH_FACE(f) {
    byte sendData = (signalState[f] << 3) + (gravityState[f]);
    setValueSentOnFace(sendData, f);
  }
}

void blankLoop(byte face) {
  //listen to hear SENDING
  //when I hear sending, I want to do some things:

  if (!isValueReceivedOnFaceExpired(face)) {

    if (getSignalState(getLastValueReceivedOnFace(face)) == SENDING) {
      //I heard sending

      //1. parse its actual message (gravity state)
      if (getGravityState(getLastValueReceivedOnFace(face)) == LEFT_DOWN) {
        tempColor = 0;
      }
      if (getGravityState(getLastValueReceivedOnFace(face)) == LEFT_UP) {
        tempColor = 20;
      }
      if (getGravityState(getLastValueReceivedOnFace(face)) == TOP) {
        tempColor = 40;
      }
      if (getGravityState(getLastValueReceivedOnFace(face)) == RIGHT_UP) {
        tempColor = 80;
      }
      if (getGravityState(getLastValueReceivedOnFace(face)) == RIGHT_DOWN) {
        tempColor = 160;
      }
      if (getGravityState(getLastValueReceivedOnFace(face)) == BOTTOM) {
        tempColor = 200;
      }


      //2. arrange my own gravity state to match
      setColor(makeColorHSB(tempColor, 255, 255));

      //3. change my own face to Received
      signalState[face] = RECEIVED;

      //4. set any faces I have that are in BLANK to sending
      FOREACH_FACE(f) {
        gravityState[f] = getGravityState(getLastValueReceivedOnFace(face));

        if (signalState[f] == BLANK) {
          signalState[f] = SENDING;
        }
      }
    }
  }


}

void sendingLoop(byte face) {
  if (!isValueReceivedOnFaceExpired(face)) {
    if (getSignalState(getLastValueReceivedOnFace(face)) == RECEIVED) {
      //if the neighbor has already received a message, stop sending and turn to blank.
      signalState[face] = BLANK;
    }
  } else {
    signalState[face] = BLANK;
  }

}

void receivedLoop(byte face) {
  if (!isValueReceivedOnFaceExpired(face)) {
    if (getSignalState(getLastValueReceivedOnFace(face)) == BLANK) {
      //if the neighbor is blank, stop sending and turn to blank.
      signalState[face] = BLANK;
    }
  } else {
    signalState[face] = BLANK;
  }
}

void displayLoop(){
 
  
  
  }

//WALL -----------------------------------------
void wallLoop() {

  if (bChangeRole) {
    blinkRole = BUCKET;
    bChangeRole = false;
  }


}


//BUCKET -------------------------
void bucketLoop() {
  if (bChangeRole) {
    blinkRole = GRAVITY;
    bChangeRole = false;
  }

  setColor(GREEN);

}

//GRAVITY -------------------------------
void gravityLoop() {

  if (bChangeRole) {
    blinkRole = WALL;
    bChangeRole = false;
  }



  if (tempColor == 0) {
    setColor(RED);
  }
  if (tempColor == 1) {
    setColor(ORANGE);
  }
  if (tempColor == 2) {
    setColor(YELLOW);
  }
  if (tempColor == 3) {
    setColor(GREEN);
  }
  if (tempColor == 4) {
    setColor(BLUE);
  }
  if (tempColor == 5) {
    setColor(MAGENTA);
  }


  if (gravityPulseTimer.isExpired()) {
    tempColor = (tempColor + 1) % 6;
    gravityPulseTimer.set(GRAVITY_PULSE);
    FOREACH_FACE(f) {
      signalState[f] = SENDING;
      gravityState[f] == gravityStates(tempColor);
    }
  }

}


//-------------------------------------

void setRole() {
  // put your main code here, to run repeatedly:
  if (hasWoken()) {
    bLongPress = false;
  }

  if (buttonLongPressed()) {
    bLongPress = true;
  }

  if (buttonReleased()) {
    if (bLongPress) {
      // now change the role
      bChangeRole = true;
      bLongPress = false;
    }

  }
  switch (blinkRole) {
    case WALL:
      wallLoop();
      break;
    case BUCKET:
      bucketLoop();
      break;
    case GRAVITY:
      gravityLoop();
      break;
  }

  if (bLongPress) {
    //transition color
    setColor(WHITE);
  }
}

byte getGravityState(byte data) {
  return (data & 7); //returns bits D, E, and F...I think?
}

byte getSignalState(byte data) {
  return ((data >> 3) & 3); //returns bits B and C...hopefully?
}
