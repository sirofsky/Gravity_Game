//GRAVITY GAME AKA TREASURE TUMBLE

//Car Salesman slaps code
//"You could fit so much incoherent code in this badboy"

//By Jacob Surovsky

enum blinkRoles
{
  WALL,
  BUCKET,
  SPAWNER
};

byte blinkRole = WALL;

enum wallRoles
{
  GO_SIDE,
  SPINNER, //no more
  SPLITTER, //no more
  DEATHTRAP, //no more
  FUNNEL,
  SWITCHER
};

byte wallRole = FUNNEL;

enum gravityStates
{
  LEFT_DOWN,
  LEFT_UP,
  TOP,
  RIGHT_UP,
  RIGHT_DOWN,
  BOTTOM,
  BUCKET_LEFT,
  BUCKET_RIGHT
};

byte gravityState[6] = {TOP, TOP, TOP, TOP, TOP, TOP};

enum signalStates
{
  BLANK,
  SENDING,
  RECEIVED,
  IM_BUCKET
};

byte signalState[6] = {BLANK, BLANK, BLANK, BLANK, BLANK, BLANK};

enum ballStates
{
  BALL,
  NO_BALL
};

byte ballState[6] = {NO_BALL, NO_BALL, NO_BALL, NO_BALL, NO_BALL, NO_BALL};

bool bChangeRole = false;
bool bLongPress = false;

bool bChangeWallRole = false;
bool bDoubleClick = false;

bool treasurePrimed;

byte bottomFace;

#define WALLRED makeColorHSB(110, 255, 255) //more like cyan <-- SUCH A NICE COLOR TOO
#define PURPLE makeColorHSB(200, 255, 230)


#define FEATURE_COLOR makeColorHSB(25, 255, 240) //orange

//#define WALLPURPLE makeColorHSB(155, 255, 220) //more like blue

#define BALL_COLOR makeColorHSB(90, 225, 255) //an emerald green

#define BG_COLOR makeColorHSB(50, 200, 255) //a temple tan ideally


//GRAVITY
Timer gravityPulseTimer;
#define GRAVITY_PULSE 50

byte gravityFace;
byte bFace;

//BUCKET
Timer marbleScoreTimer;

//BALL (some variables needed to get the ball rolling)
Timer ballDropTimer;
#define BALL_PULSE 150

bool bBallFalling = false;

byte startingFace;

byte ballPos;
byte stepCount = 0;

bool ballFell = false;

//SWITCHER
bool goLeft;

//SPLITTER
bool bSplitter;

void setup() {
  randomize();
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
        case IM_BUCKET:
          imBucketLoop(f);
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

byte sendingCounter = 0;

Timer connectedTimer;

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
      //if the neighbor has already sent a message, stop sending and say received
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

      sendingCounter = 0;
    }
  } else {
    signalState[face] = BLANK;

    sendingCounter = 0;
  }
}


void imBucketLoop(byte face) {
  signalState[face] = BLANK;
}

