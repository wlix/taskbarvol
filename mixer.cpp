#include "mixer.hpp"
#include <mmdeviceapi.h>
#include <endpointvolume.h>

IMMDeviceEnumerator*  pEnum = NULL;
IMMDevice*            pEndpoint = NULL;
IAudioEndpointVolume* pAudioEndVol = NULL;

BOOL volume_setting_init() {
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))
		|| FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&pEnum)))
		|| FAILED(pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pEndpoint))
		|| FAILED(pEndpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pAudioEndVol))) {
		volume_setting_deinit();
		return FALSE;
	}
	return	TRUE;
}

void volume_setting_deinit() {
	if (pAudioEndVol) { pAudioEndVol->Release(); }
	if (pEndpoint)    { pEndpoint->Release(); }
	if (pEnum)        { pEnum->Release(); }
	CoUninitialize();
}

BOOL get_master_volume(float *vol) {
	if (FAILED(pAudioEndVol->GetMasterVolumeLevelScalar(vol))) {
		volume_setting_deinit();
		return FALSE;
	}
	return TRUE;
}

BOOL set_master_volume(float vol) {
	if (FAILED(pAudioEndVol->SetMasterVolumeLevelScalar(vol, &GUID_NULL))) {
		volume_setting_deinit();
		return FALSE;
	}
	return TRUE;
}

BOOL adjust_master_volume(int dif) {
	float vol = .0f;

	if (get_master_volume(&vol) == FALSE) { return FALSE; }

	vol += ((float)dif / 100.0f);
	if (vol > 100.0f) vol = 100.0f;
	else if (vol < .0f) vol = .0f;

	if (set_master_volume(vol) == FALSE) { return FALSE; }

	return	TRUE;
}

BOOL get_mute_state(BOOL* state) {
	if (FAILED(pAudioEndVol->GetMute(state))) {
		volume_setting_deinit();
		return FALSE;
	}
	return TRUE;
}

BOOL set_mute_state(BOOL state) {
	if (FAILED(pAudioEndVol->SetMute(state, NULL))) {
		volume_setting_deinit();
		return FALSE;
	}
	return TRUE;
}

BOOL toggle_mute_state() {
	BOOL mute_state = FALSE;

	if (get_mute_state(&mute_state) == FALSE) { return FALSE; }

	mute_state = (mute_state == TRUE) ? FALSE : TRUE;

	if (set_mute_state(mute_state) == FALSE) { return FALSE; }

	return	TRUE;
}

BOOL is_mute_and_not_zero(BOOL* state) {
	float vol;
	BOOL mute;

	if (get_master_volume(&vol) == FALSE) { return FALSE; }
	if (get_mute_state(&mute)   == FALSE) { return FALSE; }
	*state = mute && (vol > 0.005f);

	return TRUE;
}