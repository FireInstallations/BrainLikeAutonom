////////////////////////////////////////////////////////////////////////
// Arduino Bluetooth Interface with Mindwave Mobile (BT 2.1)
//
// This is example code provided by Christian Rempel and Laurin Kettner
// derives the raw data from the data stream and derives eyeblink events, it is license free.
// The Serial Monitor has to be set to a baudrate of 57600
// March 2018
////////////////////////////////////////////////////////////////////////
#define BAUDRATE 57600

#include <SoftwareSerial.h>
//#include <Servo.h>
//Servo servoblau;
SoftwareSerial BTSerial(10, 11); // RX | TX

boolean DEBUGOUTPUT  = false,piekDetected = false;
int Data[512] = {0};
int i = 0, n = 0;
int PiekP, PiekM;
long piekTime = 0;
int payloadLength = 0;
byte payloadData[64] = {0};
byte poorQuality = 200;

//////////////////////////
// Microprocessor Setup //
//////////////////////////
void setup()

{
  //servoblau.attach(2);  //the servo is attached to Pin2
  Serial.begin(57600);
  BTSerial.begin(BAUDRATE);           // Baudrate Headset is 57600
  pinMode(6,OUTPUT); // LED which indicates, that por quality is zero
    
}

////////////////////////////////
// Read data from Serial UART //
////////////////////////////////
byte ReadOneByte()

{
  int ByteRead;
  while (!BTSerial.available());
  ByteRead = BTSerial.read();

  return ByteRead;
}

/////////////
//MAIN LOOP//
/////////////
void loop()
{
  long Hilf = 0;
  // The Data are written in a drum with the circumference of 512 values (index i), time is running with increasing i
  // i is the index of the current time, decreasing positions starting from i in the Data array means going back in the past
  if (i >= 512)
    i = 0;

  if (ReadOneByte() == 170 && ReadOneByte() == 170)// this is the beginning of each new sequence
  {
    payloadLength = ReadOneByte();
    if (payloadLength == 4)// the raw data have a payloadlength of 4, the esense values have another payloadlength < 170 bytes
    {
      if (ReadOneByte() == 128 && ReadOneByte() == 2)// the sequence announcing raw data is 170 170 4 128 2
      {

        Hilf =  ((long)ReadOneByte() * 256 + (long)ReadOneByte());// read the most significant two bytes and form a signed number of it
        if (Hilf > 32767)
          Hilf -= (long)(65535);
        Data[i] = (int)Hilf;

        ReadOneByte(); // read the third byte, without taking notice of it
        // PiekP is a gliding sum over 50 values of Data 71 values of Data in the past, 71 values are reserved for the minus peak PiekM
        PiekP += Data[(512 + i - 71) % 512];
        PiekP -= Data[(512 + i - 50 - 71) % 512];
        // Test, if PiekP exceeds a certain value and the youngest value of PiekP is negative and it has no huge values
        if ((PiekP > 3000) && (Data[(512 + i  - 70) % 512] < 0) && (PiekP < 13000))
        { // The next eye blink detection is enabled only after a certain elapse time
          if (millis() - piekTime > 100) //time
          { PiekM = 0;
            // After detecting a positive peak PiekP the following 70 values are summed up and tested, if more negative than a certain value
            for (int j = 1; j <= 70; j++)
              PiekM +=  (int)(Data[(512 + i  + j - 70) % 512]);

            //Sometimes big negative numbers appear, which are suppressed by a limit for the negative values, if they are to huge
            if (PiekM < -3000 && PiekM > -11000) {
              
              //Serial.println("I-Blink detected!");
              // n is the counter for the number of subsequent eye blinks within a certain time difference
              if ((millis() - piekTime) < 600)n++; else n = 1;
              //Serial.print(PiekP);
              //Serial.print("; ");
              
              //Serial.println(PiekM);
              //Serial.print("   n   ");
              //Serial.println(n);
              //Serial.print("   poorQuality    ");
              //Serial.println(poorQuality);
              if(poorQuality == 0)digitalWrite(6,HIGH);else digitalWrite(6,LOW);
              piekTime = millis();//piekTime is the time at which the eye blink has been detected
              piekDetected = true;// piekDetected is set true, when an eye blink has been detected
            }// end if PiekM (eyeblink detected)
          }//end elapse time
        }// end PiekP detect
        i++;// move one raw value forward
        // The Data Array can be printed out if DEBUGOUTPUT is set true
        if (DEBUGOUTPUT && i == 512)
        {
          for (int j = 0; j < 512; j++)
          {
            Serial.print(Data[j]);
            Serial.print(";");
          }
          Serial.println("");
        }// end debug output
      }// end if 128 and 2 found
    }// end if payloadLength == 4
    else if(payloadLength < 170){// this is the access to the esense values, here poor quality, which is announced by a byte with value 2
      for (int k = 1; k < payloadLength; k++)
      if(ReadOneByte()== 2)poorQuality = ReadOneByte();
    }
  }// end if 170 170 appeared
  //print n after a certain elapse time, peakDetected has the purpose, that the printing happens only once after the last eye blink
  if((millis()-piekTime)>700 && piekDetected == true)Serial.println(n),piekDetected = false;
}// end of loop
