#include "inflate.h"

namespace docs {

namespace {

// Reads bits LSB-first from a byte buffer, as required by RFC1951.
struct BitReader {
    const unsigned char* data;
    size_t size;
    size_t pos = 0;
    unsigned bitbuf = 0;
    int bitcnt = 0;

    BitReader(const unsigned char* d, size_t s) : data(d), size(s) {}

    int get_bit() {
        if (bitcnt == 0) {
            if (pos >= size) return -1;
            bitbuf = data[pos++];
            bitcnt = 8;
        }
        const int bit = static_cast<int>(bitbuf & 1u);
        bitbuf >>= 1;
        --bitcnt;
        return bit;
    }

    long get_bits(int n) {
        long value = 0;
        for (int i = 0; i < n; ++i) {
            const int bit = get_bit();
            if (bit < 0) return -1;
            value |= (static_cast<long>(bit) << i);
        }
        return value;
    }

    void align_to_byte() {
        bitbuf = 0;
        bitcnt = 0;
    }
};

// Canonical Huffman decoder built from a per-symbol code-length table
// (the standard "counts + sorted symbols" construction used by RFC1951).
struct HuffTree {
    std::vector<int> counts;
    std::vector<int> symbols;

    void build(const std::vector<int>& lengths) {
        counts.assign(16, 0);
        for (const int len : lengths) counts[static_cast<size_t>(len)]++;
        counts[0] = 0;

        std::vector<int> offsets(16, 0);
        for (int len = 1; len < 16; ++len) offsets[len] = offsets[len - 1] + counts[len - 1];

        symbols.assign(lengths.size(), 0);
        for (size_t sym = 0; sym < lengths.size(); ++sym) {
            const int len = lengths[sym];
            if (len != 0) symbols[static_cast<size_t>(offsets[len]++)] = static_cast<int>(sym);
        }
    }

