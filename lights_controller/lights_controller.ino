/*
 * Copyright 2013 Pieter Rautenbach
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * ITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Challenge request and response strings
const String CHL_REQ = "bistdudasblinkenlichten";
const String CHL_RES = "ichbindasblinkenlichten";

// Command separator
const char CMD_SEP = '=';

// Commands
const String RED_CMD = "red";
const String GRN_CMD = "green";
const String YLW_CMD = "yellow";

// Actions
const String ACTION_ON = "on";
const String ACTION_SOS = "sos";

// ACK/NACK
const String ACK = "ack";
const String NAK = "nak";

// Light pinouts
const byte RED_PIN = 4;
const byte GRN_PIN = 9;
const byte YLW_PIN = 10;

// Morse code dot/unit duration
const int dot = 200;
// Morse code state - indicates whether to service the "thread"
boolean isSosActive = false;
// Sequence of unit delays that spell SOS
// Letter: |   S    |   O    |   S    |
// On:     | 1 1 1  | 3 3 3  | 1 1 1  |
// Off:    |  1 1 3 |  1 1 3 |  1 1 7 |
const int intervals[] = {1,1,1,1,1,3,3,1,3,1,3,3,1,1,1,1,1,7};
// Current index in the sequence
int intervalIdx = 0;
unsigned long interval_ = 0;
unsigned long previousMillis = 0;
boolean sosLightState = false;

// The raw command received
String commandBuffer = "";

//////////////////////////////////////////////////////////////////////
// Setup                                                            //
//////////////////////////////////////////////////////////////////////

void setup()
{
  // Set the output PINs for the LEDs
  pinMode(RED_PIN, OUTPUT);
  pinMode(GRN_PIN, OUTPUT);
  pinMode(YLW_PIN, OUTPUT);
  
  // Default (unknown) state is all off -- with a flash
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GRN_PIN, HIGH);
  digitalWrite(YLW_PIN, HIGH);
  delay(100);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GRN_PIN, LOW);
  digitalWrite(YLW_PIN, LOW);
  
  // Set the BAUD - ignored on the Teensy
  Serial.begin(9600);
  
  // Start with a empty buffer
  commandBuffer = "";
  
  // SOS off
  isSosActive = false;
}

//////////////////////////////////////////////////////////////////////
// Main                                                             //
//////////////////////////////////////////////////////////////////////

void loop()
{
  int rxByteBuffer = 0;
  if (Serial.available() > 0)
  {
    // Read printable characters into the buffer until a control character is received
    rxByteBuffer = Serial.read();
    if (isPrintable(rxByteBuffer)) 
    {
      commandBuffer.concat((char)rxByteBuffer);
    }
    else if (isControl(rxByteBuffer)) 
    {
      // Act on valid commands
      if (isValidCommand(commandBuffer) && setLights(commandBuffer))
      {
        Serial.println(ACK);
      }
      // Respond to a challenge request
      else if (isChallenge(commandBuffer))
      {
        Serial.println(CHL_RES);
      }
      // For now, we'll just echo it back if can't do anything with it
      else
      {
        Serial.println(NAK);
      }
      // Clear the buffer for the next raw command
      commandBuffer = "";
    }   
  }
  
  // Service SOS after interpreting a command, because the command
  // could turn it off. 
  if (isSosActive)
  {
    serviceSos();
  }
}

//////////////////////////////////////////////////////////////////////
// Command handler                                                  //
//////////////////////////////////////////////////////////////////////

boolean setLights(String commandBuffer)
{
  String command = getCommand(commandBuffer);
  String actionStr = getAction(commandBuffer);
  boolean action = actionToBool(actionStr);
  if (isRedCommand(command))
  {
    boolean sosAction = isSosAction(actionStr);
    if (sosAction)
    {
      // In case the red LED is already on (most likely)
      redLight(false);
      delay(dot);
      isSosActive = true;
      intervalIdx = 0;
      interval_ = intervals[intervalIdx] * dot;
      sosLightState = false;
      previousMillis = millis();
      redLight(true);
    }
    else
    {
      isSosActive = false;
      redLight(action);
    }
  }
  else if (isGreenCommand(command))
  {
    greenLight(action);
  }
  else if (isYellowCommand(command))
  {
    yellowLight(action);
  }
  else
  {
    return false;
  }
  return true;
}

//////////////////////////////////////////////////////////////////////
// Parsing                                                          //
//////////////////////////////////////////////////////////////////////

boolean actionToBool(String action)
{
  return action.compareTo(ACTION_ON) == 0;
}

boolean isSosAction(String action)
{
  return action.compareTo(ACTION_SOS) == 0;
}

boolean isChallenge(String commandBuffer)
{
  return commandBuffer.compareTo(CHL_REQ) == 0;
}

boolean isValidCommand(String commandBuffer)
{
  return commandBuffer.indexOf(CMD_SEP) > -1;
}

String getCommand(String commandBuffer)
{
  return commandBuffer.substring(0, commandBuffer.indexOf(CMD_SEP));
}

String getAction(String commandBuffer)
{
  return commandBuffer.substring(commandBuffer.indexOf(CMD_SEP) + 1);
}

//////////////////////////////////////////////////////////////////////
// Command tests                                                    //
//////////////////////////////////////////////////////////////////////

boolean isRedCommand(String command)
{
  return command.compareTo(RED_CMD) == 0;
}

boolean isGreenCommand(String command)
{
  return command.compareTo(GRN_CMD) == 0;
}

boolean isYellowCommand(String command)
{
  return command.compareTo(YLW_CMD) == 0;
}

//////////////////////////////////////////////////////////////////////
// Light switching                                                  //
//////////////////////////////////////////////////////////////////////

void redLight(boolean on)
{
  light(RED_PIN, on);
}

void greenLight(boolean on)
{
  light(GRN_PIN, on);  
}

void yellowLight(boolean on)
{
  light(YLW_PIN, on);
}

void light(int pin, boolean on)
{
  if (on)
  {
    digitalWrite(pin, HIGH);
  }
  else
  {
    digitalWrite(pin, LOW);
  }
}

//////////////////////////////////////////////////////////////////////
// SOS                                                              //
//////////////////////////////////////////////////////////////////////

void serviceSos()
{
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > interval_)
  {
    // Debug
    //if (!sosLightState)
    //{
    //  Serial.print("cm: ");
    //  Serial.print(currentMillis);
    //  Serial.print(" pm: ");
    //  Serial.print(previousMillis);
    //  Serial.print(" interval: ");
    //  Serial.print(interval_);
    //  Serial.print(" sosLightState: ");
    //  Serial.print(sosLightState);
    //  Serial.print(" intervalIdx: ");
    //  Serial.print(intervalIdx);
    //  Serial.print(" item: ");
    //  Serial.println(intervals[intervalIdx % 18]);
    //}
    
    // Set millis immediately to get timing as accurately as 
    // possible (remember we're in a loop)
    previousMillis = currentMillis;
    
    // Set the light according to the state (on/off)
    redLight(sosLightState);
    
    // Advance
    sosLightState = !sosLightState;
    intervalIdx++;
    interval_ = intervals[intervalIdx % 18] * dot;
  }
}

