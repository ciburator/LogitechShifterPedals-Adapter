#include <Joystick.h>

// PINOUT
// A0 - A1 -> Shifter position x and y axis
// 2       -> reverse switch button

// Shifter modes
// may7be latter should be controlled by physical button
// -1 -> DEBUG mode, prints all inputs, doesnt change device type
// 0 -> H-pattern
// 1 -> Sequential
// 2 -> Handbrake
#define MODE 0

// H-patern, actual shifter position in max values
// 1-8 buttons will be used, shifter pressdown will be as separate button press
// usable x values 230 -> 700   resting 400-500
// usable y values 150 -> 900   resting 400-500
// this could be more fine tuned
#define HP_LEFT 400
#define HP_RIGHT 600
#define HP_UP 800
#define HP_DOWN 300

// Seq-pattern, register thresholds
// only shifts will be registered
// preliminar tresholds
#define SP_UP_BEGIN 670
#define SP_UP_END 600
#define SP_DOWN_BEGIN 430
#define SP_DOWN_END 500

// Handbrake register limits
// max depends on direction right now its handbrake to the front
// to the back would be 300 (also hb range has to be changed)
#define HB_MIN 400
#define HB_MAX 530
#define HB_RANGE (HB_MAX - HB_MIN)

const int Pin_Shifter_XAxis = A8;
const int Pin_Shifter_YAxis = A9;
const int Pin_Shifter_Switch = 7;

const int Pin_Pedals_Gas = A3;
const int Pin_Pedals_Brake = A2;
const int Pin_Pedals_Clutch = A1;

const int RXLED = 17; // The RX LED has a defined Arduino pin
const int TXLED = 30; // The TX LED has a defined Arduino pin


int s_button = 0;

const int ADC_MIN = 0;
const int ADC_Max = 1023;  // max value of the analog inputs, 10-bit on AVR boards

Joystick_ Joystick(
  JOYSTICK_DEFAULT_REPORT_ID,                 // default report (no additional pages)
  JOYSTICK_TYPE_JOYSTICK,                     // so that this shows up in Windows joystick manager
  8,                                          // number of buttons (none)
  0,                                          // number of hat switches (none)
  false, false,                               // no X and Y axes
  true,                                       // include Z axis for the clutch pedal
  true,                                       // include Rx axis for the brake pedal
  true,                                       // include Ry axis for the gas pedal
  false, false, false, false, false, false);  // no other axes

void setup() {
  pinMode(RXLED, OUTPUT); // Set RX LED as an output
  pinMode(Pin_Shifter_XAxis, INPUT_PULLUP);
  pinMode(Pin_Shifter_YAxis, INPUT_PULLUP);
  pinMode(Pin_Shifter_Switch, INPUT);

  pinMode(Pin_Pedals_Gas, INPUT_PULLUP);
  pinMode(Pin_Pedals_Clutch, INPUT_PULLUP);
  pinMode(Pin_Pedals_Brake, INPUT_PULLUP);

  if (MODE > -1) {
    //Joystick.setAcceleratorRange(0, 260);
    //Joystick.setBrakeRange(0, 260);

    Joystick.begin(false);

    Joystick.setZAxisRange(ADC_MIN, ADC_Max);
    Joystick.setRxAxisRange(ADC_MIN, ADC_Max);
    Joystick.setRyAxisRange(ADC_MIN, ADC_Max);
  }
}

