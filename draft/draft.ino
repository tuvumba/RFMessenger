#include <Arduino.h>
#include <U8g2lib.h> /* Projekt â†’ Pridat knihovnu â†’ spravovat knihovny â†’ vyhledat u8g2 */

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

class JoystickShieldHandler
{
  
  # define JOY_X A0
  # define JOY_Y A1
  # define JOY_BT 8
  # define UP 2
  # define RIGHT 3
  # define DOWN 4
  # define LEFT 5
  # define JOY_DEADZONE 5


public: 

  JoystickShieldHandler()
  {
    pinMode(JOY_X, INPUT);
    pinMode(JOY_Y, INPUT);
    pinMode(JOY_BT, INPUT);
    pinMode(UP, INPUT);
    pinMode(DOWN, INPUT);
    pinMode(LEFT, INPUT);
    pinMode(RIGHT, INPUT);
    update();
  }

  void update()
  {
    joyX = abs(map(analogRead(JOY_X), 0, 1023, -100, 100)) > JOY_DEADZONE ? map(analogRead(JOY_X), 0, 1023, -100, 100) : 0;
    joyY = abs(map(analogRead(JOY_Y), 0, 1023, -100, 100)) > JOY_DEADZONE ? map(analogRead(JOY_Y), 0, 1023, -100, 100) : 0;
    joyButton = digitalRead(JOY_BT) == 0;


    changed = false;
    JoyState newJoyState = updateJoyState();
    if(newJoyState != joyState)
    {
      changed = true;
      joyState = newJoyState;
    }


    ButtonState newBTNState = updateButtonState();
    if(newBTNState != btnState)
    {
      changed = true;
      btnState = newBTNState;
    }    
  }

  void writeToSerial()
  {
    if(Serial.available())
    {
        Serial.print("JoyState: ");
        Serial.print(joyState);
        Serial.print(" BtnState: ");
        Serial.println(btnState);
    }
  }

  String debugStringJoy()
  {
    String out;
    out += "JoyS: ";
    switch(joyState)
    {
      case(JOY_PRESS): 
        out += "PRESS";
        break;
      case(JOY_R):
        out += "RIGHT";
        break;
      case(JOY_L):
        out += "LEFT";
        break;
      case(JOY_U):
        out += "UP";
        break;
      case(JOY_D):
        out += "DOWN";
        break;
      case(CENTER):
        out += "CENTER";
        break;
    }

    return out;
  }

  String debugStringBTN()
  {
    String out = "BtnS: ";
    switch(btnState)
    {
      case(UP_PRESS):
        out += "UP";
        break;
      case(DOWN_PRESS):
        out += "DOWN";
        break;
      case(LEFT_PRESS):
        out += "LEFT";
        break;
      case(RIGHT_PRESS):
        out += "RIGHT";
        break;
        case(NONE):
        out += "NONE";
        break;
    }
    return out;
  }

  

  
  enum JoyState
  {
    JOY_PRESS, JOY_R, JOY_L, JOY_U, JOY_D, CENTER
  } joyState;

  enum ButtonState 
  {
    UP_PRESS, RIGHT_PRESS, LEFT_PRESS, DOWN_PRESS, NONE // only single button presses are supported right now
  } btnState;

  short joyX; // from 0 to 1023 -> will get mapped to -100 to 100
  short joyY;
  bool joyButton;
  bool changed;


  JoyState updateJoyState()
  {
    if(joyButton) return JOY_PRESS;


    if(joyX == 0 && joyY == 0)
      return CENTER;

    if(abs(joyX) > abs(joyY)) // will be ignoring joyY input
    {
       return joyX > 0 ? JOY_R : JOY_L;
    }
    else // ignoring joyX input
    {
      return joyY > 0 ? JOY_U : JOY_D;
    }
  }

  ButtonState updateButtonState()
  {
      bool up = digitalRead(UP) == 0;
      bool down = digitalRead(DOWN) == 0;
      bool left = digitalRead(LEFT) == 0;
      bool right = digitalRead(RIGHT) == 0;

      if(up + down + left + right != 1)
        return NONE;
      else
      {
        if(up)
          return UP_PRESS;
        
        if(down)
          return DOWN_PRESS;

        if(left)
          return LEFT_PRESS;

        if(right)
          return RIGHT_PRESS;
      }


  }

  
};



class Menu
{

};

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8;
JoystickShieldHandler handler;

void setup(void) {
  u8x8.begin();
  
  Serial.begin(9600);
}




void loop(void) {
  

  handler.update();
  if(handler.changed)
  {

    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.clearLine(2);
    u8x8.clearLine(6);
    u8x8.drawString(0, 2, handler.debugStringJoy().c_str());  //16 columns 8 rows
    u8x8.drawString(0, 6, handler.debugStringBTN().c_str());
  }
  
  

  
  





 


}