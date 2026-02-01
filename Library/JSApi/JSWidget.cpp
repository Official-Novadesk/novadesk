/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
 #include "JSWidget.h"
 #include "../Widget.h"
 #include "../PropertyParser.h"
 #include "../Logging.h"
 #include "../Utils.h"
 #include "JSApi.h"
 #include "JSEvents.h"
 #include "JSElement.h"
 #include "JSContextMenu.h"
 #include "../PathUtils.h"
 #include "../TimerManager.h"
 
 extern std::vector<Widget*> widgets;
 
 namespace JSApi {
     
     Widget* GetWidgetFromContext(duk_context* ctx) {
         duk_push_this(ctx);
         
         // Try Windows Method (Direct Pointer)
         if (duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr")) {
             Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
             duk_pop_2(ctx);
             if (Widget::IsValid(widget)) return widget;
             return nullptr;
         }
         duk_pop(ctx);
 
         // Try IPC Method (ID-based)
         if (duk_get_prop_string(ctx, -1, "\xFF" "id")) {
             std::wstring id = Utils::ToWString(duk_get_string(ctx, -1));
             duk_pop_2(ctx);
             for (auto w : widgets) {
                 if (w->GetOptions().id == id) return w;
             }
         } else {
             duk_pop(ctx);
         }
 
         return nullptr;
     }
 
     duk_ret_t js_create_widget_window(duk_context* ctx) {
         if (!duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;
 
         WidgetOptions options;
         options.x = CW_USEDEFAULT;
         options.y = CW_USEDEFAULT;
         options.width = 0;
         options.height = 0;
         options.m_WDefined = false;
         options.m_HDefined = false;
         options.backgroundColor = L"rgba(0,0,0,0)";
         options.zPos = ZPOSITION_NORMAL;
         options.bgAlpha = 0;
         options.windowOpacity = 255;
         options.color = RGB(0,0,0);
         options.draggable = true;
         options.clickThrough = false;
         options.keepOnScreen = false;
         options.snapEdges = true;
  
         std::wstring baseDir = PathUtils::GetParentDir(s_CurrentScriptPath);
         PropertyParser::ParseWidgetOptions(ctx, options, baseDir);
  
         if (options.id.empty()) {
             Logging::Log(LogLevel::Error, L"Widget creation failed: ID must be provided.");
             return 0;
         }
 
         if (options.scriptPath.empty()) {
             Logging::Log(LogLevel::Error, L"Widget creation failed: 'script' path must be provided.");
             return 0;
         }

         // Check for existing widget and replace it
         for (auto it = widgets.begin(); it != widgets.end(); ++it) {
             if ((*it)->GetOptions().id == options.id) {
                 // Remove from global stash widget_objects
                 duk_push_global_stash(ctx);
                 if (duk_get_prop_string(ctx, -1, "widget_objects")) {
                     duk_del_prop_string(ctx, -1, Utils::ToString(options.id).c_str());
                 }
                 duk_pop_2(ctx); // widget_objects, stash

                 // Delete the widget instance
                 delete *it;
                 widgets.erase(it);
                 break;
             }
         }
  
         Widget* widget = new Widget(options);
         if (widget->Create()) {
             if (options.show) {
                 widget->Show();
             } else {
                 widget->Hide();
             }
             widgets.push_back(widget);
         } else {
             Logging::Log(LogLevel::Error, L"Failed to create widget.");
             delete widget;
             return 0;
         }
         
         duk_push_object(ctx);
         duk_push_pointer(ctx, widget);
         duk_put_prop_string(ctx, -2, "\xFF" "widgetPtr");
         std::string idStr = Utils::ToString(options.id);
         duk_push_string(ctx, idStr.c_str());
         duk_put_prop_string(ctx, -2, "\xFF" "id");
         
         BindWidgetControlMethods(ctx);
 
         if (!widget->GetOptions().scriptPath.empty()) {
            // Restore synchronous execution. 
            // We previously deferred this to fix IPC race conditions, but since
            // IPC is now asynchronous (see JSIPC.cpp), we can safety run the script
            // immediately. This is required so that index.js can query the widget's
            // size (calculated by the script) immediately after creation.
            ExecuteWidgetScript(widget);
         }
 
         // Register in Global Stash for Event Dispatching
         duk_push_global_stash(ctx);
         duk_get_prop_string(ctx, -1, "widget_objects");
         duk_dup(ctx, -3); // The widget object
         duk_put_prop_string(ctx, -2, idStr.c_str());
         duk_pop_2(ctx); // stash, widget_objects
         
         return 1;
     }
 
     duk_ret_t js_widget_on(duk_context* ctx) {
         if (!duk_is_string(ctx, 0) || !duk_is_function(ctx, 1)) return DUK_RET_TYPE_ERROR;
         const char* eventName = duk_get_string(ctx, 0);
 
         duk_push_this(ctx);
         // Get or create the hidden events object
         if (!duk_get_prop_string(ctx, -1, "\xFF" "events")) {
             duk_pop(ctx);
             duk_push_object(ctx);
             duk_dup(ctx, -1);
             duk_put_prop_string(ctx, -3, "\xFF" "events");
         }
 
         // Get or create the array for this event
         if (!duk_get_prop_string(ctx, -1, eventName)) {
             duk_pop(ctx);
             duk_push_array(ctx);
             duk_dup(ctx, -1);
             duk_put_prop_string(ctx, -3, eventName);
         }
 
         // Add the callback
         duk_dup(ctx, 1);
         duk_put_prop_index(ctx, -2, (duk_uarridx_t)duk_get_length(ctx, -2));
 
         duk_pop_3(ctx); // array, events object, this
         return 0;
     }
 
     duk_ret_t js_widget_set_properties(duk_context* ctx) {
         Widget* widget = GetWidgetFromContext(ctx);
         if (!widget || !duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;
         PropertyParser::ApplyWidgetProperties(ctx, widget);
         duk_push_this(ctx);
         return 1;
     }
 
     duk_ret_t js_widget_get_properties(duk_context* ctx) {
         Widget* widget = GetWidgetFromContext(ctx);
         if (!widget) return DUK_RET_TYPE_ERROR;
         PropertyParser::PushWidgetProperties(ctx, widget);
         return 1;
     }
 
     duk_ret_t js_widget_close(duk_context* ctx) {
         Widget* widget = GetWidgetFromContext(ctx);
         if (!widget) return DUK_RET_TYPE_ERROR;
         duk_push_this(ctx);
         std::string idStr = "";
         if (duk_get_prop_string(ctx, -1, "\xFF" "id")) {
              idStr = duk_get_string(ctx, -1);
         }
         duk_pop(ctx);
 
         delete widget;
 
         duk_push_this(ctx);
         duk_del_prop_string(ctx, -1, "\xFF" "widgetPtr");
         duk_del_prop_string(ctx, -1, "\xFF" "id");
 
         // Unregister from Global Stash
         if (!idStr.empty()) {
             duk_push_global_stash(ctx);
             if (duk_get_prop_string(ctx, -1, "widget_objects")) {
                 duk_del_prop_string(ctx, -1, idStr.c_str());
             }
             duk_pop_2(ctx);
         }
 
         duk_pop(ctx);
         return 0;
     }
 
     duk_ret_t js_widget_refresh(duk_context* ctx) {
         Widget* widget = GetWidgetFromContext(ctx);
         if (widget) widget->Refresh();
         return 0;
     }

     duk_ret_t js_widget_set_focus(duk_context* ctx) {
         Widget* widget = GetWidgetFromContext(ctx);
         if (widget) widget->SetFocus();
         return 0;
     }

     duk_ret_t js_widget_unfocus(duk_context* ctx) {
         Widget* widget = GetWidgetFromContext(ctx);
         if (widget) widget->UnFocus();
         return 0;
     }
 
     void BindWidgetControlMethods(duk_context* ctx) {
         duk_push_c_function(ctx, js_widget_set_properties, 1);
         duk_put_prop_string(ctx, -2, "setProperties");
         duk_push_c_function(ctx, js_widget_get_properties, 0);
         duk_put_prop_string(ctx, -2, "getProperties");
         duk_push_c_function(ctx, js_widget_close, 0);
         duk_put_prop_string(ctx, -2, "close");
         duk_push_c_function(ctx, js_widget_refresh, 0); 
         duk_put_prop_string(ctx, -2, "refresh");
         duk_push_c_function(ctx, js_widget_set_focus, 0);
         duk_put_prop_string(ctx, -2, "setFocus");
         duk_push_c_function(ctx, js_widget_unfocus, 0);
         duk_put_prop_string(ctx, -2, "unFocus");
         duk_push_c_function(ctx, js_widget_on, 2);
         duk_put_prop_string(ctx, -2, "on");
 
         BindWidgetUIMethods(ctx);
         BindWidgetContextMenuMethods(ctx);
     }
 }
