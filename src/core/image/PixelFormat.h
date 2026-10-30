#pragma once

namespace luma {

// Interleaved pixel layouts luma_core moves data in. RGB_F32 is canonical
// (PLAN.md §6.1); the others exist for decode/encode/CLAHE boundaries.
enum class PixelFormat { RGB_F32, RGBA_F32, GRAY_F32, GRAY_U16, RGB_U16, RGB_U8 };

// Tone response curve applied to the pixel values, independent of PixelFormat.
enum class Transfer { Linear, sRGB, Gamma22, Unknown };

// Chromaticity primaries the pixel values are expressed in.
enum class Primaries { sRGB, AdobeRGB, ProPhoto, CameraNative, Unknown };

int channelCount(PixelFormat format);
int bytesPerChannel(PixelFormat format);

} // namespace luma
