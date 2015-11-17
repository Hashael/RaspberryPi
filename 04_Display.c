#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wiringPi.h>
#include <softPwm.h>

#include <geniePi.h>  //the ViSi-Genie-RaspPi library

int temperatureAlarm = 27;

//This a thread for handling the clock
static void *handleClock(void *data)
{
    /* Variables for timer */
    int seconds = 0;
    int hours = 0;
    time_t timer;
    struct tm* tm_info;

    for(;;)             //infinite loop
    {
        //Generate System time and convert to int
        time(&timer);
        tm_info = localtime(&timer);
        seconds = tm_info->tm_sec;
        hours = (tm_info->tm_hour * 100) + tm_info->tm_min;

        //write to Clock
        genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x00, seconds);
        genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x01, hours);

        usleep(1000000);    //1 second delay

    }
    return NULL;
}

//This a thread for handling the sensor and LEDs
static void *handleSensor(void *data)
{
    int file;
    int const factor = 16382;
    char const i2cAddress = 0x27; // The I2C address of the ADC
    char const writingCommand[2] = {0};
    char buffer[4] = {0};
    char str[35]; //the maxium characters are around 31
    float humidity = 0;
    float temperature = 0;

    /* Set up for leds */
    int intensity = 0;
    wiringPiSetupGpio();
    pinMode (18, OUTPUT); //Red Led

    pinMode (23, OUTPUT); //GREEN pin of RGB Led
    pinMode (24, OUTPUT); //BLUE pin of RGB Led
    pinMode (25, OUTPUT); //RED pin of RGB Led

    //Initialize pins to 0
    digitalWrite(23, LOW);
    digitalWrite(24, LOW);
    digitalWrite(25, LOW);

    /* Get sensor memory direction */
    char *filename = "/dev/i2c-1";
    if((file = open(filename, O_RDWR))<0){
        perror("Failed to open i2c bus");
        return NULL;
    }

    /* Open i2c communication channel */
    if (ioctl(file, I2C_SLAVE, i2cAddress) < 0) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        /* ERROR HANDLING; you can check errno to see what went wrong */
        close(file);
        return NULL;
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

            /* Print values to gauges */
            genieWriteObj(GENIE_OBJ_ANGULAR_METER, 0x00, temperature);
            genieWriteObj(GENIE_OBJ_METER, 0x00, humidity);

            /* Print values to text */
            sprintf(str, "Temperature: %0.1fC\nHumidity: %0.1f%%", temperature, humidity);
            genieWriteStr(0x00,str);

            /* Switch ON red-led if temp > 27ºC */
            if (temperature > 27) {
                 digitalWrite (18, HIGH);
            } else {
                 digitalWrite (18, LOW);
            }

            /* Switch ON software red-led if temp > alarm */
            if (temperature > temperatureAlarm) {
                 genieWriteObj(GENIE_OBJ_USER_LED, 0x00, 1);
            } else {
                 genieWriteObj(GENIE_OBJ_USER_LED, 0x00, 0);
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
    return NULL;
}

//This is the event handler. Messages received from the display
//are processed here.
void handleGenieEvent(struct genieReplyStruct * reply)
{
    if(reply->cmd == GENIE_REPORT_EVENT)    //check if the cmd byte is a report event
    {
        if(reply->object == GENIE_OBJ_KNOB) //check if the object byte is that of a slider
        {
            temperatureAlarm = reply->data;
            genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x02, temperatureAlarm);
        }
    }

    //if the received message is not a report event, print a message on the terminal window
    else
    printf("Unhandled event: command: %2d, object: %2d, index: %d, data: %d \r\n", reply->cmd, reply->object, reply->index, reply->data);
}

int main()
{
    pthread_t myClockThread;              //declare a thread for the clock
    pthread_t mySensorThread;             //declare a thread for the sensor and lights
    struct genieReplyStruct reply ;       //declare a genieReplyStruct type structure

    //print some information on the terminal window
    printf("\n\n");
    printf("Visi-Genie-Raspberry-Pi temperature & humidity project\n");
    printf("by Rubén Dominguez & Ignacio del Pozo\n");
    printf("=====================================\n");
    printf("Program is running. Press Ctrl + C to close.\n");

    //open the Raspberry Pi's onboard serial port, baud rate is 115200
    //make sure that the display module has the same baud rate
    genieSetup("/dev/ttyAMA0", 115200);
    genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x02, temperatureAlarm);
    genieWriteObj(GENIE_OBJ_KNOB, 0x00, temperatureAlarm);

    //start the thread for writing to the clock
    (void)pthread_create (&myClockThread,  NULL, handleClock, NULL);
    (void)pthread_create (&mySensorThread,  NULL, handleSensor, NULL);

    for(;;)                         //infinite loop
    {
        while(genieReplyAvail())      //check if a message is available
        {
            genieGetReply(&reply);      //take out a message from the events buffer
            handleGenieEvent(&reply);   //call the event handler to process the message
        }
        usleep(10000);                //10-millisecond delay.Don't hog the
    }	                          //CPU in case anything else is happening
    return 0;
}
