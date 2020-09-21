#pragma once

#include "taskbarvol.hpp"

BOOL volume_setting_init();
void volume_setting_deinit();

BOOL get_master_volume(float* vol);
BOOL set_master_volume(float vol);
BOOL adjust_master_volume(int dif);

BOOL get_mute_state(BOOL* state);
BOOL set_mute_state(BOOL state);
BOOL toggle_mute_state();
