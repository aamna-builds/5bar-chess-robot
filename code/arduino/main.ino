#include <Servo.h>
#include <AccelStepper.h>

// ---------------- SERVO (MAGNET) ----------------
Servo myServo;
const int servoPin = 10;

// tune these for MG90S
int angleON  = 0;    // magnet ON
int angleOFF = 180;   // magnet OFF

bool magnetState = false;

// ---------------- STEPPERS ----------------
#define dirA 5
#define stepA 6
#define dirB 2
#define stepB 3

const float stepsPerDegree = 14.0 * 6400.0 / 360.0;
const float SIM_HOME_ANGLE = 90.0;

AccelStepper motorA(1, stepA, dirA);
AccelStepper motorB(1, stepB, dirB);

float lastAngleA = SIM_HOME_ANGLE;
float lastAngleB = SIM_HOME_ANGLE;

// ---------------- FUNCTIONS ----------------
float unwrapAngle(float newAngle, float lastAngle) {
  float diff = newAngle - lastAngle;

  if (diff > 180.0) newAngle -= 360.0;
  else if (diff < -180.0) newAngle += 360.0;

  return newAngle;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2);

  // Servo
  myServo.attach(servoPin);
  myServo.write(angleOFF);

  // Steppers
  motorA.setMaxSpeed(12000);
  motorA.setAcceleration(4000);
  motorA.setMinPulseWidth(20);

  motorB.setMaxSpeed(12000);
  motorB.setAcceleration(4000);
  motorB.setMinPulseWidth(20);

  motorA.setCurrentPosition(0);
  motorB.setCurrentPosition(0);
}

// ---------------- LOOP ----------------
void loop() {

  // ---- SERIAL INPUT ----
  if (Serial.available() > 0) {

    char first = Serial.peek();

    // ---- MAGNET COMMAND ----
    if (first == 'M') {
      Serial.read(); // consume

      magnetState = !magnetState;

      if (magnetState) myServo.write(angleON);
      else myServo.write(angleOFF);

      return; // stop further parsing this loop
    }

    // ---- STEPPER ANGLES ----
    float simAngleA = Serial.parseFloat();
    float simAngleB = Serial.parseFloat();

    while (Serial.available() > 0 &&
          !isdigit(Serial.peek()) &&
          Serial.peek() != '-') {
      Serial.read();
    }

    if (simAngleA != 0.0 || simAngleB != 0.0) {
      float contA = unwrapAngle(simAngleA, lastAngleA);
      float contB = unwrapAngle(simAngleB, lastAngleB);

      lastAngleA = contA;
      lastAngleB = contB;

      long targetA = (long)((contA - SIM_HOME_ANGLE) * stepsPerDegree);
      long targetB = (long)((contB - SIM_HOME_ANGLE) * stepsPerDegree);

      motorA.moveTo(targetA);
      motorB.moveTo(targetB);
    }
  }

  // ---- RUN STEPPERS ----
  motorA.run();
  motorB.run();
}