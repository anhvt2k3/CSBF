#include <vector>
#include <list>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <string>
#include <memory>

#include "Buckets.h"
#include "../utils/Defaults.h"
#include "../utils/Utils.h"

using namespace std;
using namespace BloomFilterApp;
namespace BloomFilterModels {
// ! Filter base class
    class AbstractFilter {
    public:
    // # Compulsory methods
        virtual string getFilterName() const = 0;
        virtual uint32_t Size() const = 0;
        virtual uint32_t Capacity() const = 0;
        virtual double FPrate() const = 0;
        virtual uint32_t K() const = 0;
        virtual uint32_t Count() const = 0;
        virtual bool Test(const std::vector<uint8_t>& data) const = 0;
        virtual AbstractFilter& Add(const std::vector<uint8_t>& data) = 0;
        // virtual AbstractFilter& Reset() = 0;
    // # Specialized methods
        virtual void Init(uint32_t n, uint8_t b, double fpRate, uint32_t countExist = 0) {
            cout << "Unsupported method: Init" << endl;
        };
        virtual string getConfigure() {
            return "Unsupported method: getConfigure";
        };
        virtual bool TestAndRemove(const std::vector<uint8_t>& data){
            cout << "Unsupported method: TestAndRemove" << endl;
            return false;
        };
    };
// ! Filter parent class
    class StaticFilter : public AbstractFilter {
protected:
        unique_ptr<Buckets> buckets; // Bucket array
        uint32_t m; // Filter size (number of buckets)
        uint32_t k; // Number of hash functions
        uint32_t count; // Number of items added
        uint32_t maxCapacity; // Maximum capacity of the filter
        double fpRate; // Target false-positive rate
        
        void Init(uint32_t n, uint8_t b, double fpRate, uint32_t countExist = 0) {
            this->buckets = make_unique<Buckets>(Utils::OptimalMCounting(n, fpRate), b);
            this->m = Utils::OptimalMCounting(n, fpRate);
            this->k = Utils::OptimalKCounting(fpRate);
            this->count = countExist;
            this->maxCapacity = n;
            this->fpRate = fpRate;
        }

public:
        string getConfigure() {
            string res = "_ _ _ Filter Configuration _ _ _\n";
            res += "Filter Size: " + to_string(Size()) + "\n";
            res += "Capacity: " + to_string(Capacity()) + "\n";
            res += "Number of Hash Functions: " + to_string(K()) + "\n";
            res += "False Positive Rate: " + to_string(FPrate()) + "\n";
            res += "Number of Items Added: " + to_string(Count()) + "\n";
            return res;
        }
        StaticFilter() {}
        StaticFilter(uint32_t n, uint8_t b, double fpRate, uint32_t countExist = 0) {
            this->Init(n, b, fpRate, countExist);
        }
    };

    class DynamicFilter : public AbstractFilter {};
// ! Filter classes
    class StandardBloomFilter : public StaticFilter {
public:
        StandardBloomFilter() {}
        StandardBloomFilter(uint32_t n, uint8_t b, double fpRate, uint32_t countExist = 0) 
            : StaticFilter(n, b, fpRate, countExist)  // Call the base class constructor directly
        {}

        string getFilterName() const {
            return "StandardBloomFilter";
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
            auto hashKernel = Utils::HashKernel(data, "murmur"); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;

            // Check if all hash function indices are set in the bucket array
            for (uint32_t i = 0; i < k; ++i) {
                if (buckets->Get(uint32_t((lower + upper * i) % m)) == 0)
                {
                    return false;
                }
            }

            return true;
        }

        // Adds the data to the filter->
        // Returns a reference to the filter for chaining.
        StandardBloomFilter& Add(const std::vector<uint8_t>& data) {
            auto hashKernel = Utils::HashKernel(data, "murmur"); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;

            // Set the K bits in the bucket array
            for (uint32_t i = 0; i < k; ++i) {
                // cout << "cbf-Adding: " << uint32_t((lower + upper * i) % m) << endl;
                buckets->Set(uint32_t((lower + upper * i) % m), 1);
            }

            this->count++;
            return *this;
        }

        // Tests for membership of the data and adds it to the filter if it doesn't exist.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndAdd(const std::vector<uint8_t>& data) {
            auto hashKernel = Utils::HashKernel(data, "murmur"); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;
            bool member = true;

