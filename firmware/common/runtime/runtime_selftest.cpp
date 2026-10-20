#include "runtime_selftest.h"
#include "bounded_arena.h"
#include "clawgrid_protocol.h"
#include "state_wire.h"
#include "pir_tracker.h"
#include "policy_record.h"
#include "control_wire.h"
#include "evidence_selftest.h"
#include <string.h>
#include <stdint.h>

/* Hardware-independent checks of the exact code compiled into each board.
   The matching Python test fixes the wire vector independently. */
bool runtime_selftest() {
    if(evidence_campaign()!=((1u<<kEvidenceCases)-1)) return false;
    PolicyConfig policy,restored; uint8_t record[kPolicyRecordBytes]; encode_policy(policy,record);
    if(!decode_policy(record,sizeof(record),restored) || restored!=policy || decode_policy(record,sizeof(record)-1,restored)) return false;
    for(size_t bit=0;bit<sizeof(record)*8;++bit) {
        record[bit/8]^=1u<<(bit%8);
        if(decode_policy(record,sizeof(record),restored)) return false;
        record[bit/8]^=1u<<(bit%8);
    }
    policy.grace_ms=29999; encode_policy(policy,record);
    if(decode_policy(record,sizeof(record),restored)) return false;
    ControlCommand command; command.issued_ms=0xffffff00u; command.ttl_ms=1000;
    if(!control_fresh(command,0x100u) || control_fresh(command,0x400u) || control_fresh(command,command.issued_ms-101)) return false;
    const uint8_t golden[] = {0x43,0x47,2,2,0,0,0,1,0,2,1,2,3,4,0,3,0x0b,0xa1,'a','b','c'};
    uint8_t packet[CLAWGRID_MAX_PACKET] = {};
    size_t n = clawgrid_serialize(MSG_TYPE_HEARTBEAT, 1, 2, 0x01020304, 0,
                                 (const uint8_t *)"abc", 3, packet, sizeof(packet));
    if (n != sizeof(golden) || memcmp(packet, golden, n)) return false;
    clawgrid_header_t h; const uint8_t *payload = nullptr;
    if (!clawgrid_deserialize(golden, sizeof(golden), &h, &payload) ||
        h.seq_num != 0x01020304 || h.src_node != 1 || memcmp(payload, "abc", 3)) return false;
    packet[n-1] ^= 1;
    if (clawgrid_deserialize(packet, n, &h, &payload) || payload != nullptr) return false;
    if (clawgrid_serialize(MSG_TYPE_HEARTBEAT, 1, 2, 0, 0, nullptr, 1, packet, sizeof(packet))) return false;
    uint8_t full[CLAWGRID_MAX_PAYLOAD] = {};
    if (clawgrid_serialize(MSG_TYPE_HEARTBEAT,1,2,0,0,full,sizeof(full),packet,sizeof(packet)) != sizeof(packet)) return false;
    if (!clawgrid_deserialize(packet,sizeof(packet),&h,&payload)) return false;
    if (clawgrid_deserialize(packet,sizeof(packet)-1,&h,&payload)) return false;

    alignas(16) uint8_t storage[64]; BoundedArena arena; arena.attach(storage, sizeof(storage));
    if (arena.allocate(1) != storage || arena.allocate(48) != storage+16 ||
        arena.allocate(1) != nullptr || arena.used() != 64 || arena.peak() != 64) return false;
    arena.reset();
    if (arena.allocate(SIZE_MAX) || arena.allocate(1,3) || arena.allocate(0) || arena.used()) return false;
    if (arena.allocate(64) != storage) return false;

    RoomReport s; s.motion = true; s.light = true; s.occupancy = Occupancy::Occupied;
    s.boot_id = 0x01020304; s.uptime_ms = 0xffff0000;
    uint8_t p[kRoomReportBytes]; encode_room_report(s,p); RoomReport decoded;
    if (!decode_room_report(p,sizeof(p),decoded) || decoded.boot_id != s.boot_id ||
        decoded.uptime_ms != s.uptime_ms || !decoded.light) return false;
    p[4] = 2;
    if (decode_room_report(p,sizeof(p),decoded)) return false;
    PirTracker pir;
    if (pir.sample(true,10) || pir.sample(false,100) || pir.motion()) return false;
    pir.sample(true,0xffffff00u);
    if (!pir.sample(true,0xffffffc8u) || !pir.motion()) return false;
    pir.sample(false,0xffffffd0u);
    if (!pir.sample(false,0x98u) || pir.motion()) return false;
    if (pir.occupancy(0x98u,120000) != Occupancy::Occupied ||
        pir.occupancy(0xffffffd0u+120000u,120000) != Occupancy::Vacant) return false;
    return sequence_newer(0,0xffffffff) && !sequence_newer(10,10) && !sequence_newer(9,10);
}
