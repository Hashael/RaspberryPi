#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wiringPi.h>
#include <softPwm.h>
#include <time.h>

int main (void) {
    int file;
    int const factor = 16382;
    char const i2cAddress = 0x27; // The I2C address of the ADC
    char const writingCommand[2] = {0};
    char buffer[4] = {0};
    float humidity = 0;
    float temperature = 0;

    /* Variables for timer */
    char bufferTime[26];
    time_t timer;
    struct tm* tm_info;

    /* Set up for leds */
    int intensity = 0;
    wiringPiSetupGpio();
    pinMode (18, OUTPUT); //Red Led

    pinMode (23, OUTPUT); //GREEN pin of RGB Led
    pinMode (24, OUTPUT); //BLUE pin of RGB Led
    pinMode (25, OUTPUT); //RED pin of RGB Led

    digitalWrite(23, LOW);
    digitalWrite(24, LOW);
    digitalWrite(25, LOW);

    /* Get sensor memory direction */
    char *filename = "/dev/i2c-1";
    if((file = open(filename, O_RDWR))<0){
        perror("Failed to open i2c bus");
        return(-1);
    }

    /* Open i2c communication channel */
    if (ioctl(file, I2C_SLAVE, i2cAddress) < 0) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        /* ERROR HANDLING; you can check errno to see what went wrong */
        close(file);
        return(-2);
    }

    for(;;) {
        /*WRITING SENSOR*/
        //writingCommand = 0b00011011; // Customizable writing order
        if (write(file,writingCommand,1) != 1) {
            /* ERROR HANDLING: i2c transaction failed */
            printf("Failed to write to the i2c bus.\n");
            printf("\n\n");
        }

        /*READING SENSOR*/
        // Using I2C Read
        if (read(file,buffer,4) != 4) {
            /* ERROR HANDLING: i2c transaction failed */
            printf("Failed to read from the i2c bus.\n");
            printf("\n\n");
        } else {
            /* Calculating humidity, 14 bits without the status */
            humidity = (float) (((buffer[0] & 0b00111111) << 8) + buffer[1]);
            humidity = (humidity / factor) * 100;

            /* Calculating temperature, 14 bits without the last two */
            temperature =(float) (((buffer[2] << 8) + buffer[3]) >> 2);
            temperature = (temperature / factor) * 165 - 40;

            /* Generate timestamp format */
            time(&timer);
            tm_info = localtime(&timer);
            strftime(bufferTime, 26, "%d/%m/%Y %H:%M:%S", tm_info);

            /* Print values */
            puts(bufferTime);
            printf("temperature: %0.1f\n",temperature);
            printf("humidity: %0.1f%c\n\n", humidity, '%');

            /* Switch ON red-led if temp > 27ºC */
            if (temperature > 27) {
                 digitalWrite (18, HIGH);
            } else {
                 digitalWrite (18, LOW);
            }
            /* Switch RGB Led colors on depending on the humidity */
            if (humidity < 40) {
                softPwmCreate(25,0,100);
                softPwmStop(24);
                softPwmStop(23);

                for(intensity=0; intensity<100; intensity++){
                    softPwmWrite(25,intensity);
                    delay(30);
                }
                intensity = 100;
                while (intensity != 0) {
                    softPwmWrite(25,intensity);
                    delay(15);
                    intensity = intensity - 1;
                }
            } else if (humidity > 70) {
                softPwmCreate(24,0,100);
                softPwmStop(25);
                softPwmStop(23);

                for(intensity=0; intensity<100; intensity++){
                    softPwmWrite(24,intensity);
                    delay(15);
                }
                intensity = 100;
                while (intensity != 0) {
                    softPwmWrite(24,intensity);
                    delay(15);
                    intensity = intensity - 1;
                }
            } else {
                softPwmCreate(23,0,100);
                softPwmStop(25);
                softPwmStop(24);

                for(intensity=0; intensity<100; intensity++){
                    softPwmWrite(23,intensity);
                    delay(15);
                }
                intensity = 100;
                while (intensity != 0) {
                    softPwmWrite(23,intensity);
                    delay(15);
                    intensity = intensity - 1;
                }
            }
        }
    }
    return 0;
}
