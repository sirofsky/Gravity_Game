//GRAVITY TEST (AKA BLINK-O)
//By Jacob Surovsky

enum blinkRoles {WALL, GRAVITY, BUCKET};
byte blinkRole = WALL;

enum wallRoles {FUNNEL, GO_LEFT, GO_RIGHT, SWITCHER};
byte wallRole = FUNNEL;

enum gravityStates {LEFT_DOWN, LEFT_UP, TOP, RIGHT_UP, RIGHT_DOWN, BOTTOM, BUCKET_LEFT, BUCKET_RIGHT};
byte gravityState[6] = {TOP, TOP, TOP, TOP, TOP, TOP};

enum signalStates {BLANK, SENDING, RECEIVED, IM_BUCKET};
byte signalState[6] = {BLANK, BLANK, BLANK, BLANK, BLANK, BLANK};

enum ballStates {BALL, NO_BALL};
byte ballState[6] = {NO_BALL, NO_BALL, NO_BALL, NO_BALL, NO_BALL, NO_BALL};

bool bChangeRole = false;
bool bLongPress = false;

bool bChangeWallRole = false;
bool bDoubleClick = false;

byte bottomFace;

#define WALLRED makeColorHSB(110, 255, 255) //more like cyan <-- SUCH A NICE COLOR TOO
#define WALLBLUE makeColorHSB(200, 255, 230) //more like purple
#define WALLPURPLE makeColorHSB(155, 255, 220) //more like blue

#define BALLCOLOR makeColorRGB(255, 255, 100)


//GRAVITY
Timer gravityPulseTimer;
#define GRAVITY_PULSE 50

byte gravityFace;

//BALL (some variables needed to get the ball rolling)
Timer ballDropTimer;
#define BALL_PULSE 100

bool bBallFalling = false;

byte startingFace;

byte ballPos;
byte stepCount = 0;

bool ballFell = false;


void setup() {
  // put your setup code here, to run once:

  //gotta get some timers set here
  gravityPulseTimer.set(GRAVITY_PULSE);

  ballDropTimer.set(BALL_PULSE);

}

void loop() {

  //long press logic to figure out what role we're in
  setRole();

  //the main communication loop for signal states
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

  //the full data assembly being sent off
  //for more info on how this works, check out the Tutorial Safely Sending Signals Part 2!
  FOREACH_FACE(f) {
    byte sendData = (ballState[f] << 5) + (signalState[f] << 3) + (gravityState[f]);
    setValueSentOnFace(sendData, f);
  }
}

//SIGNAL STATES ------------------------

