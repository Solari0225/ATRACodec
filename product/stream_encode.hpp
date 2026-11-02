#ifndef ATRACODEC_STREAM_ENCODE_HPP
#define ATRACODEC_STREAM_ENCODE_HPP
#include <cstddef>
#include <cstdint>
#include <memory>

namespace atracodec_stream {
using ByteSink = int (*)(void *, const uint8_t *, size_t);
class Encoder {
public:
    virtual ~Encoder() = default;
    virtual void write(const int16_t *interleaved, size_t sample_frames) = 0;
    virtual void finish() = 0;
    virtual uint64_t accepted_frames() const = 0;
};
// Formats/bitrates match the22 published profiles. Input is signed16 stereo
//44100Hz. The exact frame count is known at creation;write must supply
//exactly that count before finish. Storage is independent of track duration.
// The sink receives complete OMA bytes in order,starting with the96-byte
//header during creation. A nonzero result/exception permanently fails the
//stream. Empty input emits only the header and never advances codec state.
// Exceptions report invalid input,failed state or codec/sink errors.
// Calls on one stream must be serialized;callbacks must not reenter it.
std::unique_ptr<Encoder> make_encoder(uint32_t format, uint32_t bitrate,
    uint64_t expected_frames, ByteSink sink, void *sink_context);
}
#endif
