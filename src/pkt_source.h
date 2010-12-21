// -*- c-basic-offset: 4; c-backslash-column: 79; indent-tabs-mode: nil -*-
// vim:sw=4 ts=4 sts=4 expandtab
#ifndef PKT_SOURCE_H_101110
#define PKT_SOURCE_H_101110

#include <stdbool.h>
#include <limits.h>
#include <stdint.h>
#include <pcap.h>
#include <pthread.h>
#include <junkie/tools/queue.h>
#include <junkie/tools/mutex.h>

/** A Packet Source is something that gives us packets (with libpcap).
 * So basically it can be either a real interface or a file.
 */
struct pkt_source {
    LIST_ENTRY(pkt_source) entry;   ///< Entry in the list of all packet sources
    char name[PATH_MAX];            ///< The name to identify this source (used for print only)
    unsigned instance;              ///< If several pkt_source uses the same name (as is frequent), distinguish them with this
    pcap_t *pcap_handle;            ///< The handle for libpcap
    pthread_t sniffer;              ///< The thread sniffing this device or file
    uint64_t nb_packets;            ///< Number of packets received from PCAP
    uint64_t nb_duplicates;         ///< Number of which that were duplicates
    uint8_t dev_id;                 ///< A numerical id which meaning is obscure
    bool is_file;                   ///< A flag to distinguish between files and ifaces
};

/** Now the frame structure that will be given to the cap parser, since
 * in addition to pcap header it also need device identifier. */
struct frame {
    struct timeval tv;  ///< timestamp of frame reception
    size_t cap_len;     ///< number of bytes captured
    size_t wire_len;    ///< number of bytes on the wire
    struct pkt_source const *pkt_source;  ///< the pkt_source this packet was read from
    uint8_t /*const*/ *data;    ///< the packet itself (FIXME: fix digest_frame then restore const)
};

void pkt_source_init(void);
void pkt_source_fini(void);

#endif
