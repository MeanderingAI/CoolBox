// GIF-flavoured variable-width LZW compressor/decompressor.
//
// This matches the algorithm mandated by the GIF89a specification:
//   - Codes start at (min_code_size + 1) bits wide.
//   - Code 0 has a special first Clear Code = 1 << min_code_size, and the
//     End Code follows immediately (Clear + 1). Literal pixel indices occupy
//     codes [0, Clear Code).
//   - New codes are assigned sequentially starting at Clear + 2.
//   - Code width grows by one bit as soon as the *next* code to be assigned
//     would not fit in the current width, capping at 12 bits.
//   - When the dictionary would overflow 4096 entries, a fresh Clear Code is
//     emitted and the dictionary/code-width reset.
//   - Bits are packed into bytes least-significant-bit first.
#include "../headers/video_container.h"

#include <stdexcept>
#include <unordered_map>

namespace trekker {
namespace video {
namespace container {

namespace {

constexpr int kMaxCodeBits = 12;
constexpr int kMaxCodeCount = 1 << kMaxCodeBits; // 4096

// Packs variable-width codes into bytes, LSB-first, matching GIF's bit order.
class LzwBitWriter {
public:
    void write_code(unsigned code, int bits) {
        bit_buffer_ |= static_cast<std::uint64_t>(code) << bit_count_;
        bit_count_ += bits;
        while (bit_count_ >= 8) {
            bytes_.push_back(static_cast<std::uint8_t>(bit_buffer_ & 0xFFu));
            bit_buffer_ >>= 8;
            bit_count_ -= 8;
        }
    }

    std::vector<std::uint8_t> finish() {
        if (bit_count_ > 0) {
            bytes_.push_back(static_cast<std::uint8_t>(bit_buffer_ & 0xFFu));
            bit_buffer_ = 0;
            bit_count_ = 0;
        }
        return std::move(bytes_);
    }

private:
    std::vector<std::uint8_t> bytes_;
    std::uint64_t bit_buffer_ = 0;
    int bit_count_ = 0;
};

class LzwBitReader {
public:
    explicit LzwBitReader(const std::vector<std::uint8_t>& data) : data_(data) {}

