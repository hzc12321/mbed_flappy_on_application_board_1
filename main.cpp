#include "C12832_lcd.h"
#include "PinDetect.h" //for the click button
#include "mbed.h"

// to change uLCD to 12832LCD using library C12832_lcd
C12832_LCD uLCD; // create a global lcd object
// to change storing audio clips in flash and play from flash
// https://os.mbed.com/users/4180_1/notebook/using-flash-to-play-audio-clips/
#include "LM75B.h"
#include "sfx_die_16000.h"
#include "sfx_hit_16000.h"
#include "sfx_point_16000.h"

// ntp and ethernet library
#include "EthernetInterface.h"
// #include "NTPClient.h"
EthernetInterface eth;
// NTPClient ntp;

#define sample_freq 16000.0
Ticker sampletick;

BusOut display(p9, p10, p12, p13, p15, p16, p17, p18);
// to change pin number to suit the application board
PwmOut speaker(p26);
PinDetect pushbutton1(p14);

// Define RGB LED pins
DigitalOut redLed(p23);
DigitalOut greenLed(p24);
DigitalOut blueLed(p25);

// Display surrouding temperature
LM75B sensor(p28, p27);
Serial pc(USBTX, USBRX);

// Potentiometer to change gameplay speed
AnalogIn potentiometer(p20);
int baseDelay = 20000;
int additionalDelay = 60000;
float slowDownFalling = 0;
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

int remainingLife;

// Sun's position
int sunX = 40;
int sunY = 6;
int sunRadius = 4;
;

enum gameState { beginn, playing, over };

int volatile state;
int i = 0;
int j = 0;
int k = 0;
void AudioSamplePoint() {
  speaker = sound_data_point[i] / 255.0; // scale to 0.0 to 1.0 for PWM
  i++;
  if (i >= NUM_ELEMENTS) {
    i = 0;
    sampletick.detach();
  }
}
void AudioSampleDie() {
  speaker = sound_data_die[j] / 255.0; // scale to 0.0 to 1.0 for PWM
  j++;
  if (j >= NUM_ELEMENTS) {
    j = 0;
    sampletick.detach();
  }
}
void AudioSampleHit() {
  speaker = sound_data_hit[k] / 255.0; // scale to 0.0 to 1.0 for PWM
  k++;
  if (k >= NUM_ELEMENTS) {
    k = 0;
    sampletick.detach();
  }
}
void playPointSound(void const *argument) {
  while (1) {
    speaker.period(1.0 / 160000.0);
    Thread::signal_wait(0x1);
    // play wav file
    sampletick.attach(&AudioSamplePoint, 1.0 / sample_freq);
  }
}

void playDeadSound(void const *argument) {
  while (1) {
    speaker.period(1.0 / 160000.0);
    Thread::signal_wait(0x1);
    // play wav file
    sampletick.attach(&AudioSampleDie, 1.0 / sample_freq);
  }
}

void pushbutton1_button1HitHandler() {

  switch (state) {
  case beginn:
    state = playing;
    break;
  case playing:
    ballVel = ballVel - 3;
    break;
  case over:
    state = beginn;
    uLCD.cls();
    break;
  }
}

void showRemainingLife() {
  // Determine the color based on the remaining life
  if (remainingLife == 3) {
    // Show green color for 3 remaining lives
    redLed = 1;
    greenLed = 1;
    blueLed = 0;
  } else if (remainingLife == 2) {
    // Show yellow color for 2 remaining lives
    redLed = 0;
    greenLed = 0;
    blueLed = 1;
  } else if (remainingLife == 1) {
    // Show red color for 1 remaining life
    redLed = 0;
    greenLed = 1;
    blueLed = 1;
  }
  wait_ms(500);
}

void updateDisplay(int score) {
  if (score >= 0 && score <= 9) {
    switch (score) {
    case 0:
      display = 0x3F; // display 0
      break;
    case 1:
      display = 0x06; // display 1
      break;
    case 2:
      display = 0x5B; // display 2
      break;
    case 3:
      display = 0x4F; // display 3
      break;
    case 4:
      display = 0x66; // display 4
      break;
    case 5:
      display = 0x6D; // display 5
      break;
    case 6:
      display = 0x7D; // display 6
      break;
    case 7:
      display = 0x07; // display 7
      break;
    case 8:
      display = 0x7F; // display 8
      break;
    case 9:
      display = 0x6F; // display 9
      break;
    }
  } else {
    display = 0x00; // turn off all segments
  }
}

