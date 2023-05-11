#include "mbed.h"

#include "C12832_lcd.h"
#include "PinDetect.h"//for the click button
//to change uLCD to 12832LCD using library C12832_lcd
C12832_LCD uLCD; // create a global lcd object
//to change storing audio clips in flash and play from flash https://os.mbed.com/users/4180_1/notebook/using-flash-to-play-audio-clips/
#include "sfx_die_16000.h"
#include "sfx_hit_16000.h"
#include "sfx_point_16000.h"
#define sample_freq 16000.0
Ticker sampletick;
//to change pin number to suit the application board
PwmOut speaker(p26);
PinDetect pb1(p14);

//Potentiometer to change gameplay speed
AnalogIn potentiometer(p20);
int baseDelay = 20000;
int additionalDelay = 60000;
float slowDownFalling =0;
int score = 0;
int wall1x1 = 80;
int wall1x2 = 100;
int wall1y1 = 0;
int wall1y2 = 6;
int wall2x1 = 80;
int wall2x2 = 100;
int wall2y1 = 30;
int wall2y2 = 36;

int ballxpos = 30;
int ballypos = 16;
int ballrad = 2;

int volatile ballVel = 1;

enum gameState{
    beginn,
    playing,
    over};
    
int volatile state;
int i=0;
int j=0;
int k=0;
void AudioSamplePoint(){
    speaker = sound_data_point[i]/255.0;//scale to 0.0 to 1.0 for PWM
    i++;
    if (i>= NUM_ELEMENTS) {
        i = 0;
        sampletick.detach();
    }
}
void AudioSampleDie(){
    speaker = sound_data_die[j]/255.0;//scale to 0.0 to 1.0 for PWM
    j++;
    if (j>= NUM_ELEMENTS) {
        j = 0;
        sampletick.detach();
    }
}
void AudioSampleHit(){
    speaker = sound_data_hit[k]/255.0;//scale to 0.0 to 1.0 for PWM
    k++;
    if (k>= NUM_ELEMENTS) {
        k = 0;
        sampletick.detach();
    }
}
void playPointSound(void const *argument) {
    while(1) {
        speaker.period(1.0/160000.0);
        Thread::signal_wait(0x1);
        //play wav file
        sampletick.attach(&AudioSamplePoint, 1.0 / sample_freq);
    }
}

void playDeadSound(void const *argument) {
    while(1) {
    speaker.period(1.0/160000.0);
    Thread::signal_wait(0x1);
    //play wav file
    sampletick.attach(&AudioSampleDie, 1.0 / sample_freq);
    }
}


void pb1_hit_callback() {
       
       switch (state) {
       case beginn:
       state = playing;
       break;
       case playing:
       ballVel = ballVel-3;
       break;
       case over:
       state = beginn;
       uLCD.cls();
       break;
       }
}

