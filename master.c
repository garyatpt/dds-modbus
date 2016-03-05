#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <modbus.h>
#include "mb.h" // generated code

#define MODBUS_SERVER_IP    "127.0.0.1"
#define MODBUS_SERVER_PORT  502
#define MODBUS_SLAVE_ID     1
#define MODBUS_TIMEOUT_SEC  3
#define MODBUS_TIMEOUT_USEC 0
#define ROTARY_ADDRESS      3
#define ROTARY_LENGTH       1
#define MAX_SAMPLES         1

// global variables
static volatile int do_loop = 1;
modbus_t *mb_ctx; // modbus context

// signal handler
static void int_handler(int dummy) 
{
    do_loop = 0;
}

// voltage AI scaling
static float voltage_scaling(int val)
{
    printf("scale: %d\n", val); // Todo: remove debug msg
    return (val - 32767.0) / 32767 * 10;
}

// init modbus context
static int modbus_init(int argc, char **argv)
{
    char * slave_ip = (argc > 1) ? argv[1] : MODBUS_SERVER_IP;
    int slave_port  = (argc > 2) ? atoi(argv[2]) : MODBUS_SERVER_PORT;
    
    mb_ctx = modbus_new_tcp(slave_ip, slave_port);
    
    modbus_set_slave(mb_ctx, MODBUS_SLAVE_ID); // set device id
    modbus_set_byte_timeout(mb_ctx, MODBUS_TIMEOUT_SEC, MODBUS_TIMEOUT_USEC); 
    modbus_set_response_timeout(mb_ctx, MODBUS_TIMEOUT_SEC, MODBUS_TIMEOUT_USEC);

    // connect to slave
    if (modbus_connect(mb_ctx) == -1) 
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(mb_ctx);
        exit(-1);
    }

    return 0;
}

// dds command arrive handler
static void data_available_handler(dds_entity_t reader) 
{
    int ret;
    dds_sample_info_t info[MAX_SAMPLES];
    void * samples[MAX_SAMPLES];
    Modbus_led sample;
    uint32_t mask = 0;

    samples[0] = &sample;
    mask = DDS_ALIVE_INSTANCE_STATE;

    // take!
    ret = dds_take(reader, samples, MAX_SAMPLES, info, mask);

    if (DDS_ERR_CHECK(ret, DDS_CHECK_REPORT)) 
    {
        printf("got dds msg: %d, %d\n", sample.id, sample.on); // TODO: remove
        // FC: 05
        ret = modbus_write_bit(mb_ctx, sample.id, sample.on);
        if (ret < 0) 
        {
            fprintf(stderr, "%s\n", modbus_strerror(errno));
        }
    }
}

int main(int argc, char *argv[]) 
{
    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);

    int status;
    uint16_t regs[1] = {0};
    
    // Initialize Modbus TCP Section
    status = modbus_init(argc, argv);

    // Initialize DDS Section
    dds_qos_t * qos = NULL;
    dds_entity_t participant;
    dds_entity_t writer_topic;  // write voltage msg
    dds_entity_t publisher;     
    dds_entity_t writer;
    Modbus_voltage writer_msg;
    writer_msg.id = ROTARY_ADDRESS;

    dds_entity_t reader_topic;  // read led trigger command
    dds_entity_t subscriber;
    dds_entity_t reader;
    dds_readerlistener_t listener;

    // Initialize DDS
    status = dds_init(argc, argv);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create participant
    status = dds_participant_create(&participant, DDS_DOMAIN_DEFAULT, qos, NULL);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create writer topic 
    status = dds_topic_create(participant, &writer_topic, &Modbus_voltage_desc, "Voltage", NULL, NULL);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create a publisher
    status = dds_publisher_create(participant, &publisher, qos, NULL);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create writer without Qos
    status = dds_writer_create(participant, &writer, writer_topic, NULL, NULL);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // ---------------------------------

    // Create reader topic
    status = dds_topic_create(participant, &reader_topic, &Modbus_led_desc, "Led", NULL, NULL);
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
    dds_qset_history (qos, DDS_HISTORY_KEEP_ALL, 0);

    status = dds_reader_create(subscriber, &reader, reader_topic, qos, &listener);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_qos_delete (qos);

    while (do_loop) 
    {
        // Modbus read: FC: 03, Addr: 3, Len: 1
        status = modbus_read_registers(mb_ctx, ROTARY_ADDRESS, ROTARY_LENGTH, regs);
        if (status < 0) 
        {
            fprintf(stderr, "%s\n", modbus_strerror(errno));
        } 
        else 
        {
            // dds write
            writer_msg.val = voltage_scaling(regs[0]); // assign from mb read
            status = dds_write(writer, &writer_msg);
            DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
            printf("write: %f\n", writer_msg.val); // todo: remove
        }
        sleep(1); // delay
    }

    // Clean up section
    dds_status_set_enabled(reader, 0); // disable reader callback
    status = dds_instance_dispose(writer, &writer_msg);
    DDS_ERR_CHECK(status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_entity_delete(participant);
    dds_fini();

    modbus_close(mb_ctx);
    modbus_free(mb_ctx);

    exit(0);
}