void blankLoop(byte face) {
  //listen to hear SENDING
  //when I hear sending, I want to do some things:

  if (!isValueReceivedOnFaceExpired(face)) {

    if (getSignalState(getLastValueReceivedOnFace(face)) == SENDING) {

      //Guess what? I heard sending

      //2. arrange my own gravity state to match
      if (getGravityState(getLastValueReceivedOnFace(face)) == BOTTOM) {
        bottomFace = (face + 3) % 6;
      }
      if (getGravityState(getLastValueReceivedOnFace(face)) == LEFT_DOWN) {
        bottomFace = (face + 2) % 6;
      }
      if (getGravityState(getLastValueReceivedOnFace(face)) == LEFT_UP) {
        bottomFace = (face + 1) % 6;
      }
      if (getGravityState(getLastValueReceivedOnFace(face)) == TOP) {
        bottomFace = face;
      }
      if (getGravityState(getLastValueReceivedOnFace(face)) == RIGHT_UP) {
        bottomFace = (face + 5) % 6;
      }
      if (getGravityState(getLastValueReceivedOnFace(face)) == RIGHT_DOWN) {
        bottomFace = (face + 4) % 6;
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
    if (getSignalState(getLastValueReceivedOnFace(face)) == IM_BUCKET) {
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

//WALL -----------------------------------------------
void wallLoop() {

  if (bChangeRole) {
    blinkRole = BUCKET;
    bChangeRole = false;
  }

  setColor(dim(WHITE, 60));

  setWallRole();

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


  if (isValueReceivedOnFaceExpired((bottomFace + 3) % 6)) { //I have nobody above me, then it's okay to spawn a ball by single clicking
    if (buttonSingleClicked()) {
      bBallFalling = true;
      startingFace = (bottomFace + 3) % 6; 
    } 
  }
  //otherwise we out here listening for anyone saying ball
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      if (getBallState(getLastValueReceivedOnFace(f)) == BALL) {
        bBallFalling = true;
        startingFace = f;
      }
    }
  }

  if (bBallFalling == true) {
    ballLogic();
  }

}

void funnelLoop() {

  if (bChangeWallRole) {
    wallRole = GO_LEFT;
    bChangeWallRole = false;
  }

  setColorOnFace(dim(WHITE, 200), bottomFace);

}

void goLeftLoop() {

  if (bChangeWallRole) {
    wallRole = GO_RIGHT;
    bChangeWallRole = false;
  }

  //  setColorOnFace(WALLRED, (bottomFace + 4) % 6);
  setColorOnFace(WALLRED, (bottomFace + 1) % 6);

}
void goRightLoop() {

  if (bChangeWallRole) {
    wallRole = SWITCHER;
    bChangeWallRole = false;
  }
  //  setColorOnFace(WALLBLUE, (bottomFace + 2) % 6);
  setColorOnFace(WALLBLUE, (bottomFace + 5) % 6);
}

void switcherLoop() {

  if (bChangeWallRole) {
    wallRole = FUNNEL;
    bChangeWallRole = false;
  }

  setColorOnFace(WALLPURPLE, (bottomFace + 4) % 6);
  setColorOnFace(WALLPURPLE, (bottomFace + 1) % 6);
}

void ballLogic () {

  if (ballDropTimer.isExpired()) {

    if (wallRole == FUNNEL) {

      if (stepCount == 1) {
        ballPos = startingFace;
      }
      if (stepCount == 2) {
        ballPos = bottomFace;
        ballState[ballPos] = BALL;
      }
      if (stepCount == 3) {
        ballFell = true;
      }
    }

    ballDropTimer.set(BALL_PULSE);
    stepCount = stepCount + 1;

  } else if (!ballDropTimer.isExpired()) {

    //we don't want to see the first frame of the ball dropping
    if (stepCount > 1) {
      setColorOnFace(YELLOW, ballPos);
    }

    if (ballFell == true) {
      stepCount = 0;
      ballState[ballPos] = NO_BALL;
      bBallFalling = false;
      ballFell = false;

    }
  }

}

//BUCKET -------------------------
void bucketLoop() {
  if (bChangeRole) {
    blinkRole = GRAVITY;
    bChangeRole = false;
  }

  setColor(GREEN);

  FOREACH_FACE(f) {
    signalState[f] = IM_BUCKET;

    if (!isValueReceivedOnFaceExpired(f)) {
      if (getGravityState(getLastValueReceivedOnFace(f)) == BUCKET_LEFT) {

        setColor(dim(WALLBLUE, 100));
      }
      if (getGravityState(getLastValueReceivedOnFace(f)) == BUCKET_RIGHT) {

        setColor(dim(WALLRED, 100));
      }
    }

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
        setColor(dim(BLUE, 180));
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
        gravityState[f] = BUCKET_LEFT; //special value just for our bucket friends
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
        gravityState[f] = BUCKET_RIGHT; //special value just for our bucket friends
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

//this is just where we set the roles and that fun stuff
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
    setColor(WHITE);
  }
}

void setWallRole() {

  if (buttonDoubleClicked()) {
    bChangeWallRole = true;
  }

  switch (wallRole) {
    case FUNNEL:
      funnelLoop();
      break;
    case GO_LEFT:
      goLeftLoop();
      break;
    case GO_RIGHT:
      goRightLoop();
      break;
    case SWITCHER:
      switcherLoop();
      break;
  }

}

//and this is all that bit stuff that allows all the signals to be safely sent!

byte getGravityState(byte data) {
  return (data & 7); //returns bits D, E, and F
}

byte getSignalState(byte data) {
  return ((data >> 3) & 3); //returns bits B and C
}

byte getBallState(byte data) {
  return ((data >> 5) & 1); //returns bit A
}
