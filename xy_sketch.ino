#include <math.h>
const int FORWARD = HIGH;
const int BACKWARD = LOW;

const int gear_reduction = 6;
const int stepsPerRotation = 25050 * gear_reduction; //1 pulse is .0144 degrees ~69 pulses per degree

int xLocation = 0;
int yLocation = 0;
int zLocation = 0;

const int xStepPin = 3;
const int yStepPin = 2;
const int zStepPin = 4;

const int xDirPin = 6;
const int yDirPin = 5;
const int zDirPin = 7;

//because instruction arrays would be wayyy to long without step multipliers
const float xStepMultiplier = .8f;
const float yStepMultiplier = .8f;
const float zStepMultiplier = 1.0f;

//directional: 2468, rotational: af
const int xbackward = 52;//4
const int xforward = 54;//6
const int ybackward = 50;//2
const int yforward = 56;//8
const int zbackward = 97;//a
const int zforward = 102;//f


int xDirection = FORWARD;
int yDirection = FORWARD;
int zDirection = FORWARD;

const int xyzEnablePin = 1;

// Other constants
//the faster a stepper motor moves, the less torque it can output
const int xfreq = 500;
const int yfreq = 500;
const int zfreq = 10000;


void setup() {
  pinMode(xStepPin, OUTPUT);
  pinMode(yStepPin, OUTPUT);
  pinMode(zStepPin, OUTPUT);
  pinMode(xDirPin, OUTPUT);
  pinMode(yDirPin, OUTPUT);
  pinMode(zDirPin, OUTPUT);
  pinMode(xyzEnablePin, OUTPUT);
  digitalWrite(xStepPin, LOW);
  digitalWrite(yStepPin, LOW);
  digitalWrite(zStepPin, LOW);
  digitalWrite(xyzEnablePin, LOW);

  Serial.begin(9600);     // opens serial port 9600 bits per second
}

//1.00f represents one quarter inch for now.
void goTo(int x, int y, int z) {
  int xDistance = x - xLocation;
  int yDistance = y - yLocation;
  int zDistance = z - zLocation;

  int xDirectionTemp = (xDistance > 0 ? BACKWARD : FORWARD);
  int yDirectionTemp = (yDistance > 0 ? BACKWARD : FORWARD);
  int zDirectionTemp = (zDistance > 0 ? BACKWARD : FORWARD);

  if(xDirectionTemp != xDirection){
    Serial.println("a");
  }

  if(yDirectionTemp != yDirection){
    Serial.println("b");
  }

  if(zDirectionTemp != zDirection){
    Serial.println("c");
  }

  xDirection = xDirectionTemp;
  yDirection = yDirectionTemp;
  zDirection = zDirectionTemp;

  digitalWrite(xDirPin, xDirection);
  digitalWrite(yDirPin, yDirection);
  digitalWrite(zDirPin, zDirection);

  int xSteps = (int)ceil((float)abs(xDistance) * xStepMultiplier);
  int ySteps = (int)ceil((float)abs(yDistance) * yStepMultiplier);
  int zSteps = (int)ceil((float)abs(zDistance) * zStepMultiplier);

  //Serial.println("Going to: "+String(x)+","+String(y)+","+String(z));
  move(xSteps, ySteps, zSteps);

  xLocation = x;
  yLocation = y;
  zLocation = z;
}

//a move function may be achieved by interleaving movements in the two (or three) requested dimensions by dividing the requested movements into X number of steps
//then moving step 0 for dimension A,B, and C
//then moving step 1 for dimension A,B, and C
//until moved step X for dimension A,B, and C
//100 steps = .25 inches, 1 step = .0025 inches
void move(int xSteps, int ySteps, int zSteps) {
  //Serial.println("Moving x,y,z steps: "+String(xSteps)+","+String(ySteps)+","+String(zSteps));
  int totalSteps = xSteps + ySteps + zSteps;
  //300 steps is about 1/2" of movement in the x or y direction
  byte instructionArray[totalSteps]; //initialized to zeros so 0 will mean 'x', 1 will mean 'y', and 2 will mean 'z'
  memset(instructionArray, 0, sizeof(instructionArray));
  distribute(instructionArray, totalSteps, xSteps + ySteps, ySteps, 1); //places ySteps 1's into the first x+y slots of the array
  distribute(instructionArray, totalSteps, totalSteps, zSteps, 2);//places zSteps 2's into the array
  printArray(instructionArray, totalSteps);

  for (int i = 0; i < totalSteps; i++) {
    switch (instructionArray[i]) {
      case 0:
        step_pulse(xStepPin, 2, xfreq);
        break;
      case 1:
        step_pulse(yStepPin, 2, yfreq);
        break;
      case 2:
        step_pulse(zStepPin, 6, zfreq);
        break;
      default:
        break;
    }
  }
}

