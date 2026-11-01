#include "Encoder.h"

#include <algorithm>
#include <cctype>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace luma {

namespace {

std::string lowerExtension(const std::string& path) {
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return "";
    }
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext;
}

} // namespace

Expected<void, IoError> encodeImage(const std::string& path, const ImageBuffer& image, const EncodeParams& params) {
    try {
        const std::string ext = lowerExtension(path);
        const bool isPng = (ext == ".png");
        const bool wantsSixteenBit = isPng && params.sixteenBitPng;

        cv::Mat quantized;
        const double scale = wantsSixteenBit ? 65535.0 : 255.0;
        const int depth = wantsSixteenBit ? CV_16U : CV_8U;
        // convertTo saturates (cv::saturate_cast) rather than wrapping, so
        // out-of-[0,1] input is clamped here, not a separate pre-pass.
        image.mat().convertTo(quantized, depth, scale);

        cv::Mat bgr;
        cv::cvtColor(quantized, bgr, cv::COLOR_RGB2BGR); // the other designated RGB<->BGR call site

        std::vector<int> writeParams;
        if (ext == ".jpg" || ext == ".jpeg" || ext == ".webp") {
            writeParams = {cv::IMWRITE_JPEG_QUALITY, params.jpegQuality};
        }

        std::vector<int> metadataTypes;
        std::vector<std::vector<unsigned char>> metadata;
        if (params.preserveIcc && image.colorSpace.icc.has_value() && !image.colorSpace.icc->data.empty()) {
            metadataTypes.push_back(cv::IMAGE_METADATA_ICCP);
            const auto& iccBytes = image.colorSpace.icc->data;
            metadata.emplace_back(reinterpret_cast<const unsigned char*>(iccBytes.data()),
                                   reinterpret_cast<const unsigned char*>(iccBytes.data()) + iccBytes.size());
        }

        const bool ok = metadataTypes.empty()
                             ? cv::imwrite(path, bgr, writeParams)
                             : cv::imwriteWithMetadata(path, bgr, metadataTypes, metadata, writeParams);
        if (!ok) {
            return make_unexpected(IoError::EncodeFailed);
        }
        return {};
    } catch (const cv::Exception&) {
        return make_unexpected(IoError::EncodeFailed); // AGENTS.md §5 rule 5
    }
}

} // namespace luma
