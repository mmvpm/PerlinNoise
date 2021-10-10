#pragma once

#include <vector>
#include "Perlin2D.hpp"

class Perlin2DPlot {
private:
    // configuration
    float start_point_x = -1.f;
    float start_point_z = -1.f;
    float end_point_x   = +1.f;
    float end_point_z   = +1.f;

    int min_grid_size = 10;
    int max_grid_size = 60;

    int min_isoline_count = 0;
    int max_isoline_count = 20;

    float perlin_eps = 0.05f;
    int perlin_tile_size = 3;

private:
    int grid_size = 20;
    bool xz_changed = true; // `true` for first uploading to buffers
    
    Perlin2D perlin = Perlin2D(perlin_tile_size);

private:
    struct color {
        std::uint8_t red;
        std::uint8_t green;
        std::uint8_t blue;
        std::uint8_t alpha;
    };

public: // read only
    int isoline_count = 0;

    std::vector<float> vertices_x;
    std::vector<float> vertices_y;
    std::vector<float> vertices_z;
    std::vector<color> vertices_color;
    std::vector<uint32_t> vertex_indices;

public:
    Perlin2DPlot();

    void improve_grid();
    void degrade_grid();
    void increase_isoline_count();
    void decrease_isoline_count();
    bool is_xz_changed_with_reset();

    void dynamic_update(bool stop_the_time = false);

private:
    [[nodiscard]] std::size_t vertices_size() const;
    [[nodiscard]] int get_index(int w, int h) const;
    [[nodiscard]] std::pair<float, float> convert(float x, float z) const;

    std::pair<float, int> compute_y(float x, float z);

    void static_update();
    void indices_update();
};