//WALL -----------------------------------------------
void wallLoop() {

  if (bChangeRole) {
    blinkRole = BUCKET;
    bChangeRole = false;
  }

  //  bool isGravity;

  FOREACH_FACE(f) {
    if (isBucket(f)) { //do I have a neighbor and are they shouting IM_BUCKET?
      byte bucketNeighbor = (f + 2) % 6; //and what about their neighbor?
      if (isBucket(bucketNeighbor)) {
        bFace = (f + 1) % 6;
        bottomFace = bFace;
        gravityLoop(); //I get to decide what direction gravity is for the entire game! Yippee!
      }
    } else { //I'm a normal wall piece
      setWallOrientation();
    }
  }

  //here's our background color drawing
  setColor(dim(BG_COLOR, 160));
  setColorOnFace(dim(BG_COLOR, 100), (bottomFace + 2) % 6);
  setColorOnFace(dim(BG_COLOR, 100), (bottomFace + 4) % 6);
  setWallRole();


  //  if (isValueReceivedOnFaceExpired((bottomFace + 3) % 6)) { //I have nobody above me, then it's okay to spawn a ball by single clicking
  //    if (buttonSingleClicked()) {
  //      bBallFalling = true;
  //      startingFace = (bottomFace + 3) % 6;
  //    }
  //  }
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

void setWallOrientation() {

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
}


//this is where we draw the graphics for each temple pattern

void funnelLoop() {

}

bool fallDown;

void goSideLoop() {

  if ((bottomFace + 1) % 6 == 0 || (bottomFace + 5) % 6 == 0) {

    setColorOnFace(FEATURE_COLOR, 0);
    fallDown = false;
  } else {
    setColorOnFace(PURPLE, 0);
    fallDown = true;
  }

}

byte spinCount;
byte fallFace;

void spinnerLoop() {

  setColorOnFace(FEATURE_COLOR, (spinCount) % 6);
  setColorOnFace(FEATURE_COLOR, (spinCount + 3) % 6);

  if (spinCount == bottomFace || (spinCount + 3) % 6 == bottomFace) {
    fallFace = bottomFace;
    setColorOnFace(PURPLE, (spinCount) % 6);
    setColorOnFace(PURPLE, (spinCount + 3) % 6);
  }

  if (spinCount == (bottomFace + 1) % 6 || (spinCount + 3) % 6 == (bottomFace + 1) % 6) {
    fallFace = (bottomFace + 1) % 6;
  }

  if (spinCount == (bottomFace + 5) % 6 || (spinCount + 3) % 6 == (bottomFace + 5) % 6) {
    fallFace = (bottomFace + 5) % 6;
  }
}

void switcherLoop() {

  if (goLeft == true) {
    setColorOnFace(PURPLE, (bottomFace + 4) % 6);
    setColorOnFace(FEATURE_COLOR, (bottomFace + 1) % 6);
  } else {
    setColorOnFace(PURPLE, (bottomFace + 2) % 6);
    setColorOnFace(FEATURE_COLOR, (bottomFace + 5) % 6);
  }

}

void splitterLoop() {

  setColorOnFace(FEATURE_COLOR, (bottomFace + 1) % 6);
  setColorOnFace(FEATURE_COLOR, (bottomFace + 5) % 6);
}

void deathtrapLoop() {
  setColorOnFace(RED, bottomFace);
  setColorOnFace(RED, (bottomFace + 2) % 6);
  setColorOnFace(RED, (bottomFace + 4) % 6);

}

//this is where the ball figures out where to go for each temple pattern

void ballLogic () {

  bSplitter = false;

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
    if (wallRole == GO_SIDE) {

      if (stepCount == 1) {
        ballPos = startingFace;
      }
      if (stepCount == 2) {

        if (fallDown == false) {
          ballPos = 0;
        } else if (fallDown == true) {
          ballPos = bottomFace;
        }
        ballState[ballPos] = BALL;
      }
      if (stepCount == 3) {
        ballFell = true;
      }
    }
    if (wallRole == SPINNER) {

      if (stepCount == 1) {
        ballPos = startingFace;
      }
      if (stepCount == 2) {

        ballPos = fallFace;

        ballState[ballPos] = BALL;
      }
      if (stepCount == 3) {
        ballFell = true;
        spinCount = spinCount + 1;
        if (spinCount == 3) {
          spinCount = 0;
        }
      }
    }
    if (wallRole == SWITCHER) {

      if (goLeft == true) {

        if (stepCount == 1) {
          ballPos = startingFace;
        }
        if (stepCount == 2) {
          ballPos = (bottomFace + 1) % 6;
          ballState[ballPos] = BALL;
        }
        if (stepCount == 3) {
          goLeft = false;
          ballFell = true;
        }
      } else {

        if (stepCount == 1) {
          ballPos = startingFace;
        }
        if (stepCount == 2) {
          ballPos = (bottomFace + 5) % 6;
          ballState[ballPos] = BALL;
        }
        if (stepCount == 3) {
          goLeft = true;
          ballFell = true;
        }
      }
    }

    if (wallRole == SPLITTER) {
      bSplitter = true;

      if (stepCount == 1) {
        ballPos = startingFace;
      }
      if (stepCount == 2) {
        ballPos = (bottomFace + 1) % 6;
        ballState[ballPos] = BALL;
        ballState[(ballPos + 4) % 6] = BALL;

      }
      if (stepCount == 3) {
        ballFell = true;
        ballState[(ballPos + 4) % 6] = NO_BALL;
      }
    }

    if (wallRole == DEATHTRAP) {

      if (stepCount == 1) {
        ballPos = startingFace;
      }
      if (stepCount == 2) {
        ballFell = true;
      }
    }

    if (blinkRole == SPAWNER) {
      if (stepCount == 1) {
        ballState[bottomFace] = BALL;
      }
      if (stepCount == 2) {
        ballFell = true;
      }

    }

    ballDropTimer.set(BALL_PULSE);
    stepCount = stepCount + 1;

  } else if (!ballDropTimer.isExpired()) {

    //we don't want to see the first frame of the ball dropping
    if (stepCount > 1) {
      setColorOnFace(BALL_COLOR, ballPos);
      if (bSplitter == true) {
        if (stepCount == 2) {
          setColorOnFace(BALL_COLOR, (ballPos + 4) % 6);
        }
      }
    }

    if (ballFell == true) {
      stepCount = 0;
      ballState[ballPos] = NO_BALL;
      bBallFalling = false;
      ballFell = false;

    }
  }

}

