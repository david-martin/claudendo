#include "config.h"
#include <stdio.h>
#include <string.h>
bool config_load(Config *c){
    FILE *f = fopen("sdmc:/claudendo/config.cfg","r");
    if(!f) return false;
    bool ok = fgets(c->host,sizeof c->host,f) && fgets(c->token,sizeof c->token,f);
    fclose(f);
    if(!ok) return false;
    c->host[strcspn(c->host,"\r\n")]=0;
    c->token[strcspn(c->token,"\r\n")]=0;
    return c->host[0] && c->token[0];
}
