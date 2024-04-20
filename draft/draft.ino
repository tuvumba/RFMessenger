#include <Arduino.h>
#include <U8g2lib.h>
#include <Array.h>


#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif





enum JoyState {
  JOY_PRESS,
  JOY_R,
  JOY_L,
  JOY_U,
  JOY_D,
  CENTER
};

enum ButtonState {
  UP_PRESS,
  RIGHT_PRESS,
  LEFT_PRESS,
  DOWN_PRESS,
  NONE  // only single button presses are supported right now
};



struct State {

  JoyState joyState;
  ButtonState btnState;

  State(JoyState joy, ButtonState btn) {
    joyState = joy;
    btnState = btn;
  }

  State() {
    joyState = CENTER;
    btnState = NONE;
  }

  String joyStateToString() {
    String out;
    switch (joyState) {
      case (JOY_PRESS):
        out += "PRESS";
        break;
      case (JOY_R):
        out += "RIGHT";
        break;
      case (JOY_L):
        out += "LEFT";
        break;
      case (JOY_U):
        out += "UP";
        break;
      case (JOY_D):
        out += "DOWN";
        break;
      case (CENTER):
        out += "CENTER";
        break;
    }
    return out;
  }

  String btnStateToString() {
    String out;
    switch (btnState) {
      case (UP_PRESS):
        out += "UP";
        break;
      case (DOWN_PRESS):
        out += "DOWN";
        break;
      case (LEFT_PRESS):
        out += "LEFT";
        break;
      case (RIGHT_PRESS):
        out += "RIGHT";
        break;
      case (NONE):
        out += "NONE";
        break;
    }
    return out;
  }
};


class JoystickShieldHandler {

#define JOY_X A0
#define JOY_Y A1
#define JOY_BT 8
#define UP 2
#define RIGHT 3
#define DOWN 4
#define LEFT 5
#define JOY_DEADZONE 10
#define STATES_SIZE 10


public:

  JoyState joystate;
  ButtonState btnstate;
  State state;


  JoystickShieldHandler() {
    pinMode(JOY_X, INPUT);
    pinMode(JOY_Y, INPUT);
    pinMode(JOY_BT, INPUT);
    pinMode(UP, INPUT);
    pinMode(DOWN, INPUT);
    pinMode(LEFT, INPUT);
    pinMode(RIGHT, INPUT);
    state = State();
    states.clear();
  }

  void update() {
    joyX = abs(map(analogRead(JOY_X), 0, 1023, -100, 100)) > JOY_DEADZONE ? map(analogRead(JOY_X), 0, 1023, -100, 100) : 0;
    joyY = abs(map(analogRead(JOY_Y), 0, 1023, -100, 100)) > JOY_DEADZONE ? map(analogRead(JOY_Y), 0, 1023, -100, 100) : 0;
    joyButton = digitalRead(JOY_BT) == 0;


    changed = false;
    JoyState newJoyState = updateJoyState();
    if (newJoyState != state.joyState) {

      state.joyState = newJoyState;
      if (newJoyState != CENTER) {
        changed = true;
        states.push_back(State(newJoyState, NONE));
      }
    }


    ButtonState newBTNState = updateButtonState();
    if (newBTNState != state.btnState) {

      state.btnState = newBTNState;
      if (newBTNState != NONE) {
        changed = true;
        states.push_back(State(CENTER, newBTNState));
      }
    }
  }



  void writeToSerial() {
    Serial.print(state.joyStateToString());
    Serial.print(" ");
    Serial.print(state.btnStateToString());
    if (changed)
      Serial.println(" Changed");
    else
      Serial.println(" Same");

    Serial.println("States:");
    for (State s : states) {
      Serial.print(s.joyStateToString());
      Serial.print(" ");
      Serial.println(s.btnStateToString());
    }
    Serial.println("----");

    if (states.size() == STATES_SIZE)
      states.clear();
  }





  short joyX;  // from 0 to 1023 -> will get mapped to -100 to 100
  short joyY;
  bool joyButton;
  bool changed;
  Array<State, STATES_SIZE> states;