void loop() {
  int s_x = analogRead(Pin_Shifter_XAxis);
  int s_y = analogRead(Pin_Shifter_YAxis);
  int s_switch = digitalRead(Pin_Shifter_Switch);

  int p_clutch = analogRead(Pin_Pedals_Clutch);
  int p_gas = analogRead(Pin_Pedals_Gas);
  int p_brake = analogRead(Pin_Pedals_Brake);

  bool shifterEnabled = checkShifterConnection(s_x, s_y);
  bool pedalsEnabled = checkPedalsConnection(p_clutch, p_gas, p_brake);

  if(shifterEnabled) {
    TXLED1;
  } else {
    TXLED0;
  }

  if (MODE == -1) debug(s_x, s_y, s_switch, p_clutch, p_gas, p_brake, shifterEnabled, pedalsEnabled);
  else {
    if (shifterEnabled) {
      if (MODE == 0) HPShifter(s_x, s_y, s_switch);
      else if (MODE == 1) SPShifter(s_x, s_y);
      else if (MODE == 2) Handbrake(s_x, s_y);
      else {
        // there should be somewhat notification if configuration is wrong
      }
    }
  }

  if (pedalsEnabled) {
    digitalWrite(RXLED, LOW);
    updatePedals(p_clutch, p_gas, p_brake);
  } else {
    digitalWrite(RXLED, HIGH);
  }

  Joystick.sendState();
}

void updatePedals(int p_clutch, int p_gas, int p_brake) {
  Joystick.setRyAxis(p_gas);
  Joystick.setRxAxis(p_brake);
  Joystick.setZAxis(p_clutch);
}

void debug(int s_x, int s_y, int s_switch, int p_clutch, int p_gas, int p_brake, bool shifterEnabled, bool pedalsEnabled) {
  if (shifterEnabled == false) {
    s_x = 0;
    s_y = 0;
  }

  if (pedalsEnabled == false) {
    p_clutch = 0;
    p_gas = 0;
    p_brake = 0;
  }

  Serial.print("s_x:");
  Serial.print(s_x);
  Serial.print(",");
  Serial.print("s_y:");
  Serial.print(s_y);
  Serial.print(",");
  Serial.print("s_switch:");
  Serial.print(s_switch);
  Serial.print(",");
  Serial.print("p_clutch:");
  Serial.print(p_clutch);
  Serial.print(",");
  Serial.print("p_gas:");
  Serial.print(p_gas);
  Serial.print(",");
  Serial.print("p_brake:");
  Serial.println(p_brake);
}

bool checkShifterConnection(int s_x, int s_y) {
  bool result = false;

  int highPoint = 1000;
  int lowPoint = 5;

  if (s_x < highPoint
      && s_x > lowPoint
      && s_y < highPoint
      && s_y > -lowPoint) {
    result = true;
  }

  return result;
}

bool checkPedalsConnection(int p_clutch, int p_gas, int p_brake) {
  bool result = false;

  int highPoint = 1000;
  int lowPoint = 100;

  if (p_clutch < highPoint
      && p_clutch > lowPoint
      && p_gas < highPoint
      && p_gas > lowPoint
      && p_brake < highPoint
      && p_brake > lowPoint) {
    result = true;
  }

  return result;
}

void HPShifter(int s_x, int s_y, int s_switch) {
  int _button = 0;

  if (s_switch == 1) {
    _button = 7;  // reverse switch
    if (s_y < HP_DOWN && s_x > HP_RIGHT) {
      _button = 8;  // reverse
    }
  } else {
    if (s_x < HP_LEFT)  // Shifter on the left?
    {
      if (s_y > HP_UP) _button = 1;    // 1st gear
      if (s_y < HP_DOWN) _button = 2;  // 2nd gear
    } else if (s_x > HP_RIGHT)         // Shifter on the right?
    {
      if (s_y > HP_UP) _button = 5;    // 5th gear
      if (s_y < HP_DOWN) _button = 6;  // 6th gear

    } else  // Shifter is in the middle
    {
      if (s_y > HP_UP) _button = 3;    // 3rd gear
      if (s_y < HP_DOWN) _button = 4;  // 4th gear
    }
  }

  if (_button != s_button) {
    s_button = _button;
    ResetJoystickButton(s_button);
    Joystick.setButton(s_button - 1, HIGH);
  }
}

void SPShifter(int s_x, int s_y) {
  // not finished need to finalize logic
}

void Handbrake(int s_x, int s_y) {
  // not finished need to finalize logic
}

// with only shifter, theres only 8 used max
void ResetJoystickButton(int button) {
  for (int i = 0; i <= 8; i++) 
  { 
    if(i != button) {
      Joystick.setButton(i, LOW);
    }
  }
}
