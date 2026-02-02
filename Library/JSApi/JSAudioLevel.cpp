#include "JSAudioLevel.h"
#include "../AudioLevel.h"
#include "../Logging.h"
#include "../Utils.h"
#include "JSUtils.h"

namespace JSApi {

    duk_ret_t js_audio_constructor(duk_context* ctx) {
        if (!duk_is_constructor_call(ctx)) return DUK_RET_TYPE_ERROR;

        AudioLevelConfig config;

        if (duk_get_top(ctx) > 0 && duk_is_object(ctx, 0)) {
            // Port
            if (duk_get_prop_string(ctx, 0, "port")) {
                if (duk_is_string(ctx, -1)) config.port = duk_get_string(ctx, -1);
            }
            duk_pop(ctx);

            // DeviceId
            if (duk_get_prop_string(ctx, 0, "id")) {
                 if (duk_is_string(ctx, -1)) config.deviceId = Utils::ToWString(duk_get_string(ctx, -1));
            }
            duk_pop(ctx);

            // FFT
            if (duk_get_prop_string(ctx, 0, "fftSize")) {
                if (duk_is_number(ctx, -1)) config.fftSize = duk_get_int(ctx, -1);
            }
            duk_pop(ctx);
            
            if (duk_get_prop_string(ctx, 0, "fftOverlap")) {
                if (duk_is_number(ctx, -1)) config.fftOverlap = duk_get_int(ctx, -1);
            }
            duk_pop(ctx);

            if (duk_get_prop_string(ctx, 0, "bands")) {
                if (duk_is_number(ctx, -1)) config.bands = duk_get_int(ctx, -1);
            }
            duk_pop(ctx);
            
            // Freq
            if (duk_get_prop_string(ctx, 0, "freqMin")) {
                if (duk_is_number(ctx, -1)) config.freqMin = duk_get_number(ctx, -1);
            }
            duk_pop(ctx);

            if (duk_get_prop_string(ctx, 0, "freqMax")) {
                if (duk_is_number(ctx, -1)) config.freqMax = duk_get_number(ctx, -1);
            }
            duk_pop(ctx);

            if (duk_get_prop_string(ctx, 0, "sensitivity")) {
                if (duk_is_number(ctx, -1)) config.sensitivity = duk_get_number(ctx, -1);
            }
            duk_pop(ctx);

            // Smoothing
            if (duk_get_prop_string(ctx, 0, "rmsAttack")) {
                if (duk_is_number(ctx, -1)) config.rmsAttack = duk_get_int(ctx, -1);
            }
            duk_pop(ctx);
            if (duk_get_prop_string(ctx, 0, "rmsDecay")) {
                if (duk_is_number(ctx, -1)) config.rmsDecay = duk_get_int(ctx, -1);
            }
            duk_pop(ctx);
            
            if (duk_get_prop_string(ctx, 0, "peakAttack")) {
                if (duk_is_number(ctx, -1)) config.peakAttack = duk_get_int(ctx, -1);
            }
            duk_pop(ctx);
            if (duk_get_prop_string(ctx, 0, "peakDecay")) {
                if (duk_is_number(ctx, -1)) config.peakDecay = duk_get_int(ctx, -1);
            }
            duk_pop(ctx);

            if (duk_get_prop_string(ctx, 0, "fftAttack")) {
                if (duk_is_number(ctx, -1)) config.fftAttack = duk_get_int(ctx, -1);
            }
            duk_pop(ctx);
            if (duk_get_prop_string(ctx, 0, "fftDecay")) {
                if (duk_is_number(ctx, -1)) config.fftDecay = duk_get_int(ctx, -1);
            }
            duk_pop(ctx);

            // Gains
            if (duk_get_prop_string(ctx, 0, "rmsGain")) {
                if (duk_is_number(ctx, -1)) config.rmsGain = duk_get_number(ctx, -1);
            }
            duk_pop(ctx);
            if (duk_get_prop_string(ctx, 0, "peakGain")) {
                if (duk_is_number(ctx, -1)) config.peakGain = duk_get_number(ctx, -1);
            }
            duk_pop(ctx);
        }

        AudioLevel* audio = new AudioLevel(config);
        
        duk_push_this(ctx);
        duk_push_pointer(ctx, audio);
        duk_put_prop_string(ctx, -2, "\xFF" "audioPtr");

        duk_push_c_function(ctx, js_audio_stats, 0);
        duk_put_prop_string(ctx, -2, "stats");

        duk_push_c_function(ctx, js_audio_destroy, 0);
        duk_put_prop_string(ctx, -2, "destroy");

        duk_push_c_function(ctx, js_audio_finalizer, 1);
        duk_set_finalizer(ctx, -2);

        return 0;
    }

    duk_ret_t js_audio_stats(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "audioPtr");
        AudioLevel* audio = (AudioLevel*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!audio) return DUK_RET_TYPE_ERROR;

        const AudioLevel::AudioData& data = audio->GetStats();

        duk_push_object(ctx);
        
        // RMS
        duk_push_array(ctx);
        duk_push_number(ctx, data.rms[0]);
        duk_put_prop_index(ctx, -2, 0);
        duk_push_number(ctx, data.rms[1]);
        duk_put_prop_index(ctx, -2, 1);
        duk_put_prop_string(ctx, -2, "rms");

        // Peak
        duk_push_array(ctx);
        duk_push_number(ctx, data.peak[0]);
        duk_put_prop_index(ctx, -2, 0);
        duk_push_number(ctx, data.peak[1]);
        duk_put_prop_index(ctx, -2, 1);
        duk_put_prop_string(ctx, -2, "peak");

        // Bands
        duk_push_array(ctx);
        for (size_t i = 0; i < data.bands.size(); i++) {
            duk_push_number(ctx, data.bands[i]);
            duk_put_prop_index(ctx, -2, (duk_uarridx_t)i);
        }
        duk_put_prop_string(ctx, -2, "bands");

        return 1;
    }

    duk_ret_t js_audio_destroy(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "audioPtr");
        AudioLevel* audio = (AudioLevel*)duk_get_pointer(ctx, -1);
        if (audio) {
            delete audio;
            duk_push_null(ctx);
            duk_put_prop_string(ctx, -2, "\xFF" "audioPtr");
        }
        duk_pop_2(ctx);
        return 0;
    }

    duk_ret_t js_audio_finalizer(duk_context* ctx) {
        duk_get_prop_string(ctx, 0, "\xFF" "audioPtr");
        AudioLevel* audio = (AudioLevel*)duk_get_pointer(ctx, -1);
        if (audio) {
            delete audio;
        }
        duk_pop(ctx);
        return 0;
    }
}
