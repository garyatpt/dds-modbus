#include <mosquitto.h>
#include <signal.h>
#include <unistd.h>
#include "mb.h" // generated code

// DDS
#define MAX_SAMPLES 1

// MQTT 
#define MQTT_HOST       "127.0.0.1"
#define MQTT_PORT       1883
#define MQTT_Voltage    "/sensors/rotary/3"
#define MQTT_LED_RED    "/sensors/led/16"
#define MQTT_LED_GREEN  "/sensors/led/17"
#define MQTT_LED_ALL    "/sensors/led/+"

// global variables
static volatile int do_loop = 1;
struct mosquitto *mosq; // mosquitto instance
dds_entity_t writer;    // dds writer
Modbus_led writer_msg;  // dds: led write message

// signal handler
static void int_handler(int dummy) 
{
    do_loop = 0;
}

// dds data arrive handler (rotary)
static void data_available_handler(dds_entity_t reader) 
{
    int ret;
    dds_sample_info_t info[MAX_SAMPLES];
    void * samples[MAX_SAMPLES];
    Modbus_voltage sample;
    uint32_t mask = 0;

    samples[0] = &sample;
    mask = DDS_ALIVE_INSTANCE_STATE;
    // take!
    ret = dds_take(reader, samples, MAX_SAMPLES, info, mask);

    char val[10];
    
    if (DDS_ERR_CHECK(ret, DDS_CHECK_REPORT)) 
    {
        printf("got dds data: %d, %f\n", sample.id, sample.val);
        sprintf(val, "%f", sample.val);
        // pub voltage value to mqtt broker
        mosquitto_publish(mosq, NULL, MQTT_Voltage, strlen(val), val, 0, false);
    }
}

// mqtt connection callback
static void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    printf("mqtt connected, rc=%d\n", result);
}

// mqtt subscriber callback (led command)
static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    bool match = 0;
    int status = 0;
    writer_msg.id = 0;
    
    mosquitto_topic_matches_sub(MQTT_LED_RED, message->topic, &match); 
    if (match) 
    {
        printf("got mqtt cmd: %s for topic %s\n", (char*) message->payload, message->topic);
        writer_msg.id = 16;
    }

    mosquitto_topic_matches_sub(MQTT_LED_GREEN, message->topic, &match); 
    if (match) 
    {
        printf("got mqtt cmd: %s for topic %s\n", (char*) message->payload, message->topic);
        writer_msg.id = 17;
    }

    if (writer_msg.id != 0)
    {
        writer_msg.on = atoi(message->payload); // void pointer to int
        status = dds_write(writer, &writer_msg);
        DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
        dds_sleepfor(DDS_MSECS(10));
        printf("write to dds: id: %d, on: %d\n", writer_msg.id, writer_msg.on);
    }
}

// init mosquitto context
static void mosquitto_init(void)
{
    char clientid[24];
    mosquitto_lib_init();
    memset(clientid, 0, 24);
    snprintf(clientid, 23, "helloworld_%d", getpid());
    mosq = mosquitto_new(clientid, true, 0);
    
    // set callback
    if (mosq)
    {
        mosquitto_connect_callback_set(mosq, connect_callback);
        mosquitto_message_callback_set(mosq, message_callback);
    }
}

int main(int argc, char *argv[]) {
    
    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);

    int status = 0;

    mosquitto_init(); // init mosquitto instance

    // dds pub
    dds_qos_t * qos = NULL;
    dds_entity_t participant;
    dds_entity_t writer_topic;
    dds_entity_t publisher;     

    // dds sub
    dds_entity_t reader_topic;
    dds_entity_t subscriber;
    dds_entity_t reader;
    dds_readerlistener_t listener;

    // Initialize DDS
    status = dds_init(argc, argv);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create participant
    status = dds_participant_create(&participant, DDS_DOMAIN_DEFAULT, qos, NULL);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create writer topic (led)
    status = dds_topic_create(participant, &writer_topic, &Modbus_led_desc, "Led", NULL, NULL);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create a publisher
    status = dds_publisher_create(participant, &publisher, qos, NULL);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create writer without Qos
    status = dds_writer_create(participant, &writer, writer_topic, NULL, NULL);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_sleepfor(DDS_SECS(1)); // delay 1 second

    // Create reader topic (voltage)
    status = dds_topic_create(participant, &reader_topic, &Modbus_voltage_desc, "Voltage", NULL, NULL);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create a subscriber
    status = dds_subscriber_create(participant, &subscriber, qos, NULL);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    
    // data available callback
    memset(&listener, 0, sizeof (listener));
    listener.on_data_available = data_available_handler;
    
    // Create reader with DATA_AVAILABLE status condition enabled
    qos = dds_qos_create();
    dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_SECS (1));
    dds_qset_history(qos, DDS_HISTORY_KEEP_ALL, 0);

    status = dds_reader_create(subscriber, &reader, reader_topic, qos, &listener);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_qos_delete(qos);
    
    if (mosq) 
    {
        // read mqtt ip & port from console
        char * mqtt_ip = (argc > 1) ? argv[1] : MQTT_HOST;
        int mqtt_port  = (argc > 2) ? atoi(argv[2]) : MQTT_PORT;
        status = mosquitto_connect(mosq, mqtt_ip, mqtt_port, 60);
        mosquitto_subscribe(mosq, NULL, MQTT_LED_ALL, 0);

        while (do_loop) 
        {
            // handle subscribe
            status = mosquitto_loop(mosq, -1, 1);
            
            if (do_loop && status)
            {
                printf("connection error!\n");
                sleep(3);
                mosquitto_reconnect(mosq);
                mosquitto_subscribe(mosq, NULL, MQTT_LED_ALL, 0);
            }
            sleep(0.1);
        }
    }

    // cleanup
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    dds_status_set_enabled(reader, 0); // disable reader callback
    status = dds_instance_dispose(writer, &writer_msg);
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_entity_delete(participant);
    dds_fini();
    exit(0);
}