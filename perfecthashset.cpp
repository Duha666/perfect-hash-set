#include <vector>
#include <cstdlib>
#include <random>
#include <exception>

typedef unsigned int ui32;

const unsigned long long modulo = 4294967311LLU;

std::default_random_engine generator;
std::uniform_int_distribution<long long> all_ui(0, modulo - 1);
std::uniform_int_distribution<long long> all_ui_without_0(1, modulo - 1);

class PerfectHashSet {
private:
    struct UniversalHash {
        ui32 a, b, p;

        ui32 calculate(ui32 value) const {
            return (((unsigned long long)a * value + b) % modulo) % p;
        }
    };
    UniversalHash makeRandomHash(ui32 real_modulo) const {
        UniversalHash random_hash;
        random_hash.a = all_ui_without_0(generator);
        random_hash.b = all_ui(generator);
        random_hash.p = real_modulo;
        return random_hash;
    }

    UniversalHash first_universal_hash;
    std::vector<UniversalHash> second_universal_hashes;
    ui32 keys_count;
    ui32 inserted_count;
    std::vector< std::vector<ui32> > hashes;
    std::vector< std::vector<bool> > inserted;

    bool isGoodFirstHash() const {
        ui32 summary = 0;
        for (int i = 0; i < keys_count; i++)
            summary += hashes[i].size() * hashes[i].size();
        return summary <= 3 * keys_count;
    }

    bool hasNaiveCollisions() const {
        for (int i = 0; i < keys_count; i++) {
            if (hashes[i].size() == 3) {
                if (hashes[i][0] == hashes[i][1] || hashes[i][0] == hashes[i][2] || hashes[i][1] == hashes[i][2])
                    return true;
            }
            else
                for (int j = 1; j < hashes[i].size(); j++)
                    if (hashes[i][j - 1] == hashes[i][j])
                        return true;
        }
        return false;
    }

    void initFirstTable(const std::vector<ui32> & keys) {
        inserted.assign(keys_count, std::vector<bool>());
        while (true) {
            for (int i = 0; i < keys_count; i++)
                hashes[i].clear();
            first_universal_hash = makeRandomHash(keys_count);
            for (ui32 i = 0; i < keys.size(); i++)
                hashes[first_universal_hash.calculate(keys[i])].push_back(keys[i]);
            if (hasNaiveCollisions())
                throw MultipleKeyException();
            if (isGoodFirstHash())
                break;
        }
    }

    void initTable(ui32 index) {
        std::vector<ui32> elements = hashes[index];
        inserted[index].assign(elements.size() * elements.size(), false);
        hashes[index].assign(elements.size() * elements.size(), 0);
        std::vector<bool> used;
        for (ui32 i = 0; i < elements.size(); i++)
            for (ui32 j = i + 1; j < elements.size(); j++)
                if (elements[i] == elements[j])
                    throw MultipleKeyException();
        while(true) {
            bool good_hash = true;
            second_universal_hashes[index] = makeRandomHash(elements.size() * elements.size());
            used.assign(elements.size() * elements.size(), false);
            for (ui32 i = 0; i < elements.size(); i++) {
                ui32 current_hash = second_universal_hashes[index].calculate(elements[i]);
                if (used[current_hash]) {
                    good_hash = false;
                    break;
                }
                used[current_hash] = true;
            }
            if (good_hash)
                break;
        }
        for (ui32 i = 0; i < elements.size(); i++)
            hashes[index][second_universal_hashes[index].calculate(elements[i])] = elements[i];
    }

    void initSecondTables() {
        for (ui32 i = 0; i < keys_count; i++)
            initTable(i);
    }
public:
    class MultipleKeyException: public std::exception {};
    class ImpossibleKeyException: public std::exception {};

    PerfectHashSet() {
        inserted_count = 0;
    }

    void init(const std::vector<ui32> & keys) {
        keys_count = keys.size();
        hashes.assign(keys_count, std::vector<ui32>());
        second_universal_hashes.assign(keys_count, UniversalHash());
        initFirstTable(keys);
        initSecondTables();
    }

    bool isPossibleKey(ui32 key) const {
        ui32 index = first_universal_hash.calculate(key);
        if (hashes[index].empty())
            return false;
        return hashes[index][second_universal_hashes[index].calculate(key)] == key;
    }

    bool has(ui32 key) const {
        ui32 index = first_universal_hash.calculate(key);
        return isPossibleKey(key) && inserted[index][second_universal_hashes[index].calculate(key)];
    }

    void insert(ui32 key) {
        if (not isPossibleKey(key))
            throw ImpossibleKeyException();
        ui32 index = first_universal_hash.calculate(key);
        ui32 current_hash = second_universal_hashes[index].calculate(key);
        if (!inserted[index][current_hash]) {
            inserted[index][current_hash] = true;
            inserted_count++;
        }
    }

    void erase(ui32 key) {
        if (not isPossibleKey(key))
            throw ImpossibleKeyException();
        ui32 index = first_universal_hash.calculate(key);
        ui32 current_hash = second_universal_hashes[index].calculate(key);
        if (inserted[index][current_hash]) {
            inserted[index][current_hash] = false;
            inserted_count--;
        }
    }

    ui32 size() const {
        return inserted_count;
    }
};
