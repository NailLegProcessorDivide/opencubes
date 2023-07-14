#pragma once
#ifndef OPENCUBES_CACHE_HPP
#define OPENCUBES_CACHE_HPP
#include <unordered_set>
#include <fstream>
#include <string>
#include "structs.hpp"

Hashy load(string path)
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
    Hashy cubes;
    cubes.init(cubelen);
    for (int i = 0; i < numCubes; ++i)
    {
        Cube next;
        next.sparse.resize(cubelen);
        XYZ shape;
        for (int k = 0; k < cubelen; ++k)
        {
            ifs.read((char *)&next.sparse[k].joined, 4);
            if (next.sparse[k].x > shape.x)
                shape.x = next.sparse[k].x;
            if (next.sparse[k].y > shape.y)
                shape.y = next.sparse[k].y;
            if (next.sparse[k].z > shape.z)
                shape.z = next.sparse[k].z;
        }
        cubes.insert(next, shape);
    }
    printf("  loaded %lu cubes\n\r", cubes.size());
    return cubes;
}

void save(string path, Hashy &cubes, uint8_t n)
{
    if (cubes.size() == 0)
        return;
    ofstream ofs(path, ios::binary);
    ofs << n;
    for (const auto &s : cubes.byshape)
        for (const auto &c : s.second.set)
        {
            for (const auto &p : c.sparse)
            {
                ofs.write((const char *)&p.joined, sizeof(p.joined));
            }
        }
}

#endif
