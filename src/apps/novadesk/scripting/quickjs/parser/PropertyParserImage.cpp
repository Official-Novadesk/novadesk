/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */


#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../shared/PathUtils.h"
#include "../../../shared/Utils.h"
#include "../engine/JSEngine.h"
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <cwctype>
#include <string>
#include "../../render/RotatorElement.h"
#include "../../render/ButtonElement.h"
#include "../../render/BitmapElement.h"
#include "../../render/ImageElement.h"
namespace PropertyParser
{
    using namespace Js;
    void ParseGeneralImageOptions(JSContext *ctx, JSValueConst obj, GeneralImageOptions &options)
    {
        std::wstring imageFlip = GetStringProp(ctx, obj, "imageFlip");
        std::transform(imageFlip.begin(), imageFlip.end(), imageFlip.begin(), ::towlower);
        if (imageFlip == L"horizontal")
            options.imageFlip = IMAGE_FLIP_HORIZONTAL;
        else if (imageFlip == L"vertical")
            options.imageFlip = IMAGE_FLIP_VERTICAL;
        else if (imageFlip == L"both")
            options.imageFlip = IMAGE_FLIP_BOTH;
        else if (imageFlip == L"none")
            options.imageFlip = IMAGE_FLIP_NONE;

        std::vector<float> imageCrop;
        if (GetFloatArrayProp(ctx, obj, "imageCrop", imageCrop, 4))
        {
            if (imageCrop.size() >= 4)
            {
                options.hasImageCrop = true;
                options.imageCropX = imageCrop[0];
                options.imageCropY = imageCrop[1];
                options.imageCropW = imageCrop[2];
                options.imageCropH = imageCrop[3];
                options.imageCropOrigin = IMAGE_CROP_ORIGIN_TOP_LEFT;
                if (imageCrop.size() >= 5)
                {
                    int origin = (int)imageCrop[4];
                    if (origin < (int)IMAGE_CROP_ORIGIN_TOP_LEFT)
                        origin = (int)IMAGE_CROP_ORIGIN_TOP_LEFT;
                    if (origin > (int)IMAGE_CROP_ORIGIN_CENTER)
                        origin = (int)IMAGE_CROP_ORIGIN_CENTER;
                    options.imageCropOrigin = (ImageCropOrigin)origin;
                }
            }
        }

        GetBoolProp(ctx, obj, "grayscale", options.grayscale);
        GetBoolProp(ctx, obj, "useExifOrientation", options.useExifOrientation);
        int alpha = 255;
        if (GetIntProp(ctx, obj, "imageAlpha", alpha))
            options.imageAlpha = static_cast<BYTE>(alpha);

        std::wstring tint = GetStringProp(ctx, obj, "imageTint");
        if (!tint.empty() && ColorUtil::ParseRGBA(tint, options.imageTint, options.imageTintAlpha))
        {
            options.hasImageTint = true;
        }

        std::vector<float> colorMatrix;
        if (GetFloatArrayProp(ctx, obj, "colorMatrix", colorMatrix, 20))
        {
            if (colorMatrix.size() >= 20)
            {
                for (int i = 0; i < 20; ++i)
                    options.colorMatrix[i] = colorMatrix[i];
                options.hasColorMatrix = true;
            }
        }
    }

    void ParseImageOptions(JSContext *ctx, JSValueConst obj, ImageOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);
        ParseGeneralImageOptions(ctx, obj, options);

        options.path = GetStringProp(ctx, obj, "path");
        if (!options.path.empty())
        {
            options.path = PathUtils::ResolvePath(options.path, baseDir);
            std::error_code ec;
            const bool exists = std::filesystem::exists(std::filesystem::path(options.path), ec);
        }

        std::wstring aspect = GetStringProp(ctx, obj, "preserveAspectRatio");
        if (!aspect.empty())
        {
            if (aspect == L"preserve")
                options.preserveAspectRatio = IMAGE_ASPECT_PRESERVE;
            else if (aspect == L"crop")
                options.preserveAspectRatio = IMAGE_ASPECT_CROP;
            else if (aspect == L"stretch")
                options.preserveAspectRatio = IMAGE_ASPECT_STRETCH;
        }

