#include "Clahe.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include <opencv2/core/utility.hpp>

namespace luma {

namespace {

constexpr float kLMax = 100.0f;

int binIndex(float value, int bins) {
    const float clamped = std::clamp(value, 0.0f, kLMax);
    const int bin = static_cast<int>((clamped / kLMax) * static_cast<float>(bins));
    return std::clamp(bin, 0, bins - 1);
}

// One tile's clipped-and-redistributed CDF, rescaled to [0,100]. Mirrors
// OpenCV's 8-bit clahe.cpp clip/redistribute formula (clip limit, excess
// spread evenly with a residual-spreading pass so the redistributed
// total still sums to exactly `tilePixels`), generalised to float.
std::vector<float> buildTileLut(const cv::Mat& L, const cv::Rect& tileRoi, int bins,
                                 float clipLimit) {
    std::vector<int> hist(static_cast<std::size_t>(bins), 0);
    const int tilePixels = tileRoi.width * tileRoi.height;
    for (int row = tileRoi.y; row < tileRoi.y + tileRoi.height; ++row) {
        const float* rowPtr = L.ptr<float>(row);
        for (int col = tileRoi.x; col < tileRoi.x + tileRoi.width; ++col) {
            ++hist[static_cast<std::size_t>(binIndex(rowPtr[col], bins))];
        }
    }

    if (clipLimit > 0.0f) {
        const int clip =
            std::max(1, static_cast<int>(clipLimit * static_cast<float>(tilePixels) /
                                          static_cast<float>(bins)));
        int clipped = 0;
        for (int i = 0; i < bins; ++i) {
            if (hist[static_cast<std::size_t>(i)] > clip) {
                clipped += hist[static_cast<std::size_t>(i)] - clip;
                hist[static_cast<std::size_t>(i)] = clip;
            }
        }
        const int redistBatch = clipped / bins;
        int residual = clipped - redistBatch * bins;
        for (int i = 0; i < bins; ++i) {
            hist[static_cast<std::size_t>(i)] += redistBatch;
        }
        if (residual != 0) {
            const int step = std::max(bins / residual, 1);
            for (int i = 0; i < bins && residual > 0; i += step, --residual) {
                ++hist[static_cast<std::size_t>(i)];
            }
        }
    }

    std::vector<float> lut(static_cast<std::size_t>(bins));
    const float scale = kLMax / static_cast<float>(tilePixels);
    int sum = 0;
    for (int i = 0; i < bins; ++i) {
        sum += hist[static_cast<std::size_t>(i)];
        lut[static_cast<std::size_t>(i)] = std::clamp(static_cast<float>(sum) * scale, 0.0f, kLMax);
    }
    return lut;
}

} // namespace

ClaheResult clahe(const cv::Mat& lab, const ClaheParams& params) {
    CV_Assert(lab.channels() == 3 && lab.depth() == CV_32F);
    CV_Assert(params.tilesX > 0 && params.tilesY > 0 && params.bins > 0);

    ClaheDiag diag;
    diag.clipLimit = params.clipLimit;
    diag.tilesX = params.tilesX;
    diag.tilesY = params.tilesY;
    diag.bins = params.bins;

    if (params.bypass) {
        return {lab.clone(), diag};
    }

    std::vector<cv::Mat> channels;
    cv::split(lab, channels);
    const cv::Mat& L = channels[0];

    const int tilesX = params.tilesX;
    const int tilesY = params.tilesY;
    const int tileWidth = (L.cols + tilesX - 1) / tilesX;
    const int tileHeight = (L.rows + tilesY - 1) / tilesY;

    std::vector<std::vector<float>> tileLuts(static_cast<std::size_t>(tilesX * tilesY));
    cv::parallel_for_(cv::Range(0, tilesX * tilesY), [&](const cv::Range& range) {
        for (int k = range.start; k < range.end; ++k) {
            const int tx = k % tilesX;
            const int ty = k / tilesX;
            const cv::Rect roi(tx * tileWidth, ty * tileHeight,
                                std::min(tileWidth, L.cols - tx * tileWidth),
                                std::min(tileHeight, L.rows - ty * tileHeight));
            tileLuts[static_cast<std::size_t>(k)] =
                buildTileLut(L, roi, params.bins, params.clipLimit);
        }
    });

    const auto lutAt = [&](int ty, int tx, int bin) {
        return tileLuts[static_cast<std::size_t>(ty * tilesX + tx)][static_cast<std::size_t>(bin)];
    };

    // Tile-center interpolation geometry, copied exactly from OpenCV's own
    // CLAHE_Interpolation_Body (modules/imgproc/src/clahe.cpp): position is
    // measured in tile-width/-height units offset by half a tile, floored
    // for the "earlier" neighbour, with the two neighbours clamped (not
    // wrapped) at the grid edges.
    const float tileHeightF = static_cast<float>(tileHeight);
    const float tileWidthF = static_cast<float>(tileWidth);

    cv::Mat outputL(L.rows, L.cols, CV_32F);
    cv::parallel_for_(cv::Range(0, L.rows), [&](const cv::Range& range) {
        for (int row = range.start; row < range.end; ++row) {
            const float tyf = static_cast<float>(row) / tileHeightF - 0.5f;
            const int ty1raw = static_cast<int>(std::floor(tyf));
            const int ty2raw = ty1raw + 1;
            const float ya = tyf - static_cast<float>(ty1raw);
            const int ty1 = std::clamp(ty1raw, 0, tilesY - 1);
            const int ty2c = std::clamp(ty2raw, 0, tilesY - 1);

            const float* srcRow = L.ptr<float>(row);
            float* dstRow = outputL.ptr<float>(row);
            for (int col = 0; col < L.cols; ++col) {
                const float txf = static_cast<float>(col) / tileWidthF - 0.5f;
                const int tx1raw = static_cast<int>(std::floor(txf));
                const int tx2raw = tx1raw + 1;
                const float xa = txf - static_cast<float>(tx1raw);
                const int tx1 = std::clamp(tx1raw, 0, tilesX - 1);
                const int tx2c = std::clamp(tx2raw, 0, tilesX - 1);

                const int bin = binIndex(srcRow[col], params.bins);
                const float v11 = lutAt(ty1, tx1, bin);
                const float v12 = lutAt(ty1, tx2c, bin);
                const float v21 = lutAt(ty2c, tx1, bin);
                const float v22 = lutAt(ty2c, tx2c, bin);

                dstRow[col] = v11 * (1.0f - xa) * (1.0f - ya) + v12 * xa * (1.0f - ya) +
                              v21 * (1.0f - xa) * ya + v22 * xa * ya;
            }
        }
    });

    channels[0] = outputL;
    ClaheResult result;
    cv::merge(channels, result.lab);
    result.diag = diag;
    return result;
}

} // namespace luma
