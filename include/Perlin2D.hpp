#pragma once

#include <cmath>
#include <map>
#include <random>
#include <ctime>

using point_int = std::pair<int, int>;
using point_float = std::pair<float, float>;

class Perlin2D {
private:
    int tile_size;
    int octaves;
    float scale_factor = (float) std::sqrt(2);
    std::map<point_int, float> angles;

    static float smooth_step(float t) {
        return t * t * (3.f - 2.f * t);
    }

    static float linear_interpolation(float t, float a, float b) {
        return a + t * (b - a);
    }

    static point_float angle_to_point(float angle) {
        return { std::cos(angle), std::sin(angle) };
    }

    static float generate_angle() {
        static std::mt19937 gen(time(nullptr));
        static std::uniform_real_distribution<float> urd(0, 2 * M_PI);
        return urd(gen);
    }

    float get_plain_noise(point_float point);

public:
    explicit Perlin2D(int tile_size, int octaves = 4);

    void update_angles(float eps);

    float compute_noise(float x, float y);
};