    int decode(BitReader& br) const {
        int code = 0;
        int first = 0;
        int index = 0;
        for (int len = 1; len < 16; ++len) {
            const int bit = br.get_bit();
            if (bit < 0) return -1;
            code |= bit;
            const int count = counts[static_cast<size_t>(len)];
            if (code - first < count) return symbols[static_cast<size_t>(index + (code - first))];
            index += count;
            first += count;
            first <<= 1;
            code <<= 1;
        }
        return -1;
    }
};

bool inflate_block_stored(BitReader& br, std::string& out) {
    br.align_to_byte();
    if (br.pos + 4 > br.size) return false;
    const unsigned len = static_cast<unsigned>(br.data[br.pos]) | (static_cast<unsigned>(br.data[br.pos + 1]) << 8);
    br.pos += 4; // skip LEN and its one's-complement NLEN
    if (br.pos + len > br.size) return false;
    out.append(reinterpret_cast<const char*>(br.data + br.pos), len);
    br.pos += len;
    return true;
}

bool inflate_block_huffman(BitReader& br, const HuffTree& lit_tree, const HuffTree& dist_tree, std::string& out) {
    static const int length_base[29] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27,
        31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static const int length_extra[29] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
        2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static const int dist_base[30] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
        193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    static const int dist_extra[30] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
        6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

    while (true) {
        const int sym = lit_tree.decode(br);
        if (sym < 0) return false;
        if (sym < 256) {
            out.push_back(static_cast<char>(sym));
            continue;
        }
        if (sym == 256) return true; // end-of-block marker

        const int lsym = sym - 257;
        if (lsym < 0 || lsym >= 29) return false;
        const long lextra = length_extra[lsym] != 0 ? br.get_bits(length_extra[lsym]) : 0;
        if (lextra < 0) return false;
        const int length = length_base[lsym] + static_cast<int>(lextra);

        const int dsym = dist_tree.decode(br);
        if (dsym < 0 || dsym >= 30) return false;
        const long dextra = dist_extra[dsym] != 0 ? br.get_bits(dist_extra[dsym]) : 0;
        if (dextra < 0) return false;
        const int distance = dist_base[dsym] + static_cast<int>(dextra);

        if (static_cast<size_t>(distance) > out.size()) return false;
        const size_t start = out.size() - static_cast<size_t>(distance);
        for (int i = 0; i < length; ++i) out.push_back(out[start + static_cast<size_t>(i)]);
    }
}

void build_fixed_trees(HuffTree& lit_tree, HuffTree& dist_tree) {
    std::vector<int> lit_lengths(288);
    for (int i = 0; i < 144; ++i) lit_lengths[static_cast<size_t>(i)] = 8;
    for (int i = 144; i < 256; ++i) lit_lengths[static_cast<size_t>(i)] = 9;
    for (int i = 256; i < 280; ++i) lit_lengths[static_cast<size_t>(i)] = 7;
    for (int i = 280; i < 288; ++i) lit_lengths[static_cast<size_t>(i)] = 8;
    lit_tree.build(lit_lengths);

    const std::vector<int> dist_lengths(30, 5);
    dist_tree.build(dist_lengths);
}

bool inflate_block_dynamic(BitReader& br, std::string& out) {
    static const int order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    long hlit = br.get_bits(5);
    if (hlit < 0) return false;
    hlit += 257;
    long hdist = br.get_bits(5);
    if (hdist < 0) return false;
    hdist += 1;
    long hclen = br.get_bits(4);
    if (hclen < 0) return false;
    hclen += 4;

    std::vector<int> cl_lengths(19, 0);
    for (int i = 0; i < hclen; ++i) {
        const long v = br.get_bits(3);
        if (v < 0) return false;
        cl_lengths[static_cast<size_t>(order[i])] = static_cast<int>(v);
    }
    HuffTree cl_tree;
    cl_tree.build(cl_lengths);

    std::vector<int> lengths;
    lengths.reserve(static_cast<size_t>(hlit + hdist));
    while (static_cast<long>(lengths.size()) < hlit + hdist) {
        const int sym = cl_tree.decode(br);
        if (sym < 0) return false;
        if (sym < 16) {
            lengths.push_back(sym);
        } else if (sym == 16) {
            if (lengths.empty()) return false;
            const long rep = br.get_bits(2);
            if (rep < 0) return false;
            const int prev = lengths.back();
            for (long i = 0; i < rep + 3; ++i) lengths.push_back(prev);
        } else if (sym == 17) {
            const long rep = br.get_bits(3);
            if (rep < 0) return false;
            for (long i = 0; i < rep + 3; ++i) lengths.push_back(0);
        } else {
            const long rep = br.get_bits(7);
            if (rep < 0) return false;
            for (long i = 0; i < rep + 11; ++i) lengths.push_back(0);
        }
    }

    const std::vector<int> lit_lengths(lengths.begin(), lengths.begin() + hlit);
    const std::vector<int> dist_lengths(lengths.begin() + hlit, lengths.end());

    HuffTree lit_tree;
    HuffTree dist_tree;
    lit_tree.build(lit_lengths);
    dist_tree.build(dist_lengths);

    return inflate_block_huffman(br, lit_tree, dist_tree, out);
}

} // namespace

bool inflate_raw(const std::vector<unsigned char>& compressed, std::string& out) {
    BitReader br(compressed.data(), compressed.size());
    bool final_block = false;
    while (!final_block) {
        const int bfinal = br.get_bit();
        if (bfinal < 0) return false;
        final_block = (bfinal == 1);

        const long btype = br.get_bits(2);
        if (btype < 0) return false;

        if (btype == 0) {
            if (!inflate_block_stored(br, out)) return false;
        } else if (btype == 1) {
            HuffTree lit_tree;
            HuffTree dist_tree;
            build_fixed_trees(lit_tree, dist_tree);
            if (!inflate_block_huffman(br, lit_tree, dist_tree, out)) return false;
        } else if (btype == 2) {
            if (!inflate_block_dynamic(br, out)) return false;
        } else {
            return false;
        }
    }
    return true;
}

bool zlib_inflate(const std::vector<unsigned char>& zlib_data, std::string& out) {
    if (zlib_data.size() < 2) return false;
    // Skip the 2-byte zlib header (CMF/FLG); the trailing 4-byte Adler32
    // checksum after the deflate stream is simply left unread.
    const std::vector<unsigned char> body(zlib_data.begin() + 2, zlib_data.end());
    return inflate_raw(body, out);
}

} // namespace docs
