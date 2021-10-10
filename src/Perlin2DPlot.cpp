#include "include/Perlin2DPlot.hpp"

Perlin2DPlot::Perlin2DPlot() {
    static_update();
    indices_update();
}

// get `xz_changed` and reset it to false
bool Perlin2DPlot::is_xz_changed_with_reset() {
    if (xz_changed) {
        xz_changed = false;
        return true;
    }
    return false;
}

// increasing `grid_size` by one (if possible)
void Perlin2DPlot::improve_grid() {
    if (grid_size + 1 <= max_grid_size) {
        grid_size += 1;
        static_update();
        indices_update();
        xz_changed = true;
    }
}

// decreasing `grid_size` by one (if possible)
void Perlin2DPlot::degrade_grid() {
    if (grid_size - 1 >= min_grid_size) {
        grid_size -= 1;
        static_update();
        indices_update();
        xz_changed = true;
    }
}

// increasing `isoline_count` by one (if possible)
void Perlin2DPlot::increase_isoline_count() {
    if (isoline_count + 1 <= max_isoline_count) {
        isoline_count += 1;
    }
}

// decreasing `isoline_count` by one (if possible)
void Perlin2DPlot::decrease_isoline_count() {
    if (isoline_count - 1 >= min_isoline_count) {
        isoline_count -= 1;
    }
}

// updating y coordinate and color
void Perlin2DPlot::dynamic_update(bool stop_the_time) {
    if (!stop_the_time)
        perlin.update_angles(perlin_eps);

    // updating sizes
    vertices_y.resize(vertices_size());
    vertices_color.resize(vertices_size());

    float current_x;
    float current_y;
    float current_z;
    uint8_t new_color;

    // computing y coordinate (height) and color (in [0..255])
    for (std::size_t i = 0; i < vertices_size(); ++i) {
        current_x = vertices_x[i];
        current_z = vertices_z[i];
        std::tie(current_y, new_color) = compute_y(current_x, current_z);

        vertices_y[i] = current_y;
        vertices_color[i].red = 255 - new_color;
        vertices_color[i].green = 255 - new_color / 2;
        vertices_color[i].blue = new_color;
    }
}

// actual size of vertices
[[nodiscard]] std::size_t Perlin2DPlot::vertices_size() const {
    return (grid_size + 1) * (grid_size + 1);
}

// 2D indices -> 1D indices
[[nodiscard]] int Perlin2DPlot::get_index(int w, int h) const {
    return w * (grid_size + 1) + h;
};

// [start_point, end_point] -> [0, perlin_tile_size]
[[nodiscard]] std::pair<float, float> Perlin2DPlot::convert(float x, float z) const {
    return {
        (x - start_point_x) / (end_point_x - start_point_x) * (float) perlin_tile_size,
        (z - start_point_z) / (end_point_z - start_point_z) * (float) perlin_tile_size
    };
}

// (x, z, time) -> y (height)
std::pair<float, int> Perlin2DPlot::compute_y(float x, float z) {
    std::tie(x, z) = convert(x, z);
    float cut = 0.5f;
    float y = perlin.compute_noise(x, z);
    float y_cut = std::min(cut, std::max(-cut, y)); // in [-cut, cut]
    float y_normalized = (y_cut + cut) / (cut * 2); // in [0, 1]
    int color = std::lround(y_normalized * 255);
    return { y, color };
}

// updating x and z coordinates
void Perlin2DPlot::static_update() {
    // updating sizes
    vertices_x.resize(vertices_size());
    vertices_z.resize(vertices_size());

    float current_x;
    float current_z;

    // computing (x, z) coordinates in 2D grid
    for (int w = 0; w <= grid_size; ++w) {
        current_x = start_point_x + (float) w * (end_point_x - start_point_x) / (float) grid_size;
        for (int h = 0; h <= grid_size; ++h) {
            current_z = start_point_z + (float) h * (end_point_z - start_point_z) / (float) grid_size;
            vertices_x[get_index(w, h)] = current_x;
            vertices_z[get_index(w, h)] = current_z;
        }
    }
}

// updating plot indices
void Perlin2DPlot::indices_update() {
    vertex_indices.clear();
    for (int w = 0; w < grid_size; ++w) {
        for (int h = 0; h < grid_size; ++h) {
            // bottom-left triangle in current square
            vertex_indices.push_back(get_index(w + 0, h + 0));
            vertex_indices.push_back(get_index(w + 1, h + 0));
            vertex_indices.push_back(get_index(w + 0, h + 1));
            // top-right triangle in current square
            vertex_indices.push_back(get_index(w + 0, h + 1));
            vertex_indices.push_back(get_index(w + 1, h + 0));
            vertex_indices.push_back(get_index(w + 1, h + 1));
        }
    }
}