    // Returns false once the underlying byte stream is exhausted and no more
    // whole codes can be produced.
    bool read_code(int bits, unsigned& out_code) {
        while (bit_count_ < bits) {
            if (byte_pos_ >= data_.size()) {
                if (bit_count_ == 0) return false;
                break; // allow reading trailing padding bits at EOF
            }
            bit_buffer_ |= static_cast<std::uint64_t>(data_[byte_pos_++]) << bit_count_;
            bit_count_ += 8;
        }
        out_code = static_cast<unsigned>(bit_buffer_ & ((1u << bits) - 1u));
        bit_buffer_ >>= bits;
        bit_count_ -= bits;
        return true;
    }

private:
    const std::vector<std::uint8_t>& data_;
    std::size_t byte_pos_ = 0;
    std::uint64_t bit_buffer_ = 0;
    int bit_count_ = 0;
};

// Dictionary key: (prefix code, next literal byte).
inline std::uint64_t dict_key(unsigned prefix, std::uint8_t suffix) {
    return (static_cast<std::uint64_t>(prefix) << 8) | suffix;
}

} // namespace

std::vector<std::uint8_t> gif_lzw_compress(const std::vector<std::uint8_t>& indices,
                                           int min_code_size) {
    if (min_code_size < 2 || min_code_size > 8) {
        throw std::invalid_argument("gif_lzw_compress: min_code_size must be in [2,8]");
    }
    const unsigned clear_code = 1u << min_code_size;
    const unsigned end_code   = clear_code + 1u;

    LzwBitWriter writer;

    auto reset_dictionary = [&](std::unordered_map<std::uint64_t, unsigned>& dict,
                                unsigned& next_code, int& code_size) {
        dict.clear();
        next_code = end_code + 1u;
        code_size = min_code_size + 1;
    };

    std::unordered_map<std::uint64_t, unsigned> dict;
    unsigned next_code = 0;
    int code_size = 0;
    reset_dictionary(dict, next_code, code_size);
    writer.write_code(clear_code, code_size);

    if (indices.empty()) {
        writer.write_code(end_code, code_size);
        return writer.finish();
    }

    unsigned prefix = indices[0]; // literal codes map 1:1 to pixel index values
    for (std::size_t i = 1; i < indices.size(); ++i) {
        const std::uint8_t symbol = indices[i];
        const std::uint64_t key = dict_key(prefix, symbol);
        const auto it = dict.find(key);
        if (it != dict.end()) {
            prefix = it->second;
            continue;
        }

        writer.write_code(prefix, code_size);

        if (next_code == kMaxCodeCount) {
            // Dictionary full — emit Clear and start over.
            writer.write_code(clear_code, code_size);
            reset_dictionary(dict, next_code, code_size);
        } else {
            dict.emplace(key, next_code);
            ++next_code;
            // Grow code width the moment the dictionary needs one more bit —
            // i.e. as soon as next_code reaches the current width's capacity
            // (2^code_size), since that next code will need one more bit the
            // next time it is emitted.
            if (next_code >= (1u << code_size) && code_size < kMaxCodeBits) {
                ++code_size;
            }
        }
        prefix = symbol;
    }
    writer.write_code(prefix, code_size);
    writer.write_code(end_code, code_size);
    return writer.finish();
}

std::vector<std::uint8_t> gif_lzw_decompress(const std::vector<std::uint8_t>& compressed,
                                             int min_code_size,
                                             std::size_t expected_count) {
    if (min_code_size < 2 || min_code_size > 8) {
        throw std::invalid_argument("gif_lzw_decompress: min_code_size must be in [2,8]");
    }
    const unsigned clear_code = 1u << min_code_size;
    const unsigned end_code   = clear_code + 1u;

    LzwBitReader reader(compressed);

    std::vector<std::vector<std::uint8_t>> table;
    int code_size = 0;
    unsigned next_code = 0;

    auto reset_dictionary = [&]() {
        table.clear();
        table.resize(end_code + 1u);
        for (unsigned c = 0; c < clear_code; ++c) table[c] = {static_cast<std::uint8_t>(c)};
        next_code = end_code + 1u;
        code_size = min_code_size + 1;
    };
    reset_dictionary();

    std::vector<std::uint8_t> output;
    output.reserve(expected_count);

    std::vector<std::uint8_t> prev_entry;
    bool have_prev = false;

    // Whether a dictionary slot has been reserved for the code about to be
    // read, and its index if so.
    bool have_pending_slot = false;
    unsigned pending_slot = 0;

    while (true) {
        // Mirror the encoder's timing exactly: the encoder inserts a new
        // dictionary entry (and possibly grows the code width) immediately
        // after emitting each code, which affects the width of the *very
        // next* code it writes. The decoder only learns a new entry's byte
        // content once it has decoded the following code (chicken-and-egg),
        // but the entry's *slot index* — and therefore whether this triggers
        // a width bump — is known as soon as the previous code has been
        // decoded. So the slot/width bookkeeping must happen here, before
        // reading the next code, rather than after decoding it.
        if (have_prev && next_code < static_cast<unsigned>(kMaxCodeCount)) {
            have_pending_slot = true;
            pending_slot = next_code;
            ++next_code;
            if (next_code >= (1u << code_size) && code_size < kMaxCodeBits) {
                ++code_size;
            }
        } else {
            have_pending_slot = false;
        }

        unsigned code = 0;
        if (!reader.read_code(code_size, code)) break;

        if (code == clear_code) {
            reset_dictionary();
            have_prev = false;
            continue;
        }
        if (code == end_code) {
            break;
        }

        std::vector<std::uint8_t> entry;
        if (code < table.size() && !table[code].empty()) {
            entry = table[code];
        } else if (have_pending_slot && code == pending_slot && have_prev) {
            // The classic LZW "KwKwK" special case: the code being decoded is
            // the one this very slot was reserved for (the encoder hasn't
            // finished defining it yet) — it must be prev_entry + prev_entry's
            // own first byte.
            entry = prev_entry;
            entry.push_back(prev_entry.front());
        } else {
            throw std::runtime_error("gif_lzw_decompress: invalid code sequence");
        }

        output.insert(output.end(), entry.begin(), entry.end());

        if (have_pending_slot) {
            std::vector<std::uint8_t> new_entry = prev_entry;
            new_entry.push_back(entry.front());
            if (pending_slot >= table.size()) table.resize(pending_slot + 1u);
            table[pending_slot] = std::move(new_entry);
        }

        prev_entry = entry;
        have_prev = true;

        if (output.size() >= expected_count) break;
    }

    return output;
}

} // namespace container
} // namespace video
} // namespace trekker
