/*****************************************************************************
 * Alpine Terrain Renderer
 * Copyright (C) 2023 Adam Celarek
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include "utils.h"

#include <QBuffer>

#include "stb_slim/stb_image_write.h"


namespace nucleus::tile_scheduler {

namespace utils {

QByteArray create_single_color_jpg(unsigned int size, const std::array<uint8_t, 3>& rgb, int quality) {
    QByteArray arr;
    QBuffer buffer(&arr);
    std::vector<std::array<uint8_t, 4>> texture_data(size * size, {rgb[0], rgb[1], rgb[2], uint8_t(0)}); // in RBGA32 format, as expected by stbi_image
    auto write_func = [] (void* context, void* data, int size) {
        static_cast<QBuffer*>(context)->write(static_cast<char*>(data), size);
    };
    buffer.open(QIODevice::WriteOnly);
    stbi_write_jpg_to_func(write_func, &buffer, size, size, 4, texture_data.data(), quality);
    buffer.close();
    return arr;
}

QByteArray create_single_color_png(unsigned int size, const std::array<uint8_t, 4>& rgba) {
    QByteArray arr;
    QBuffer buffer(&arr);
    std::vector<std::array<uint8_t, 4>> texture_data(size * size, rgba); // in RBGA32 format, as expected by stbi_image
    auto write_func = [] (void* context, void* data, int size) {
        static_cast<QBuffer*>(context)->write(static_cast<char*>(data), size);
    };
    buffer.open(QIODevice::WriteOnly);
    stbi_write_png_to_func(write_func, &buffer, size, size, 4, texture_data.data(), 0);
    buffer.close();
    return arr;
}

}

}
