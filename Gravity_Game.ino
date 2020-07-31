//GRAVITY TEST


enum blinkRoles {WALL, GRAVITY, BUCKET};
byte blinkRole = WALL;

enum gravityStates {LEFT_DOWN, LEFT_UP, TOP, RIGHT_UP, RIGHT_DOWN, BOTTOM, NOTHING};
byte gravityState[6] = {NOTHING, NOTHING, NOTHING, NOTHING, NOTHING, NOTHING};

enum signalStates {BLANK, SENDING, RECEIVED, IM_BUCKET};
byte signalState[6] = {BLANK, BLANK, BLANK, BLANK, BLANK, BLANK};

bool bChangeRole = false;
bool bLongPress = false;

byte tFace;
byte bFace;
byte luFace;
byte ldFace;
byte ruFace;
byte rdFace;

byte bottomFace;



//GRAVITY
Timer gravityPulseTimer;
#define GRAVITY_PULSE 2000

byte gravityFace;

bool bBlank;
bool bSending;
bool bReceived;

void setup() {
  // put your setup code here, to run once:

  gravityPulseTimer.set(GRAVITY_PULSE);

}

void loop() {

  displayLoop();

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


      //2. arrange my own gravity state to match
      FOREACH_FACE(f) {
        if (!isValueReceivedOnFaceExpired(f)) {

          if (getGravityState(getLastValueReceivedOnFace(f)) == BOTTOM) {
            bottomFace = (f + 3) % 6;
          }
          if (getGravityState(getLastValueReceivedOnFace(f)) == LEFT_DOWN) {
            bottomFace = (f + 2) % 6;
          }
          if (getGravityState(getLastValueReceivedOnFace(f)) == LEFT_UP) {
            bottomFace = (f + 1) % 6;
          }
          if (getGravityState(getLastValueReceivedOnFace(f)) == TOP) {
            bottomFace = f;
          }
          if (getGravityState(getLastValueReceivedOnFace(f)) == RIGHT_UP) {
            bottomFace = (f + 5) % 6;
          }
          if (getGravityState(getLastValueReceivedOnFace(f)) == RIGHT_DOWN) {
            bottomFace = (f + 4) % 6;
          }
        }
      }

      FOREACH_FACE(f) {
        if (f == bottomFace) {
          gravityState[f] = BOTTOM;
        }
        if (f == (bottomFace + 1) % 6) {
          gravityState[f] = LEFT_DOWN;
        }
        if (f == (bottomFace + 2) % 6) {
          gravityState[f] = LEFT_UP;
        }
        if (f == (bottomFace + 3) % 6) {
          gravityState[f] = TOP;
        }
        if (f == (bottomFace + 4) % 6) {
          gravityState[f] = RIGHT_UP;
        }
        if (f == (bottomFace + 5) % 6) {
          gravityState[f] = RIGHT_DOWN;
        }
      }

      //3. change my own face to Received
      signalState[face] = RECEIVED;

      //4. set any faces I have that are in BLANK to sending

      FOREACH_FACE(f) {
        if (!isValueReceivedOnFaceExpired(f)) {
          if (signalState[f] == BLANK) {
            signalState[f] = SENDING;
          }
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
    if (getSignalState(getLastValueReceivedOnFace(face)) == SENDING) {
      //if the neighbor has already received a message, stop sending and turn to blank.
      signalState[face] = RECEIVED;
    }
    //    if (blinkRole == GRAVITY) {
    if (getSignalState(getLastValueReceivedOnFace(face)) == IM_BUCKET) {
      signalState[face] = BLANK;
      //      }
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

//DISPLAY LOOP --------------------------------------------------

void displayLoop() {

  //This is where we're going to control all visuals I think...maybe that's a terrible idea?

  if (blinkRole == BUCKET) {
    setColor(GREEN);
  }

  if (blinkRole == WALL) {
    setColor(YELLOW);
    setColorOnFace(WHITE, bottomFace);
  }

  if (blinkRole != BUCKET) {
    FOREACH_FACE(f) {
      if (signalState[f] == BLANK) {
        //        setColorOnFace(OFF, f);
      }
      if (signalState[f] == SENDING) {
        setColorOnFace(RED, f);
      }
      if (signalState[f] == RECEIVED) {
        setColorOnFace(BLUE, f);
      }
    }
  }

}

//WALL -----------------------------------------------
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

  FOREACH_FACE(f) {
    signalState[f] = IM_BUCKET;
  }

}

//GRAVITY -------------------------------
void gravityLoop() {

  if (bChangeRole) {
    blinkRole = WALL;
    bChangeRole = false;
  }

  setColor(CYAN);

  byte bFace;

  //figure out which face is down.

  FOREACH_FACE(f) {
    if (isBucket(f)) {
      byte bucketNeighbor = (f + 2) % 6;
      if (isBucket(bucketNeighbor)) {
        bFace = (f + 1) % 6;
        setColor(BLUE);
        setColorOnFace(WHITE, bFace); //this face is the "bottom", the direction of gravity
      }
    }
  }

  if (gravityPulseTimer.isExpired()) {
    gravityPulseTimer.set(GRAVITY_PULSE);
    FOREACH_FACE(f) {
      signalState[f] = SENDING;
      if (f == bFace) {
        gravityState[f] = BOTTOM;
      }
      if (f == (bFace + 1) % 6) {
        gravityState[f] = LEFT_DOWN;
      }
      if (f == (bFace + 2) % 6) {
        gravityState[f] = LEFT_UP;
      }
      if (f == (bFace + 3) % 6) {
        gravityState[f] = TOP;
      }
      if (f == (bFace + 4) % 6) {
        gravityState[f] = RIGHT_UP;
      }
      if (f == (bFace + 5) % 6) {
        gravityState[f] = RIGHT_DOWN;
      }
    }
  }
}

bool isBucket (byte face) {
  if (!isValueReceivedOnFaceExpired(face)) { //I have a neighbor
    if (getSignalState(getLastValueReceivedOnFace(face)) == IM_BUCKET) { //if the neighbor is a bucket, return true
      return true;
    } else if (getSignalState(getLastValueReceivedOnFace(face)) != IM_BUCKET) { //if the neighbor isn't a bucket, return false
      return false;
    }
  } else {
    return false; //or if I don't have a neighbor, that's also a false
  }
}


//------------------------------------------

void setRole() {
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
    setColor(MAGENTA);
  }
}

byte getGravityState(byte data) {
  return (data & 7); //returns bits D, E, and F...I think?
}

byte getSignalState(byte data) {
  return ((data >> 3) & 3); //returns bits B and C...hopefully?
}
