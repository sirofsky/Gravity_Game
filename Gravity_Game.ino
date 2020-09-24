// Demo showing how to coordinate a global compass direction across a group of blinks.

// Each blink normally shows its own north face (face #0) in blue
// Pressing the button on a blink makes it the "center" and all the other
// connected blinks will sync to its "north" and show that in green.

// Each tile (starting with the center) sends out the compass angle on each face,
// so the center blink would send angle 0 on its face 0. When a tile recieves a
// compass angle from a parent, it uses it to compute which one of its own faces
// is pointing north, and then uses that to send a correct global angle to its children.
// For example, if a tile recieves an angle 0 on a face, then it knows the opsite face is
// pointing north (angle 0).

const byte NO_PARENT_FACE = FACE_COUNT ;   // Signals that we do not currently have a parent

byte parent_face = NO_PARENT_FACE;

Timer lockout_timer;    // This prevents us from forming packet loops by giving changes time to die out.
// Remember that timers init to expired state

byte bottomFace;    // Which one of our faces is pointing north? Only valid when parent_face != NO_PARENT_FACE

const byte IR_IDLE_VALUE = 7;   // A value we send when we do not know which way is north

const int LOCKOUT_TIMER_MS = 250;               // This should be long enough for very large loops, but short enough to be unnoticable

//------------------------------------------------------

#define NICE_BLUE makeColorHSB(110, 255, 255) //more like cyan <-- SUCH A NICE COLOR TOO
#define PURPLE makeColorHSB(200, 255, 230)
#define FEATURE_COLOR makeColorHSB(25, 255, 240) //very red orange
#define BALL_COLOR makeColorHSB(90, 225, 255) //an emerald green
#define BG_COLOR makeColorHSB(50, 200, 255) //a temple tan ideally
#define CRUMBLE_COLOR makeColorHSB(30, 255, 100)

bool amGod = false;
byte gravitySignal[6] = {IR_IDLE_VALUE, IR_IDLE_VALUE, IR_IDLE_VALUE, IR_IDLE_VALUE, IR_IDLE_VALUE, IR_IDLE_VALUE};

enum spawnerSignals {IM_SPAWNER, NOT_SPAWNER};
byte spawnerSignal[6] = {NOT_SPAWNER, NOT_SPAWNER, NOT_SPAWNER, NOT_SPAWNER, NOT_SPAWNER, NOT_SPAWNER};

enum blinkRoles {WALL, BUCKET, SPAWNER};
byte blinkRole = WALL;

bool bLongPress;
bool bChangeRole;

bool didIRandomize = true;

byte randomWallRole;

byte goFace;
byte leftFace;
byte rightFace;
byte topLeftFace;
byte topRightFace;
byte topFace;

//wall role stuff
byte neighborCount;
byte firstFace;
byte secondFace;

//SPAWNER
bool treasurePrimed = false;
bool dropTreasure;

Timer crumbleTimer;
#define CRUMBLE_TIME 6000

void setup() {
  randomize();
}

void loop() {

  setRole();

  if (buttonDoubleClicked()) {
    amGod = !amGod;
  }

  //send data
  FOREACH_FACE(f) {
    byte sendData = (spawnerSignal[f] << 3) + (gravitySignal[f]);
    setValueSentOnFace(sendData, f);
  }
}

void wallLoop() {
  if (bChangeRole) {
    blinkRole = BUCKET;
    bChangeRole = false;
  }

  //  setColor(OFF);
  amGod = false;

  FOREACH_FACE(f) {
    spawnerSignal[f] = NOT_SPAWNER;

    if (isBucket(f)) { //do I have a neighbor and are they shouting IM_BUCKET?
      byte bucketNeighbor = (f + 2) % 6; //and what about their neighbor?
      if (isBucket(bucketNeighbor)) {
        bottomFace = (f + 1) % 6;
        //        randomWallRole = 10;
        //        imSwitcher = true;
        amGod = true; //I get to decide what direction gravity is for the entire game! Yippee!
      }
    }
  }

  setColor(dim(BG_COLOR, 190));
  setColorOnFace(dim(BG_COLOR, 140), (bottomFace + 2) % 6);
  setColorOnFace(dim(BG_COLOR, 140), (bottomFace + 4) % 6);

  countNeighbors();
  setWallRole();
  gravityLoop();

  leftFace = (bottomFace + 1) % 6;
  topLeftFace = (bottomFace + 2) % 6;
  topFace = (bottomFace + 3) % 6;
  topRightFace = (bottomFace + 4) % 6;
  rightFace = (bottomFace + 5) % 6;
}

bool isBucket (byte face) {
  if (!isValueReceivedOnFaceExpired(face)) { //I have a neighbor
    if (getGravitySignal(getLastValueReceivedOnFace(face)) == 6) { //if the neighbor is a bucket, return true
      return true;
    } else if (getGravitySignal(getLastValueReceivedOnFace(face)) != 6) { //if the neighbor isn't a bucket, return false
      return false;
    }
  } else {
    return false; //or if I don't have a neighbor, that's also a false
  }
}

