/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* novadesk_context;

struct NovadeskHostAPI {
    void (*RegisterString)(novadesk_context ctx, const char* name, const char* value);
    void (*RegisterNumber)(novadesk_context ctx, const char* name, double value);
    void (*RegisterBool)(novadesk_context ctx, const char* name, int value);
    void (*RegisterObjectStart)(novadesk_context ctx, const char* name);
    void (*RegisterObjectEnd)(novadesk_context ctx, const char* name);
    void (*RegisterArrayString)(novadesk_context ctx, const char* name, const char** values, size_t count);
    void (*RegisterArrayNumber)(novadesk_context ctx, const char* name, const double* values, size_t count);
    void (*RegisterFunction)(novadesk_context ctx, const char* name, int (*func)(novadesk_context ctx), int nargs);
    void (*PushString)(novadesk_context ctx, const char* value);
    void (*PushNumber)(novadesk_context ctx, double value);
    void (*PushBool)(novadesk_context ctx, int value);
    void (*PushNull)(novadesk_context ctx);
    void (*PushObject)(novadesk_context ctx);
    double (*GetNumber)(novadesk_context ctx, int index);
    const char* (*GetString)(novadesk_context ctx, int index);
    int (*GetBool)(novadesk_context ctx, int index);
    int (*IsNumber)(novadesk_context ctx, int index);
    int (*IsString)(novadesk_context ctx, int index);
    int (*IsBool)(novadesk_context ctx, int index);
    int (*IsObject)(novadesk_context ctx, int index);
    int (*IsFunction)(novadesk_context ctx, int index);
    int (*IsNull)(novadesk_context ctx, int index);
    int (*GetTop)(novadesk_context ctx);
    void (*Pop)(novadesk_context ctx);
    void (*PopN)(novadesk_context ctx, int n);
    void (*ThrowError)(novadesk_context ctx, const char* message);
    void* (*JsGetFunctionPtr)(novadesk_context ctx, int index);
    void (*JsCallFunction)(novadesk_context ctx, void* funcPtr, int nargs);
};

typedef void (*NovadeskAddonInitFn)(novadesk_context ctx, HWND hMsgWnd, const NovadeskHostAPI* host);

typedef void (*NovadeskAddonUnloadFn)();

#ifdef __cplusplus
}
#endif
