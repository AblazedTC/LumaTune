#include "Decoder.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <optional>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "../image/BufferPool.h"

namespace luma {

namespace {

std::uint16_t readU16(const unsigned char* p, bool bigEndian) {
    return bigEndian ? static_cast<std::uint16_t>((p[0] << 8) | p[1])
                      : static_cast<std::uint16_t>((p[1] << 8) | p[0]);
}

std::uint32_t readU32(const unsigned char* p, bool bigEndian) {
    return bigEndian
               ? ((static_cast<std::uint32_t>(p[0]) << 24) | (static_cast<std::uint32_t>(p[1]) << 16) |
                  (static_cast<std::uint32_t>(p[2]) << 8) | static_cast<std::uint32_t>(p[3]))
               : ((static_cast<std::uint32_t>(p[3]) << 24) | (static_cast<std::uint32_t>(p[2]) << 16) |
                  (static_cast<std::uint32_t>(p[1]) << 8) | static_cast<std::uint32_t>(p[0]));
}

constexpr std::uint16_t kOrientationTag = 0x0112;
constexpr std::uint16_t kTypeShort = 3;

// Walks the TIFF IFD0 in `tiff` looking for the Orientation tag. `tiff` is
// exactly what cv::imreadWithMetadata returns for IMAGE_METADATA_EXIF: it
// starts directly at the byte-order marker ("II"/"MM"), with the "Exif\0\0"
// APP1 prefix already stripped (verified against OpenCV's JPEG decoder and
// its own test suite, not assumed).
std::optional<std::uint16_t> readOrientationTag(const std::vector<unsigned char>& tiff) {
    if (tiff.size() < 8) {
        return std::nullopt;
    }
    bool bigEndian;
    if (tiff[0] == 'M' && tiff[1] == 'M') {
        bigEndian = true;
    } else if (tiff[0] == 'I' && tiff[1] == 'I') {
        bigEndian = false;
    } else {
        return std::nullopt;
    }
    if (readU16(tiff.data() + 2, bigEndian) != 42) {
        return std::nullopt;
    }

    const std::uint32_t ifdOffset = readU32(tiff.data() + 4, bigEndian);
    if (static_cast<std::size_t>(ifdOffset) + 2 > tiff.size()) {
        return std::nullopt;
    }

    const std::uint16_t entryCount = readU16(tiff.data() + ifdOffset, bigEndian);
    const std::size_t entriesStart = static_cast<std::size_t>(ifdOffset) + 2;
    for (std::uint16_t i = 0; i < entryCount; ++i) {
        const std::size_t entryOffset = entriesStart + static_cast<std::size_t>(i) * 12;
        if (entryOffset + 12 > tiff.size()) {
            break;
        }
        if (readU16(tiff.data() + entryOffset, bigEndian) == kOrientationTag) {
            if (readU16(tiff.data() + entryOffset + 2, bigEndian) != kTypeShort) {
                return std::nullopt;
            }
            return readU16(tiff.data() + entryOffset + 8, bigEndian);
        }
    }
    return std::nullopt;
}

// Pillow's ImageOps.exif_transpose source is the verified reference for
// this table (tag -> correction applied to go from stored to displayed).
Orientation orientationFromExifValue(std::uint16_t value) {
    switch (value) {
        case 1:
            return Orientation::Normal;
        case 2:
            return Orientation::FlipH;
        case 3:
            return Orientation::Rotate180;
        case 4:
            return Orientation::FlipV;
        case 5:
            return Orientation::Transpose;
        case 6:
            return Orientation::Rotate90Cw;
        case 7:
            return Orientation::Transverse;
        case 8:
            return Orientation::Rotate90Ccw;
        default:
            return Orientation::Normal;
    }
}

} // namespace

Expected<ImageBuffer, IoError> decodeImage(const std::string& path) {
    std::error_code existsError;
    if (!std::filesystem::exists(path, existsError) || existsError) {
        return make_unexpected(IoError::FileNotFound);
    }

    cv::Mat decoded;
    std::vector<int> metadataTypes;
    std::vector<std::vector<unsigned char>> metadata;
    try {
        decoded = cv::imreadWithMetadata(path, metadataTypes, metadata, cv::IMREAD_COLOR | cv::IMREAD_ANYDEPTH);
    } catch (const cv::Exception&) {
        return make_unexpected(IoError::DecodeFailed); // AGENTS.md §5 rule 5: no exception crosses this boundary
    }
    if (decoded.empty()) {
        return make_unexpected(IoError::DecodeFailed);
    }

    Orientation orientation = Orientation::Normal;
    IccProfile icc;
    for (std::size_t i = 0; i < metadataTypes.size(); ++i) {
        if (metadataTypes[i] == cv::IMAGE_METADATA_EXIF) {
            if (const auto tag = readOrientationTag(metadata[i])) {
                orientation = orientationFromExifValue(*tag);
            }
        } else if (metadataTypes[i] == cv::IMAGE_METADATA_ICCP && !metadata[i].empty()) {
            icc.data.resize(metadata[i].size());
            std::memcpy(icc.data.data(), metadata[i].data(), metadata[i].size());
        }
    }

    cv::Mat rgb;
    cv::cvtColor(decoded, rgb, cv::COLOR_BGR2RGB); // this is the designated BGR->RGB call site

    ColorSpace colorSpace = icc.data.empty() ? ColorSpace{Primaries::sRGB, Transfer::sRGB, std::nullopt}
                                              : ColorSpace{Primaries::Unknown, Transfer::Unknown, std::move(icc)};

    cv::Mat rgbFloat;
    const double scale = (rgb.depth() == CV_16U) ? (1.0 / 65535.0) : (1.0 / 255.0);
    rgb.convertTo(rgbFloat, CV_32F, scale);

    ImageBuffer buffer =
        BufferPool::instance().acquire(rgbFloat.cols, rgbFloat.rows, PixelFormat::RGB_F32, std::move(colorSpace));
    rgbFloat.copyTo(buffer.mat());
    buffer.exifOrientation = orientation;
    return buffer;
}

} // namespace luma