            // Check if all hash function indices are set in the bucket array and set them if not
            for (uint32_t i = 0; i < k; ++i) {
                uint32_t index = uint32_t((lower + upper * i) % m);
                if (buckets->Get(index) == 0) {
                    member = false;
                }
                buckets->Set(index, 1);
            }

            count++;
            return member;
        }

        ~StandardBloomFilter() {}
    };
    class StandardCountingBloomFilter : public StaticFilter {
public:
        StandardCountingBloomFilter() {}
        StandardCountingBloomFilter(uint32_t n, uint8_t b, double fpRate, uint32_t countExist = 0) 
            : StaticFilter(n, b, fpRate, countExist)  // Call the base class constructor directly
        {}

        string getFilterName() const {
            return "StandardCountingBloomFilter";
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
            auto hashKernel = Utils::HashKernel(data, "murmur"); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;

            // Check if all hash function indices are set in the bucket array
            for (uint32_t i = 0; i < k; ++i) {
                if (buckets->Get(uint32_t((lower + upper * i) % m)) == 0)
                {
                    return false;
                }
            }

            return true;
        }

        // Adds the data to the filter->
        // Returns a reference to the filter for chaining.
        StandardCountingBloomFilter& Add(const std::vector<uint8_t>& data) {
            auto hashKernel = Utils::HashKernel(data, "murmur"); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;

            // Set the K bits in the bucket array
            for (uint32_t i = 0; i < k; ++i) {
                // cout << "cbf-Adding: " << uint32_t((lower + upper * i) % m) << endl;
                buckets->Increment(uint32_t((lower + upper * i) % m), 1);
            }

            this->count++;
            return *this;
        }

        // Tests for membership of the data and adds it to the filter if it doesn't exist.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndAdd(const std::vector<uint8_t>& data) {
            auto hashKernel = Utils::HashKernel(data, "murmur"); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;
            bool member = true;

            // Check if all hash function indices are set in the bucket array and set them if not
            for (uint32_t i = 0; i < k; ++i) {
                uint32_t index = uint32_t((lower + upper * i) % m);
                if (buckets->Get(index) == 0) {
                    member = false;
                }
                buckets->Increment(index, 1);
            }

            count++;
            return member;
        }

        // Tests for membership of the data and removes it from the filter if it exists.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndRemove(const std::vector<uint8_t>& data) {
            auto hashKernel = Utils::HashKernel(data, "murmur"); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;
            bool member = true;
            std::vector<uint32_t> indices(k); // Store hash function indices

            // Calculate hash function indices and check if all are set in the bucket array
            for (uint32_t i = 0; i < k; ++i) {
                indices[i] = uint32_t((lower + upper * i) % m);
                if (buckets->Get(indices[i]) == 0) {
                    member = false;
                }
            }

            // If the data is probably in the filter, decrement the bucket values at the calculated indices
            if (member) {
                for (auto index : indices) {
                    buckets->Increment(index, -1);
                }
                count--;
            }

            return member;
        }

        // Tests for membership of the data and removes it from the filter if it exists.
        // Returns true if the data was probably in the filter, false otherwise.
        bool Remove(const std::vector<uint8_t>& data) {
            auto hashKernel = Utils::HashKernel(data, "murmur"); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;
            uint32_t index; // Store hash function indices

            // Calculate hash function indices and check if all are set in the bucket array
            for (uint32_t i = 0; i < k; ++i) {
                index = uint32_t((lower + upper * i) % m);
                buckets->Increment(index, -1);
            }
            count--;
            return true;
        }

        ~StandardCountingBloomFilter() {}
    };
    class CountingBloomFilter : public StaticFilter {
        // std::unique_ptr<HashAlgorithm> hash; // Hash algorithm object
public:
        CountingBloomFilter() {}
        CountingBloomFilter(uint32_t n, uint8_t b, double fpRate, uint32_t countExist = 0) 
            : StaticFilter(n, b, fpRate, countExist)  // Call the base class constructor directly
        {}

        string getFilterName() const {
            return "CountingBloomFilter";
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
            auto hashKernel = Utils::HashKernel(data); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;

            // Check if all hash function indices are set in the bucket array
            for (uint32_t i = 0; i < k; ++i) {
                if (buckets->Get(uint32_t((lower + upper * i) % m)) == 0)
                {
                    return false;
                }
            }

            return true;
        }

        // Adds the data to the filter->
        // Returns a reference to the filter for chaining.
        CountingBloomFilter& Add(const std::vector<uint8_t>& data) {
            auto hashKernel = Utils::HashKernel(data); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;

            // Set the K bits in the bucket array
            for (uint32_t i = 0; i < k; ++i) {
                // cout << "cbf-Adding: " << uint32_t((lower + upper * i) % m) << endl;
                buckets->Increment(uint32_t((lower + upper * i) % m), 1);
            }

            this->count++;
            return *this;
        }

        // Resets the filter to its original state.
        // Returns a reference to the filter for chaining.
        CountingBloomFilter& Reset() {
            buckets->Reset(); // Clear bucket array
            count = 0; // Reset count
            return *this;
        }

        // Tests for membership of the data and adds it to the filter if it doesn't exist.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndAdd(const std::vector<uint8_t>& data) {
            auto hashKernel = Utils::HashKernel(data); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;
            bool member = true;

            // Check if all hash function indices are set in the bucket array and set them if not
            for (uint32_t i = 0; i < k; ++i) {
                uint32_t index = uint32_t((lower + upper * i) % m);
                if (buckets->Get(index) == 0) {
                    member = false;
                }
                buckets->Increment(index, 1);
            }

            count++;
            return member;
        }

        // Tests for membership of the data and removes it from the filter if it exists.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndRemove(const std::vector<uint8_t>& data) {
            auto hashKernel = Utils::HashKernel(data); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;
            bool member = true;
            std::vector<uint32_t> indices(k); // Store hash function indices

            // Calculate hash function indices and check if all are set in the bucket array
            for (uint32_t i = 0; i < k; ++i) {
                indices[i] = uint32_t((lower + upper * i) % m);
                if (buckets->Get(indices[i]) == 0) {
                    member = false;
                }
            }

            // If the data is probably in the filter, decrement the bucket values at the calculated indices
            if (member) {
                for (auto index : indices) {
                    buckets->Increment(index, -1);
                }
                count--;
            }

            return member;
        }

        // Returns true if the data was probably in the filter, false otherwise.
        bool Remove(const std::vector<uint8_t>& data) {
            auto hashKernel = Utils::HashKernel(data, "murmur"); // Generate hash kernels
            uint32_t lower = hashKernel.LowerBaseHash;
            uint32_t upper = hashKernel.UpperBaseHash;
            uint32_t index; // Store hash function indices

            // Calculate hash function indices and check if all are set in the bucket array
            for (uint32_t i = 0; i < k; ++i) {
                index = uint32_t((lower + upper * i) % m);
                buckets->Increment(index, -1);
            }
            count--;
            return true;
        }

        ~CountingBloomFilter() {
        }
    }; // end of CountingBloomFilter
    class CountingScalableBloomFilter : public DynamicFilter {
        vector<shared_ptr<CountingBloomFilter>> filters;
        double r; // Tightening ratio
        double fp; // Target false-positive rate
        uint32_t p; // Maximum item count for each CountingBloomFilter
        uint32_t s; // Scalable growth factor
        // time_t syncDate; // Synchronization date

        // Adds a new filter to the list with restricted false-positive rate.
        int AddFilter(const vector<vector<uint8_t>>& data = {}) {
            // Calculate false-positive rate and capacity for the new filter
            double fpRate = fp * pow(r, filters.size());
            uint32_t capacity = p * pow(s, filters.size());
            
            filters.push_back(make_shared<CountingBloomFilter>(capacity, 4, fpRate));
            // cout << "Filter added " << filters.size()  << " "<< newFilter->Size() << endl;
            return 0;
        }
    public:
        CountingScalableBloomFilter() : r   (Defaults::FILL_RATIO),
                                        fp  (Defaults::FALSE_POSITIVE_RATE),
                                        p   (Defaults::MAX_COUNT_NUMBER),
                                        s   (Defaults::SCALABLE_GROWTH)
        {
            AddFilter(); // Add the first filter
        }
        
        string getFilterName() const {
            return "CountingScalableBloomFilter";
        }

        std::string getConfigure() {
            std::string res = "_ _ _ CSBF Scope _ _ _ \n";
            res += "Tightening-ratio: " + std::__cxx11::to_string(r) + "\n";
            res += "False positive rate: " + std::__cxx11::to_string(fp) + "\n";
            res += "Current max capacity: " + std::__cxx11::to_string(p) + "\n";
            res += "Current filter capacity: " + std::__cxx11::to_string(Size()) + "\n";
            res += "Scale growth: " + std::__cxx11::to_string(s) + "\n";
            res += "CBF Scope \n";
            res += "Number of filters: " + std::__cxx11::to_string(filters.size()) + "\n";
            for (int i=0; i<filters.size(); i++) {
                auto filter = filters[i];
                res += "_ _ _ Filter " + std::__cxx11::to_string(i) + " _ _ _\n";
                res += "Filter max capacity: " + std::__cxx11::to_string(filter->Capacity()) + "\n";
                res += "Filter size: " + std::__cxx11::to_string(filter->Size()) + "\n";
                res += "Filter count: " + std::__cxx11::to_string(filter->Count()) + "\n";
                res += "Filter K: " + std::__cxx11::to_string(filter->K()) + "\n";
            }
            return res;
        }

        // Returns the maximum capacity of the filter->
        uint32_t Capacity() const {
            uint32_t c = 0;
            for (const auto& filter : filters) {
                if (filter->Count() < filter->Capacity()) {
                    c += filter->Capacity();
                }
            }
            return c;
        }

        // Return False Positive Rate
        double FPrate() const {
            return fp;
        }

        // Returns the current filter size.
        uint32_t Size() const {
            uint32_t size = 0;
            for (const auto& filter : filters) {
                size += filter->Size();
            }
            return size;
        }

        // Returns the number of hash functions used in each filter->
        uint32_t K() const {
            return filters.empty() ? 0 : filters.front()->K(); // Return the K of the first filter
        }

        uint32_t Count() const {
            uint32_t count = 0;
            for (const auto& filter : filters) {
                count += filter->Count();
            }
            return count;
        }

        // Tests for membership of the data.
        // Returns true if the data is probably a member, false otherwise.
        bool Test(const std::vector<uint8_t>& data) const {
            // Check for membership in each filter
            for (const auto& filter : filters) {
                if (filter->Test(data)) {
                    return true;
                }
            }
            return false;
        }

        // Adds the data to the filter->
        // Returns a reference to the filter for chaining.
        CountingScalableBloomFilter& Add(const std::vector<uint8_t>& data) {
            if (std::all_of(filters.begin(), filters.end(), [](const auto& filter) { return filter->Count() == filter->Capacity(); })) {
                AddFilter(); // Add a new filter if all filters are full
            }
            // cout << "csbf-Adding: " << data.data() << endl;
            // cout << filters.back().Size() << endl;
            // cout << filters.back().Capacity() << endl;

            filters.back()->Add(data); // Add data to the last filter
            // cout << "csbf-Added: " << data.data() << endl;
            return *this;
        }

        // Tests for membership of the data and adds it to the filter if it doesn't exist.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndAdd(const std::vector<uint8_t>& data) {
            bool member = Test(data);
            if (!member) {
                Add(data);
            }
            return !member;
        }

        // Tests for membership of the data and removes it from the filter if it exists.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndRemove(const std::vector<uint8_t>& data) {
            for (auto& filter : filters) { //@ deleting the key from every filter -> possible False Negative
                if (filter->Test(data)) {
                    filter->TestAndRemove(data);
                    return true;
                }
            }
            return false;
        }

        // Resets the filter to its original state.
        // Returns a reference to the filter for chaining.
        CountingScalableBloomFilter& Reset() {
            filters.clear(); // Clear the filter list
            AddFilter(); // Add a new filter
            return *this;
        }

        ~CountingScalableBloomFilter() {
        }

    };
    class StandardCountingScalableBloomFilter : public DynamicFilter {
        vector<shared_ptr<StandardCountingBloomFilter>> filters;
        double r; // Tightening ratio
        double fp; // Target false-positive rate
        uint32_t p; // Maximum item count for each StandardCountingBloomFilter
        uint32_t s; // Scalable growth factor
        // time_t syncDate; // Synchronization date

        // Adds a new filter to the list with restricted false-positive rate.
        int AddFilter(const vector<vector<uint8_t>>& data = {}) {
            // Calculate false-positive rate and capacity for the new filter
            double fpRate = fp * pow(r, filters.size());
            uint32_t capacity = p * pow(s, filters.size());
            
            filters.push_back(make_shared<StandardCountingBloomFilter>(capacity, 4, fpRate));
            // cout << "Filter added " << filters.size()  << " "<< newFilter->Size() << endl;
            return 0;
        }
    public:
        StandardCountingScalableBloomFilter() : r   (Defaults::FILL_RATIO),
                                        fp  (Defaults::FALSE_POSITIVE_RATE),
                                        p   (Defaults::MAX_COUNT_NUMBER),
                                        s   (Defaults::SCALABLE_GROWTH)
        {
            AddFilter(); // Add the first filter
        }
        
        string getFilterName() const {
            return "StandardCountingScalableBloomFilter";
        }

        std::string getConfigure() {
            std::string res = "_ _ _ CSBF Scope _ _ _ \n";
            res += "Tightening-ratio: " + std::__cxx11::to_string(r) + "\n";
            res += "False positive rate: " + std::__cxx11::to_string(FPrate()) + "\n";
            res += "Current capacity: " + std::__cxx11::to_string(Capacity()) + "\n";
            res += "Current filter size: " + std::__cxx11::to_string(Size()) + "\n";
            res += "Scale growth: " + std::__cxx11::to_string(s) + "\n";
            res += "CBF Scope \n";
            res += "Number of filters: " + std::__cxx11::to_string(filters.size()) + "\n";
            for (int i=0; i<filters.size(); i++) {
                auto filter = filters[i];
                res += "_ _ _ Filter " + std::__cxx11::to_string(i) + " _ _ _\n";
                res += "Filter capacity: " + std::__cxx11::to_string(filter->Capacity()) + "\n";
                res += "Filter size: " + std::__cxx11::to_string(filter->Size()) + "\n";
                res += "Filter count: " + std::__cxx11::to_string(filter->Count()) + "\n";
                res += "Filter K: " + std::__cxx11::to_string(filter->K()) + "\n";
            }
            return res;
        }

        // Returns the maximum capacity of the filter->
        uint32_t Capacity() const {
            uint32_t c = 0;
            for (const auto& filter : filters) {
                if (filter->Count() < filter->Capacity()) {
                    c += filter->Capacity();
                }
            }
            return c;
        }

        // Return False Positive Rate
        double FPrate() const {
            return fp;
        }

        // Returns the current filter size.
        uint32_t Size() const {
            uint32_t size = 0;
            for (const auto& filter : filters) {
                size += filter->Size();
            }
            return size;
        }

        // Returns the number of hash functions used in each filter->
        uint32_t K() const {
            return filters.empty() ? 0 : filters.front()->K(); // Return the K of the first filter
        }

        uint32_t Count() const {
            uint32_t count = 0;
            for (const auto& filter : filters) {
                count += filter->Count();
            }
            return count;
        }

        // Tests for membership of the data.
        // Returns true if the data is probably a member, false otherwise.
        bool Test(const std::vector<uint8_t>& data) const {
            // Check for membership in each filter
            for (const auto& filter : filters) {
                if (filter->Test(data)) {
                    return true;
                }
            }
            return false;
        }

        // Adds the data to the filter->
        // Returns a reference to the filter for chaining.
        StandardCountingScalableBloomFilter& Add(const std::vector<uint8_t>& data) {
            if (std::all_of(filters.begin(), filters.end(), [](const auto& filter) { return filter->Count() == filter->Capacity(); })) {
                AddFilter(); // Add a new filter if all filters are full
            }
            // cout << "csbf-Adding: " << data.data() << endl;
            // cout << filters.back().Size() << endl;
            // cout << filters.back().Capacity() << endl;

            filters.back()->Add(data); // Add data to the last filter
            // cout << "csbf-Added: " << data.data() << endl;
            return *this;
        }

        // Tests for membership of the data and adds it to the filter if it doesn't exist.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndAdd(const std::vector<uint8_t>& data) {
            bool member = Test(data);
            if (!member) {
                Add(data);
            }
            return !member;
        }

        // Tests for membership of the data and removes it from the filter if it exists.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndRemove(const std::vector<uint8_t>& data) {
            for (auto& filter : filters) { //@ False Negative introduced
                if (filter->Test(data)) {
                    filter->TestAndRemove(data);
                    return true;
                }
            }
            return false;
        }

        // Resets the filter to its original state.
        // Returns a reference to the filter for chaining.
        StandardCountingScalableBloomFilter& Reset() {
            filters.clear(); // Clear the filter list
            AddFilter(); // Add a new filter
            return *this;
        }

        ~StandardCountingScalableBloomFilter() {
        }

    };
    class DynamicBloomFilter : public DynamicFilter {
        vector<shared_ptr<CountingBloomFilter>> filters;
        double fp; // Target false-positive rate
        uint32_t c; // Maximum item count for each CountingBloomFilter
        // time_t syncDate; // Synchronization date

        // Adds a new filter to the list with restricted false-positive rate.
        int AddFilter(const vector<vector<uint8_t>>& data = {}) {
            // Calculate false-positive rate and capacity for the new filter
            filters.push_back(make_shared<CountingBloomFilter>(c, 4, fp));
            return 0;
        }
public:
        DynamicBloomFilter(uint32_t c = Defaults::MAX_COUNT_NUMBER, double fp = Defaults::FALSE_POSITIVE_RATE) : c(c), fp(fp) {
            AddFilter(); // Add the first filter
        }

        string getFilterName() const {
            return "DynamicBloomFilter";
        }

        std::string getConfigure() {
            std::string res = "_ _ _ CSBF Scope _ _ _ \n";
            res += "False positive rate: " + std::__cxx11::to_string(FPrate()) + "\n";
            res += "Current capacity: " + std::__cxx11::to_string(Capacity()) + "\n";
            res += "Current filter size: " + std::__cxx11::to_string(Size()) + "\n";
            res += "CBF Scope \n";
            res += "Number of filters: " + std::__cxx11::to_string(filters.size()) + "\n";
            for (int i=0; i<filters.size(); i++) {
                auto filter = filters[i];
                res += "_ _ _ Filter " + std::__cxx11::to_string(i) + " _ _ _\n";
                res += "Filter capacity: " + std::__cxx11::to_string(filter->Capacity()) + "\n";
                res += "Filter size: " + std::__cxx11::to_string(filter->Size()) + "\n";
                res += "Filter count: " + std::__cxx11::to_string(filter->Count()) + "\n";
                res += "Filter K: " + std::__cxx11::to_string(filter->K()) + "\n";
            }
            return res;
        }

        // Returns the maximum capacity of the filter->
        uint32_t Capacity() const {
            uint32_t c = 0;
            for (const auto& filter : filters) {
                c += filter->Capacity();
            }
            return c;
        }

        // Return False Positive Rate
        double FPrate() const {
            return fp;
        }

        // Returns the current filter size.
        uint32_t Size() const {
            uint32_t size = 0;
            for (const auto& filter : filters) {
                size += filter->Size();
            }
            return size;
        }

        // Returns the number of hash functions used in each filter->
        uint32_t K() const {
            return filters.empty() ? 0 : filters.front()->K(); // Return the K of the first filter
        }

        uint32_t Count() const {
            uint32_t count = 0;
            for (const auto& filter : filters) {
                count += filter->Count();
            }
            return count;
        }

        // Tests for membership of the data.
        // Returns true if the data is probably a member, false otherwise.
        bool Test(const std::vector<uint8_t>& data) const {
            // Check for membership in each filter
            for (const auto& filter : filters) {
                if (filter->Test(data)) {
                    return true;
                }
            }
            return false;
        }

        // Adds the data to the filter->
        // Returns a reference to the filter for chaining.
        DynamicBloomFilter& Add(const std::vector<uint8_t>& data) {
            if (std::all_of(filters.begin(), filters.end(), [](const auto& filter) { return filter->Count() == filter->Capacity(); })) {
                AddFilter(); // Add a new filter if all filters are full
            }

            filters.back()->Add(data); // Add data to the last filter
            return *this;
        }

        // Tests for membership of the data and adds it to the filter if it doesn't exist.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndAdd(const std::vector<uint8_t>& data) {
            bool member = Test(data);
            if (!member) {
                Add(data);
            }
            return !member;
        }

        // Tests for membership of the data and removes it from the filter if it exists.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndRemove(const std::vector<uint8_t>& data) {
            int isNotUnique = -1; //# -1: value not found, 0: value found once, >0: value found multiple times
            int i = 0;
            int filter_pos = 0;
            for (auto& filter : filters) {
                if (filter->Test(data)) {
                    isNotUnique++;
                    filter_pos = i;
                }
                i++;
            }
            if (isNotUnique == 0) {
                filters[filter_pos]->TestAndRemove(data);
                return true;
            }
            return false;
        }

    };
    class DynamicStdCountingBloomFilter : public DynamicFilter {
        vector<shared_ptr<StandardCountingBloomFilter>> filters;
        double fp; // Target false-positive rate
        uint32_t c; // Maximum item count for each StandardCountingBloomFilter
        // time_t syncDate; // Synchronization date

        // Adds a new filter to the list with restricted false-positive rate.
        int AddFilter(const vector<vector<uint8_t>>& data = {}) {
            // Calculate false-positive rate and capacity for the new filter
            filters.push_back(make_shared<StandardCountingBloomFilter>(c, 4, fp));
            return 0;
        }
public:
        DynamicStdCountingBloomFilter(uint32_t c = Defaults::MAX_COUNT_NUMBER, double fp = Defaults::FALSE_POSITIVE_RATE) : c(c), fp(fp) {
            AddFilter(); // Add the first filter
        }

        string getFilterName() const {
            return "DynamicStdCountingBloomFilter";
        }

        std::string getConfigure() {
            std::string res = "_ _ _ CSBF Scope _ _ _ \n";
            res += "False positive rate: " + std::__cxx11::to_string(FPrate()) + "\n";
            res += "Current capacity: " + std::__cxx11::to_string(Capacity()) + "\n";
            res += "Current filter size: " + std::__cxx11::to_string(Size()) + "\n";
            res += "CBF Scope \n";
            res += "Number of filters: " + std::__cxx11::to_string(filters.size()) + "\n";
            for (int i=0; i<filters.size(); i++) {
                auto filter = filters[i];
                res += "_ _ _ Filter " + std::__cxx11::to_string(i) + " _ _ _\n";
                res += "Filter capacity: " + std::__cxx11::to_string(filter->Capacity()) + "\n";
                res += "Filter size: " + std::__cxx11::to_string(filter->Size()) + "\n";
                res += "Filter count: " + std::__cxx11::to_string(filter->Count()) + "\n";
                res += "Filter K: " + std::__cxx11::to_string(filter->K()) + "\n";
            }
            return res;
        }

        // Returns the maximum capacity of the filter->
        uint32_t Capacity() const {
            uint32_t c = 0;
            for (const auto& filter : filters) {
                c += filter->Capacity();
            }
            return c;
        }

        // Return False Positive Rate
        double FPrate() const {
            return fp;
        }

        // Returns the current filter size.
        uint32_t Size() const {
            uint32_t size = 0;
            for (const auto& filter : filters) {
                size += filter->Size();
            }
            return size;
        }

        // Returns the number of hash functions used in each filter->
        uint32_t K() const {
            return filters.empty() ? 0 : filters.front()->K(); // Return the K of the first filter
        }

        uint32_t Count() const {
            uint32_t count = 0;
            for (const auto& filter : filters) {
                count += filter->Count();
            }
            return count;
        }

        // Tests for membership of the data.
        // Returns true if the data is probably a member, false otherwise.
        bool Test(const std::vector<uint8_t>& data) const {
            // Check for membership in each filter
            for (const auto& filter : filters) {
                if (filter->Test(data)) {
                    return true;
                }
            }
            return false;
        }

        // Adds the data to the filter->
        // Returns a reference to the filter for chaining.
        DynamicStdCountingBloomFilter& Add(const std::vector<uint8_t>& data) {
            if (std::all_of(filters.begin(), filters.end(), [](const auto& filter) { return filter->Count() == filter->Capacity(); })) {
                AddFilter(); // Add a new filter if all filters are full
            }

            filters.back()->Add(data); // Add data to the last filter
            return *this;
        }

        // Tests for membership of the data and adds it to the filter if it doesn't exist.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndAdd(const std::vector<uint8_t>& data) {
            bool member = Test(data);
            if (!member) {
                Add(data);
            }
            return !member;
        }

        // Tests for membership of the data and removes it from the filter if it exists.
        // Returns true if the data was probably in the filter, false otherwise.
        bool TestAndRemove(const std::vector<uint8_t>& data) {
            int isNotUnique = -1; //# -1: value not found, 0: value found once, >0: value found multiple times
            int i = 0;
            int filter_pos = 0;
            for (auto& filter : filters) {
                if (filter->Test(data)) {
                    isNotUnique++;
                    filter_pos = i;
                }
                i++;
            }
            if (isNotUnique == 0) {
                filters[filter_pos]->TestAndRemove(data);
                return true;
            }
            return false;
        }

    };

} // namespace BloomFilterModels