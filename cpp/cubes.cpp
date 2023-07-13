#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <thread>
#include <unordered_set>
#include <vector>
#include <fstream>

using namespace std;
// #define DBG 1
struct XYZ
{
    union
    {
        struct
        {
            int8_t x, y, z, res;
        };
        int8_t data[4];
        int32_t joined;
    };
    explicit XYZ(int a = 0, int b = 0, int c = 0) : x(a), y(b), z(c), res(0) {}
    bool operator==(const XYZ &rhs) const { return joined == rhs.joined; }
    bool operator<(const XYZ &b) const { return joined < b.joined; }
};
namespace std
{
    template <>
    struct hash<XYZ>
    {
        size_t operator()(const XYZ &x) const { return x.joined; }
    };
} // namespace std

struct Rotations
{
    static constexpr array<int, 6> LUT[] = {
        {0, 1, 2, -1, -1, 1},
        {0, 1, 2, -1, 1, -1},
        {0, 1, 2, 1, -1, -1},
        {0, 1, 2, 1, 1, 1},
        {0, 2, 1, -1, -1, -1},
        {0, 2, 1, -1, 1, 1},
        {0, 2, 1, 1, -1, 1},
        {0, 2, 1, 1, 1, -1},
        {1, 0, 2, -1, -1, -1},
        {1, 0, 2, -1, 1, 1},
        {1, 0, 2, 1, -1, 1},
        {1, 0, 2, 1, 1, -1},
        {1, 2, 0, -1, -1, 1},
        {1, 2, 0, -1, 1, -1},
        {1, 2, 0, 1, -1, -1},
        {1, 2, 0, 1, 1, 1},
        {2, 0, 1, -1, -1, 1},
        {2, 0, 1, -1, 1, -1},
        {2, 0, 1, 1, -1, -1},
        {2, 0, 1, 1, 1, 1},
        {2, 1, 0, -1, -1, -1},
        {2, 1, 0, -1, 1, 1},
        {2, 1, 0, 1, -1, 1},
        {2, 1, 0, 1, 1, -1},
    };
    static std::vector<XYZ> rotate(int i, std::array<int, 3> shape, const std::vector<XYZ> &orig)
    {
        std::vector<XYZ> res;
        res.reserve(orig.size());
        const auto L = LUT[i];
        for (const auto &o : orig)
        {
            XYZ next;
            if (L[3] < 0)
                next.x = shape[L[0]] - o.data[L[0]];
            else
                next.x = o.data[L[0]];

            if (L[4] < 0)
                next.y = shape[L[1]] - o.data[L[1]];
            else
                next.y = o.data[L[1]];

            if (L[5] < 0)
                next.z = shape[L[2]] - o.data[L[2]];
            else
                next.z = o.data[L[2]];
            res.push_back(next);
        }
        return res;
    }
};

