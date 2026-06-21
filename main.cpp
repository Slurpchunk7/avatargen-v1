#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <functional>
#include <cstdint>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct Color
{
    unsigned char r, g, b;
};

Color HSVtoRGB(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r, g, b;

    if (h < 60)       { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }

    return {
        (unsigned char)((r + m) * 255),
        (unsigned char)((g + m) * 255),
        (unsigned char)((b + m) * 255)
    };
}

Color pixelColor(int x, int y, std::size_t seed)
{
    uint64_t h = seed;

    h ^= x * 0x9E3779B185EBCA87ULL;
    h ^= y * 0xC2B2AE3D27D4EB4FULL;

    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;

    return {
        (unsigned char)((h >>  0) & 0xFF),
        (unsigned char)((h >>  8) & 0xFF),
        (unsigned char)((h >> 16) & 0xFF)
    };
}

Color gradientColor(int x, int y, int imageSize, Color base)
{
    float cx = imageSize * 0.5f;
    float cy = imageSize * 0.5f;

    float dx = x - cx;
    float dy = y - cy;

    float dist = std::sqrt(dx * dx + dy * dy);
    float maxDist = std::sqrt(cx * cx + cy * cy);

    float t = dist / maxDist;

    int variation = int((0.5f - t) * 100);

    auto clamp = [](int v)
    {
        return (unsigned char)std::max(0, std::min(255, v));
    };

    return {
        clamp(base.r + variation),
        clamp(base.g + variation),
        clamp(base.b + variation)
    };
}

int main()
{
    std::string username;
    std::cout << "Username: ";
    std::getline(std::cin, username);

    std::size_t hash = std::hash<std::string>{}(username);

    Color base{
        (unsigned char)(64 + ((hash >>  0) & 127)),
        (unsigned char)(64 + ((hash >>  8) & 127)),
        (unsigned char)(64 + ((hash >> 16) & 127))
    };

    Color bg{240, 240, 240};

    constexpr int gridSize = 5;
    constexpr int cellSize = 64;
    constexpr int imageSize = gridSize * cellSize;

    std::vector<unsigned char> image(imageSize * imageSize * 3);

    auto setPixel = [&](int x, int y, Color c)
    {
        int idx = (y * imageSize + x) * 3;
        image[idx + 0] = c.r;
        image[idx + 1] = c.g;
        image[idx + 2] = c.b;
    };

    for (int y = 0; y < imageSize; y++)
    {
        for (int x = 0; x < imageSize; x++)
        {
            setPixel(x, y, bg);
        }
    }

    int bitIndex = 0;

    for (int row = 0; row < gridSize; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            bool filled = ((hash * 0x9E3779B97F4A7C15ULL) >> bitIndex) & 1;
            bitIndex++;

            if (!filled)
                continue;

            int mirrorCol = gridSize - 1 - col;

            for (int py = 0; py < cellSize; py++)
            {
                for (int px = 0; px < cellSize; px++)
                {
                    int x1 = col * cellSize + px;
                    int x2 = mirrorCol * cellSize + px;
                    int y  = row * cellSize + py;

                    setPixel(
                        x1,
                        y,
                        gradientColor(x1, y, imageSize, base)
                    );

                    setPixel(
                        x2,
                        y,
                        gradientColor(x2, y, imageSize, base)
                    );
                }
            }
        }
    }

    if (!stbi_write_png(
        "avatar.png",
        imageSize,
        imageSize,
        3,
        image.data(),
        imageSize * 3))
    {
        std::cerr << "Failed to save PNG\n";
        return 1;
    }

    return 0;
}