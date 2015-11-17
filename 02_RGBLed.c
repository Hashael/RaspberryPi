#include <wiringPi.h>
#include <softPwm.h>

int main (void) {
    int intensity = 0;
    wiringPiSetupGpio();

    pinMode (18, PWM_OUTPUT);
    pinMode (23, OUTPUT);
    pinMode (24, OUTPUT);

    for(;;){
         for(intensity=0; intensity<1024; intensity++){
            pwmWrite(18,intensity);
            delay(1);
        }
        pwmWrite(18,0);
        softPwmCreate(23,0,100);
        for(intensity=0; intensity<100; intensity++){
            softPwmWrite(23,intensity);
            delay(20);
        }
        softPwmStop(23);
        softPwmCreate(24,0,100);
        for(intensity=0; intensity<100; intensity++){
            softPwmWrite(24,intensity);
            delay(20);
        }
        softPwmStop(24);

    }
    return 0;
}