  JoyState updateJoyState() {
    if (joyButton) return JOY_PRESS;


    if (joyX == 0 && joyY == 0)
      return CENTER;

    if (abs(joyX) > abs(joyY))  // will be ignoring joyY input
    {
      return joyX > 0 ? JOY_R : JOY_L;
    } else  // ignoring joyX input
    {
      return joyY > 0 ? JOY_U : JOY_D;
    }
  }

  ButtonState updateButtonState() {
    bool up = digitalRead(UP) == 0;
    bool down = digitalRead(DOWN) == 0;
    bool left = digitalRead(LEFT) == 0;
    bool right = digitalRead(RIGHT) == 0;

    if (up + down + left + right != 1)
      return NONE;
    else {
      if (up)
        return UP_PRESS;

      if (down)
        return DOWN_PRESS;

      if (left)
        return LEFT_PRESS;

      if (right)
        return RIGHT_PRESS;
    }
  }
};

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8;
JoystickShieldHandler handler;


class Menu

// 16 by 8 display
// open_iconic_all_2x

{
  /*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
           _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
          |                               | 0
          |                               | 1
          |                               | 2
          |                               | 3 
          |                               | 4 
          |                               | 5
          |                               | 6
          |                               | 7
           - - - - - - - - - - - - - - - - 
  */

public:


  enum PAGE {
    INTRO,
    SELECTION,
    DIALOGUE,
    DEBUG,
    CHAT_HISTORY,
    INPUT
  } page,
    selectionPage;

  bool changed = true;


  Menu() {
    page = SELECTION;
    selectionPage = DIALOGUE;
  }

  void drawIntro(String tmp = "tuvumba :3", int row = 3, int column = 3) {
    clear();
    String drawn;
    for (int i = 0; i < tmp.length(); i++) {
      u8x8.drawString(column, row, drawn.c_str());
      for (int j = 15; j >= column + i; j--) {
        u8x8.drawGlyph(j, row, tmp[i]);
        if (j != 15) {
          u8x8.drawGlyph(j + 1, row, '\x20');
        }
        delay(40);
      }
      drawn += tmp[i];
    }
    delay(100);
  }

  void drawSelection(int row = 2, int column = 7, int spacing = 2) {  // DIALOGUE, DEBUG, CHAT HISTORY

    if (changed) {
      clear();
      switch (selectionPage) {
        case DIALOGUE:
          u8x8.setFont(u8x8_font_open_iconic_embedded_2x2);
          u8x8.drawString(column, row, "\x45");
          u8x8.setFont(u8x8_font_open_iconic_arrow_1x1);
          u8x8.drawString(column + spacing + 1, row + 1, "\x52");

          u8x8.setFont(u8x8_font_chroma48medium8_r);
          u8x8.drawString(column - 1, 6, "CHAT");
          break;
        case DEBUG:

          u8x8.setFont(u8x8_font_open_iconic_embedded_2x2);
          u8x8.drawString(column, row, "\x48");
          u8x8.setFont(u8x8_font_open_iconic_arrow_1x1);
          u8x8.drawString(column + spacing + 1, row + 1, "\x52");
          u8x8.setFont(u8x8_font_open_iconic_arrow_1x1);
          u8x8.drawString(column - spacing, row + 1, "\x51");

          u8x8.setFont(u8x8_font_chroma48medium8_r);
          u8x8.drawString(column - 2, 6, "DEBUG");
          break;
        case CHAT_HISTORY:
          u8x8.setFont(u8x8_font_open_iconic_embedded_2x2);
          u8x8.drawString(column, row, "\x4C");
          u8x8.setFont(u8x8_font_open_iconic_arrow_1x1);
          u8x8.drawString(column - spacing, row + 1, "\x51");

          u8x8.setFont(u8x8_font_chroma48medium8_r);
          u8x8.drawString(column - 2, 6, "HISTORY");
          break;
      }
    }

    changed = false;
  }

  void drawDialogue() {
    if (changed) {
      clear();
      u8x8.drawString(0, 3, "DIALOGUE");
    }
    changed = false;
  }

  void drawDebug() {
    if (changed) {
      clear();
      u8x8.drawString(0, 3, "DEBUG");
    }

    changed = false;
  }

  void drawHistory() {
    if (changed) {
      clear();
      u8x8.drawString(0, 3, "HISTORY");
    }
    changed = false;
  }


  struct KeyboardState {
    KeyboardState() {
      extra = false;
      rowCar = 0;
      colCar = 0;
    }
    bool extra;
    int rowCar;
    int colCar;
  };

  KeyboardState current;
  bool keyboardDrawn = false;


  KeyboardState nextKeyboardState(KeyboardState old, JoyState joystate) {
    KeyboardState newState;
    switch (joystate) {
      case JoyState::JOY_D:
        if (old.rowCar == 2) {
          if (old.extra)
            return old;
          else {
            newState.extra = true;
            newState.rowCar = 0;
            newState.colCar = old.colCar;
            changed = true;
            return newState;
          }
        } else {
          newState.extra = old.extra;
          newState.rowCar = old.rowCar + 1;
          newState.colCar = old.colCar;
          changed = true;
          return newState;
        }
        break;
      case JoyState::JOY_U:
        if (old.rowCar == 0) {
          if (!old.extra)
            return old;
          else {
            newState.extra = false;
            newState.rowCar = 2;
            newState.colCar = old.colCar;
            changed = true;
            return newState;
          }
        } else {
          newState.extra = old.extra;
          newState.rowCar = old.rowCar - 1;
          newState.colCar = old.colCar;
          changed = true;
          return newState;
        }
        break;
      case JoyState::JOY_L:
        if (old.colCar == 0)
          return old;
        else {
          newState.extra = old.extra;
          newState.colCar = old.colCar - 1;
          newState.rowCar = old.rowCar;
          changed = true;
          return newState;
        }
        break;
      case JoyState::JOY_R:
        if (old.colCar == 10)
          return old;
        else {
          newState.extra = old.extra;
          newState.colCar = old.colCar + 1;
          newState.rowCar = old.rowCar;
          changed = true;
          return newState;
        }
        break;
    }
    return old;
  }


  void drawKeyboardMain(bool extra = false) {
    clear();
    const char letters[3][10] = { { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p' }, { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '!' }, { 'z', 'x', 'c', 'v', 'b', 'n', 'm', '.', ',', '?' } };
    const char extras[3][10] = { { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' }, { '^', ':', ';', '<', '>', '/', '=', '+', '[', ']' }, { '%', '$', '-', '(', ')', '"', '~', '#', '*', '&' } };
    short xpos = 0;
    short ypos = 2;

    u8x8.drawString(11, ypos, "SEND");
    u8x8.drawString(11, ypos + 2, "DEL");
    u8x8.drawString(11, ypos + 4, "SPACE");

    u8x8.setFont(u8x8_font_open_iconic_arrow_1x1);
    if (extra) {
      u8x8.drawString(5, 1, "\x53");
    } else {
      u8x8.drawString(5, 7, "\x50");
    }
    u8x8.setFont(u8x8_font_chroma48medium8_r);

    for (int row = 0; row < 3; row++) {
      xpos = 0;
      for (int column = 0; column < 10; column++) {

        u8x8.drawGlyph(xpos, ypos, (extra ? extras[row][column] : letters[row][column]));
        xpos++;
      }
      ypos += 2;
    }
    keyboardDrawn = true;
  }

  void drawKeyboardState(KeyboardState state, KeyboardState oldState) {


    u8x8.setFont(u8x8_font_chroma48medium8_r);

    u8x8.drawGlyph(oldState.colCar, 3 + oldState.rowCar * 2, '\x20');
    if (oldState.colCar == 10) {
      u8x8.drawString(oldState.colCar + 1,  3 + oldState.rowCar * 2, "\x20\x20\x20\x20\x20");
    }

  


    u8x8.setFont(u8x8_font_open_iconic_arrow_1x1);
    if (state.colCar == 10) {
      u8x8.drawString(state.colCar + 1, 3 + state.rowCar * 2, "\x4F");
      u8x8.drawString(state.colCar + 2, 3 + state.rowCar * 2, "\x4F");
      u8x8.drawString(state.colCar + 3, 3 + state.rowCar * 2, "\x4F");
      switch (state.rowCar) {
        case 0:
          u8x8.drawString(state.colCar + 4, 3 + state.rowCar * 2, "\x4F");
          break;
        case 2:
          u8x8.drawString(state.colCar + 4, 3 + state.rowCar * 2, "\x4F");
          u8x8.drawString(state.colCar + 5, 3 + state.rowCar * 2, "\x4F");
          break;
      }
    } else {
      u8x8.drawString(state.colCar, 3 + state.rowCar * 2, "\x4F");
    }

    u8x8.setFont(u8x8_font_chroma48medium8_r);
  }

  void clear() {
    u8x8.clearDisplay();
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setCursor(0, 0);
  }

  PAGE nextSelection() {
    switch (selectionPage) {
      case DIALOGUE:
        changed = true;
        return DEBUG;
      case DEBUG:
        changed = true;
        return CHAT_HISTORY;
      case CHAT_HISTORY:
        changed = false;
        return CHAT_HISTORY;
    }
  }

  PAGE previousSelection() {
    switch (selectionPage) {
      case DIALOGUE:
        changed = false;
        return DIALOGUE;
      case DEBUG:
        changed = true;
        return DIALOGUE;
      case CHAT_HISTORY:
        changed = true;
        return DEBUG;
    }
  }


  void nextScreen() {

    State action = State(JoyState::CENTER, ButtonState::NONE);
    if (!handler.states.empty())
      action = handler.states.at(0);



    switch (page) {
      case SELECTION:
        drawSelection();
        switch (action.btnState) {
          case DOWN_PRESS:
            page = selectionPage;
            changed = true;
            break;
          default:
            break;
        }

        switch (action.joyState) {
          case JoyState::JOY_R:
            selectionPage = nextSelection();
            break;
          case JoyState::JOY_L:
            selectionPage = previousSelection();
            break;
        }
        break;
      case DIALOGUE:
        // process dialogue
        drawDialogue();

        switch (action.btnState) {
          case RIGHT_PRESS:
            page = SELECTION;
            selectionPage = DIALOGUE;
            changed = true;
            break;

          case DOWN_PRESS:
            page = INPUT;
            changed = true;
          default:
            break;
        }

        break;
      case DEBUG:
        // process Debug
        drawDebug();
        switch (action.btnState) {
          case RIGHT_PRESS:
            page = SELECTION;
            selectionPage = DEBUG;
            changed = true;
            break;
          default:
            break;
        }

        break;
      case CHAT_HISTORY:
        // process history
        drawHistory();
        switch (action.btnState) {
          case RIGHT_PRESS:
            page = SELECTION;
            selectionPage = CHAT_HISTORY;
            changed = true;
            break;
          default:
            break;
        }
        break;
      case INPUT:
        if (!keyboardDrawn)
          drawKeyboardMain(current.extra);

        KeyboardState newState = nextKeyboardState(current, action.joyState);
      
        if (changed) {
          if(current.extra != newState.extra)
            drawKeyboardMain(newState.extra);
          drawKeyboardState(newState, current);
          current = newState;
          changed = false;
        } else if (action.btnState == RIGHT_PRESS) {
          page = DIALOGUE;
          keyboardDrawn = false;
          changed = true;
          current.colCar = 0;
          current.extra = false;
          current.rowCar = 0;
        }

        break;
    }


    handler.states.remove(0);
  }
};

Menu menu;


void setup(void) {
  u8x8.begin();

  Serial.begin(9600);
  menu.drawIntro();
}


void loop(void) {

  handler.update();
  menu.nextScreen();

  // handler.update();
  // menu.nextScreen();

  /*
  handler.writeToSerial();


  if (handler.changed) {
    u8x8.clearDisplay();
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.drawString(0, 3, handler.state.joyStateToString().c_str());  //16 columns 8 rows
    u8x8.drawString(0, 6, handler.state.btnStateToString().c_str());
    u8x8.setFont(u8x8_font_open_iconic_embedded_2x2);
    u8x8.drawString(0, 0, "\x41\x42\x43\x44\x45\x46\x47");
  }

  */

  delay(50);
}