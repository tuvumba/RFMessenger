#include <Arduino.h>
#include <U8g2lib.h>
#include <Array.h>


#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// Indicates the joystick state
enum JoyState {
  JOY_PRESS,
  JOY_R,
  JOY_L,
  JOY_U,
  JOY_D,
  CENTER
};

// Indicates the button state
// Current implementation supports only single button presses
// Multiple buttons pressed at once will be ignored
enum ButtonState {
  UP_PRESS,
  RIGHT_PRESS,
  LEFT_PRESS,
  DOWN_PRESS,
  NONE  
};


// Combines the joystick and button data in one. 
// Mainly used in the JoystickShieldHandler
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

  // Used for debugging purposes
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

  // Used for debugging purposes
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
  /*
    Handles the inputs from the Arduino Joystick Shield

    public interface:
      update(void) -- updates the internal joystick and button states. 
      writeToSerial(void) -- used for debugging purposes
      
      Array<State> -- an array of STATES_SIZE entries of State.
      Only singular actions are passed to the Array.
      For example, if you push the joystick to the left while also pressing a right button, 
      the Array entries will be {"JOY_L","NONE"}, {"CENTER","RIGHT_PRESS"}.

      Future implementations and improvements might include support for input combinations.

  */

#define JOY_X A0
#define JOY_Y A1
#define JOY_BT 8
#define UP 2
#define RIGHT 3
#define DOWN 4
#define LEFT 5
#define JOY_DEADZONE 5
#define STATES_SIZE 5


public:

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
    // reads and maps values from joystick inputs.
    // joyX ranges from -100(left-most position) to 100(right-most position)
    // joyY ranges from -100(down position) to 100(up position)
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

  Array<State, STATES_SIZE> states;



private:

  JoyState joystate;
  ButtonState btnstate;
  State state;

  short joyX;  // from 0 to 1023 -> will get mapped to -100 to 100
  short joyY;
  bool joyButton;
  bool changed; // indicates whether the State has changed
  


  // returns a JoyState based on the current joystick position
  // only a single direction is returned
  JoyState updateJoyState() {
    if (joyButton) return JOY_PRESS;


    if (joyX == 0 && joyY == 0)
      return CENTER;

    // horizontal input is more significant
    if (abs(joyX) > abs(joyY)) {
      return joyX > 0 ? JOY_R : JOY_L;
    } 
    else  // vertical input is more significant
    {
      return joyY > 0 ? JOY_U : JOY_D;
    }
  }


  // reads button inputs and returs a new ButtonState
  // only singular button pressed are allowed, everything else is ignored
  ButtonState updateButtonState() {
    bool up = digitalRead(UP) == 0;
    bool down = digitalRead(DOWN) == 0;
    bool left = digitalRead(LEFT) == 0;
    bool right = digitalRead(RIGHT) == 0;

    if (up + down + left + right != 1) // multiple buttons are pressed at the moment
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
{

public:

  Menu() {
    page = SELECTION;
    selectionPage = DIALOGUE;
    chatStartMessageIndex = 0;
  }

  const short TEXT_MAX_LENGTH = 16;


  // TODO simplify and clean up
  /*
      sets the screen output to the next screen based on the inputs.
      does NOT read and update input values, that is done separately
      handles one action at a time
  */
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
              page = KEYBOARD_INPUT;
              changed = true;
            default:
              break;
          }

          switch(action.joyState)
          {
            case JoyState::JOY_D:
              if(chat.size() - chatStartMessageIndex > u8x8.getRows() - 1)
              {
                chatStartMessageIndex++;
                changed = true;
              }
              break;
            case JoyState::JOY_U:
              if(chatStartMessageIndex != 0)
              {
                chatStartMessageIndex--;
                changed = true;
              }
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
        case KEYBOARD_INPUT:
          if (!keyboardDrawn)
            drawKeyboardMain(current.extra);

          KeyboardState newState = nextKeyboardState(current, action.joyState);
        
          if (changed) {
            if(current.extra != newState.extra)
              drawKeyboardMain(newState.extra);
            drawKeyboardState(newState, current);
            current = newState;
            changed = false;
            u8x8.drawString(0,0,enteredText.c_str());
          } else if (action.btnState == RIGHT_PRESS) {
            page = DIALOGUE;
            keyboardDrawn = false;
            changed = true;
            current = KeyboardState();
            //current.colCar = 0;
            //current.extra = false;
            //current.rowCar = 0;
            enteredText = "";
          }
          else if (action.btnState == DOWN_PRESS)
          {
             char selected = symbolSelected(current);
             switch(selected)
             {
                case 0:
                  // handle SEND
                  sendText(enteredText);
                  enteredText = "";
                  u8x8.clearLine(0);
                  break; 
                case 1:
                  if(enteredText.length() != 0)
                  {
                    u8x8.drawGlyph(enteredText.length() - 1, 0, '\x20');
                    enteredText.remove(enteredText.length() - 1, 1);
                  }
                    
                  
                  break; // handle DEL
                default:
                  if(enteredText.length() < TEXT_MAX_LENGTH)
                  {
                    enteredText += selected;
                    u8x8.drawGlyph(enteredText.length() - 1, 0, selected);
                  }
                    
             }
          }

          

          break;
      }


      handler.states.remove(0);
    }

    // displays an animation of a text string appearing letter by letter
    void drawIntro(String tmp = "tuvumba :3", int row = 3, int column = 3) {
    clear();
    String drawn;
    for (int i = 0; i < tmp.length(); i++) {
      u8x8.drawString(column, row, drawn.c_str()); //draw the string that has already slided in place
      for (int j = 15; j >= column + i; j--) {
        u8x8.drawGlyph(j, row, tmp[i]); // draw the next position for the "sliding" character
        if (j != 15) {
          u8x8.drawGlyph(j + 1, row, '\x20'); // \x20 is an empty symbol, clean up the previous position of the letter without refreshing the line
        }
        delay(40);
      }
      drawn += tmp[i];
    }
    delay(100);
  }



