#include "config.h"
#include <stdio.h>
#include <string.h>

// Host/token normally come from sdmc:/claudendo/config.cfg (line 1 = host,
// line 2 = token). For a self-contained "shareable" build they can be baked in
// at compile time via -DCLAUDENDO_HOST / -DCLAUDENDO_TOKEN (see the Makefile).
// NOTE: a baked build embeds the relay token in the binary — never commit it,
// and treat that .3dsx/.cia as sensitive. Baked values take priority; if absent
// we fall back to the SD config.
bool config_load(Config *c){
#if defined(CLAUDENDO_HOST) && defined(CLAUDENDO_TOKEN)
    strncpy(c->host,  CLAUDENDO_HOST,  sizeof c->host  - 1); c->host[sizeof c->host - 1] = 0;
    strncpy(c->token, CLAUDENDO_TOKEN, sizeof c->token - 1); c->token[sizeof c->token - 1] = 0;
    if (c->host[0] && c->token[0]) return true;
#endif
    FILE *f = fopen("sdmc:/claudendo/config.cfg","r");
    if(!f) return false;
    bool ok = fgets(c->host,sizeof c->host,f) && fgets(c->token,sizeof c->token,f);
    fclose(f);
    if(!ok) return false;
    c->host[strcspn(c->host,"\r\n")]=0;
    c->token[strcspn(c->token,"\r\n")]=0;
    return c->host[0] && c->token[0];
}
