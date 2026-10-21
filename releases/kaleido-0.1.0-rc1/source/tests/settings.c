/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "kaleido_settings.h"
int main(void)
{
    remove("build/prefs/kaleido0.prf");remove("build/prefs/kaleido1.prf");
    kaleido_settings a={1,1,17,250,750},b={0,2,179,1000,0},got={0,0,0,500,500};
    assert(!kaleido_settings_load(&got));assert(got.center_x==500);
    assert(kaleido_settings_save(&a));assert(kaleido_settings_load(&got));assert(!memcmp(&got,&a,sizeof(a)));
    assert(kaleido_settings_save(&b));assert(kaleido_settings_load(&got));assert(!memcmp(&got,&b,sizeof(b)));
    /* A torn newest slot must recover the previous checked generation. */
    FILE*f=fopen("build/prefs/kaleido1.prf","wb");assert(f);fwrite("bad",1,3,f);fclose(f);
    assert(kaleido_settings_load(&got));assert(!memcmp(&got,&a,sizeof(a)));
    kaleido_settings invalid=a;invalid.rotation=360;assert(!kaleido_settings_save(&invalid));
    invalid=a;invalid.segments=3;assert(!kaleido_settings_save(&invalid));
    invalid=a;invalid.center_x=1001;assert(!kaleido_settings_save(&invalid));
    invalid=a;invalid.center_y=1001;assert(!kaleido_settings_save(&invalid));
    invalid=a;invalid.enabled=2;assert(!kaleido_settings_save(&invalid));
    assert(kaleido_settings_save(&b));assert(kaleido_settings_load(&got));assert(!memcmp(&got,&b,sizeof(b)));
    /* Distinct identity: an old Mirror Worlds preference is never a slot. */
    assert(!kaleido_settings_save(0));puts("Settings: round-trip, torn-write fallback, recovery and invalid controls PASS");
    return 0;
}
