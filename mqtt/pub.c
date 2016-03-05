/*
gcc pub.c -lmosquitto -o pub
*/

#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <mosquitto.h>

#define MQTT_HOST "192.168.0.107"
#define MQTT_PORT 1883

static int run = 1;

void handle_signal(int s)
{
    run = 0;
}

void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    printf("connect callback, rc=%d\n", result);
}

int main(int argc, char *argv[])
{
    uint8_t reconnect = true;
    char clientid[24];
    struct mosquitto *mosq;
    int rc = 0;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    mosquitto_lib_init();

    memset(clientid, 0, 24);
    snprintf(clientid, 23, "helloworld_%d", getpid());
    mosq = mosquitto_new(clientid, true, 0);

    if (mosq) 
    {
        mosquitto_connect_callback_set(mosq, connect_callback);

        rc = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60);

        while (run) 
        {
            rc = mosquitto_loop(mosq, -1, 1);
            mosquitto_publish(mosq, NULL, "/device/1", strlen("HELLO"), "HELLO", 0, false);
            sleep(2);
            
            if (run && rc) 
            {
                printf("connection error!\n");
                sleep(10);
                mosquitto_reconnect(mosq);
            }
        }
        mosquitto_destroy(mosq);
    }

    mosquitto_lib_cleanup();

    return rc;
}
