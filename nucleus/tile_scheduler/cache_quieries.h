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

#pragma once

#include "nucleus/srs.h"
#include "nucleus/tile_scheduler/Cache.h"
#include "radix/height_encoding.h"

#include <QImage>

#include "stb_slim/stb_image.h"

namespace nucleus::tile_scheduler::cache_queries {

inline float query_altitude(MemoryCache* cache, const glm::dvec2& lat_long)
{
    const auto world_space = srs::lat_long_to_world(lat_long);
    nucleus::tile_scheduler::tile_types::TileQuad selected_quad;
    cache->visit([&](const nucleus::tile_scheduler::tile_types::TileQuad& tile) {
        if (srs::tile_bounds(tile.id).contains(world_space)) {
            selected_quad = tile;
            return true;
        }
        return false;
    });

    for (const auto& layered_tile : selected_quad.tiles) {
        if (srs::tile_bounds(layered_tile.id).contains(world_space)) {
            const auto bounds = srs::tile_bounds(layered_tile.id);
            const auto uv = (world_space - bounds.min) / bounds.size();

            int width, height;
            auto data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(layered_tile.height->constData()),
                                              layered_tile.height->size(), &width, &height, nullptr, 4);
            assert(data != 0);
            if (data == 0)
                return 1000;

            const auto getPixelData = [&data, &width] (int x, int y) -> std::array<uint8_t, 3> {
                const auto index = 4 * (x + y * width); //4 components per color
                return {data[index], data[index + 1], data[index + 2]};
            };

            const auto x = int(uv.x * width);
            const auto y = int((1 - uv.y) * height);
            const auto rgb = getPixelData(x, y);
            return radix::height_encoding::to_float({rgb[0], rgb[1], rgb[2]});
        }
    }

    assert(false);
    return 1000;
}

} // namespace nucleus::tile_scheduler::cache_queries