int main() {
    
    state = beginn;
    Thread thread(playPointSound);
    Thread thread2(playDeadSound);

    int ready = 0;
    
    pb1.mode(PullDown);
    pb1.attach_asserted(&pb1_hit_callback);
    pb1.setSampleFrequency();

    uLCD.set_auto_up(0);
    uLCD.cls();

    int go = 1;
    int scoreWrite = 1;
    while(go) {
        wait_us(baseDelay+potentiometer*additionalDelay);
        switch (state) {
        case beginn:
        uLCD.cls();
        uLCD.locate(4,2);
        uLCD.printf("Flappy mbed");
        
        uLCD.locate(30,16);
        uLCD.printf("Press to Start");
        uLCD.copy_to_lcd();
        ready = 0;
        score = 0;
        break;
        
        case playing:
        
        if (!ready)
        {
            wall1x1 = 80;
            wall1x2 = 100;
            wall1y1 = 0;
            wall1y2 = 6;
            wall2x1 = 80;
            wall2x2 = 100;
            wall2y1 = 30;
            wall2y2 = 36;

            ballxpos = 30;
            ballypos = 16;
            ballrad = 2;
            ballVel = 1;
            //uLCD.fillrect(0,0,128,128,1);
            uLCD.cls();
            uLCD.locate(14,0);
            uLCD.printf("%04d", score);
            uLCD.locate(45,16);
            uLCD.printf("Ready");
            uLCD.fillcircle(ballxpos, ballypos, ballrad, 1);
            uLCD.fillrect(wall1x1, wall1y1, wall1x2, wall1y2, 1);
            uLCD.fillrect(wall2x1, wall2y1, wall2x2, wall2y2, 1);
            uLCD.copy_to_lcd();
            ready = 1;
            wait_us(1000000);
            uLCD.cls();
            uLCD.locate(14,0);
            uLCD.printf("%04d", score);
            uLCD.locate(50,16);
            uLCD.printf("Go!");
            uLCD.fillcircle(ballxpos, ballypos, ballrad, 1);
            uLCD.fillrect(wall1x1, wall1y1, wall1x2, wall1y2, 1);
            uLCD.fillrect(wall2x1, wall2y1, wall2x2, wall2y2, 1);
            uLCD.copy_to_lcd();
            wait_us(500000);
        }
        
        wall1x1--;
        wall2x1--;
        wall1x2--;
        wall2x2--;
        if (wall1x2 < -1)
        {
            wall1x2 = 148;
            wall1x1 = 128;
            wall2x2 = 148;
            wall2x1 = 128;
        
            wall1y2 = rand() % (5) + 2;
            wall2y1 = wall1y2 + 20;
            scoreWrite = 1;
        }
        
        if(wall1x2 < 95 && scoreWrite == 1)
        {
            uLCD.cls();
            uLCD.locate(14,0);
            uLCD.printf("%04d", score);
            uLCD.copy_to_lcd();
            scoreWrite = 0;
        }
        uLCD.cls();
        uLCD.locate(14,0);
        uLCD.printf("%04d", score);
        uLCD.fillrect(wall1x1, wall1y1, wall1x2, wall1y2, 1);
        uLCD.fillrect(wall2x1, wall2y1, wall2x2, wall2y2, 1);
        uLCD.copy_to_lcd();
        
        if (ballxpos == (wall1x1 + 10))
        {
            thread.signal_set(0x1);
            score++;
            uLCD.locate(14,0);
            uLCD.printf("%04d", score);   
            uLCD.copy_to_lcd();
        } 
        uLCD.fillcircle(ballxpos, ballypos, ballrad, 1);
        uLCD.copy_to_lcd();
        slowDownFalling+=potentiometer+0.2;
        if (ballVel < 1 && slowDownFalling>=1){
            ballVel++;
            slowDownFalling = 0;
        }
            
        ballypos = ballypos + ballVel;
        uLCD.fillcircle(ballxpos, ballypos, ballrad, 1);
        uLCD.copy_to_lcd();
        
        if (ballypos - 2 < wall1y2) {
            if (ballxpos+2 > wall1x1 && ballxpos-2 < wall1x2) {
                state = over;
                sampletick.attach(&AudioSampleHit, 1.0 / sample_freq);
                wait_us(200000);
            }
        }
        else if (ballypos + 2 > wall2y1) {
            if (ballxpos+2 > wall2x1 && ballxpos-2 < wall2x2) {
                state = over;    
                sampletick.attach(&AudioSampleHit, 1.0 / sample_freq);
                wait_us(200000);
            }
        }
        
        if (ballypos > 32 || ballypos < 0) {
            state = over;
        }
        
        break;
        
        case over:
        if (ready)
        {
            Thread::wait(300);
            uLCD.locate(40,16);
            uLCD.printf("Game Over");
            uLCD.locate(14,0);
            uLCD.printf("%04d", score);
            uLCD.copy_to_lcd();
            thread2.signal_set(0x1);
            wait_us(300000);
            ready = 0;
            score = 0;
        }
        break;
        }
    }
}
