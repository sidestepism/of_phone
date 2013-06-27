/*
 Streaming Audio

 Programmed by
 Matthew Vaterlaus
 23 August 2010

 Adapted largely from Michael Smith's speaker_pcm.
 <michael@hurts.ca>

 Plays 8-bit PCM audio on pin 11 using pulse-width modulation.
 For Arduino with Atmega at 16 MHz.

 The audio data needs to be unsigned, 8-bit, 8000 Hz.

 Although Smith's speaker_pcm was very well programmed, it
 had two major limitations: 1. The size of program memory only
 allows ~5 seconds of audio at best. 2. To change the adudio,
 the microcontroller would need to be re-programmed with a new
 sounddata.h file.

 StreamingAudio overcomes these limitations by dynamically
 sending audio samples to the Arduino via Serial.

 It uses a 1k circular buffer for the audio samples, since
 the ATMEGA328 only has 2k of RAM. For chips with less RAM,
 the BUFFER_SIZE variable can be reduced.
 */
#define BUTTONPIN 8
#define MICPIN A0
#define SPEAKERPIN 11

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#define SAMPLE_RATE 8000
#define BUFFER_SIZE 512

unsigned char sounddata_data[BUFFER_SIZE];
int BufferHead=0;
int BufferTail=0;

unsigned char micdata[BUFFER_SIZE];//could be different size from sounddata, if needed. Actually, shorter might suffice
int micbufHead=0;
int micbufTail=0;
int micdatasize = 0;//how much we have buffered right now

unsigned int callEndCounter = 0; //counts up to 128 consequtive "-" in received data, signifying a hang-up.
unsigned long sample=0;//how many samples have been played. Could be useful for call length logging. Loops around after n^ridiculous seconds.
unsigned long BytesReceived=0;
unsigned long Temp=0;
unsigned long NewTemp=0;

int ledPin = 13;
int speakerPin = 11;
int Playing = 0;
int buflen;

//Interrupt Service Routine (ISR)
// This is called at 8000 Hz to load the next sample.
ISR(TIMER1_COMPA_vect)
{
    //If not at the end of audio
    if (sample < 4000000000) //about the max size of unsigned long
    {
        //Set the PWM Freq.
        OCR2A = sounddata_data[BufferTail];
        //If circular buffer is not empty
        if (BufferTail != BufferHead)
        {
            //Increment Buffer's tail index.
            BufferTail = ((BufferTail+1) % BUFFER_SIZE);
            //Increment sample number.
            sample++;
        }//End if
    }//End if
    else //We need to loop around the sample counter.
    {
        //back to beginning.
        sample = 0;
    }//End Else
    //Record audio sample by Akaki
    micdata[micbufHead] = (unsigned int)(analogRead(A0)/4);//gets converted to one byte. Could be optimized by ditching the prebuilt function and just reading one register.
    micbufHead = (micbufHead+1)%BUFFER_SIZE;//advance head.
    micdatasize++;//increment size. this could of course be calculated on sending by looking up the dirrence between head and tail, but not too big of an overhead.
}//End Interrupt

