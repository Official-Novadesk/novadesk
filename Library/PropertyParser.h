/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "duktape/duktape.h"
#include "Widget.h"
#include "Element.h"

namespace PropertyParser {
    /*
    ** Parse WidgetOptions from a Duktape object at the top of the stack.
    ** Optionally loads settings if an 'id' is present.
    */
    void ParseWidgetOptions(duk_context* ctx, WidgetOptions& options);

    /*
    ** Apply properties from a Duktape object to an existing Widget instance.
    */
    void ApplyWidgetProperties(duk_context* ctx, Widget* widget);

    /*
    ** Push a Widget's current properties as a JavaScript object onto the stack.
    */
    void PushWidgetProperties(duk_context* ctx, Widget* widget);

    /*
    ** Parse mouse/scroll event handlers for an Element.
    */
    void ParseElementOptions(duk_context* ctx, Element* element);
}