private:

  enum PAGE {
    INTRO,
    SELECTION,
    DIALOGUE,
    DEBUG,
    CHAT_HISTORY,
    KEYBOARD_INPUT
  } page,
    selectionPage;

  struct KeyboardState {
    KeyboardState() {
      extra = false;
      rowCar = 1;
      colCar = 5;
    }
    bool extra; // is the carriage at extra symbols?
    int rowCar; // carriage row
    int colCar; // carriage column
  } current;

  const char KeyboardLetters[3][10] = { { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p' }, { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '!' }, { 'z', 'x', 'c', 'v', 'b', 'n', 'm', '.', ',', '?' } };
  const char KeyboardExtras[3][10] = { { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' }, { '^', ':', ';', '<', '>', '/', '=', '+', '[', ']' }, { '%', '$', '-', '(', ')', '"', '~', '#', '*', '&' } };

  bool changed = true; // flag for whether the screen update is needed 
  bool keyboardDrawn = false; // flag for whether the keyboard "frame" has already been drawn

  struct Message
  {
    bool ingoing;
    String message;

    Message(String message, bool in)
    {
      this->message = message;
      this->ingoing = in;
    }

    Message()
    {
      this->ingoing = false;
    }
  };

  String enteredText;
  Array<Message, 32> chat;
  short chatStartMessageIndex;

  void sendText(String message)
  {
      Serial.print("Arduino sends: ");
      Serial.println(message.c_str());
      if(chat.full())
          chat.remove(0);

      chat.push_back(Message(message, false));
      if(chat.size() - chatStartMessageIndex > u8x8.getRows() - 1)
          chatStartMessageIndex = chat.size() - u8x8.getRows() + 1;
  }
  
  // handles the SELECTION page screen output
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

  void refreshMessages()
  {
      if(Serial.available() > 0)
      {
        changed = true;
        if(chat.full())
          chat.remove(0);
        
        String got = Serial.readString();
        got.trim();
        if(got.length() != 0)
        {
          chat.push_back(Message(got.c_str(), true));
        }

        if(chat.size() - chatStartMessageIndex > u8x8.getRows() - 1)
          chatStartMessageIndex = chat.size() - u8x8.getRows() + 1;

      }
  }

  void drawDialogue() {
    refreshMessages();
    if (changed) {
      u8x8.clear();
      bool downArrow = false;

      u8x8.setFont(u8x8_font_open_iconic_arrow_1x1);
      

      if(chat.size() - chatStartMessageIndex > u8x8.getRows() - 1)
      {
        u8x8.drawGlyph(0, u8x8.getRows() - 1, '\x50');
        downArrow = true;
      }

      u8x8.setFont(u8x8_font_chroma48medium8_r);


      int lineCounter = 0;
      for(int i = chatStartMessageIndex; i < chat.size() && lineCounter < u8x8.getRows() - downArrow ? 1 : 0; i++, lineCounter++)
      {
        if(chat.at(i).ingoing)
        { 
          u8x8.drawString(1, lineCounter, chat.at(i).message.c_str());
          u8x8.drawGlyph(0, lineCounter, '>');
        }
        else
        {
          u8x8.drawString(max(15 - chat.at(i).message.length(), 0), lineCounter, chat.at(i).message.c_str());
          u8x8.drawGlyph(15, lineCounter, '<');
        }
        
      }

      u8x8.drawString(4, 7, "C to write");
      u8x8.setFont(u8x8_font_open_iconic_embedded_1x1);
      u8x8.drawString(15, 7, "\x45");
      u8x8.setFont(u8x8_font_chroma48medium8_r);
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


  

  // TODO rewrite, too unwieldy
  // handles updating the carriage position of the keyboard
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

  char symbolSelected(KeyboardState state)
  {
      if(state.colCar == 10)
      {
        switch(state.rowCar)
        {
          case 0: // SEND
          case 1: // DEL
            return char(state.rowCar);
          default:
            return ' ';
        }
      }
      else
      {
         return state.extra ? KeyboardExtras[state.rowCar][state.colCar] : KeyboardLetters[state.rowCar][state.colCar];
      }
  }


  // Draws the main "frame" of the on-screen keyboard
  void drawKeyboardMain(bool extra = false) {
    clear();
    
    short xpos = 0;
    short ypos = 2;

    //draw the special buttons
    u8x8.drawString(11, ypos, "SEND");
    u8x8.drawString(11, ypos + 2, "DEL");
    u8x8.drawString(11, ypos + 4, "SPACE");

    // draw the arrow indicating the direction of possible carriage moves
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

        u8x8.drawGlyph(xpos, ypos, (extra ? KeyboardExtras[row][column] : KeyboardLetters[row][column])); // draw the keyboard symbols
        xpos++;
      }
      ypos += 2;
    }
    keyboardDrawn = true;
  }


  // redraws the carriage at the new position, cleans up the previous position
  void drawKeyboardState(KeyboardState state, KeyboardState oldState) {
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.drawGlyph(oldState.colCar, 3 + oldState.rowCar * 2, '\x20'); // remove the old carriage symbol
    if (oldState.colCar == 10) {
      u8x8.drawString(oldState.colCar + 1,  3 + oldState.rowCar * 2, "\x20\x20\x20\x20\x20"); // remove all the carriage symbols in the case of it being on a special button
    }

    u8x8.setFont(u8x8_font_open_iconic_arrow_1x1);
    if (state.colCar == 10) { // handle drawing indicatiors on special buttons
      u8x8.drawString(state.colCar + 1, 3 + state.rowCar * 2, "\x4F\x4F\x4F"); // \x4F is an arrow poiting upwards
      switch (state.rowCar) {
        case 0:
          u8x8.drawString(state.colCar + 4, 3 + state.rowCar * 2, "\x4F");
          break;
        case 2:
          u8x8.drawString(state.colCar + 4, 3 + state.rowCar * 2, "\x4F\x4F");
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
  delay(50);
}