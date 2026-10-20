/* Legacy API name retained. This is a deterministic text baseline, not a NN.
   Bounded storage, explicit slots, no global dialogue state or room guessing. */
#include "neural_agent.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void neural_agent_init(void) {}
static bool eq(const char *a, const char *b) { return strcmp(a,b)==0; }
static void clarify(neural_parse_result_t *out, const char *why) {
    memset(out,0,sizeof(*out)); out->requires_clarification=true;
    snprintf(out->clarification_prompt,sizeof(out->clarification_prompt),"%s",why);
    snprintf(out->spoken_response,sizeof(out->spoken_response),"%s",why);
}
void neural_agent_process(const char *input, const char *active_room, neural_parse_result_t *out) {
    (void)active_room; // A browser request is not evidence of the user's location.
    if (!out) return;
    clarify(out,"Specify one action, room, and device.");
    if (!input) return;
    size_t n=strnlen(input,128);
    if (!n || n>=128) return;
    char text[128];
    for(size_t i=0;i<=n;++i) {
        unsigned char c=(unsigned char)input[i];
        if(c>126) return;
        text[i]=(char)tolower(c);
        if(c=='_' || c==',' || c=='.' || c=='!' || c=='?') text[i]=' ';
    }
    char *tokens[24]; size_t count=0; char *p=text;
    while(*p) {
        while(*p==' ') ++p;
        if(!*p) break;
        if(count==24) return;
        tokens[count++]=p;
        while(*p && *p!=' ') ++p;
        if(*p) *p++=0;
    }
    const char *action=NULL,*room=NULL,*device=NULL;
    unsigned actions=0,rooms=0,devices=0;
    for(size_t i=0;i<count;++i) {
        const char *w=tokens[i];
        if(eq(w,"not") || eq(w,"don't") || eq(w,"dont") || eq(w,"never") || eq(w,"unless") || eq(w,"if")) {
            clarify(out,"Conditional or negative commands need clarification."); return;
        }
        if(eq(w,"on") || eq(w,"off") || eq(w,"toggle")) {
            action=eq(w,"on")?"ON":eq(w,"off")?"OFF":"TOGGLE"; ++actions;
        } else if(eq(w,"bedroom") || eq(w,"kitchen") || eq(w,"office")) { room=w; ++rooms; }
        else if(eq(w,"living") && i+1<count && eq(tokens[i+1],"room")) { room="living_room"; ++rooms; ++i; }
        else if(eq(w,"light") || eq(w,"lamp") || eq(w,"fan")) { device=eq(w,"fan")?"fan":"light"; ++devices; }
        else if(!eq(w,"turn") && !eq(w,"switch") && !eq(w,"the") && !eq(w,"in") && !eq(w,"please")) return;
    }
    if(actions!=1 || rooms!=1 || devices!=1) return;
    memset(out,0,sizeof(*out)); out->intent=INTENT_DEVICE_CONTROL;
    snprintf(out->action,sizeof(out->action),"%s",action);
    snprintf(out->room,sizeof(out->room),"%s",room);
    snprintf(out->device,sizeof(out->device),"%s",device);
    out->confidence=1.0f; // Exact grammar match flag, not a calibrated probability.
    snprintf(out->spoken_response,sizeof(out->spoken_response),"Proposed %s %s in %s.",action,device,room);
}
bool neural_agent_selftest(void) {
    neural_parse_result_t r;
    neural_agent_process("turn on office light",NULL,&r);
    if(r.requires_clarification || !eq(r.action,"ON") || !eq(r.room,"office")) return false;
    const char *rejected[]={"don't turn on bedroom light","turn on light","turn on bedroom","turn on bedroom fan and kitchen light","turn on bedroom light tomorrow","turn onoff bedroom light"};
    for(size_t i=0;i<sizeof(rejected)/sizeof(rejected[0]);++i) {
        neural_agent_process(rejected[i],"bedroom",&r);
        if(!r.requires_clarification || r.action[0]) return false;
    }
    return true;
}
