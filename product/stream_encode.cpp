#include "stream_encode.hpp"
#include "sony_original_encoder.h"
#include "atracodec_fenv.hpp"
#include "../shared/pcm132/recovered_atrac3_pcm132.h"
#include "../shared/joint66/recovered_atrac3_pcm66.h"
#include "../shared/plus_startup/recovered_plus_stream_timing.h"
#include "../shared/plus_pcm/recovered_plus_pcm_core.h"
#include "../shared/aal_original/original_fixed_predictor.hpp"
#include "../shared/aalp_original/original_aalp_reader.h"
#include "../shared/aal_original/reused/src/aal_format.hpp"
#include "../shared/aal_original/reused/compression/enhancement.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace atracodec_stream {
namespace {
using atracodec_detail::FloatingEnvironment;
using independent_aal::compression::EnhancementPacket;

class StreamEncoder final : public Encoder {
    // The longest retained source span is the original plus7-call startup:
    //at output ordinal o,up to(o+8)*2048 input frames have been accepted,
    //while enhancement starts at(o-1)*2048-368. Twelve slots cover it.
    static constexpr size_t source_capacity = 12 * 2048;
    sony_original_profile profile_{};
    const independent_aal::EncodingProfile *aal_profile_ = nullptr;
    uint64_t expected_ = 0, accepted_ = 0, expected_packets_ = 0, packets_ = 0;
    uint32_t block_frames_ = 0, buffered_ = 0;
    uint16_t tail_ = 0;
    ByteSink sink_ = nullptr;
    void *sink_context_ = nullptr;
    bool failed_ = false, finished_ = false;
    std::array<int16_t,4096> input_{};
    std::array<int16_t,source_capacity * 2> source_{};
    std::unique_ptr<sony_atrac3_independent_stereo_stream> stereo_;
    std::unique_ptr<sony_atrac3_pcm66_stream> joint_;
    std::unique_ptr<sony_plus_pcm_core_state> plus_;
    std::unique_ptr<sony_aal_original::OriginalFixedPredictor> aal3_predictor_;
    std::unique_ptr<sony_aalp_predictor_state> aalp_predictor_;
    sony_plus_stream_timing timing_{};
    std::array<std::array<uint8_t,2048>,6> fifo_{};
    std::array<uint64_t,6> fifo_id_{};

