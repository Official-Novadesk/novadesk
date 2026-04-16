/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "BitmapElement.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
    constexpr ULONGLONG kTransitionStepMs = 100ULL;
}

BitmapElement::BitmapElement(const std::wstring &id, int x, int y, const std::wstring &path)
    : Element(ELEMENT_BITMAP, id, x, y, 0, 0)
{
    m_BitmapImage.SetPath(path);
}

BitmapElement::~BitmapElement()
{
}

void BitmapElement::SetValue(double value)
{
    m_Value = value;
}

void BitmapElement::SetBitmapFrames(int frameCount)
{
    m_FrameCount = (frameCount <= 0) ? 1 : frameCount;
}

void BitmapElement::SetBitmapOrientation(const std::wstring &orientation)
{
    std::wstring lower = orientation;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    if (lower == L"vertical")
    {
        m_Vertical = true;
        m_AutoOrientation = false;
    }
    else if (lower == L"horizontal")
    {
        m_Vertical = false;
        m_AutoOrientation = false;
    }
    else
    {
        m_AutoOrientation = true;
    }
}

std::wstring BitmapElement::GetBitmapOrientation() const
{
    if (m_AutoOrientation)
        return L"auto";
    return m_Vertical ? L"vertical" : L"horizontal";
}

bool BitmapElement::ComputeFrameGeometry(float imageWidth, float imageHeight, bool &verticalFrames, float &frameWidth, float &frameHeight) const
{
    if (imageWidth <= 0.0f || imageHeight <= 0.0f || m_FrameCount <= 0)
    {
        return false;
    }

    if (m_AutoOrientation)
    {
        verticalFrames = imageHeight > imageWidth;
    }
    else
    {
        verticalFrames = m_Vertical;
    }

    frameWidth = imageWidth;
    frameHeight = imageHeight;

    if (verticalFrames)
    {
        frameHeight = imageHeight / static_cast<float>(m_FrameCount);
    }
    else
    {
        frameWidth = imageWidth / static_cast<float>(m_FrameCount);
    }

    return frameWidth > 0.0f && frameHeight > 0.0f;
}

int BitmapElement::GetRealFrameCount() const
{
    return (m_FrameCount <= 0) ? 1 : m_FrameCount;
}

int BitmapElement::GetDigitCountForValue(long long value, int realFrames) const
{
    if (m_Digits > 0)
        return m_Digits;

    const long long baseValue = (realFrames == 1) ? 2LL : static_cast<long long>(realFrames);
    long long tmp = (value < 0) ? 0 : value;
    int count = 0;

    do
    {
        ++count;
        tmp /= baseValue;
    } while (tmp > 0LL);

    return (count <= 0) ? 1 : count;
}

int BitmapElement::ResolveFrameForNormalizedValue(double value) const
{
    const int realFrames = GetRealFrameCount();
    int frame = 0;
    if (m_ZeroFrame)
    {
        if (value > 0.0)
        {
            frame = static_cast<int>(value * static_cast<double>(realFrames - 1));
        }
    }
    else
    {
        frame = static_cast<int>(value * static_cast<double>(realFrames));
    }

    if (m_FrameCount > 0)
    {
        frame = frame % m_FrameCount;
        frame = (std::max)(0, frame);
    }
    return frame;
}

int BitmapElement::ResolveFrameForDigit(int digit)
{
    const int realFrames = GetRealFrameCount();
    int frame = (digit % realFrames);
    if (m_FrameCount > 0)
        frame %= m_FrameCount;
    return (std::max)(0, frame);
}

