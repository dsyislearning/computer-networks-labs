#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "datalink.h"

#define DATA_TIMER 2000
#define ACK_TIMER 2000
#define MAX_PKT 256
#define MAX_SEQ 15
#define NR_BUFS ((MAX_SEQ + 1) / 2)

typedef enum {false, true} boolean;
typedef unsigned char frame_kind;
typedef unsigned char seq_nr;
typedef struct {unsigned char data[MAX_PKT];} packet;

typedef struct {
    frame_kind kind;
    seq_nr ack;
    seq_nr seq;
    packet info;
    unsigned int crc;
} frame;

static boolean no_nak = true;
static int phl_ready = 1;
static int len;

static packet out_buf[NR_BUFS];
static packet in_buf[NR_BUFS];
static boolean arrived[NR_BUFS];

void inc(seq_nr *seq)
{
    *seq = (*seq + 1) % (MAX_SEQ + 1);
}

boolean between(seq_nr a, seq_nr b, seq_nr c)
{
    return ((a <= b) && (b < c) || (c < a) && (a <= b) || (b < c) && (c < a));
}

void to_physical_layer(unsigned char *frame)
{
    *(unsigned int *)(frame + len) = crc32(frame, len);
    send_frame(frame, len + 4);
    phl_ready = 0;
}

void put_frame(frame_kind fk, seq_nr frame_nr, seq_nr frame_expected, packet buffer[])
{
    frame s;

    s.kind = fk;
    s.seq = frame_nr;
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);

    if (fk == FRAME_ACK) {
        dbg_frame("Send ACK %d\n", s.ack);
        len = 2;
    }

    if (fk == FRAME_NAK) {
        dbg_frame("Send NAK %d\n", s.ack);
        len = 2;
        no_nak = false;
    }

    if (fk == FRAME_DATA) {
        len = 3 + PKT_LEN;
        s.info = buffer[frame_nr % NR_BUFS];
        start_timer(frame_nr % NR_BUFS, DATA_TIMER);
        dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.info.data);
    }

    to_physical_layer((unsigned char *)&s);

    stop_ack_timer();
}

boolean get_frame(unsigned char *r)
{
    len = recv_frame(r, sizeof(*(frame*)r));
    if (len < 5 || crc32(r, len) != 0) {
        dbg_event("**** Receiver Error, Bad CRC Checksum\n");
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    int event, arg;
    seq_nr ack_expected;
    seq_nr next_frame_to_send;
    seq_nr frame_expected;
    seq_nr too_far;
    seq_nr nbuffered;
    frame r;

    protocol_init(argc, argv); 
    lprintf("Designed by Deng Siyang, build: " __DATE__"  "__TIME__"\n");

    ack_expected = 0;
    next_frame_to_send = 0;
    frame_expected = 0;
    too_far = NR_BUFS;
    nbuffered = 0;
    for (int i = 0; i < NR_BUFS; i++) arrived[i] = false;

    enable_network_layer();

    while (true) {
        event = wait_for_event(&arg);

        switch (event) {
            case NETWORK_LAYER_READY:
                get_packet((unsigned char *)&out_buf[next_frame_to_send % NR_BUFS]);
                put_frame(FRAME_DATA, next_frame_to_send, frame_expected, out_buf);
                inc(&next_frame_to_send);
                nbuffered++;
                break;

            case PHYSICAL_LAYER_READY:
                phl_ready = 1;
                break;

            case FRAME_RECEIVED:
                if (get_frame((unsigned char *)&r) == false) {
                    if (no_nak)
                        put_frame(FRAME_NAK, 0, frame_expected, out_buf);
                    break;
                }

                if (r.kind == FRAME_DATA) {
                    dbg_frame("Recv DATA %d %d, ID %d\n", r.seq, r.ack, *(short *)r.info.data);
                    
                    if ((r.seq != frame_expected) && no_nak)
                        put_frame(FRAME_NAK, 0, frame_expected, out_buf);
                    else
                        start_ack_timer(ACK_TIMER);

                    if (between(frame_expected, r.seq, too_far) && (arrived[r.seq % NR_BUFS] == false)) {
                        arrived[r.seq % NR_BUFS] = true;
                        in_buf[r.seq % NR_BUFS] = r.info;
                        while (arrived[frame_expected % NR_BUFS] == true) {
                            put_packet(in_buf[frame_expected % NR_BUFS].data, PKT_LEN);
                            no_nak = true;
                            arrived[frame_expected % NR_BUFS] = false;
                            inc(&frame_expected);
                            inc(&too_far);
                            start_ack_timer(ACK_TIMER);
                        }
                    }
                }

                if ((r.kind == FRAME_NAK) && between(ack_expected, (r.ack + 1) % (MAX_SEQ + 1), next_frame_to_send)) {
                    dbg_frame("Recv NAK %d\n", r.ack);
                    put_frame(FRAME_DATA, (r.ack + 1) % (MAX_SEQ + 1), frame_expected, out_buf);
                }

                while (between(ack_expected, r.ack, next_frame_to_send)) {
                    dbg_frame("Recv ACK %d\n", r.ack);
                    nbuffered--;
                    stop_timer(ack_expected % NR_BUFS);
                    inc(&ack_expected);
                }
                break;

            case DATA_TIMEOUT:
                dbg_event("---- DATA %d timeout\n", arg);
                put_frame(FRAME_DATA, (unsigned char)arg, frame_expected, out_buf);
                break;

            case ACK_TIMEOUT:
                dbg_event("---- ACK %d timeout\n", arg);
                put_frame(FRAME_ACK, 0, frame_expected, out_buf);
                break;
        }

        if (nbuffered < NR_BUFS && phl_ready)
            enable_network_layer();
        else
            disable_network_layer();
   }
}