int main() {

  //   const char *ip = "192.168.1.2";
  //   const char *mask = "255.255.255.0";
  //   const char *gateway = "192.168.1.1";
  //   eth.set_network(ip, mask, gateway);

  set_time(1686648600);

  int wallHit = 0;

  state = beginn;
  Thread thread(playPointSound);
  Thread thread2(playDeadSound);

  int ready = 0;

  pushbutton1.mode(PullDown);
  pushbutton1.attach_asserted(&pushbutton1_button1HitHandler);
  pushbutton1.setSampleFrequency();

  uLCD.set_auto_up(0);
  uLCD.cls();

  int go = 1;
  int scoreWrite = 1;
  while (go) {
    wait_us(baseDelay + potentiometer * additionalDelay);
    switch (state) {
    case beginn:
      uLCD.cls();
      uLCD.locate(4, 0);
      uLCD.printf("Flappy mbed");

      uLCD.locate(30, 10);
      uLCD.printf("Press to Start");
      uLCD.locate(75, 0);
      uLCD.printf("Temp:%.2f\n", sensor.read());
      uLCD.locate(9, 20); // time location
      //   if (ntp.setTime("0.pool.ntp.org") == 0) {
      time_t ctTime;
      ctTime = time(NULL);
      uLCD.printf("%s\r\n", ctime(&ctTime));
      //   }

      uLCD.copy_to_lcd();
      ready = 0;
      score = 0;
      updateDisplay(score);

      remainingLife = 3;
      showRemainingLife();

      break;

    case playing:

      if (!ready) {
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
        // uLCD.fillrect(0,0,128,128,1);
        uLCD.cls();
        uLCD.locate(14, 0);
        uLCD.printf("%04d", score);
        updateDisplay(score);
        uLCD.locate(45, 16);
        uLCD.printf("Ready");
        uLCD.fillcircle(ballxpos, ballypos, ballrad, 1);
        uLCD.fillrect(wall1x1, wall1y1, wall1x2, wall1y2, 1);
        uLCD.fillrect(wall2x1, wall2y1, wall2x2, wall2y2, 1);

        uLCD.copy_to_lcd();
        ready = 1;
        wait_us(1000000);
        uLCD.cls();
        uLCD.locate(14, 0);
        uLCD.printf("%04d", score);
        updateDisplay(score);
        uLCD.locate(50, 16);
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

      // Update Sun's position
      sunX--;
      if (sunX < -sunRadius) {
        sunX = 128;
      }
      if (wall1x2 < -1) {
        wall1x2 = 148;
        wall1x1 = 128;
        wall2x2 = 148;
        wall2x1 = 128;

        wall1y2 = rand() % (5) + 2;
        wall2y1 = wall1y2 + 20;
        scoreWrite = 1;
      }

      if (wall1x2 < 95 && scoreWrite == 1) {
        uLCD.cls();
        uLCD.locate(14, 0);
        uLCD.printf("%04d", score);
        updateDisplay(score);
        uLCD.copy_to_lcd();
        scoreWrite = 0;
      }
      uLCD.cls();
      uLCD.locate(14, 0);
      uLCD.printf("%04d", score);
      updateDisplay(score);
      uLCD.fillrect(wall1x1, wall1y1, wall1x2, wall1y2, 1);
      uLCD.fillrect(wall2x1, wall2y1, wall2x2, wall2y2, 1);
      // Draw a Sun if temperature > 40.0
      if (sensor.read() > 40.0) {
        uLCD.fillcircle(sunX, sunY, sunRadius, 1);
      }

      uLCD.copy_to_lcd();

      if (ballxpos == (wall1x1 + 10)) {
        thread.signal_set(0x1);
        score++;
        uLCD.locate(14, 0);
        uLCD.printf("%04d", score);
        updateDisplay(score);
        uLCD.copy_to_lcd();
      }
      uLCD.fillcircle(ballxpos, ballypos, ballrad, 1);
      uLCD.copy_to_lcd();
      slowDownFalling += potentiometer + 0.2;
      if (ballVel < 1 && slowDownFalling >= 1) {
        ballVel++;
        slowDownFalling = 0;
      }

      ballypos = ballypos + ballVel;
      uLCD.fillcircle(ballxpos, ballypos, ballrad, 1);
      uLCD.copy_to_lcd();

      if (ballypos - 2 < wall1y2) {
        if (ballxpos + 2 > wall1x1 && ballxpos - 2 < wall1x2) {
          if (remainingLife > 0) {
            remainingLife--;
            showRemainingLife();
          } else {
            state = over;
            sampletick.attach(&AudioSampleHit, 1.0 / sample_freq);
            wait_us(200000);
          }
        }
      } else if (ballypos + 2 > wall2y1) {
        if (ballxpos + 2 > wall2x1 && ballxpos - 2 < wall2x2) {
          if (remainingLife > 0) {
            remainingLife--;
            showRemainingLife();
          } else {
            state = over;
            sampletick.attach(&AudioSampleHit, 1.0 / sample_freq);
            wait_us(200000);
          }
        }
      }

      if (ballypos > 32 || ballypos < 0) {
        state = over;
      }

      // game is over if touch the Sun
      if (sensor.read() > 40.0 && ballxpos + ballrad >= sunX - sunRadius &&
          ballxpos - ballrad <= sunX + sunRadius &&
          ballypos - ballrad <= sunY + sunRadius) {

        state = over;
      }

      break;

    case over:
      if (ready) {
        Thread::wait(300);
        // uLCD.locate(40, 16);
        // uLCD.printf("Game Over");
        uLCD.locate(14, 0);
        uLCD.printf("%04d", score);
        updateDisplay(score);
        // uLCD.locate(40,16);
        if (sensor.read() < 40.0) {
          uLCD.locate(40, 13);
          uLCD.printf("Game Over");
          uLCD.locate(80, 2);
          uLCD.printf("T: %.2f\n", sensor.read());
        } else {
          uLCD.locate(21, 14);
          uLCD.printf("Game Over (Overheat!)");
          uLCD.locate(80, 0);
          uLCD.printf("T: %.1f\n", sensor.read());
        }
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