struct Cube
{
    vector<XYZ> sparse;
    bool operator==(const Cube &rhs) const { return this->sparse == rhs.sparse; }
    void print()
    {
        for (auto &p : sparse)
            printf("  (%2d %2d %2d)\n\r", p.x, p.y, p.z);
    }
};
namespace std
{
    template <>
    struct hash<Cube>
    {
        size_t operator()(const Cube &cube) const
        {
            // https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector/72073933#72073933
            std::size_t seed = cube.sparse.size();
            for (auto &p : cube.sparse)
            {
                auto x = std::hash<XYZ>()(p);
                // x = ((x >> 16) ^ x) * 0x45d9f3b;
                // x = ((x >> 16) ^ x) * 0x45d9f3b;
                // x = (x >> 16) ^ x;
                seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };
} // namespace std

unordered_set<Cube> load(string path)
{
    auto ifs = ifstream(path, ios::binary);
    if (!ifs.is_open())
        return {};
    uint8_t cubelen = 0;
    uint filelen = ifs.tellg();
    ifs.seekg(0, ios::end);
    filelen = (uint)ifs.tellg() - filelen;
    ifs.seekg(0, ios::beg);
    ifs.read((char *)&cubelen, 1);
    printf("loading cache file \"%s\" (%u bytes) with N = %d\n\r", path.c_str(), filelen, cubelen);

    auto cubeSize = 4 * (int)cubelen;
    auto numCubes = (filelen - 1) / cubeSize;
    if (numCubes * cubeSize + 1 != filelen)
    {
        printf("error reading file, size does not match");
        return {};
    }
    printf("  num polycubes loading: %d\n\r", numCubes);
    unordered_set<Cube> cubes;
    for (int i = 0; i < numCubes; ++i)
    {
        Cube next;
        next.sparse.resize(cubelen);
        for (int k = 0; k < cubelen; ++k)
        {
            ifs.read((char *)&next.sparse[k].joined, 4);
        }
        cubes.insert(next);
    }
    printf("  loaded %lu cubes\n\r", cubes.size());
    return cubes;
}

void save(string path, unordered_set<Cube> &cubes)
{
    if (cubes.size() == 0)
        return;
    ofstream ofs(path, ios::binary);
    ofs << (uint8_t)cubes.begin()->sparse.size();
    for (const auto &c : cubes)
    {
        for (const auto &p : c.sparse)
        {
            ofs.write((const char *)&p.joined, sizeof(p.joined));
        }
    }
}

void expand(const Cube &c, unordered_set<Cube> &hashes)
{
    unordered_set<XYZ> candidates;
    for (const auto &p : c.sparse)
    {
        candidates.insert(XYZ{p.x + 1, p.y, p.z});
        candidates.insert(XYZ{p.x - 1, p.y, p.z});
        candidates.insert(XYZ{p.x, p.y + 1, p.z});
        candidates.insert(XYZ{p.x, p.y - 1, p.z});
        candidates.insert(XYZ{p.x, p.y, p.z + 1});
        candidates.insert(XYZ{p.x, p.y, p.z - 1});
    }
    for (const auto &p : c.sparse)
    {
        candidates.erase(XYZ{p.x, p.y, p.z});
    }
#ifdef DBG
    printf("candidates: %lu\n\r", candidates.size());
#endif
    for (const auto &p : candidates)
    {
#ifdef DBG
        printf("(%2d %2d %2d)\n\r", p.x, p.y, p.z);
#endif
        int ax = (p.x < 0) ? 1 : 0;
        int ay = (p.y < 0) ? 1 : 0;
        int az = (p.z < 0) ? 1 : 0;
        Cube newCube;
        newCube.sparse.push_back(XYZ{p.x + ax, p.y + ay, p.z + az});
        std::array<int, 3> shape{p.x + ax, p.y + ay, p.z + az};
        for (const auto &np : c.sparse)
        {
            auto nx = np.x + ax;
            auto ny = np.y + ay;
            auto nz = np.z + az;
            if (nx > shape[0])
                shape[0] = nx;
            if (ny > shape[1])
                shape[1] = ny;
            if (nz > shape[2])
                shape[2] = nz;
            newCube.sparse.push_back(XYZ{nx, ny, nz});
        }
        // printf("shape %2d %2d %2d\n\r", shape[0], shape[1], shape[2]);
        // newCube.print();

        // check rotations
        Cube rotatedCube;
        bool found = false;
        Cube lowestHash;
        size_t currentHash = -1;
        for (int i = 0; i < 24; ++i)
        {
            rotatedCube = Cube{Rotations::rotate(i, shape, newCube.sparse)};
            std::sort(rotatedCube.sparse.begin(), rotatedCube.sparse.end());
            // printf("%d --- ---\n\r", i);
            // rotatedCube.print();
            if (hashes.count(rotatedCube))
            {
                found = true;
                break;
            }
            auto h = hash<Cube>()(rotatedCube);
            if (currentHash > h)
            {
                currentHash = h;
                lowestHash = rotatedCube;
            }
        }
        if (!found)
        {
            hashes.insert(lowestHash);
#ifdef DBG
            printf("=====\n\r");
            rotatedCube.print();
            printf("inserted! (num %2lu)\n\n\r", hashes.size());
#endif
        }
    }
#ifdef DBG
    printf("new hashes: %lu\n\r", hashes.size());
#endif
}

void expandPart(vector<Cube> &base, unordered_set<Cube> &hashes, size_t start, size_t end)
{
    printf("  start from %lu to %lu\n\r", start, end);
    auto t_start = chrono::steady_clock::now();

    for (auto i = start; i < end; ++i)
    {
        expand(base[i], hashes);
        auto count = i - start;
        if (start == 0 && (count % 100 == 0))
        {
            auto t_end = chrono::steady_clock::now();
            auto dt_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
            auto perc = 100 * count / (end - start);
            auto its = 1000.f * count / dt_ms;
            auto remaining = (end - i) / its;
            printf(" %3lu%% %5.0f it/s, remaining: %.0fs\033[0K\r", perc, its, remaining);
            flush(cout);
        }
    }
    auto t_end = chrono::steady_clock::now();
    auto dt_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
    printf("  done from %lu to %lu: found %lu\n\r", start, end, hashes.size());
    printf("  took %.2f s\033[0K\n\r", dt_ms / 1000.f);
}

unordered_set<Cube> gen(int n, int threads = 1)
{
    if (n < 1)
        return {};
    else if (n == 1)
        return {{{XYZ{0, 0, 0}}}};
    else if (n == 2)
        return {{{XYZ{0, 0, 0}, XYZ{1, 0, 0}}}};
    auto hashes =
        load("cubes_" + to_string(n) + ".bin");

    if (hashes.size() != 0)
        return hashes;

    auto base = gen(n - 1, threads);
    printf("N = %d || generating new cubes from %lu base cubes.\n\r", n, base.size());
    int count = 0;
    if (threads == 1 || base.size() < 100)
    {
        auto start = chrono::steady_clock::now();

        for (const auto &b : base)
        {
            expand(b, hashes);
            count++;
            if (count % 100 == 0)
            {
                auto end = chrono::steady_clock::now();
                auto dt_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
                auto perc = 100 * count / (int)base.size();
                auto its = 1000.f * count / dt_ms;
                auto remaining = ((int)base.size() - count) / its;
                printf(" %3d%% %5.0f it/s, remaining: %.0fs\033[0K\r", perc, its, remaining);
                flush(cout);
            }
        }
        auto end = chrono::steady_clock::now();
        auto dt_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        printf("  took %.2f s\033[0K\n\r", dt_ms / 1000.f);
    }
    else
    {
        vector<Cube> baseCubes;
        baseCubes.insert(baseCubes.end(), base.begin(), base.end());
        base.clear();
        base.reserve(1);
        printf("starting %d threads\n\r", threads);
        vector<thread> ts;
        vector<unordered_set<Cube>> multihash(threads);
        for (int i = 0; i < threads; ++i)
        {
            auto start = baseCubes.size() * i / threads;
            auto end = baseCubes.size() * (i + 1) / threads;

            ts.push_back(thread(expandPart, ref(baseCubes), ref(multihash[i]), start, end));
        }
        for (int i = 0; i < threads; ++i)
        {
            ts[i].join();
            hashes.insert(multihash[i].begin(), multihash[i].end());
            multihash[i].clear();
            multihash[i].reserve(1);
        }
    }
    printf("  num cubes: %lu\n\r", hashes.size());
    save("cubes_" + to_string(n) + ".bin", hashes);
    return hashes;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("usage: %s N [NUM_THREADS]\n\r", argv[0]);
        exit(-1);
    }
    int n = atoi(argv[1]);
    int threads = 1;
    if (argc > 2)
        threads = atoi(argv[2]);
    gen(n, threads);
    return 0;
}