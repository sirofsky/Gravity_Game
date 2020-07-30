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
    if (getSignalState(getLastValueReceivedOnFace(face)) == SENDING) {
      //if the neighbor has already received a message, stop sending and turn to blank.
      signalState[face] = RECEIVED;
    }
    if (blinkRole == GRAVITY) {
      if (getSignalState(getLastValueReceivedOnFace(face)) == IM_BUCKET) {
        signalState[face] = BLANK;
      }
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

  if (blinkRole != BUCKET) {
    FOREACH_FACE(f) {
      if (signalState[f] == BLANK) {
        //        setColorOnFace(WHITE, f);
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

  //how many buckets is gravity touching?

  byte bucketCount = 0;

  //  bool bFace[6] = {false, false, false, false, false, false};

//  bool bFace0 = false;
//  bool bFace1 = false;
//  bool bFace2 = false;
//  bool bFace3 = false;
//  bool bFace4 = false;
//  bool bFace5 = false;
//
//  bool bGravityOn = false;

  setColor(CYAN);

  FOREACH_FACE(f) {

    if (isBucket(f)) {
      byte bucketNeighbor = (f + 2) % 6;
      if (isBucket(bucketNeighbor)) {
        setColor(BLUE);
      }
    }


  }


  //    if (bFace[f] == false) {
  //      //      setColorOnFace(ORANGE, f);
  //    }
  //
  //    if (bucketCount == 2) {
  //      if (bFace[f] == true) {
  //        setColorOnFace(MAGENTA, f);
  //        byte matchingBucketFace = (f + 2) % 6;
  //        byte bonusFace = (f + 1) % 6;
  //        if (bFace[matchingBucketFace] == true) {
  //          setColorOnFace(BLUE, bonusFace);
  //        }
  //      }
  //
  //    } else {
  //      //          setColor(CYAN);
  //    }




  if (gravityPulseTimer.isExpired()) {
    gravityPulseTimer.set(GRAVITY_PULSE);
    FOREACH_FACE(f) {
      signalState[f] = SENDING;
      gravityState[f] = gravityStates(1);
    }
  }


}

bool isBucket (byte face) {
  if (!isValueReceivedOnFaceExpired(face)) { //I have a neighbor

    if (getSignalState(getLastValueReceivedOnFace(face)) == IM_BUCKET) {
      return true;
    } else if (getSignalState(getLastValueReceivedOnFace(face)) != IM_BUCKET) {
      return false;
    }
  } else {
    return false;
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
