
#pragma once
#include "JSCommon.h"

namespace JSApi {
    duk_ret_t js_audio_constructor(duk_context* ctx);
    duk_ret_t js_audio_stats(duk_context* ctx); // Renamed from get_levels
    duk_ret_t js_audio_destroy(duk_context* ctx); // Standard monitor pattern
    duk_ret_t js_audio_finalizer(duk_context* ctx);
}