static void distribute(byte base[], int arrayLength, int placementMaxSpaces, int count, int value) {
    int insertCount = 0;
    int spacing = 0;
    int zeroCount;
   
    while(insertCount < count) {
      zeroCount = 0;
      for(int i = 0; i<arrayLength; i++) {
        if(base[i] == 0) {
          zeroCount++;
        }
      }
      
      int zeroLocations[zeroCount];
      memset(zeroLocations, 0, sizeof(zeroLocations));
      
      int zeroLocationsIndex = 0;
      for(int i = 0; i<arrayLength; i++) {
        if(base[i] == 0) {
          zeroLocations[zeroLocationsIndex++] = i;
        }
      }
      
        spacing = (int) ceil((float)zeroCount / (float)(count - insertCount));
        for(int i = 0; i < zeroCount; i+=spacing) {
          base[zeroLocations[i]] = (byte) value;
          insertCount++;
        }
    }
}


//inserts value at given index in the base array
void insert(byte* base, int arrayLength, int index, int value) {
  rightshift(base, arrayLength, index);
  *(base + index) = value;
  //Serial.println("Inserted "+String(value)+" at index "+String(index));
}

void rightshift(byte* base, int arrayLength, int index) {
  for (int i = arrayLength - 1; i >= index; i--) {
    *(base + i) = *(base + i - 1);
  }
  *(base + index) = 0;
}

void fill(byte* base, int arrayLength) {
  for (int i = 0; i < arrayLength; i++) {
    *(base + i) = i + 1;
  }
  Serial.println();
}

void printArray(byte* base, int arrayLength) {
  //Serial.print("Move Instructions: ");
  for (int i = 0; i < arrayLength; i++) {
    Serial.print(String(*(base + i)));
  }
  Serial.println();
}

int freq = 0;
void loop() {
  if (Serial.available() > 0) {
    byte inByte = Serial.read();
    switch (inByte) {
      case xforward:
        goTo(xLocation + 100, yLocation, zLocation);
        break;
      case xbackward:
        goTo(xLocation - 100, yLocation, zLocation);
        break;
      case yforward:
        goTo(xLocation, yLocation + 100, zLocation);
        break;
      case ybackward:
        goTo(xLocation, yLocation - 100, zLocation);
        break;
      case zforward:
        goTo(xLocation, yLocation, zLocation + 100);
        break;
      case zbackward:
        goTo(xLocation, yLocation, zLocation - 100);
        break;
      case 122://z - Zero out the arduino's idea of location.
        xLocation = 0;
        yLocation = 0;
        zLocation = 0;
        break;

      case 116://t for test
        xLocation = 0;
        yLocation = 0;
        zLocation = 0;

        goTo(30  ,0,0   );
        goTo(50  ,0,-100);
        goTo(67  ,0,-200);
        goTo(115 ,0,-300);
        goTo(200 ,0,-367);
        goTo(300 ,0,-425);
        goTo(400 ,0,-450);
        goTo(500 ,0,-450);
        goTo(600 ,0,-425);
        goTo(700 ,0,-400);
        goTo(800 ,0,-350);
        goTo(900 ,0,-305);
        goTo(1000,0,-250);
        goTo(1050,0,-175);
        goTo(1120,0,-100);
        goTo(1167,0,0   );
        goTo(1185,0,100 );
        goTo(1167,0,200 );
        goTo(1133,0,250 );
        goTo(1060,0,300 );
        goTo(1000,0,350 );
        goTo(900 ,0,400 );
        goTo(800 ,0,425 );
        goTo(700 ,0,430 );
        goTo(600 ,0,430 );
        goTo(500 ,0,425 );
        goTo(400 ,0,400 );
        goTo(300 ,0,367 );
        goTo(200 ,0,300 );
        goTo(100 ,0,200 );
        goTo(50  ,0,100 );
        goTo(30  ,0,0   );
        break;
      default:
        break;
    }
    //Serial.print("byte received: ");
    //Serial.println(inByte, DEC);  
  }
}

void delayMilliseconds(int mills) {
  delay(mills);
}

//send steps to a particular axis
void step_pulse(int stepPin, int pulseCount, int freq) {
  int wait = 1000000 / freq;
  for ( int i = 0; i < pulseCount; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(5);//millionths of a second
    digitalWrite(stepPin, LOW);
    delayMicroseconds(5);//millionths of a second
    delayMicroseconds(wait);
  }
}


