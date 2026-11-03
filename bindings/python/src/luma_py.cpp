// Minimal nanobind surface (PLAN.md P1.8): decode() and run_pipeline(),
// exactly the two functions the task names. No per-stage bindings - T-5's
// parity suite (a separate, still-pending follow-up, same as this
// module's pip-installable packaging) works against this same surface.
//
// nanobind/Python.h must be the first include in this file, full stop:
// Python's pyconfig.h unconditionally #defines _POSIX_C_SOURCE/
// _XOPEN_SOURCE, and if anything else (even a standard header like
// <string>) defines them first with a different value, pyconfig.h's
// redefinition becomes a hard error under -Werror. Confirmed by a real
// CI build failure, not just docs lore.
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>

#include "engine/ParamsJson.h"
#include "engine/Pipeline.h"
#include "image/BufferPool.h"
#include "io/Decoder.h"

namespace nb = nanobind;

namespace {

using ImageArray = nb::ndarray<float, nb::numpy, nb::c_contig, nb::device::cpu>;

void checkRgbShape(const ImageArray& array) {
    if (array.ndim() != 3 || array.shape(2) != 3) {
        throw std::runtime_error("expected a (height, width, 3) float32 array");
    }
}

luma::ImageBuffer imageBufferFromArray(const ImageArray& array) {
    checkRgbShape(array);
    const int height = static_cast<int>(array.shape(0));
    const int width = static_cast<int>(array.shape(1));

    luma::ColorSpace colorSpace{luma::Primaries::sRGB, luma::Transfer::sRGB, std::nullopt};
    luma::ImageBuffer image = luma::BufferPool::instance().acquire(
        width, height, luma::PixelFormat::RGB_F32, colorSpace);
    const std::size_t byteCount =
        static_cast<std::size_t>(height) * static_cast<std::size_t>(width) * 3 * sizeof(float);
    std::memcpy(image.mat().data, array.data(), byteCount);
    return image;
}

// Caller takes ownership via the nanobind capsule; nanobind frees `data`
// through it once the returned numpy array's refcount drops to zero.
ImageArray arrayFromImageBuffer(const luma::ImageBuffer& image) {
    const std::size_t count =
        static_cast<std::size_t>(image.height) * static_cast<std::size_t>(image.width) * 3;
    float* data = new float[count];
    std::memcpy(data, image.mat().data, count * sizeof(float));
    nb::capsule owner(data, [](void* p) noexcept { delete[] static_cast<float*>(p); });
    const std::size_t shape[3] = {static_cast<std::size_t>(image.height),
                                   static_cast<std::size_t>(image.width), 3};
    return ImageArray(data, 3, shape, owner);
}

ImageArray decode(const std::string& path) {
    auto result = luma::decodeImage(path);
    if (!result.has_value()) {
        throw std::runtime_error("decode failed: " + path);
    }
    return arrayFromImageBuffer(*result);
}

nb::tuple runPipeline(const ImageArray& image, const std::string& paramsJson) {
    auto params = luma::paramsFromJson(paramsJson);
    if (!params.has_value()) {
        throw std::runtime_error("invalid params_json");
    }

    const luma::ImageBuffer input = imageBufferFromArray(image);
    auto result = luma::runPipeline(input, *params);
    if (!result.has_value()) {
        throw std::runtime_error("pipeline failed (ColorError " +
                                  std::to_string(static_cast<int>(result.error())) + ")");
    }

    ImageArray outArray = arrayFromImageBuffer(result->image);
    const std::string diagJson = luma::diagnosticsToJson(result->diag);
    return nb::make_tuple(outArray, diagJson);
}

} // namespace

NB_MODULE(luma_py, m) {
    m.def("decode", &decode, nb::arg("path"),
          "Decode an image file to a (height, width, 3) float32 sRGB array in [0,1].");
    m.def("run_pipeline", &runPipeline, nb::arg("image"), nb::arg("params_json"),
          "Run the full classical pipeline. image is a (height, width, 3) float32 sRGB "
          "array in [0,1]; params_json is a (optionally partial) JSON object matching "
          "luma::Params. Returns (output_array, diagnostics_json).");
}
