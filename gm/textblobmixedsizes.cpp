/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "sk_tool_utils.h"

#include "Resources.h"
#include "SkBlurMask.h"
#include "SkCanvas.h"
#include "SkGradientShader.h"
#include "SkImage.h"
#include "SkMaskFilter.h"
#include "SkRandom.h"
#include "SkStream.h"
#include "SkSurface.h"
#include "SkTextBlob.h"
#include "SkTypeface.h"

namespace skiagm {
class TextBlobMixedSizes : public GM {
public:
    // This gm tests that textblobs of mixed sizes with a large glyph will render properly
    TextBlobMixedSizes(bool useDFT) : fUseDFT(useDFT) {}

protected:
    void onOnceBeforeDraw() override {
        SkTextBlobBuilder builder;

        // make textblob.  To stress distance fields, we choose sizes appropriately
        SkFont font(MakeResourceAsTypeface("fonts/HangingS.ttf"), 262);
        font.setSubpixel(true);
        font.setEdging(SkFont::Edging::kSubpixelAntiAlias);

        const char* text = "Skia";

        sk_tool_utils::add_to_text_blob(&builder, text, font, 0, 0);

        // large
        SkRect bounds;
        font.measureText(text, strlen(text), kUTF8_SkTextEncoding, &bounds);
        SkScalar yOffset = bounds.height();
        font.setSize(162);

        sk_tool_utils::add_to_text_blob(&builder, text, font, 0, yOffset);

        // Medium
        font.measureText(text, strlen(text), kUTF8_SkTextEncoding, &bounds);
        yOffset += bounds.height();
        font.setSize(72);

        sk_tool_utils::add_to_text_blob(&builder, text, font, 0, yOffset);

        // Small
        font.measureText(text, strlen(text), kUTF8_SkTextEncoding, &bounds);
        yOffset += bounds.height();
        font.setSize(32);

        sk_tool_utils::add_to_text_blob(&builder, text, font, 0, yOffset);

        // micro (will fall out of distance field text even if distance field text is enabled)
        font.measureText(text, strlen(text), kUTF8_SkTextEncoding, &bounds);
        yOffset += bounds.height();
        font.setSize(14);

        sk_tool_utils::add_to_text_blob(&builder, text, font, 0, yOffset);

        // Zero size.
        font.measureText(text, strlen(text), kUTF8_SkTextEncoding, &bounds);
        yOffset += bounds.height();
        font.setSize(0);

        sk_tool_utils::add_to_text_blob(&builder, text, font, 0, yOffset);

        // build
        fBlob = builder.make();
    }

    SkString onShortName() override {
        return SkStringPrintf("textblobmixedsizes%s",
                              fUseDFT ? "_df" : "");
    }

    SkISize onISize() override {
        return SkISize::Make(kWidth, kHeight);
    }

    void onDraw(SkCanvas* inputCanvas) override {
        SkCanvas* canvas = inputCanvas;
        sk_sp<SkSurface> surface;
        if (fUseDFT) {
            // Create a new Canvas to enable DFT
            GrContext* ctx = inputCanvas->getGrContext();
            SkISize size = onISize();
            sk_sp<SkColorSpace> colorSpace = inputCanvas->imageInfo().refColorSpace();
            SkImageInfo info = SkImageInfo::MakeN32(size.width(), size.height(),
                                                    kPremul_SkAlphaType, colorSpace);
            SkSurfaceProps props(SkSurfaceProps::kUseDeviceIndependentFonts_Flag,
                                 SkSurfaceProps::kLegacyFontHost_InitType);
            surface = SkSurface::MakeRenderTarget(ctx, SkBudgeted::kNo, info, 0, &props);
            canvas = surface.get() ? surface->getCanvas() : inputCanvas;
            // init our new canvas with the old canvas's matrix
            canvas->setMatrix(inputCanvas->getTotalMatrix());
        }
        canvas->drawColor(SK_ColorWHITE);

        SkRect bounds = fBlob->bounds();

        const int kPadX = SkScalarFloorToInt(bounds.width() / 3);
        const int kPadY = SkScalarFloorToInt(bounds.height() / 3);

        int rowCount = 0;
        canvas->translate(SkIntToScalar(kPadX), SkIntToScalar(kPadY));
        canvas->save();
        SkRandom random;

        SkPaint paint;
        if (!fUseDFT) {
            paint.setColor(SK_ColorWHITE);
        }
        paint.setAntiAlias(false);

        const SkScalar kSigma = SkBlurMask::ConvertRadiusToSigma(SkIntToScalar(8));

        // setup blur paint
        SkPaint blurPaint(paint);
        blurPaint.setColor(SK_ColorBLACK);
        blurPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, kSigma));

        for (int i = 0; i < 4; i++) {
            canvas->save();
            switch (i % 2) {
                case 0:
                    canvas->rotate(random.nextF() * 45.f);
                    break;
                case 1:
                    canvas->rotate(-random.nextF() * 45.f);
                    break;
            }
            if (!fUseDFT) {
                canvas->drawTextBlob(fBlob, 0, 0, blurPaint);
            }
            canvas->drawTextBlob(fBlob, 0, 0, paint);
            canvas->restore();
            canvas->translate(bounds.width() + SK_Scalar1 * kPadX, 0);
            ++rowCount;
            if ((bounds.width() + 2 * kPadX) * rowCount > kWidth) {
                canvas->restore();
                canvas->translate(0, bounds.height() + SK_Scalar1 * kPadY);
                canvas->save();
                rowCount = 0;
            }
        }
        canvas->restore();

        // render offscreen buffer
        if (surface) {
            SkAutoCanvasRestore acr(inputCanvas, true);
            // since we prepended this matrix already, we blit using identity
            inputCanvas->resetMatrix();
            inputCanvas->drawImage(surface->makeImageSnapshot().get(), 0, 0, nullptr);
        }
    }

private:
    sk_sp<SkTextBlob> fBlob;

    static constexpr int kWidth = 2100;
    static constexpr int kHeight = 1900;

    bool fUseDFT;

    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

DEF_GM( return new TextBlobMixedSizes(false); )
DEF_GM( return new TextBlobMixedSizes(true); )
}
