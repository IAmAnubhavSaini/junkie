// -*- c-basic-offset: 4; c-backslash-column: 79; indent-tabs-mode: nil -*-
// vim:sw=4 ts=4 sts=4 expandtab
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include "lib.h"

struct dynports *dynports = NULL;

static uint8_t *random_buffer(size_t *size, size_t min_size, size_t max_size)
{
    *size = min_size + (rand() % (max_size - min_size));
    uint8_t *buf = malloc(*size);
    assert(buf);

    unsigned i;
    for (i = 0; i < *size; i++) {
        buf[i] = rand() & 0xff;
    }

    return buf;
}

void stress_check(struct proto *proto)
{
    struct timeval now;
    timeval_set_now(&now);
    log_set_level(LOG_ERR, NULL);
    srand(time(NULL));

    struct parser *parser = proto->ops->parser_new(proto, &now);
    assert(parser);

    for (unsigned nb_tests = 0; nb_tests < 10000 ; nb_tests ++) {
        size_t size;
        uint8_t *buf = random_buffer(&size, 20, 60);
        parser->proto->ops->parse(parser, NULL, rand()%1, buf, size, size, &now, NULL);
        free(buf);
    }

    parser_unref(parser);
}

/*
 * Build fake IP traffic
 */

#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

int iph_ctor(void *ip_, size_t len, uint32_t src, uint32_t dst)
{
    struct iphdr *ip = ip_;
    if (len < sizeof(*ip)) return -1;

    ip->ihl = sizeof(*ip)/4;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(len);
    ip->id = 0;
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->check = 0x1234;
    ip->saddr = htonl(src);
    ip->daddr = htonl(dst);

    return 0;
}

int udph_ctor(void *udp_, size_t len, uint16_t src, uint16_t dst)
{
    struct udphdr *udp = udp_;
    if (len < sizeof(*udp)) return -1;

    udp->source = htons(src);
    udp->dest = htons(dst);
    udp->len = htons(len);
    udp->check = 0x1234;

    return 0;
}

int udp_ctor_random(void *packet, size_t len)
{
    if (0 != iph_ctor(packet, len, rand(), rand())) return -1;
    return udph_ctor((void *)((struct iphdr *)packet+1), len - sizeof(struct iphdr), rand(), rand());
}

int tcph_ctor(void *tcp_, size_t len, uint16_t src, uint16_t dst, uint32_t seqnum, uint32_t acknum, bool syn, bool fin, bool rst, bool ack)
{
    struct tcphdr *tcp = tcp_;
    if (len < sizeof(*tcp)) return -1;

    tcp->source = htons(src);
    tcp->dest = htons(dst);
    tcp->seq = htonl(seqnum);
    tcp->ack_seq = htonl(acknum);
    tcp->res1 = tcp->psh = tcp->urg = tcp->res2 = 0;
    tcp->doff = sizeof(*tcp)/4;  // no options
    tcp->syn = syn;
    tcp->fin = fin;
    tcp->ack = ack;
    tcp->rst = rst;
    tcp->window = 0x8000;
    tcp->check = 0x1234;
    tcp->urg_ptr = 0;

    return 0;
}

int tcp_ctor_random(void *packet, size_t len)
{
    if (0 != iph_ctor(packet, len, rand(), rand())) return -1;
    return tcph_ctor((void *)((struct iphdr *)packet+1), len - sizeof(struct iphdr), rand(), rand(), rand(), rand(), rand()&1, rand()&1, rand()&1, rand()&1);
}

int tcp_stream_ctor(struct tcp_stream *stream, size_t len, size_t mtu, uint16_t service_port)
{
    stream->packet = malloc(mtu);
    if (! stream->packet) return -1;

    stream->mtu = mtu;
    stream->len = len;
    stream->past_len[0] = stream->past_len[1] = 0;
    stream->isn[0] = rand();
    stream->isn[1] = rand();
    stream->ip[0] = rand();
    stream->ip[1] = rand();
    stream->port[0] = rand();
    stream->port[1] = service_port;
    stream->fin_acked[0] = false;
    stream->fin_acked[1] = false;

    return 0;
}

void tcp_stream_dtor(struct tcp_stream *stream)
{
    free(stream->packet);
}

static ssize_t stream_packet(struct tcp_stream *stream, int way)
{
    bool syn = stream->past_len[way] == 0;
    size_t payload = syn ? 0 : stream->mtu - 100 /* enought room for IP + TCP headers */;
    bool fin;

    if (stream->past_len[way] >= stream->len) { // we are already fined, just ack
        syn = false;
        fin = false;
        payload = 0;
    } else {
        fin = stream->past_len[way] + payload >= stream->len;
        if (fin) {
            payload = stream->len - stream->past_len[way];
        }
    }

    size_t const stream_len = syn + fin + payload;
    size_t packet_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + payload;

    if (0 != iph_ctor(stream->packet, packet_len, stream->ip[way], stream->ip[!way])) return -1;

    if (0 != tcph_ctor((void *)((struct iphdr *)stream->packet+1),
        sizeof(struct tcphdr) + payload,
        stream->port[way], stream->port[!way],
        stream->isn[way] + stream->past_len[way],
        stream->isn[!way] + stream->past_len[!way],
        syn, fin,
        false,
        stream->past_len[!way] > 0)) return -1;

    stream->past_len[way] += stream_len;
    if (stream->past_len[!way] >= stream->len) stream->fin_acked[!way] = true;

    return packet_len;
}

ssize_t tcp_stream_next(struct tcp_stream *stream, unsigned *way_)
{
    unsigned way;

    if (stream->past_len[0] >= stream->len) {
        if (stream->past_len[1] >= stream->len) {
            if (! stream->fin_acked[0]) {
                way = 1;
            } else if (! stream->fin_acked[1]) {
                way = 0;
            } else {    // nothing left to be done
                return 0;
            }
        } else {
            way = 1;
        }
    } else if (stream->past_len[1] >= stream->len) {
        way = 0;
    } else if (stream->past_len[0] == 0 && stream->past_len[1] == 0) {
        way = 0;    // we want client to be way=0
    } else if (stream->past_len[0] == 1 && stream->past_len[1] == 0) {
        way = 1;    // synack
    } else if (stream->past_len[0] == 1 && stream->past_len[1] == 1) {
        way = 0;    // ack of synack (+ first datas)
    } else {
        way = !!(rand() & 0x100);
    }

    if (way_) *way_ = way;

    return stream_packet(stream, way);
}

void gsubr_init() {}

