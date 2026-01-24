/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSPower.h"
#include "../Utils.h"
#include <windows.h>
#include <Powrprof.h>

#pragma comment(lib, "powrprof.lib")

namespace JSApi {

    typedef struct _PROCESSOR_POWER_INFORMATION
    {
        ULONG Number;
        ULONG MaxMhz;
        ULONG CurrentMhz;
        ULONG MhzLimit;
        ULONG MaxIdleState;
        ULONG CurrentIdleState;
    } PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

    duk_ret_t js_system_getPowerStatus(duk_context* ctx) {
        SYSTEM_POWER_STATUS sps;
        if (!GetSystemPowerStatus(&sps)) {
            return 0;
        }

        duk_push_object(ctx);

        // AC Line status: 0 (offline/battery), 1 (online), 255 (unknown)
        duk_push_int(ctx, sps.ACLineStatus == 1 ? 1 : 0);
        duk_put_prop_string(ctx, -2, "acline");

        // Battery Status mapping (Rainmeter style)
        int status = 4; // High/Full
        if (sps.BatteryFlag & 128) status = 0; // No battery
        else if (sps.BatteryFlag & 8) status = 1; // Charging
        else if (sps.BatteryFlag & 4) status = 2; // Critical
        else if (sps.BatteryFlag & 2) status = 3; // Low

        duk_push_int(ctx, status);
        duk_put_prop_string(ctx, -2, "status");

        // Raw Battery Flag
        duk_push_int(ctx, sps.BatteryFlag);
        duk_put_prop_string(ctx, -2, "status2");

        // Battery Life Time (seconds)
        duk_push_number(ctx, (double)sps.BatteryLifeTime);
        duk_put_prop_string(ctx, -2, "lifetime");

        // Battery Life Percent (0-100)
        duk_push_int(ctx, sps.BatteryLifePercent == 255 ? 0 : sps.BatteryLifePercent);
        duk_put_prop_string(ctx, -2, "percent");

        // CPU Frequency (MHz and Hz)
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        UINT numProcessors = (UINT)si.dwNumberOfProcessors;

        double mhz = 0;
        if (numProcessors > 0) {
            PROCESSOR_POWER_INFORMATION* ppi = new PROCESSOR_POWER_INFORMATION[numProcessors];
            if (CallNtPowerInformation(ProcessorInformation, NULL, 0, ppi, sizeof(PROCESSOR_POWER_INFORMATION) * numProcessors) == 0) {
                mhz = (double)ppi[0].CurrentMhz;
            }
            delete[] ppi;
        }

        duk_push_number(ctx, mhz);
        duk_put_prop_string(ctx, -2, "mhz");

        duk_push_number(ctx, mhz * 1000000.0);
        duk_put_prop_string(ctx, -2, "hz");

        return 1;
    }

    void BindPowerMethods(duk_context* ctx) {
        duk_push_c_function(ctx, js_system_getPowerStatus, 0);
        duk_put_prop_string(ctx, -2, "getPowerStatus");
    }
}