    bool lossless() const { return aal_profile_ != nullptr; }
    bool is_plus() const {
        return profile_.format == SONY_ORIGINAL_ATRAC3PLUS ||
               profile_.format == SONY_ORIGINAL_AALP;
    }
    void open() const {
        if(failed_) throw std::runtime_error("Encoder stream has failed");
        if(finished_) throw std::runtime_error("Encoder stream is already finished");
    }
    void emit(const uint8_t *bytes, size_t count) {
        // A client callback may alter its thread's rounding/trap state. Restore
        //the codec's canonical environment before performing more mathematics.
        const FloatingEnvironment callback_environment;
        if(sink_(sink_context_, bytes, count)) {
            failed_ = true;
            throw std::runtime_error("Encoded output sink failed");
        }
    }
    void retain_source(const int16_t *pcm, size_t count) {
        if(!lossless()) return;
        size_t offset = size_t(accepted_ % source_capacity);
        const size_t first = std::min(count, source_capacity - offset);
        std::copy_n(pcm, first * 2, source_.data() + offset * 2);
        std::copy_n(pcm + first * 2, (count - first) * 2, source_.data());
    }
    int16_t source_sample(uint64_t index, unsigned channel) const {
        if(index >= expected_) return 0;
        if(index >= accepted_ || accepted_ - index > source_capacity)
            throw std::runtime_error("Encoder source alignment failed");
        return source_[size_t(index % source_capacity) * 2 + channel];
    }
    int32_t residual(uint64_t ordinal, unsigned j, unsigned ch,
                     int16_t prediction) const {
        const uint64_t position = (ordinal - 1) * block_frames_ + j;
        if(position < aal_profile_->pcm_trim) return 0;
        const uint16_t difference = uint16_t(uint16_t(source_sample(
            position - aal_profile_->pcm_trim, ch)) - uint16_t(prediction));
        int16_t signed_difference;
        std::memcpy(&signed_difference, &difference, sizeof(difference));
        return signed_difference;
    }
    void output_core(const uint8_t *core) {
        if(packets_ >= expected_packets_)
            throw std::runtime_error("Encoder packet count exceeded its plan");
        if(!lossless()) {
            emit(core, profile_.core_frame_bytes);
        } else {
            EnhancementPacket enhancement;
            if(profile_.format == SONY_ORIGINAL_AAL3) {
                const auto predicted = aal3_predictor_->decode(core);
                if(packets_) {
                    independent_aal::compression::StereoFrame errors{};
                    for(unsigned ch=0;ch<2;++ch) for(unsigned j=0;j<1024;++j)
                        errors[ch][j] = residual(packets_, j, ch, predicted[j*2+ch]);
                    enhancement = independent_aal::compression::encode_enhancement(errors);
                }
            } else {
                sony_aalp_core_fields fields;
                uint32_t positions[10];
                int16_t predicted[2][2048];
                if(sony_original_aalp_read_core(core, profile_.core_frame_bytes,
                       &fields, positions) || fields.channels != 2)
                    throw std::runtime_error("AAL physical core parsing failed");
                const uint32_t cap = profile_.core_frame_bytes <= 744 ? 8 :
                    (profile_.core_frame_bytes <= 1488 ? 12 : 16);
                if(sony_original_aalp_predict(&fields, cap, aalp_predictor_.get(), predicted))
                    throw std::runtime_error("AAL integer prediction failed");
                if(packets_) {
                    independent_aal::compression::PlusStereoFrame errors{};
                    for(unsigned ch=0;ch<2;++ch) for(unsigned j=0;j<2048;++j)
                        errors[ch][j] = residual(packets_, j, ch, predicted[ch][j]);
                    enhancement = independent_aal::compression::encode_plus_enhancement(errors);
                }
            }
            // Only this one bounded packet is allocated. No preceding core,
            //prediction,residual or serialized output is retained.
            const auto packet = independent_aal::aal_block(core, uint32_t(packets_),
                enhancement.bytes, packets_ + 1 == expected_packets_, tail_,
                enhancement.padding_bytes, *aal_profile_);
            emit(packet.data(), packet.size());
        }
        ++packets_;
    }
    void plus_action(const sony_plus_stream_action &action) {
        if(sony_recovered_plus_pcm_core_analyze(plus_.get(), input_.data()) < 0)
            throw std::runtime_error("ATRAC3plus block encoding failed");
        if(action.should_pack) {
            const size_t slot = size_t(action.packet_ordinal % 6);
            if(sony_recovered_plus_pcm_core_pack(plus_.get(), fifo_[slot].data(),
                    uint32_t(fifo_[slot].size())) != int32_t(profile_.core_frame_bytes))
                throw std::runtime_error("ATRAC3plus frame packing failed");
            fifo_id_[slot] = action.packet_ordinal;
        }
        if(action.should_emit) {
            const size_t slot = size_t(action.output_packet_ordinal % 6);
            if(fifo_id_[slot] != action.output_packet_ordinal ||
               action.output_packet_ordinal != packets_)
                throw std::runtime_error("ATRAC3plus packet delay failed");
            output_core(fifo_[slot].data());
        }
    }
    void atrac3_block() {
        std::array<uint8_t,387> core{};
        uint32_t end_bits[2];
        const int status = joint_ ? sony_recovered_atrac3_pcm66_stream_block(
            joint_.get(), input_.data(), core.data(), end_bits) :
            sony_recovered_atrac3_stereo_stream_block(stereo_.get(), input_.data(),
                core.data(), uint32_t(core.size()), end_bits);
        if(status < 0) throw std::runtime_error("ATRAC3 block encoding failed");
        if(status) output_core(core.data());
    }
    void data_block() {
        if(is_plus()) {
            sony_plus_stream_action action;
            if(sony_recovered_plus_stream_next(&timing_, &action) || action.is_eof ||
               action.source_samples != buffered_ ||
               action.source_offset != accepted_ - buffered_)
                throw std::runtime_error("ATRAC3plus input schedule failed");
            plus_action(action);
        } else atrac3_block();
        buffered_ = 0;
        input_.fill(0);
    }
    void header() {
        std::array<uint8_t,96> bytes{};
        auto *h = bytes.data();
        std::memcpy(h, "EA3", 3); h[3] = lossless() ? 3 : 1;
        h[5] = 96; h[6] = 0xff; h[7] = lossless() ? 0x80 : 0xff;
        if(lossless()) {
            h[32] = aal_profile_->oma_codec_id;
            h[33] = aal_profile_->oma_frame_parameters;
            independent_aal::be16put(h+34, tail_);
            // Actual original empty-file headers retain the full block tail
            //and the signed negative one-block duration modulo uint32.
            uint64_t milliseconds = is_plus() ? 0xffffffd2u : 0xffffffe9u;
            if(expected_) {
                const uint64_t declared = (expected_packets_-2)*block_frames_ + tail_;
                milliseconds = declared / 44100 * 1000 +
                    declared % 44100 * 1000 / 44100;
            }
            if(milliseconds > UINT32_MAX)
                throw std::length_error("AAL duration exceeds its32-bit millisecond field");
            independent_aal::be32put(h+36, uint32_t(milliseconds));
            independent_aal::be32put(h+40, uint32_t(expected_packets_));
            h[48] = is_plus() ? 1 : 0;
            h[49] = uint8_t(aal_profile_->core_parameters >> 16);
            independent_aal::be16put(h+50, uint16_t(aal_profile_->core_parameters));
        } else {
            const uint32_t parameters = is_plus() ?
                0x002800 | (profile_.core_frame_bytes/8 - 1) :
                (profile_.nominal_bitrate == 66000 ? 0x022018 :
                 (profile_.nominal_bitrate == 105000 ? 0x002026 : 0x002030));
            h[32] = is_plus() ? 1 : 0;
            h[33] = uint8_t(parameters >> 16);
            h[34] = uint8_t(parameters >> 8); h[35] = uint8_t(parameters);
        }
        emit(h, bytes.size());
    }
public:
    StreamEncoder(const sony_original_profile &profile, uint64_t expected,
                  ByteSink sink, void *context)
        : profile_(profile), expected_(expected), block_frames_(profile.core_frame_samples),
          sink_(sink), sink_context_(context) {
        if(!sink_) throw std::invalid_argument("Encoded output sink is null");
        if(profile.lossless) {
            for(const auto &p : independent_aal::kOfficialProfiles)
                if(p.nominal_bitrate == profile.nominal_bitrate &&
                   ((p.codec == independent_aal::CoreCodec::Atrac3plus) == is_plus()))
                    aal_profile_ = &p;
            if(!aal_profile_) throw std::invalid_argument("Unsupported AAL profile");
            if(expected_) expected_packets_ = expected_ / block_frames_ +
                (expected_ % block_frames_ + aal_profile_->pcm_trim + block_frames_ - 1) /
                 block_frames_ + 1;
            if(expected_packets_ > UINT32_MAX)
                throw std::length_error("AAL packet count exceeds its32-bit field");
            tail_ = expected_ ? uint16_t((expected_-1) % block_frames_ + 1) :
                uint16_t(block_frames_);
        }
        if(!expected_) { header(); return; }
        if(is_plus()) {
            sony_plus_spectrum_config config;
            sony_plus_stream_plan plan;
            if(sony_recovered_plus_spectrum_default(profile.nominal_bitrate, &config) ||
               sony_recovered_plus_stream_plan(expected_, profile.core_frame_bytes,
                   lossless() ? 0 : 1, 1, &plan) ||
               sony_recovered_plus_stream_initialize(&timing_, &plan))
                throw std::length_error("ATRAC3plus stream plan is invalid or overflows");
            if(lossless() && expected_packets_ != plan.output_frames)
                throw std::runtime_error("AAL frame plan mismatch");
            expected_packets_ = plan.output_frames;
            plus_ = std::make_unique<sony_plus_pcm_core_state>();
            if(sony_recovered_plus_pcm_core_initialize(profile.nominal_bitrate,
                    config.allocation.initial.request_mode, plus_.get()))
                throw std::runtime_error("ATRAC3plus initialization failed");
            fifo_id_.fill(UINT64_MAX);
        } else {
            if(!lossless()) expected_packets_ = expected_ / 1024 +
                (expected_ % 1024 != 0) + 2;
            if(profile.nominal_bitrate == 66000) {
                joint_ = std::make_unique<sony_atrac3_pcm66_stream>();
                sony_recovered_atrac3_pcm66_stream_init(joint_.get());
            } else {
                stereo_ = std::make_unique<sony_atrac3_independent_stereo_stream>();
                if(sony_recovered_atrac3_stereo_stream_init(stereo_.get(), profile.nominal_bitrate))
                    throw std::runtime_error("ATRAC3 initialization failed");
            }
        }
        if(profile.format == SONY_ORIGINAL_AAL3)
            aal3_predictor_ = std::make_unique<sony_aal_original::OriginalFixedPredictor>();
        if(profile.format == SONY_ORIGINAL_AALP) {
            aalp_predictor_ = std::make_unique<sony_aalp_predictor_state>();
            sony_original_aalp_predictor_reset(aalp_predictor_.get());
        }
        header();
    }
    void write(const int16_t *pcm, size_t frames) override {
        open();
        try {
            const FloatingEnvironment environment;
            if((frames && !pcm) || frames > SIZE_MAX/(2*sizeof(int16_t)) ||
               uint64_t(frames) > expected_ - accepted_)
                throw std::invalid_argument("PCM write exceeds its declared frame count or has invalid storage");
            while(frames) {
                const size_t take = std::min<size_t>(frames, block_frames_ - buffered_);
                retain_source(pcm, take);
                std::copy_n(pcm, take*2, input_.data()+buffered_*2);
                pcm += take*2; frames -= take;
                buffered_ += uint32_t(take); accepted_ += take;
                if(buffered_ == block_frames_) data_block();
            }
        } catch(...) { failed_ = true; throw; }
    }
    void finish() override {
        open();
        try {
            const FloatingEnvironment environment;
            if(accepted_ != expected_)
                throw std::invalid_argument("PCM stream ended before its declared frame count");
            if(!expected_) { finished_ = true; return; }
            if(buffered_) data_block(); // The retained input tail is already zero.
            if(is_plus()) {
                sony_plus_stream_action action;
                int result;
                while((result = sony_recovered_plus_stream_next(&timing_, &action)) == 0) {
                    if(!action.is_eof) throw std::runtime_error("ATRAC3plus EOF schedule failed");
                    plus_action(action);
                }
                if(result != 1) throw std::runtime_error("ATRAC3plus EOF failed");
            } else if(lossless()) {
                while(packets_ < expected_packets_) atrac3_block();
            } else {
                std::array<uint8_t,387> core{};
                uint32_t end_bits[2];
                for(unsigned j=0;j<3;++j) {
                    const int status = joint_ ? sony_recovered_atrac3_pcm66_stream_finish(
                        joint_.get(), core.data(), end_bits) :
                        sony_recovered_atrac3_stereo_stream_finish(stereo_.get(), core.data(),
                            uint32_t(core.size()), end_bits);
                    if(status < 0) throw std::runtime_error("ATRAC3 EOF failed");
                    if(status) output_core(core.data());
                }
            }
            if(packets_ != expected_packets_)
                throw std::runtime_error("Encoder packet count differs from its plan");
            finished_ = true;
        } catch(...) { failed_ = true; throw; }
    }
    uint64_t accepted_frames() const override { return accepted_; }
};
}

std::unique_ptr<Encoder> make_encoder(uint32_t format, uint32_t bitrate,
    uint64_t expected_frames, ByteSink sink, void *sink_context) {
    const FloatingEnvironment environment;
    sony_original_profile profile;
    for(uint32_t i=0;i<sony_original_encoder_profile_count();++i) {
        if(sony_original_encoder_profile_at(i, &profile))
            throw std::runtime_error("Encoder profile enumeration failed");
        if(profile.format == format && profile.nominal_bitrate == bitrate)
            return std::make_unique<StreamEncoder>(profile, expected_frames, sink, sink_context);
    }
    throw std::invalid_argument("Unsupported format and bitrate");
}
}
