#include "runtime_selftest.h"
#include "safety_selftest.h"
#include "automation_selftest.h"
#include "house_selftest.h"
#include "evidence_selftest.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static bool lease_boundaries() {
    ControlCommand c; c.issued_ms=0xffffff00u; c.ttl_ms=1000;
    if(!control_fresh(c,c.issued_ms-100) || control_fresh(c,c.issued_ms-101) ||
       !control_fresh(c,c.issued_ms+1000) || control_fresh(c,c.issued_ms+1001)) return false;
    uint8_t bytes[kControlBytes]; c.id=1; encode_control(c,bytes); ControlCommand decoded;
    if(!decode_control(bytes,sizeof(bytes),decoded) || decode_control(bytes,16,decoded)) return false;
    wire_put32(bytes+20,5001); if(decode_control(bytes,sizeof(bytes),decoded)) return false;
    wire_put32(bytes+20,99); return !decode_control(bytes,sizeof(bytes),decoded);
}
static bool sensor_boundaries() {
    SimInput input; input.boot=1; input.sequence=1; input.issued_ms=0xffffff00u;
    uint8_t bytes[kSimInputBytes]; encode_sim_input(input,bytes); SimInput decoded;
    if(!decode_sim_input(bytes,sizeof(bytes),decoded)) return false;
    bytes[16]=64; if(decode_sim_input(bytes,sizeof(bytes),decoded)) return false;
    bytes[16]=0; bytes[17]=2; bytes[18]=89; // 601 deci-Celsius
    if(decode_sim_input(bytes,sizeof(bytes),decoded)) return false;
    safety_kernel_init();
    safety_kernel_register_device("bedroom","light",DEVICE_TYPE_MOCK,-1,false);
    safety_kernel_register_device("bedroom","fan",DEVICE_TYPE_MOCK,-1,false);
    HouseSimulation house; if(!house.begin()) return false;
    if(!house.input(input,1,0x100u)) return false;
    input.sequence=2; input.issued_ms=100;
    if(house.input(input,1,3101) || house.input(input,1,0xffffffffu)) return false;
    // Dropout does not erase a human manual hold. The state remains reported,
    // while the evidence becomes unknown; these are different observations.
    ControlCommand c; c.device=0; c.action=1;
    if(house.manual(c,10000)!=ControlStatus::Applied) return false;
    auto state=house.tick(1,14000);
    return state.known==0 && state.outputs==1;
}
int main(int argc,char **argv) {
    if(argc!=2) return 2;
    bool passed=false;
    if(!strcmp(argv[1],"runtime")) passed=runtime_selftest();
    else if(!strcmp(argv[1],"safety")) passed=safety_boot_selftest();
    else if(!strcmp(argv[1],"automation")) passed=automation_selftest();
    else if(!strcmp(argv[1],"house")) passed=house_selftest();
    else if(!strcmp(argv[1],"leases")) passed=lease_boundaries();
    else if(!strcmp(argv[1],"sensor_boundaries")) passed=sensor_boundaries();
    else if(!strncmp(argv[1],"evidence_",9)) passed=evidence_case((unsigned)atoi(argv[1]+9));
    printf("{\"source\":\"HOST_MEASURED\",\"case\":\"%s\",\"passed\":%s}\n",argv[1],passed?"true":"false");
    return passed?0:1;
}
