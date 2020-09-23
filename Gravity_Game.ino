//TREASURE TUMBLE

//Car Salesman slaps code
//"You could fit so much incoherent code in this badboy"

//By Jacob Surovsky

enum blinkRoles
{
  WALL,
  BUCKET,
  GRAVITY,
  SPAWNER
};

byte blinkRole = WALL;

enum wallRoles
{
  GO_SIDE,
  //  SPINNER, //no more
  SPLITTER,
  DEATHTRAP,
  //  FUNNEL, //NO MORE
  SWITCHER
};

byte wallRole = GO_SIDE;

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

#define NICE_BLUE makeColorHSB(110, 255, 255) //more like cyan <-- SUCH A NICE COLOR TOO
#define PURPLE makeColorHSB(200, 255, 230)
#define FEATURE_COLOR makeColorHSB(25, 255, 240) //orange
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
#define BALL_PULSE 300 //slowed down for Dan's tired eyes

bool bBallFalling = false;

byte startingFace;

byte ballPos;
byte stepCount = 0;

bool ballFell = false;

//SWITCHER
bool goLeft;
bool imSwitcher = false;

//SPLITTER


byte randomWallRole;

void setup() {
  randomize(); //gotta do this to make sure our random is truly random
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

  FOREACH_FACE(f) {
    if (isBucket(f)) { //do I have a neighbor and are they shouting IM_BUCKET?
      byte bucketNeighbor = (f + 2) % 6; //and what about their neighbor?
      if (isBucket(bucketNeighbor)) {
        bFace = (f + 1) % 6;
        bottomFace = bFace;
        randomWallRole = 10;
        imSwitcher = true;
        gravityLoop(); //I get to decide what direction gravity is for the entire game! Yippee!
      }
    } else { //I'm a normal wall piece
      setWallOrientation();
    }
  }

  shouldIRandomize();

  //here's our background color drawing
  setColor(dim(BG_COLOR, 160));
  setColorOnFace(dim(BG_COLOR, 100), (bottomFace + 2) % 6);
  setColorOnFace(dim(BG_COLOR, 100), (bottomFace + 4) % 6);
  setWallRole();

  //We out here listening for anyone saying ball
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


bool bShouldIRandomize = false;
bool didIRandomize = true;

void shouldIRandomize() {

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {

      bShouldIRandomize = false;

      if (didValueOnFaceChange(f)) { //am I recieving new information and have been rejoined to the tower?
        connectedTimer.set(400); //check this timer I think it's causing some of the current bugs. Might want to move it or make it longer
        didIRandomize = false;
      }
      if (didValueOnFaceChange(f) == false) { //has new information stopped coming in? I'm disconnected from the tower? If so, prime treasure

        if (connectedTimer.isExpired()) {
          bShouldIRandomize = true;
        }
      }
    }

    if (isAlone() == true) { //am I entirely alone? if so, I should randomize
      bShouldIRandomize = true;
    }
  }

  if (bShouldIRandomize == true) {
    if (didIRandomize == false) {
      randomWallRole = random(9); //might want to make this number higher
      didIRandomize = true;
      bShouldIRandomize = false;
    }
  }
}

//this is where we draw the graphics for each temple pattern

bool fallDown;
byte goFace;

void goSideLoop() {

  if (goFace == (bottomFace + 2) % 6  || goFace == (bottomFace + 3) % 6 || goFace == (bottomFace + 4) % 6) {
    goFace = (goFace + 3) % 6;
  }

  setColorOnFace(dim(BG_COLOR, 160), (bottomFace + 2) % 6);
  setColorOnFace(dim(BG_COLOR, 160), (bottomFace + 3) % 6);
  setColorOnFace(dim(BG_COLOR, 160), (bottomFace + 4) % 6);
  setColorOnFace(FEATURE_COLOR, bottomFace);
  setColorOnFace(FEATURE_COLOR, (bottomFace + 1) % 6);
  setColorOnFace(FEATURE_COLOR, (bottomFace + 5) % 6);
  setColorOnFace(OFF, goFace);
  setColorOnFace(OFF, (goFace + 3) % 6);

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

  setColorOnFace(PURPLE, (bottomFace + 1) % 6);
  setColorOnFace(PURPLE, (bottomFace + 5) % 6);
  setColorOnFace(NICE_BLUE, bottomFace);
}

void deathtrapLoop() {
  setColorOnFace(WHITE, bottomFace);
  setColorOnFace(RED, (bottomFace + 1) % 6);
  setColorOnFace(RED, (bottomFace + 5) % 6);
}

//this is where the ball figures out where to go for each temple pattern

bool pickRandomWallRole;