void startPlayback()
{
    //Set pin for OUTPUT mode.
    pinMode(speakerPin, OUTPUT);
    //---------------TIMER 2-------------------------------------
    // Set up Timer 2 to do pulse width modulation on the speaker
    // pin.
    //This plays audio at the frequency of the audio sample.
    // Use internal clock (datasheet p.160)
    //ASSR = Asynchronous Status Register
    ASSR &= ~(_BV(EXCLK) | _BV(AS2));
    // Set fast PWM mode (p.157)
    //Timer/Counter Control Register A/B for Timer 2
    TCCR2A |= _BV(WGM21) | _BV(WGM20);
    TCCR2B &= ~_BV(WGM22);
    // Do non-inverting PWM on pin OC2A (p.155)
    // On the Arduino this is pin 11.
    TCCR2A = (TCCR2A | _BV(COM2A1)) & ~_BV(COM2A0);
    TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0));
    // No prescaler (p.158)
    TCCR2B = (TCCR2B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
    //16000000 cycles 1 increment 2000000 increments
    //-------- * ---- = -------
    // 1 second 8 cycles 1 second
    //Continued...
    //2000000 increments 1 overflow 7812 overflows
    //------- * --- = -----
    // 1 second 256 increments 1 second
    // Set PWM Freq to the sample at the end of the buffer.
    OCR2A = sounddata_data[BufferTail];
    //--------TIMER 1----------------------------------
    // Set up Timer 1 to send a sample every interrupt.
    // This will interrupt at the sample rate (8000 hz)
    //
    cli();
    // Set CTC mode (Clear Timer on Compare Match) (p.133)
    // Have to set OCR1A *after*, otherwise it gets reset to 0!
    TCCR1B = (TCCR1B & ~_BV(WGM13)) | _BV(WGM12);
    TCCR1A = TCCR1A & ~(_BV(WGM11) | _BV(WGM10));
    // No prescaler (p.134)
    TCCR1B = (TCCR1B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
    // Set the compare register (OCR1A).
    // OCR1A is a 16-bit register, so we have to do this with
    // interrupts disabled to be safe.
    OCR1A = F_CPU / SAMPLE_RATE; // 16e6 / 8000 = 2000
    //Timer/Counter Interrupt Mask Register
    // Enable interrupt when TCNT1 == OCR1A (p.136)
    TIMSK1 |= _BV(OCIE1A);
    //Init Sample. Start from the beginning of audio.
    sample = 0;

    //Enable Interrupts
    sei();
}//End StartPlayback
void stopPlayback()
{
    // Disable playback per-sample interrupt.
    TIMSK1 &= ~_BV(OCIE1A);
    // Disable the per-sample timer completely.
    TCCR1B &= ~_BV(CS10);
    // Disable the PWM timer.
    TCCR2B &= ~_BV(CS10);
    digitalWrite(speakerPin, LOW);
}//End StopPlayback


void ringtoneAndPause() {//pauses operation and plays a ringtone until a button is pushed
    pinMode(BUTTONPIN,INPUT);
    //define some notes
#define cc 261
#define dd 294
#define ee 329
#define ff 349
#define gg 392
#define aa 440
#define bb 493
#define CC 523
#define oneb 150 //length of one beat
    int melodylength = 14;
    int melody[14] = {gg,ff,aa,bb,ee,dd,ff,gg,0,dd,cc,ee,gg,CC};//Nokia tune
    int lengths[14] = {oneb,oneb,oneb*2,oneb*2,oneb,oneb,oneb*3,oneb*3,oneb*1.5,oneb,oneb,oneb*2,oneb*2};//length of each note
    int i = 0;
    while (1) { //digitalRead(BUTTONPIN)==LOW) {//while no input
        tone(SPEAKERPIN, melody[i], lengths[i]);
        delay(lengths[i]);
        i = (i+1) %melodylength;
        delay(oneb/10);//leave just a little moment between tones.
    }
    noTone(SPEAKERPIN);
}

void setup()
{
    //Set LED for OUTPUT mode
    pinMode(ledPin, OUTPUT);

    //Start Serial port. If your application can handle a
    //faster baud rate, that would increase your bandwidth
    //115200 only allows for 14,400 Bytes/sec. Audio will
    //require 8000 bytes / sec to play at the correct speed.
    //This only leaves 44% of the time free for processing
    //bytes.
    Serial.begin(115200);

    //PC sends audio length as 10-digit ASCII
    //While audio length hasn't arrived yet
    while (Serial.available() < 10)
    {
        //Blink the LED on pin 13.
        digitalWrite(ledPin, !digitalRead(ledPin));
        delay(100);
    }

    //Tell the remote PC/device that the Arduino is ready
    //to begin receiving samples.
    Serial.println(128);

    //There's data now, so start playing.
    //startPlayback();
    Playing =0;

    // defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
    // set prescale to 16. Speeds up analogRead but lowers resolution.
    sbi(ADCSRA,ADPS2) ;
    cbi(ADCSRA,ADPS1) ;
    cbi(ADCSRA,ADPS0) ;
}//End Setup
void loop()
{

    //ringtoneAndPause();
    //If audio not started yet...
    if (Playing == 0)
    {
        //Check to see if the first 1000 bytes are buffered.
        if (BufferHead = 1023)
        {
            //ringtoneAndPause();//rings a tone and waits for button input before proceeding.
            Playing = 1;
            startPlayback();
        }//End if
    }//End if


    while(1){
      //While the serial port buffer has data
      if (Serial.available() > 0)
      {
          //If the sample buffer isn't full
          if (((BufferHead+1) % BUFFER_SIZE) != BufferTail)
          {
              //Increment the buffer's head index.
              BufferHead = (BufferHead+1) % BUFFER_SIZE;
              //Store the sample freq.
              sounddata_data[BufferHead] = Serial.read();
              //Increment the bytes received
              BytesReceived++;
  
  /* -- が続いたら終了
              if (sounddata_data[BufferHead] == '-') {//if received 128 consecutive minuses, call should be ended.
                  if (++callEndCounter > 128) {//if end of ever incrementing counter...
                      stopPlayback();
                      Playing = 0;
                  }
              } else {//if not a minus,
                  callEndCounter = 0;//reset hang-up counter.
              }
  */
  
          }//End if
      }//End While
  
  
      if (micdatasize > 0) //if we have some audio recorded.
          //this is not full buffer size, as the 8000 hz interrupt might fill the buffer up ever further during transmit.
      {
        
        /** いっきにおくる
          //Serial.println("Hello");
          //while (micdatasize>0) //go until all is sent out
          {
              if (micbufHead < micbufTail) {//if the tail has looped around
                  // -- head -> ----------- tail -> ------
                  buflen = BUFFER_SIZE - micbufTail;//only go up to the end of the buffer
              } else {
                  // -- tail -> ------------ head -> ------
                  buflen = micbufHead - micbufTail;//the difference
              }
              // int buflen = (micbufHead - micbufTail)>0 ? (micbufHead - micbufTail) : (BUFFER_SIZE - micbufTail);//if not looped around ? the difference betwwen head and tail : else until buffer end
              Serial.write(micdata + micbufTail, buflen);
              //you could join it all in one string, but im not sure how the interrupts would like it if you were sending long serials.
              micdatasize -= buflen;
              micbufTail = (micbufTail+buflen) % BUFFER_SIZE;
          }
          //Serial.flush();
          */

          //while (micdatasize>0) //go until all is sent out
          {
              if (micbufHead < micbufTail) {//if the tail has looped around
                  // -- head -> ----------- tail -> ------
                  buflen = BUFFER_SIZE - micbufTail;//only go up to the end of the buffer
              } else {
                  // -- tail -> ------------ head -> ------
                  buflen = micbufHead - micbufTail;//the difference
              }
              // int buflen = (micbufHead - micbufTail)>0 ? (micbufHead - micbufTail) : (BUFFER_SIZE - micbufTail);//if not looped around ? the difference betwwen head and tail : else until buffer end
              Serial.write(micdata[micbufTail]);
              //you could join it all in one string, but im not sure how the interrupts would like it if you were sending long serials.
              micdatasize -= 1;
              micbufTail = (micbufTail + 1) % BUFFER_SIZE;
          }

      }
    }
    

}//End Loop