byte marbleScore = 0;

//BUCKET -------------------------
void bucketLoop() {
  if (bChangeRole) {
    blinkRole = SPAWNER;
    bChangeRole = false;
  }

  bool addScore;

  setColor(FEATURE_COLOR);

  FOREACH_FACE(f) {

    signalState[f] = IM_BUCKET;

    if (!isValueReceivedOnFaceExpired(f)) {
      if (getBallState(getLastValueReceivedOnFace(f)) == BALL) {

        if (marbleScoreTimer.isExpired()) {
          marbleScore = (marbleScore + 1) % 6;
          marbleScoreTimer.set(BALL_PULSE * 2); //not super elegant, but this way it only counts a ball once
        }
      }

      //now we need to find out which face is on the bottom to get our bearings.
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

  if (buttonDoubleClicked()) {
    marbleScore = 0;
  }

  FOREACH_FACE(f) {
    if (f <= marbleScore) {
      setColorOnFace(BALL_COLOR, ((bottomFace + f ) % 6));
    }
  }

}


//GRAVITY -------------------------------
void gravityLoop() {

  //I'm the face between the two buckets

  if (bChangeRole) {
    blinkRole = WALL;
    bChangeRole = false;
  }

  wallRole = SWITCHER; //this piece will always alternate between passing the treasure to either bucket

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

//SPAWNER------------------------------------------

//the piece that drops the treasure

void spawnerLoop() {
  if (bChangeRole) {
    blinkRole = WALL;
    bChangeRole = false;
  }
  setColor(dim(BALL_COLOR, 125));

  setWallOrientation();

  bool treasureDropped;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {

      treasureDropped = true;

      if (didValueOnFaceChange(f)) { //am I recieving new information and have been rejoined to the tower?
        connectedTimer.set(200);
      }
      if (didValueOnFaceChange(f) == false) { //has new information stopped coming in? I'm disconnected from the tower? If so, prime treasure

        if (connectedTimer.isExpired()) {
//          setColorOnFace(BLUE, f);
          treasurePrimed = true;

          treasureDropped = false;
        }
      }
    }

    if (isAlone() == true) { //am I entirely alone? if so, prime treasure
      //are any of us?
      //dang that's deep

      treasurePrimed = true;

      treasureDropped = false;
//      setColorOnFace(MAGENTA, bottomFace);
    }
  }

  if (treasureDropped == true && treasurePrimed == true) {
    bBallFalling = true;
    startingFace = (bottomFace + 3) % 6;
    treasurePrimed = false;
  }

  if (treasurePrimed == true) {
    treasurePrimedAnimation();
  }

  if (bBallFalling == true) {
    ballLogic();
  }
}

#define TREASURE_TIME 100
byte treasureFace;
Timer treasureTimer;

void treasurePrimedAnimation() {

  if (treasureTimer.isExpired()) {

    treasureFace = (treasureFace + 1) % 6;
    treasureTimer.set(TREASURE_TIME);

  }

  setColorOnFace(BALL_COLOR, treasureFace);
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
    case SPAWNER:
      spawnerLoop();
      break;
  }

  if (bLongPress) {
    //transition color
    setColor(WHITE);
  }
}

void setWallRole() {

  if (buttonSingleClicked()) { //prime a treasure piece to be dropped

    treasurePrimed = true;
    sendingCounter = 0;

  }


  if (buttonDoubleClicked()) { //switch the role randomly
    setColor(BALL_COLOR); //I'd like to add a fun animation here.

    wallRole = (wallRole + random(2) + 1) % 4;

    bSplitter = false;
  }


  if (buttonMultiClicked()) {
    if (buttonClickCount() == 3) {
      wallRole = wallRole + 1;
      if (wallRole == 6) {
        wallRole = 0;
      }
    }

    bSplitter = false;
  }

  switch (wallRole) {
    case GO_SIDE:
      goSideLoop();
      break;
    case SPINNER:
      spinnerLoop();
      break;
    case SPLITTER:
      splitterLoop();
      break;
    case DEATHTRAP:
      deathtrapLoop();
      break;
    case FUNNEL:
      funnelLoop();
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
