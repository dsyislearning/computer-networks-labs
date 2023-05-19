#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "datalink.h"

#define DATA_TIMER 3200
#define ACK_TIMER 2000
#define MAX_SEQ 7
#define NR_BUFS ((MAX_SEQ + 1) / 2)

typedef enum {false, true} boolean;
typedef unsigned char packet[PKT_LEN];

typedef struct {
    unsigned char kind;
    unsigned char ack;
    unsigned char seq;
    packet data;
    unsigned int crc;
} frame;

static boolean no_nak = true;
static int phl_ready = 1;

static packet out_buf[NR_BUFS];
static packet in_buf[NR_BUFS];
static boolean arrived[NR_BUFS];

static unsigned char ack_expected;
static unsigned char next_frame_to_send;
static unsigned char frame_expected;
static unsigned char too_far;
static unsigned char nbuffered;

void inc(unsigned char *seq)
{
    *seq = (*seq + 1) % (MAX_SEQ + 1);
}

boolean between(unsigned char a, unsigned char b, unsigned char c)
{
    return ((a <= b) && (b < c) || (c < a) && (a <= b) || (b < c) && (c < a));
}

void to_physical_layer(unsigned char *frame, int len)
{
    *(unsigned int *)(frame + len) = crc32(frame, len);
    send_frame(frame, len + 4);
    phl_ready = 0;
}

void put_frame(unsigned char fk, unsigned char frame_nr, unsigned char frame_expected, packet buffer[NR_BUFS])
{
    frame s;

    s.kind = fk;
    s.seq = frame_nr;
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);

    if (fk == FRAME_ACK) {
        dbg_frame("Send ACK %d\n", s.ack);
        to_physical_layer((unsigned char *)&s, 2);
    }

    if (fk == FRAME_NAK) {
        dbg_frame("Send NAK %d\n", s.ack);
        no_nak = false;
        to_physical_layer((unsigned char *)&s, 2);
    }

    if (fk == FRAME_DATA) {
        memcpy(s.data, buffer[frame_nr % NR_BUFS], PKT_LEN);
        start_timer(frame_nr, DATA_TIMER);
        dbg_frame("Send DATA %d %d, ID %d | ack_expected=%d, next_frame_to_send=%d\n",
            s.seq, s.ack, *(short *)s.data, ack_expected, next_frame_to_send);
        to_physical_layer((unsigned char *)&s, 3 + PKT_LEN);
    }

    stop_ack_timer();
}

boolean get_frame(unsigned char *r)
{
    int len = recv_frame(r, sizeof(*(frame*)r));
    if (len < 6 || crc32(r, len) != 0) {
        dbg_event("**** Receiver Error, Bad CRC Checksum\n");
        return false;
    }
    return true;
}

static frame r;

int main(int argc, char **argv)
{
    int event, arg;

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
                get_packet(out_buf[next_frame_to_send % NR_BUFS]);
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
                    dbg_frame("Recv DATA %d %d, ID %d | frame_expected=%d, too_far=%d\n",
                        r.seq, r.ack, *(short *)r.data, frame_expected, too_far);

                    if ((r.seq != frame_expected) && no_nak)
                        put_frame(FRAME_NAK, 0, frame_expected, out_buf);
                    else
                        start_ack_timer(ACK_TIMER);

                    if (between(frame_expected, r.seq, too_far) && (arrived[r.seq % NR_BUFS] == false)) {
                        arrived[r.seq % NR_BUFS] = true;
                        memcpy(in_buf[r.seq % NR_BUFS], r.data, PKT_LEN);
                        while (arrived[frame_expected % NR_BUFS] == true) {
                            put_packet(in_buf[frame_expected % NR_BUFS], PKT_LEN);
                            dbg_frame("Put PACKET %d\n", *(short *)in_buf[frame_expected % NR_BUFS]);
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
                    stop_timer(ack_expected);
                    inc(&ack_expected);
                }
                break;

            case DATA_TIMEOUT:
                dbg_event("---- DATA %d timeout\n", arg);
                put_frame(FRAME_DATA, (unsigned char)arg, frame_expected, out_buf);
                break;

            case ACK_TIMEOUT:
                dbg_event("---- ACK timeout\n");
                put_frame(FRAME_ACK, 0, frame_expected, out_buf);
                break;
        }

        if (nbuffered < NR_BUFS && phl_ready)
            enable_network_layer();
        else
            disable_network_layer();
   }
}
