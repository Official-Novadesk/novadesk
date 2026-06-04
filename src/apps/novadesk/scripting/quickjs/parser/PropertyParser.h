/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "quickjs.h"
#include "PropertyParserTypes.h"

class Element;
class ImageElement;
class TextElement;
class BarElement;
class LineElement;
class HistogramElement;
class RoundLineElement;
class ShapeElement;
class AreaGraphElement;
class ButtonElement;
class BitmapElement;
class RotatorElement;
class GeneralImage;
class ElementLayoutBox;

namespace PropertyParser
{
    bool ParseGradientString(const std::wstring &str, GradientInfo &out);
    D2D1_CAP_STYLE GetCapStyle(const std::wstring &str);
    D2D1_LINE_JOIN GetLineJoin(const std::wstring &str);

    void ParseElementOptions(JSContext *ctx, JSValueConst obj, ElementOptions &options, const std::wstring &baseDir = L"");
    void ParseImageOptions(JSContext *ctx, JSValueConst obj, ImageOptions &options, const std::wstring &baseDir = L"");
    void ParseTextOptions(JSContext *ctx, JSValueConst obj, TextOptions &options, const std::wstring &baseDir = L"");
    void ParseButtonOptions(JSContext *ctx, JSValueConst obj, ButtonOptions &options, const std::wstring &baseDir = L"");
    void ParseBitmapOptions(JSContext *ctx, JSValueConst obj, BitmapOptions &options, const std::wstring &baseDir = L"");
    void ParseRotatorOptions(JSContext *ctx, JSValueConst obj, RotatorOptions &options, const std::wstring &baseDir = L"");
    void ParseBarOptions(JSContext *ctx, JSValueConst obj, BarOptions &options, const std::wstring &baseDir = L"");
    void ParseLineOptions(JSContext *ctx, JSValueConst obj, LineOptions &options, const std::wstring &baseDir = L"");
    void ParseHistogramOptions(JSContext *ctx, JSValueConst obj, HistogramOptions &options, const std::wstring &baseDir = L"");
    void ParseRoundLineOptions(JSContext *ctx, JSValueConst obj, RoundLineOptions &options, const std::wstring &baseDir = L"");
    void ParseShapeOptions(JSContext *ctx, JSValueConst obj, ShapeOptions &options, const std::wstring &baseDir = L"");
    void ParseLayoutBoxOptions(JSContext *ctx, JSValueConst obj, LayoutBoxOptions &options, const std::wstring &baseDir = L"");
    void ParseAnimationOptions(JSContext *ctx, JSValueConst obj, AnimationOptions &options);
    void ParseAreaGraphOptions(JSContext *ctx, JSValueConst obj, AreaGraphOptions &options, const std::wstring &baseDir = L"");

    void ApplyElementOptions(Element *element, const ElementOptions &options);
    void ApplyImageOptions(ImageElement *element, const ImageOptions &options);
    void ApplyTextOptions(TextElement *element, const TextOptions &options);
    void ApplyButtonOptions(ButtonElement *element, const ButtonOptions &options);
    void ApplyBitmapOptions(BitmapElement *element, const BitmapOptions &options);
    void ApplyRotatorOptions(RotatorElement *element, const RotatorOptions &options);
    void ApplyBarOptions(BarElement *element, const BarOptions &options);
    void ApplyLineOptions(LineElement *element, const LineOptions &options);
    void ApplyHistogramOptions(HistogramElement *element, const HistogramOptions &options);
    void ApplyRoundLineOptions(RoundLineElement *element, const RoundLineOptions &options);
    void ApplyShapeOptions(ShapeElement *element, const ShapeOptions &options);
    void ApplyLayoutBoxOptions(ElementLayoutBox *element, const LayoutBoxOptions &options);
    void ApplyAreaGraphOptions(AreaGraphElement *element, const AreaGraphOptions &options);

    void PreFillElementOptions(ElementOptions &options, Element *element);
    void PreFillImageOptions(ImageOptions &options, ImageElement *element);
    void PreFillTextOptions(TextOptions &options, TextElement *element);
    void PreFillButtonOptions(ButtonOptions &options, ButtonElement *element);
    void PreFillBitmapOptions(BitmapOptions &options, BitmapElement *element);
    void PreFillRotatorOptions(RotatorOptions &options, RotatorElement *element);
    void PreFillBarOptions(BarOptions &options, BarElement *element);
    void PreFillLineOptions(LineOptions &options, LineElement *element);
    void PreFillHistogramOptions(HistogramOptions &options, HistogramElement *element);
    void PreFillRoundLineOptions(RoundLineOptions &options, RoundLineElement *element);
    void PreFillShapeOptions(ShapeOptions &options, ShapeElement *element);
    void PreFillLayoutBoxOptions(
        LayoutBoxOptions &options,
        ElementLayoutBox *element,
        const std::wstring *direction = nullptr,
        const int *gap = nullptr,
        const std::wstring *align = nullptr,
        const std::wstring *justify = nullptr,
        const int *paddingLeft = nullptr,
        const int *paddingTop = nullptr,
        const int *paddingRight = nullptr,
        const int *paddingBottom = nullptr);
    void PreFillAreaGraphOptions(AreaGraphOptions &options, AreaGraphElement *element);

    void ParseGeneralImageOptions(JSContext *ctx, JSValueConst obj, GeneralImageOptions &options);
    void ApplyGeneralImageOptions(GeneralImage *image, const GeneralImageOptions &options);
    void PreFillGeneralImageOptions(GeneralImageOptions &options, GeneralImage *image);
} // namespace PropertyParser

namespace novadesk::scripting::quickjs::parser
{
    void ParseWidgetWindowOptions(JSContext *ctx, JSValueConst options, WidgetWindowOptions &out);
    void ParseWidgetWindowSize(JSContext *ctx, JSValueConst options, int &width, int &height);
} // namespace novadesk::scripting::quickjs::parser
