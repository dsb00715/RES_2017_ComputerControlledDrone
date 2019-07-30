/*
 * PPM generator originally written by David Hasko
 * on https://code.google.com/p/generate-ppm-signal/ 
 */

//////////////////////CONFIGURATION///////////////////////////////
#define CHANNEL_NUMBER 12  //set the number of chanels
#define CHANNEL_DEFAULT_VALUE 1500  //set the default servo value
#define FRAME_LENGTH 22500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PULSE_LENGTH 300  //set the pulse length
#define onState 1  //set polarity of the pulses: 1 is positive, 0 is negative
#define sigPin 10  //set PPM signal output pin on the arduino
//////////////////////////////////////////////////////////////////

#define SWITCH_PIN 16
#define CHANNEL_TO_MODIFY 11
#define SWITCH_STEP 100

byte previousSwitchValue;

/*this array holds the servo values for the ppm signal
 change theese values in your code (usually servo values move between 1000 and 2000)*/
int ppm[CHANNEL_NUMBER];

int currentChannelStep;

uint8_t throtal = 0;
uint8_t aileron = 5;
uint8_t elevator= 5;
uint8_t rudder  = 5; 

uint16_t thr_min = 988;
uint16_t ail_min = 990;
uint16_t elv_min = 988;
uint16_t rud_min = 988;

uint16_t thr_mid = 1502;
uint16_t ail_mid = 1502;
uint16_t elv_mid = 1514;
uint16_t rud_mid = 1502;

uint16_t thr_max = 2012;
uint16_t ail_max = 2012;
uint16_t elv_max = 2012;
uint16_t rud_max = 2012;

uint8_t stick_min = 0;
uint8_t stick_max = 10; 

void setup(){  
  Serial.begin(115200);
  previousSwitchValue = HIGH;
  
  //initiallize default ppm values
  for(int i=0; i<CHANNEL_NUMBER; i++){
    if (i == 0 || i == CHANNEL_TO_MODIFY) {
      ppm[i] = thr_min;
    } else {
      ppm[i]= CHANNEL_DEFAULT_VALUE;
    }
  }
  ppm[1] = ail_mid;
  ppm[2] = elv_mid;
  ppm[3] = rud_mid;
  pinMode(sigPin, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  digitalWrite(sigPin, !onState);  //set the PPM signal pin to the default state (off)
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();

  currentChannelStep = SWITCH_STEP;
}



void loop(){

  int switchState;
  byte command;
  switchState = digitalRead(SWITCH_PIN);

  if (switchState == LOW && previousSwitchValue == HIGH) {

    static int val = SWITCH_STEP;

    ppm[CHANNEL_TO_MODIFY] = ppm[CHANNEL_TO_MODIFY] + currentChannelStep;
    
    if (ppm[CHANNEL_TO_MODIFY] > 2000 || ppm[CHANNEL_TO_MODIFY] < 1000) {
      currentChannelStep = currentChannelStep * -1;
      ppm[CHANNEL_TO_MODIFY] = ppm[CHANNEL_TO_MODIFY] + currentChannelStep;
    }
    
  }

  previousSwitchValue = switchState;

  if(Serial.available()){
    command = Serial.read(); 
    
    if(command == 98){
      digitalWrite(12, LOW);
      delay(1000);
      digitalWrite(12, HIGH);   
    }
        /* 
      LIMITS 
      THROTAL  988 -- 2012
      AILERON  990 -- 2012
      ELEVATOR 988 -- 2012
      RUDER    988 -- 2012 
    */
    else if(command == 'w' && throtal < stick_max  ){
      throtal += 1; 
      ppm[0] = map(throtal, stick_min, stick_max, thr_min, thr_max);
    }
    else if(command == 's' && throtal > stick_min ){
      throtal -= 1;
      ppm[0] = map(throtal, stick_min, stick_max, thr_min, thr_max);  
    }
    else if(command == 'a' && aileron > stick_min){
      aileron -= 1; 
      ppm[1] = map(aileron, stick_min, stick_max, ail_min, ail_max);
    }
    else if(command == 'd' && aileron < stick_max){
      aileron +=1;
      ppm[1] = map(aileron, stick_min, stick_max, ail_min, ail_max);
    }
    else if(command == 'q' && rudder > stick_min){
      rudder -=1;
      if(rudder == 5){ ppm[3] = rud_mid; }
      else {
        ppm[3] = map(rudder, stick_min, stick_max, rud_min, rud_max);}
    }
    else if(command == 'e' && rudder < stick_max){
      rudder +=1;
      if(rudder == 5){ ppm[3] = rud_mid; } 
      else{
        ppm[3] = map(rudder, stick_min, stick_max, rud_min, rud_max);} 
    }
//    else if(command == ''){
//    }
    else if(command == 't' ){
      ppm[0] = 988;
    }
    Serial.println((char)command);
    Serial.print("Thr: ");
    Serial.print(ppm[0]);
    Serial.print(" | Ail: ");
    Serial.print(ppm[1]);
    Serial.print(" | Elv: ");
    Serial.print(ppm[2]);
    Serial.print(" | Rud: ");
    Serial.println(ppm[3]);
  }
}

ISR(TIMER1_COMPA_vect){  //leave this alone
  static boolean state = true;
  
  TCNT1 = 0;
  
  if (state) {  //start pulse
    digitalWrite(sigPin, onState);
    OCR1A = PULSE_LENGTH * 2;
    state = false;
  } else{  //end pulse and calculate when to start the next pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;
  
    digitalWrite(sigPin, !onState);
    state = true;

    if(cur_chan_numb >= CHANNEL_NUMBER){
      cur_chan_numb = 0;
      calc_rest = calc_rest + PULSE_LENGTH;// 
      OCR1A = (FRAME_LENGTH - calc_rest) * 2;
      calc_rest = 0;
    }
    else{
      OCR1A = (ppm[cur_chan_numb] - PULSE_LENGTH) * 2;
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}