void bucketLoop() {
  if (bChangeRole) {
    blinkRole = SPAWNER;
    bChangeRole = false;
  }

  setColor(ORANGE);

  FOREACH_FACE(f) {
    gravitySignal[f] = 6; //6 means bucket, I don't make the rules
    spawnerSignal[f] = NOT_SPAWNER;
  }
}

void spawnerLoop() {
  if (bChangeRole) {
    blinkRole = WALL;
    bChangeRole = false;
  }
  setValueSentOnAllFaces(7);

  FOREACH_FACE(f) {
    spawnerSignal[f] = IM_SPAWNER;
  }

  gravityLoop();

  setColor(dim(BALL_COLOR, 120));

  if (treasurePrimed == true) {
    treasurePrimedAnimation();
  } else {
    setColor(dim(BALL_COLOR, 120));
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

void gravityLoop() {

  //  setColor( OFF );

  if (amGod) {

    // I am the center blink!

    FOREACH_FACE(f) {

      gravitySignal[f] = (f + 6 - bottomFace) % 6; //eveything is being broadcast relative to the bottomFace

    }

    // This stuff is to reset everything clean for when the button is released....

    parent_face = NO_PARENT_FACE;

    lockout_timer.set(LOCKOUT_TIMER_MS);

    //setColor(dim(WHITE,128));

    //    setColorOnFace( WHITE , bottomFace );

    randomWallRole = 10; //this one should always be the switcher

    // That's all for the center blink!

  } else {

    // I am not the center blink!

    if (parent_face == NO_PARENT_FACE ) {   // If we have no parent, then look for one

      if (lockout_timer.isExpired()) {      // ...but only if we are not on lockout

        FOREACH_FACE(f) {

          if (!isValueReceivedOnFaceExpired(f)) {

            if (getGravitySignal(getLastValueReceivedOnFace(f)) < FACE_COUNT) {

              // Found a parent!

              parent_face = f;

              // Compute the oposite face of the one we are reading from

              byte our_oposite_face = (f + (FACE_COUNT / 2) ) % FACE_COUNT;

              // Grab the compass heading from the parent face we are facing
              // (this is also compass heading of our oposite face)

              byte parent_face_heading = getGravitySignal(getLastValueReceivedOnFace(f));

              // Ok, so now we know that `our_oposite_face` has a heading of `parent_face_heading`.

              // Compute which face is our north face from that

              bottomFace = ( (our_oposite_face + FACE_COUNT) - parent_face_heading) % FACE_COUNT;     // This +FACE_COUNT is to keep it positive

              // I guess we could break here, but breaks are ugly so instead we will keep looking
              // and use whatever the highest face with a parent that we find.
            }
          }
        }
      }

    } else {

      // Make sure our parent is still there and good

      if (isValueReceivedOnFaceExpired(parent_face) || ( getGravitySignal(getLastValueReceivedOnFace(parent_face )) == IR_IDLE_VALUE) ) {

        // We had a parent, but our parent is now gone

        parent_face = NO_PARENT_FACE;

        //setValueSentOnAllFaces( IR_IDLE_VALUE );  // Propigate the no-parentness to everyone resets viraly
        FOREACH_FACE(ff) {
          gravitySignal[ff] = IR_IDLE_VALUE;
        }
        lockout_timer.set(LOCKOUT_TIMER_MS);    // Wait this long before accepting a new parent to prevent a loop
      }
    }


    if (parent_face != NO_PARENT_FACE) {
      //I am connected to the tower
      didIRandomize = false;
      crumbleTimer.set(CRUMBLE_TIME);
      treasurePrimed = false;
      dropTreasure = true;

    } else {
      //I'm not connected to the tower
      //      randomizeWallRole();
      crumbleAnimation();

      treasurePrimed = true;

    }

    // Set our output values relative to our north

    FOREACH_FACE(f) {

      if (parent_face == NO_PARENT_FACE ||  f == parent_face ) {

        //setValueSentOnFace( IR_IDLE_VALUE , f );
        gravitySignal[f] = IR_IDLE_VALUE;
      } else {

        // Map directions onto our face indexes.
        // (It was surpsringly hard for my brain to wrap around this simple formula!)

        //setValueSentOnFace( ((f + FACE_COUNT) - bottomFace) % FACE_COUNT , f  );
        gravitySignal[f] = ((f + FACE_COUNT) - bottomFace) % FACE_COUNT;
      }
    }
  }
}

void crumbleAnimation() { //9% of the code! Too big!
  //  if (!crumbleTimer.isExpired()) {
  //    int timeLeft = crumbleTimer.getRemaining();
  //    byte brightness = timeLeft % 256;
  //    if (timeLeft < CRUMBLE_TIME) {
  //      //do nothing!
  //      if (timeLeft < (3.5 * (CRUMBLE_TIME / 5))) {
  //        setColorOnFace(dim(CRUMBLE_COLOR, brightness), 4);
  //        if (timeLeft < (2.5 * (CRUMBLE_TIME / 5))) {
  //          setColorOnFace(dim(CRUMBLE_COLOR, brightness), 1);
  //          if (timeLeft < (1.5 * (CRUMBLE_TIME / 5))) {
  //            setColorOnFace(dim(CRUMBLE_COLOR, brightness), 3);
  //            if (timeLeft < (1 * (CRUMBLE_TIME / 5))) {
  //              setColorOnFace(dim(CRUMBLE_COLOR, brightness), 5);
  //              if (timeLeft < (.6 * (CRUMBLE_TIME / 5))) {
  //                setColorOnFace(dim(CRUMBLE_COLOR, brightness), 0);
  //                if (timeLeft < (0.3 * (CRUMBLE_TIME / 5))) {
  //                  setColorOnFace(dim(CRUMBLE_COLOR, brightness), 2);
  //                }
  //              }
  //            }
  //          }
  //        }
  //      }
  //    }
  //  } else {
  //    randomWallRole = 7; //deathtrap!
  //  }
}

bool neighborFaces[6] = {false, false, false, false, false, false};

void countNeighbors() { //count how many neighbors are around me and keep it in an array

  neighborCount = 0;

  FOREACH_FACE(f) {
    if (getSpawnerSignal(getLastValueReceivedOnFace(f)) == NOT_SPAWNER) {
      if (!isValueReceivedOnFaceExpired(f)) {

        neighborCount++;
        neighborFaces[f] = true;
      } else {
        neighborFaces[f] = false;
      }
    }
  }
}

void setWallRole() {

  if (neighborFaces[bottomFace] == false) { //no one directly beneath me
    if (neighborFaces[leftFace] == true && neighborFaces[rightFace] == false) { //but one neighbor to the left
      goFace = leftFace;
      goSideLoop();
    }
    else if (neighborFaces[leftFace] == false && neighborFaces[rightFace] == true) { //but one neighbor to the right
      goFace = rightFace;
      goSideLoop();
    }
    else if (neighborFaces[leftFace] == true && neighborFaces[rightFace] == true) { //but two neighbors to the left and right
      switcherLoop();
    }
    else if (neighborFaces[leftFace] == false && neighborFaces[rightFace] == false) { //no one underneath me anywhere
      deathtrapLoop();
    }

  } else if (neighborFaces[bottomFace] == true) { //someone directly beneath me
    if (neighborFaces[leftFace] == false && neighborFaces[rightFace] == false) { //but no one to either side
      goFace = bottomFace;
      goSideLoop();
    }
    else if (neighborFaces[leftFace] == true && neighborFaces[rightFace] == false) { //and someone to the left
      goFace = leftFace;
      goSideLoop();
    }
    else if (neighborFaces[leftFace] == false && neighborFaces[rightFace] == true) { //and someone to the right
      goFace = rightFace;
      goSideLoop();
    }
    else if (neighborFaces[leftFace] == true && neighborFaces[rightFace] == true) { //there's three people beneath me

      if (neighborCount == 3) { //and somehow that's it
        splitterLoop();
      }
      else if (neighborCount == 4) {
        if (neighborFaces[topFace] == true) { //one blink is directly above me
          splitterLoop();
        } else { //one blink is above me to either side
          switcherLoop();
        }
      } else if (neighborCount == 5) { //two blinks are above me
        deathtrapLoop();
      } else if (neighborCount == 6) { //I'm fully surrounded
        splitterLoop();
      }
    }
  }
}

void goSideLoop() {

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      setColorOnFace(OFF, f);
    }
  }

  setColorOnFace(FEATURE_COLOR, leftFace);
  setColorOnFace(FEATURE_COLOR, bottomFace);
  setColorOnFace(FEATURE_COLOR, rightFace);
  setColorOnFace(OFF, goFace);
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

bool goLeft;

void switcherLoop() {
  if (goLeft == true) {
    setColorOnFace(PURPLE, (bottomFace + 4) % 6);
    setColorOnFace(FEATURE_COLOR, (bottomFace + 1) % 6);
  } else {
    setColorOnFace(PURPLE, (bottomFace + 2) % 6);
    setColorOnFace(FEATURE_COLOR, (bottomFace + 5) % 6);
  }
}

void setRole() {
  if (hasWoken()) {
    bLongPress = false;
  }

  if (buttonLongPressed()) {
    bLongPress = true;
  }

  if (buttonReleased()) {
    if (bLongPress) {
      //now change the role
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

byte getGravitySignal(byte data) { //all the gravity communication happens here
  return (data & 7); //returns bits D, E, and F
}

byte getSpawnerSignal(byte data) {
  return ((data >> 3) & 1); //returns bit C
}