void BitmapElement::Render(ID2D1DeviceContext *context)
{
    if (!m_Show || !context)
        return;

    m_BitmapImage.EnsureBitmap(context);
    ID2D1Bitmap *bitmap = m_BitmapImage.GetBitmap();
    if (!bitmap || m_FrameCount <= 0)
        return;

    D2D1_SIZE_F imageSize = bitmap->GetSize();
    bool verticalFrames = false;
    float frameWidth = 0.0f;
    float frameHeight = 0.0f;
    if (!ComputeFrameGeometry(imageSize.width, imageSize.height, verticalFrames, frameWidth, frameHeight))
        return;

    D2D1_MATRIX_3X2_F originalTransform = D2D1::Matrix3x2F::Identity();
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    Microsoft::WRL::ComPtr<ID2D1Image> finalImage;
    if (!m_BitmapImage.BuildProcessedImage(context, finalImage) || !finalImage)
    {
        RenderBevel(context);
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    const int contentX = m_X + m_PaddingLeft;
    const int contentY = m_Y + m_PaddingTop;
    const float opacity = m_BitmapImage.GetImageAlpha() / 255.0f;
    const D2D1_BITMAP_INTERPOLATION_MODE interp = m_AntiAlias ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

    if (m_Extend)
    {
        long long value = static_cast<long long>(m_Value);
        if (value < 0)
            value = 0;

        const int realFrames = GetRealFrameCount();
        const int digitCount = GetDigitCountForValue(value, realFrames);
        const int spacing = m_Separation;

        const float totalWidth = frameWidth * static_cast<float>(digitCount) +
                                 static_cast<float>((digitCount > 1 ? (digitCount - 1) : 0) * spacing);

        float startX = static_cast<float>(contentX);
        if (m_Align == BITMAP_ALIGN_CENTER)
        {
            startX -= totalWidth / 2.0f;
        }
        else if (m_Align == BITMAP_ALIGN_RIGHT)
        {
            startX -= totalWidth;
        }

        long long tmpValue = value;
        const long long baseValue = (realFrames == 1) ? 2LL : static_cast<long long>(realFrames);

        std::vector<int> digits(static_cast<size_t>(digitCount), 0);
        for (int i = digitCount - 1; i >= 0; --i)
        {
            digits[static_cast<size_t>(i)] = static_cast<int>(tmpValue % baseValue);
            tmpValue /= baseValue;
        }

        for (int i = 0; i < digitCount; ++i)
        {
            const int frame = ResolveFrameForDigit(digits[static_cast<size_t>(i)]);

            float srcX = 0.0f;
            float srcY = 0.0f;
            if (verticalFrames)
                srcY = frameHeight * static_cast<float>(frame);
            else
                srcX = frameWidth * static_cast<float>(frame);

            const D2D1_RECT_F srcRect = D2D1::RectF(srcX, srcY, srcX + frameWidth, srcY + frameHeight);
            const float digitX = startX + static_cast<float>(i) * (frameWidth + static_cast<float>(spacing));
            const D2D1_RECT_F dstRect = D2D1::RectF(digitX, static_cast<float>(contentY), digitX + frameWidth, static_cast<float>(contentY) + frameHeight);

            if (finalImage.Get() == static_cast<ID2D1Image *>(bitmap))
            {
                context->DrawBitmap(bitmap, &dstRect, opacity, interp, &srcRect);
            }
            else
            {
                D2D1_MATRIX_3X2_F effectTransform;
                context->GetTransform(&effectTransform);
                context->SetTransform(D2D1::Matrix3x2F::Translation(dstRect.left, dstRect.top) * effectTransform);
                context->DrawImage(finalImage.Get(), nullptr, &srcRect, m_AntiAlias ? D2D1_INTERPOLATION_MODE_LINEAR : D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
                context->SetTransform(effectTransform);
            }
        }
    }
    else
    {
        // When extend=false, normalize the value to m_MinValue...m_MaxValue range
        double range = m_MaxValue - m_MinValue;
        double normalizedValue = (range > 0.0) ? (m_Value - m_MinValue) / range : 0.0;
        if (normalizedValue < 0.0)
            normalizedValue = 0.0;
        if (normalizedValue > 1.0)
            normalizedValue = 1.0;

        int frame = ResolveFrameForNormalizedValue(normalizedValue);
        float srcX = 0.0f;
        float srcY = 0.0f;
        if (verticalFrames)
            srcY = frameHeight * static_cast<float>(frame);
        else
            srcX = frameWidth * static_cast<float>(frame);

        const D2D1_RECT_F srcRect = D2D1::RectF(srcX, srcY, srcX + frameWidth, srcY + frameHeight);
        const D2D1_RECT_F dstRect = D2D1::RectF(
            static_cast<float>(contentX),
            static_cast<float>(contentY),
            static_cast<float>(contentX) + frameWidth,
            static_cast<float>(contentY) + frameHeight);

        if (finalImage.Get() == static_cast<ID2D1Image *>(bitmap))
        {
            context->DrawBitmap(bitmap, &dstRect, opacity, interp, &srcRect);
        }
        else
        {
            D2D1_MATRIX_3X2_F effectTransform;
            context->GetTransform(&effectTransform);
            context->SetTransform(D2D1::Matrix3x2F::Translation(dstRect.left, dstRect.top) * effectTransform);
            context->DrawImage(finalImage.Get(), nullptr, &srcRect, m_AntiAlias ? D2D1_INTERPOLATION_MODE_LINEAR : D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
            context->SetTransform(effectTransform);
        }
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}

int BitmapElement::GetAutoWidth()
{
    if (!m_BitmapImage.IsLoaded())
    {
        return m_PaddingLeft + m_PaddingRight;
    }

    const int imageW = m_BitmapImage.GetAutoWidth();
    const int imageH = m_BitmapImage.GetAutoHeight();
    if (imageW <= 0 || imageH <= 0 || m_FrameCount <= 0)
    {
        return m_PaddingLeft + m_PaddingRight;
    }

    const bool verticalFrames = imageH > imageW;
    int frameW = imageW;
    if (!verticalFrames)
    {
        frameW = imageW / m_FrameCount;
    }

    if (m_Extend)
    {
        const int digits = (m_Digits > 0) ? m_Digits : 1;
        const int spacing = (digits > 1) ? ((digits - 1) * m_Separation) : 0;
        frameW = frameW * digits + spacing;
    }

    return frameW + m_PaddingLeft + m_PaddingRight;
}

int BitmapElement::GetAutoHeight()
{
    if (!m_BitmapImage.IsLoaded())
    {
        return m_PaddingTop + m_PaddingBottom;
    }

    const int imageW = m_BitmapImage.GetAutoWidth();
    const int imageH = m_BitmapImage.GetAutoHeight();
    if (imageW <= 0 || imageH <= 0 || m_FrameCount <= 0)
    {
        return m_PaddingTop + m_PaddingBottom;
    }

    const bool verticalFrames = imageH > imageW;
    int frameH = imageH;
    if (verticalFrames)
    {
        frameH = imageH / m_FrameCount;
    }

    return frameH + m_PaddingTop + m_PaddingBottom;
}