void ballLogic() {

  if (ballDropTimer.isExpired()) {
    if (wallRole == GO_SIDE) {

      if (stepCount == 1) {
        ballPos = startingFace;
      }
      if (stepCount == 2) {

        ballPos = goFace;
        ballState[ballPos] = BALL;
      }
      if (stepCount == 3) {
        ballFell = true;
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
    if (wallRole == SPLITTER) { //1 treasure comes in, 3 come out

      if (stepCount == 1) {
        ballPos = startingFace;
      }
      if (stepCount == 2) {
        ballPos = bottomFace;
        ballState[ballPos] = BALL;
        ballState[(bottomFace + 1) % 6] = BALL;
        ballState[(bottomFace + 5) % 6] = BALL;
        setColorOnFace(WHITE, (bottomFace + 1) % 6);
        setColorOnFace(WHITE, (bottomFace + 5) % 6); //I should summon an animation here that takes as long as a frame

      }
      if (stepCount == 3) {
        ballState[(bottomFace + 1) % 6] = NO_BALL;
        ballState[(bottomFace + 5) % 6] = NO_BALL;
        ballFell = true;
      }
    }
    if (wallRole == DEATHTRAP) { //1 treasure comes in, NONE come out

      if (stepCount == 1) {
        ballPos = startingFace;
      }
      if (stepCount == 2) {
        ballFell = true;
      }
    }
    if (blinkRole == SPAWNER) {
      if (stepCount == 1) {
        ballPos = startingFace;
      }
      if (stepCount == 2) {
        ballPos = bottomFace;
        ballState[bottomFace] = BALL;
        ballState[(bottomFace + 1) % 6] = NO_BALL;
        ballState[(bottomFace + 5) % 6] = NO_BALL;
      }
      if (stepCount == 3) {
        ballFell = true;
      }
    }
    ballDropTimer.set(BALL_PULSE);
    stepCount = stepCount + 1;

  } else if (!ballDropTimer.isExpired()) {
    //we don't want to see the first frame of the ball dropping
    byte pulseMapped = map(millis() % BALL_PULSE, 0, BALL_PULSE, 0, 255); //give the ball a smoother pulse animation while falling
    byte dimness = sin8_C(pulseMapped);

    if (stepCount > 1) {
      setColorOnFace(dim(BALL_COLOR, dimness), ballPos);
    }
  }

  if (ballFell == true) {
    stepCount = 0;
    ballState[ballPos] = NO_BALL;
    bBallFalling = false;
    ballFell = false;
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

    signalState[f] = IM_BUCKET; //<-- this line takes up 5% of the space somehow

    if (!isValueReceivedOnFaceExpired(f)) {
      if (getBallState(getLastValueReceivedOnFace(f)) == BALL) {

        if (marbleScoreTimer.isExpired()) {
          marbleScore = (marbleScore + 1) % 6;
          marbleScoreTimer.set(BALL_PULSE * 2); //not super elegant, but this way it only counts a ball once
        }
      }

      //now we need to find out which face is on the bottom to get our bearings.
      //but we don't need ALL of these because we know where the central source of truth is...
      //cool we saved 1%
      //      if (getGravityState(getLastValueReceivedOnFace(f)) == BOTTOM) {
      //        bottomFace = (f + 3) % 6;
      //      }
      if (getGravityState(getLastValueReceivedOnFace(f)) == LEFT_DOWN) {
        bottomFace = (f + 2) % 6;
      }
      //      if (getGravityState(getLastValueReceivedOnFace(f)) == LEFT_UP) {
      //        bottomFace = (f + 1) % 6;
      //      }
      //      if (getGravityState(getLastValueReceivedOnFace(f)) == TOP) {
      //        bottomFace = f;
      //      }
      //      if (getGravityState(getLastValueReceivedOnFace(f)) == RIGHT_UP) {
      //        bottomFace = (f + 5) % 6;
      //      }
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

  if (gravityPulseTimer.isExpired()) { //gravity pulse is how frequently the gravity refresh message will be sent
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
        connectedTimer.set(400); //check this timer I think it's causing some of the current bugs. Might want to move it or make it longer
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
    if (isValueReceivedOnFaceExpired((bottomFace + 3) % 6)) { //I have nobody above me, then it's okay to spawn a ball

      bBallFalling = true;
      //      startingFace = (bottomFace + 3) % 6;
      treasurePrimed = false;
    }
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

  if (randomWallRole <= 5) {
    wallRole = GO_SIDE;
    goFace = randomWallRole;
  } else if (randomWallRole == 6 || randomWallRole == 7) {
    wallRole = SPLITTER;
  } else if (randomWallRole == 8) {
    wallRole = DEATHTRAP;
  } else if (randomWallRole == 9 || randomWallRole == 10) {
    wallRole = SWITCHER;
  }

  switch (wallRole) {
    case GO_SIDE:
      goSideLoop();
      break;
    case SPLITTER:
      splitterLoop();
      break;
    case DEATHTRAP:
      deathtrapLoop();
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