        std::vector<float> scaleMargins;
        if (GetFloatArrayProp(ctx, obj, "scaleMargins", scaleMargins, 4))
        {
            options.hasScaleMargins = true;
            options.scaleMarginLeft = scaleMargins[0];
            options.scaleMarginTop = scaleMargins[1];
            options.scaleMarginRight = scaleMargins[2];
            options.scaleMarginBottom = scaleMargins[3];
        }

        GetBoolProp(ctx, obj, "tile", options.tile);
    }

    void ParseButtonOptions(JSContext *ctx, JSValueConst obj, ButtonOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);
        ParseGeneralImageOptions(ctx, obj, options);
        
        std::wstring buttonImageName = GetStringProp(ctx, obj, "buttonImageName");
        if (!buttonImageName.empty())
        {
            options.buttonImageName = PathUtils::ResolvePath(buttonImageName, baseDir);
        }

        GetEventCallbackProp(ctx, obj, "buttonAction", options.onLeftMouseUpCallbackId);
    }

    void ParseBitmapOptions(JSContext *ctx, JSValueConst obj, BitmapOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);
        ParseGeneralImageOptions(ctx, obj, options);

        // Bitmap meter behavior is frame-driven; width/height are ignored.
        options.width = 0;
        options.height = 0;

        // ImageCrop is intentionally ignored for Bitmap element semantics.
        options.hasImageCrop = false;

        // Read as double to preserve numeric precision from JS.
        JSValue v = JS_GetPropertyStr(ctx, obj, "value");
        if (!JS_IsException(v) && !JS_IsUndefined(v) && !JS_IsNull(v))
        {
            double d = 0.0;
            if (JS_ToFloat64(ctx, &d, v) == 0)
            {
                options.value = d;
            }
        }
        JS_FreeValue(ctx, v);

        std::wstring bitmapImageName = GetStringProp(ctx, obj, "bitmapImageName");
        if (!bitmapImageName.empty())
        {
            options.bitmapImageName = PathUtils::ResolvePath(bitmapImageName, baseDir);
        }

        GetIntProp(ctx, obj, "bitmapFrames", options.bitmapFrames);
        GetBoolProp(ctx, obj, "bitmapZeroFrame", options.bitmapZeroFrame);
        GetBoolProp(ctx, obj, "bitmapExtend", options.bitmapExtend);
        { float tmp = static_cast<float>(options.minValue); if (GetFloatProp(ctx, obj, "minValue", tmp)) options.minValue = static_cast<double>(tmp); }
        { float tmp = static_cast<float>(options.maxValue); if (GetFloatProp(ctx, obj, "maxValue", tmp)) options.maxValue = static_cast<double>(tmp); }
        std::wstring bitmapOrientation = GetStringProp(ctx, obj, "bitmapOrientation");
        if (!bitmapOrientation.empty())
            options.bitmapOrientation = bitmapOrientation;

        GetIntProp(ctx, obj, "bitmapDigits", options.bitmapDigits);
        GetIntProp(ctx, obj, "bitmapSeparation", options.bitmapSeparation);

        std::wstring align = GetStringProp(ctx, obj, "bitmapAlign");
        if (!align.empty())
        {
            std::transform(align.begin(), align.end(), align.begin(), ::towlower);
            if (align == L"center")
                options.bitmapAlign = BITMAP_ALIGN_CENTER;
            else if (align == L"right")
                options.bitmapAlign = BITMAP_ALIGN_RIGHT;
            else if (align == L"left")
                options.bitmapAlign = BITMAP_ALIGN_LEFT;
        }
    }

    void ParseRotatorOptions(JSContext *ctx, JSValueConst obj, RotatorOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);
        ParseGeneralImageOptions(ctx, obj, options);

        options.hasImageCrop = false;

        // Read value
        JSValue v = JS_GetPropertyStr(ctx, obj, "value");
        if (!JS_IsException(v) && !JS_IsUndefined(v) && !JS_IsNull(v))
        {
            double d = 0.0;
            if (JS_ToFloat64(ctx, &d, v) == 0)
            {
                options.value = d;
            }
        }
        JS_FreeValue(ctx, v);

        std::wstring rotatorImageName = GetStringProp(ctx, obj, "rotatorImageName");
        if (!rotatorImageName.empty())
        {
            options.rotatorImageName = PathUtils::ResolvePath(rotatorImageName, baseDir);
        }

        { float tmp = static_cast<float>(options.offsetX); if (GetFloatProp(ctx, obj, "offsetX", tmp)) options.offsetX = static_cast<double>(tmp); }
        { float tmp = static_cast<float>(options.offsetY); if (GetFloatProp(ctx, obj, "offsetY", tmp)) options.offsetY = static_cast<double>(tmp); }
        { float tmp = static_cast<float>(options.startAngle); if (GetFloatProp(ctx, obj, "startAngle", tmp)) options.startAngle = static_cast<double>(tmp); }
        { float tmp = static_cast<float>(options.rotationAngle); if (GetFloatProp(ctx, obj, "rotationAngle", tmp)) options.rotationAngle = static_cast<double>(tmp); }
        GetIntProp(ctx, obj, "valueRemainder", options.valueRemainder);
        { float tmp = static_cast<float>(options.minValue); if (GetFloatProp(ctx, obj, "minValue", tmp)) options.minValue = static_cast<double>(tmp); }
        { float tmp = static_cast<float>(options.maxValue); if (GetFloatProp(ctx, obj, "maxValue", tmp)) options.maxValue = static_cast<double>(tmp); }
    }
    void ApplyGeneralImageOptions(GeneralImage *image, const GeneralImageOptions &options)
    {
        if (!image)
            return;
        image->SetImageFlip(options.imageFlip);
        if (options.hasImageCrop)
            image->SetImageCrop(options.imageCropX, options.imageCropY, options.imageCropW, options.imageCropH, options.imageCropOrigin);
        image->SetUseExifOrientation(options.useExifOrientation);
        image->SetGrayscale(options.grayscale);
        image->SetImageAlpha(options.imageAlpha);
        if (options.hasImageTint)
            image->SetImageTint(options.imageTint, options.imageTintAlpha);
        if (options.hasColorMatrix)
            image->SetColorMatrix(options.colorMatrix.data());
    }

    void ApplyImageOptions(ImageElement *element, const ImageOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        if (!options.path.empty())
            element->UpdateImage(options.path);
        element->SetPreserveAspectRatio(options.preserveAspectRatio);
        if (options.hasScaleMargins)
            element->SetScaleMargins(options.scaleMarginLeft, options.scaleMarginTop, options.scaleMarginRight, options.scaleMarginBottom);
        element->SetTile(options.tile);
        
        element->SetImageFlip(options.imageFlip);
        if (options.hasImageCrop)
            element->SetImageCrop(options.imageCropX, options.imageCropY, options.imageCropW, options.imageCropH, options.imageCropOrigin);
        element->SetUseExifOrientation(options.useExifOrientation);
        element->SetGrayscale(options.grayscale);
        element->SetImageAlpha(options.imageAlpha);
        if (options.hasImageTint)
            element->SetImageTint(options.imageTint, options.imageTintAlpha);
        if (options.hasColorMatrix)
            element->SetColorMatrix(options.colorMatrix.data());
    }

    void ApplyButtonOptions(ButtonElement *element, const ButtonOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        if (!options.buttonImageName.empty())
            element->UpdateImage(options.buttonImageName);

        element->SetUseExifOrientation(options.useExifOrientation);
        element->SetGrayscale(options.grayscale);
        element->SetImageAlpha(options.imageAlpha);
        element->SetImageFlip(options.imageFlip);
        if (options.hasImageCrop)
            element->SetImageCrop(options.imageCropX, options.imageCropY, options.imageCropW, options.imageCropH, options.imageCropOrigin);
        else
            element->ClearImageCrop();

        if (options.hasImageTint)
            element->SetImageTint(options.imageTint, options.imageTintAlpha);
        if (options.hasColorMatrix)
            element->SetColorMatrix(options.colorMatrix.data());
    }

    void ApplyBitmapOptions(BitmapElement *element, const BitmapOptions &options)
    {
        if (!element)
            return;

        ApplyElementOptions(element, options);
        if (!options.bitmapImageName.empty())
            element->UpdateImage(options.bitmapImageName);

        element->SetValue(options.value);
        element->SetBitmapFrames(options.bitmapFrames);
        element->SetBitmapZeroFrame(options.bitmapZeroFrame);
        element->SetBitmapExtend(options.bitmapExtend);
        element->SetMinValue(options.minValue);
        element->SetMaxValue(options.maxValue);
        element->SetBitmapOrientation(options.bitmapOrientation);
        element->SetBitmapDigits(options.bitmapDigits);
        element->SetBitmapAlign(options.bitmapAlign);
        element->SetBitmapSeparation(options.bitmapSeparation);

        element->SetUseExifOrientation(options.useExifOrientation);
        element->SetGrayscale(options.grayscale);
        element->SetImageAlpha(options.imageAlpha);
        element->SetImageFlip(options.imageFlip);
        if (options.hasImageTint)
            element->SetImageTint(options.imageTint, options.imageTintAlpha);
        if (options.hasColorMatrix)
            element->SetColorMatrix(options.colorMatrix.data());
    }

    void ApplyRotatorOptions(RotatorElement *element, const RotatorOptions &options)
    {
        if (!element)
            return;

        ApplyElementOptions(element, options);
        if (!options.rotatorImageName.empty())
            element->UpdateImage(options.rotatorImageName);

        element->SetValue(options.value);
        element->SetOffsetX(options.offsetX);
        element->SetOffsetY(options.offsetY);
        element->SetStartAngle(options.startAngle);
        element->SetRotationAngle(options.rotationAngle);
        element->SetValueRemainder(options.valueRemainder);
        element->SetMinValue(options.minValue);
        element->SetMaxValue(options.maxValue);

        element->SetUseExifOrientation(options.useExifOrientation);
        element->SetGrayscale(options.grayscale);
        element->SetImageAlpha(options.imageAlpha);
        element->SetImageFlip(options.imageFlip);
        if (options.hasImageTint)
            element->SetImageTint(options.imageTint, options.imageTintAlpha);
        if (options.hasColorMatrix)
            element->SetColorMatrix(options.colorMatrix.data());
    }
    void PreFillGeneralImageOptions(GeneralImageOptions &options, GeneralImage *image)
    {
        if (!image) return;
        options.imageFlip = image->GetImageFlip();
        options.hasImageCrop = image->HasImageCrop();
        if (options.hasImageCrop)
        {
            options.imageCropX = image->GetImageCropX();
            options.imageCropY = image->GetImageCropY();
            options.imageCropW = image->GetImageCropW();
            options.imageCropH = image->GetImageCropH();
            options.imageCropOrigin = image->GetImageCropOrigin();
        }
        options.useExifOrientation = image->GetUseExifOrientation();
        options.imageAlpha = image->GetImageAlpha();
        options.grayscale = image->IsGrayscale();
        if (image->HasImageTint())
        {
            options.hasImageTint = true;
            options.imageTint = image->GetImageTint();
            options.imageTintAlpha = image->GetImageTintAlpha();
        }
        if (image->HasColorMatrix())
        {
            options.hasColorMatrix = true;
            const float *m = image->GetColorMatrix();
            for (int i = 0; i < 20; ++i) options.colorMatrix[i] = m[i];
        }
    }

    void PreFillImageOptions(ImageOptions &options, ImageElement *element)
    {
        if (!element)
            return;
        PreFillElementOptions(options, element);
        options.path = element->GetImagePath();
        options.preserveAspectRatio = element->GetPreserveAspectRatio();
        options.tile = element->IsTile();

        options.imageFlip = element->GetImageFlip();
        options.hasImageCrop = element->HasImageCrop();
        if (options.hasImageCrop)
        {
            options.imageCropX = element->GetImageCropX();
            options.imageCropY = element->GetImageCropY();
            options.imageCropW = element->GetImageCropW();
            options.imageCropH = element->GetImageCropH();
            options.imageCropOrigin = element->GetImageCropOrigin();
        }
        options.hasScaleMargins = element->HasScaleMargins();
        if (options.hasScaleMargins)
        {
            options.scaleMarginLeft = element->GetScaleMarginLeft();
            options.scaleMarginTop = element->GetScaleMarginTop();
            options.scaleMarginRight = element->GetScaleMarginRight();
            options.scaleMarginBottom = element->GetScaleMarginBottom();
        }
        options.useExifOrientation = element->GetUseExifOrientation();
        options.imageAlpha = element->GetImageAlpha();
        options.grayscale = element->IsGrayscale();
        if (element->HasImageTint())
        {
            options.hasImageTint = true;
            options.imageTint = element->GetImageTint();
            options.imageTintAlpha = element->GetImageTintAlpha();
        }
        if (element->HasColorMatrix())
        {
            options.hasColorMatrix = true;
            const float *m = element->GetColorMatrix();
            for (int i = 0; i < 20; ++i) options.colorMatrix[i] = m[i];
        }
    }

    void PreFillButtonOptions(ButtonOptions &options, ButtonElement *element)
    {
        if (!element)
            return;
        PreFillElementOptions(options, element);

        options.buttonImageName = element->GetImagePath();
        options.useExifOrientation = element->GetUseExifOrientation();
        options.grayscale = element->IsGrayscale();
        options.imageAlpha = element->GetImageAlpha();
        options.imageFlip = element->GetImageFlip();
        options.hasImageCrop = element->HasImageCrop();
        if (options.hasImageCrop)
        {
            options.imageCropX = element->GetImageCropX();
            options.imageCropY = element->GetImageCropY();
            options.imageCropW = element->GetImageCropW();
            options.imageCropH = element->GetImageCropH();
            options.imageCropOrigin = element->GetImageCropOrigin();
        }
        if (element->HasImageTint())
        {
            options.hasImageTint = true;
            options.imageTint = element->GetImageTint();
            options.imageTintAlpha = element->GetImageTintAlpha();
        }
        if (element->HasColorMatrix())
        {
            options.hasColorMatrix = true;
            const float *m = element->GetColorMatrix();
            for (int i = 0; i < 20; ++i) options.colorMatrix[i] = m[i];
        }
    }

    void PreFillBitmapOptions(BitmapOptions &options, BitmapElement *element)
    {
        if (!element)
            return;

        PreFillElementOptions(options, element);

        // Keep width/height invalid for bitmap element options.
        options.width = 0;
        options.height = 0;

        options.value = element->GetValue();
        options.bitmapImageName = element->GetImagePath();
        options.bitmapFrames = element->GetBitmapFrames();
        options.bitmapZeroFrame = element->GetBitmapZeroFrame();
        options.bitmapExtend = element->GetBitmapExtend();
        options.minValue = element->GetMinValue();
        options.maxValue = element->GetMaxValue();
        options.bitmapOrientation = element->GetBitmapOrientation();
        options.bitmapDigits = element->GetBitmapDigits();
        options.bitmapAlign = element->GetBitmapAlign();
        options.bitmapSeparation = element->GetBitmapSeparation();

        options.useExifOrientation = element->GetUseExifOrientation();
        options.grayscale = element->IsGrayscale();
        options.imageAlpha = element->GetImageAlpha();
        options.imageFlip = element->GetImageFlip();
        if (element->HasImageTint())
        {
            options.hasImageTint = true;
            options.imageTint = element->GetImageTint();
            options.imageTintAlpha = element->GetImageTintAlpha();
        }
        if (element->HasColorMatrix())
        {
            options.hasColorMatrix = true;
            const float *m = element->GetColorMatrix();
            for (int i = 0; i < 20; ++i) options.colorMatrix[i] = m[i];
        }
    }

    void PreFillRotatorOptions(RotatorOptions &options, RotatorElement *element)
    {
        if (!element)
            return;

        PreFillElementOptions(options, element);

        options.value = element->GetValue();
        options.rotatorImageName = element->GetImagePath();
        options.offsetX = element->GetOffsetX();
        options.offsetY = element->GetOffsetY();
        options.startAngle = element->GetStartAngle();
        options.rotationAngle = element->GetRotationAngle();
        options.valueRemainder = element->GetValueRemainder();
        options.minValue = element->GetMinValue();
        options.maxValue = element->GetMaxValue();

        options.useExifOrientation = element->GetUseExifOrientation();
        options.grayscale = element->IsGrayscale();
        options.imageAlpha = element->GetImageAlpha();
        options.imageFlip = element->GetImageFlip();
        if (element->HasImageTint())
        {
            options.hasImageTint = true;
            options.imageTint = element->GetImageTint();
            options.imageTintAlpha = element->GetImageTintAlpha();
        }
        if (element->HasColorMatrix())
        {
            options.hasColorMatrix = true;
            const float *m = element->GetColorMatrix();
            for (int i = 0; i < 20; ++i) options.colorMatrix[i] = m[i];
        }
    }

}
