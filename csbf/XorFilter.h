#include "BaseBF.h"
#include "./CountingBF.h"

namespace BloomFilterModels {
    class XorFilter : public StaticFilter {
public:
    vector<int> h; //* Hash seeds
    int b; //* Number of bits per element

    void Init(vector<vector<uint8_t>> data, uint32_t countExist = 0) {
        m = data.size() * 1.23 + 32; //* Filter size (as c in the paper)
        k = 3; //* Number of hash functions (as h1,h2,h3 in the paper)
        b = sizeof(uint8_t) * 8; //* Number of bits per element
        count = 0; // Number of items added
        maxCapacity = data.size(); // Maximum capacity of the filter
        fpRate = Defaults::FALSE_POSITIVE_RATE; // Target false-positive rate
        construction(data);
    }
/*
    * Original approach: shift bits right 32 bit and XOR with current value
    * Using Bitwise XOR: mixing the bits across all characters in the input vector -> prevent the fingerprint from relying too heavily on the position of characters
*/
    std::uint8_t fingerprint(const std::vector<uint8_t>& data) const {
        std::uint8_t result = 0;
        for (auto byte : data) {
            result ^= byte;
        }
        return result;
    }
    
    //* Value of h1,h2,h3(key)
    std::uint32_t* _hash(const std::vector<uint8_t>& key) const {
        std::uint32_t* hv = new std::uint32_t[3];
        auto hashKernel = BloomFilterApp::Utils::HashKernel(key, "murmur"); // Generate hash kernels
        uint32_t lower = hashKernel.LowerBaseHash;
        uint32_t upper = hashKernel.UpperBaseHash;
        for (int i = 0; i < 3; i++) {
            hv[i] = lower + upper * h[i] % m;
        }
        return hv;
    };

    XorFilter& construction(const std::vector<vector<uint8_t>>& data) {
        struct Pair {
            vector<uint8_t> key;
            int index;
        };
        vector<Pair> pairs; //* aka sigma as in paper
        h = {0, 1, 2};

        
        auto _map = [&](const std::vector<vector<uint8_t>> data) -> bool {
            vector<vector<vector<uint8_t>>> H; //* array of keys
            pairs.clear();
            for (auto key : data)
            {
                uint32_t* h_ = _hash(key);
                H[h_[0]].push_back(key);
                H[h_[1]].push_back(key);
                H[h_[2]].push_back(key);
            }
            vector<int> Q;
            for (auto keys : H)
            {
                if (keys.size() == 1)
                {
                    Q.push_back(fingerprint(keys[0]));
                }
            }
            while (!Q.empty()) {
                int x = Q.back(); Q.pop_back();
                if (H[x].size() == 1) {
                    pairs.push_back({H[x][0], x});
                    uint32_t* h_ = _hash(H[x][0]);
                    for (int j=0; j<k; j++) {
                        H[h_[j]].erase(std::remove(H[h_[j]].begin(), H[h_[j]].end(), H[x][0]), H[h_[j]].end());
                        if (H[h_[j]].size() == 1) {
                            Q.push_back(h_[j]);
                        }
                    }
                }
            }
            return pairs.size() == data.size();
        };

        auto _assign = [&](vector<vector<uint8_t>> data) -> void {
            for (auto p : pairs) {
                uint32_t* h_ = _hash(p.key);
                uint8_t rvalue = fingerprint(p.key);
                buckets->Set(p.index, 0);
                for (int i=0; i<k; i++) rvalue ^= buckets->Get(h_[i]);
                buckets->Set(p.index, rvalue);
            }
        };
        
        while (_map(data)) {
            h[0] += 1; h[1] += 1; h[2] += 1;
        }
        this->buckets = make_unique<Buckets>(m, sizeof(std::uint8_t));
        _assign(data);
        return *this;
    }

    XorFilter() {}
    
    string getFilterName() const {
        return "XorFilter";
    }

    // Returns the maximum capacity of the filter
    uint32_t Capacity() const {
        return maxCapacity;
    }

    // Returns the filter capacity
    uint32_t Size() const {
        return m;
    }

    // Returns the number of hash functions
    uint32_t K() const {
        return k;
    }

    // Returns the number of items in the filter
    uint32_t Count() const {
        return count;
    }

    // Returns the target false-positive rate
    double FPrate() const {
        return fpRate;
    }

    // Tests for membership of the data.
    // Returns true if the data is probably a member, false otherwise.
    bool Test(const std::vector<uint8_t>& data) const {
        uint32_t* h_ = _hash(data);
        return fingerprint(data) == buckets->Get(h_[0]) ^ buckets->Get(h_[1]) ^ buckets->Get(h_[2]);
    }

    ~XorFilter() {}

    
    };
};