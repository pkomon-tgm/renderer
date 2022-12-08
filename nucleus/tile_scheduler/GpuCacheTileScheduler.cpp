/*****************************************************************************
 * Alpine Terrain Builder
 * Copyright (C) 2022 Adam Celarek
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

#include "GpuCacheTileScheduler.h"
#include <QBuffer>
#include <unordered_set>

#include "nucleus/Tile.h"
#include "nucleus/tile_scheduler/utils.h"
#include "nucleus/utils/QuadTree.h"
#include "nucleus/utils/tile_conversion.h"
#include "sherpa/geometry.h"
#include "sherpa/iterator.h"

GpuCacheTileScheduler::GpuCacheTileScheduler()
{
    {
        QImage default_tile(QSize { int(m_ortho_tile_size), int(m_ortho_tile_size) }, QImage::Format_ARGB32);
        default_tile.fill(Qt::GlobalColor::white);
        QByteArray arr;
        QBuffer buffer(&arr);
        buffer.open(QIODevice::WriteOnly);
        default_tile.save(&buffer, "JPEG");
        m_default_ortho_tile = std::make_shared<QByteArray>(arr);
    }

    {
        QImage default_tile(QSize { int(m_height_tile_size), int(m_height_tile_size) }, QImage::Format_ARGB32);
        default_tile.fill(Qt::GlobalColor::black);
        QByteArray arr;
        QBuffer buffer(&arr);
        buffer.open(QIODevice::WriteOnly);
        default_tile.save(&buffer, "PNG");
        m_default_height_tile = std::make_shared<QByteArray>(arr);
    }
    m_purge_timer.setSingleShot(true);
    m_update_timer.setSingleShot(true);
    m_purge_timer.setInterval(5);
    m_update_timer.setInterval(2);
    connect(&m_purge_timer, &QTimer::timeout, this, &GpuCacheTileScheduler::purge_cache_from_old_tiles);
    connect(&m_update_timer, &QTimer::timeout, this, &GpuCacheTileScheduler::do_update);
}

TileScheduler::TileSet GpuCacheTileScheduler::loadCandidates(const camera::Definition& camera, const tile_scheduler::AabbDecoratorPtr& aabb_decorator)
{
    std::unordered_set<tile::Id, tile::Id::Hasher> all_tiles;
    const auto all_leaves = quad_tree::onTheFlyTraverse(
        tile::Id { 0, { 0, 0 } },
        tile_scheduler::refineFunctor(camera, aabb_decorator, 2.0, m_ortho_tile_size),
        [&all_tiles](const tile::Id& v) { all_tiles.insert(v); return v.children(); });
    std::vector<tile::Id> visible_leaves;
    visible_leaves.reserve(all_leaves.size());

    all_tiles.reserve(all_tiles.size() + all_leaves.size());
    std::copy(all_leaves.begin(), all_leaves.end(), sherpa::unordered_inserter(all_tiles));
    return all_tiles;
}

size_t GpuCacheTileScheduler::numberOfTilesInTransit() const
{
    return m_pending_tile_requests.size();
}

size_t GpuCacheTileScheduler::numberOfWaitingHeightTiles() const
{
    return m_received_height_tiles.size();
}

size_t GpuCacheTileScheduler::numberOfWaitingOrthoTiles() const
{
    return m_received_ortho_tiles.size();
}

TileScheduler::TileSet GpuCacheTileScheduler::gpuTiles() const
{
    return m_gpu_tiles;
}

void GpuCacheTileScheduler::updateCamera(const camera::Definition& camera)
{
    if (!enabled())
        return;
    m_current_camera = camera;
    if (!m_update_timer.isActive())
        m_update_timer.start();
}

void GpuCacheTileScheduler::do_update()
{
    const auto aabb_decorator = this->aabb_decorator();

    const auto load_candidates = loadCandidates(m_current_camera, aabb_decorator);
    std::vector<tile::Id> tiles_to_load;
    tiles_to_load.reserve(load_candidates.size());
    std::copy_if(load_candidates.begin(), load_candidates.end(), std::back_inserter(tiles_to_load), [this](const tile::Id& id) {
        if (m_pending_tile_requests.contains(id))
            return false;
        if (m_gpu_tiles.contains(id))
            return false;
        return true;
    });

    const auto n_available_load_slots = m_max_n_simultaneous_requests - m_pending_tile_requests.size();
    assert(n_available_load_slots <= m_max_n_simultaneous_requests);
    const auto n_load_requests = std::min(tiles_to_load.size(), n_available_load_slots);
    assert(n_load_requests <= tiles_to_load.size());
    const auto last_load_tile_iter = tiles_to_load.begin() + int(n_load_requests);
    std::nth_element(tiles_to_load.begin(), last_load_tile_iter, tiles_to_load.end());

    std::for_each(tiles_to_load.begin(), last_load_tile_iter, [this](const auto& t) {
        m_pending_tile_requests.insert(t);
        emit tileRequested(t);
    });

    if (tiles_to_load.size() > n_available_load_slots && !m_update_timer.isActive())
        m_update_timer.start();
}

void GpuCacheTileScheduler::receiveOrthoTile(tile::Id tile_id, std::shared_ptr<QByteArray> data)
{
    m_received_ortho_tiles[tile_id] = data;
    checkLoadedTile(tile_id);
}

void GpuCacheTileScheduler::receiveHeightTile(tile::Id tile_id, std::shared_ptr<QByteArray> data)
{
    m_received_height_tiles[tile_id] = data;
    checkLoadedTile(tile_id);
}

void GpuCacheTileScheduler::notifyAboutUnavailableOrthoTile(tile::Id tile_id)
{
    receiveOrthoTile(tile_id, m_default_ortho_tile);
}

void GpuCacheTileScheduler::notifyAboutUnavailableHeightTile(tile::Id tile_id)
{
    receiveHeightTile(tile_id, m_default_height_tile);
}

void GpuCacheTileScheduler::set_tile_cache_size(unsigned tile_cache_size)
{
    m_tile_cache_size = tile_cache_size;
}

void GpuCacheTileScheduler::purge_cache_from_old_tiles()
{
    if (m_gpu_tiles.size() <= m_tile_cache_size)
        return;

    const auto necessary_tiles = loadCandidates(m_current_camera, this->aabb_decorator());

    std::vector<tile::Id> unnecessary_tiles;
    unnecessary_tiles.reserve(m_gpu_tiles.size());
    std::copy_if(m_gpu_tiles.cbegin(), m_gpu_tiles.cend(), std::back_inserter(unnecessary_tiles), [&necessary_tiles](const auto& tile_id) { return !necessary_tiles.contains(tile_id); });

    const auto n_tiles_to_be_removed = m_gpu_tiles.size() - m_tile_cache_size;
    if (n_tiles_to_be_removed >= unnecessary_tiles.size()) {
        remove_gpu_tiles(unnecessary_tiles); // cache too small. can't remove 'enough', so remove everything we can
        return;
    }
    const auto last_remove_tile_iter = unnecessary_tiles.begin() + int(n_tiles_to_be_removed);
    assert(last_remove_tile_iter < unnecessary_tiles.end());

    std::nth_element(unnecessary_tiles.begin(), last_remove_tile_iter, unnecessary_tiles.end(), [](const tile::Id& a, const tile::Id& b) {
        return !(a < b);
    });
    unnecessary_tiles.resize(n_tiles_to_be_removed);
    remove_gpu_tiles(unnecessary_tiles);
}

void GpuCacheTileScheduler::checkLoadedTile(const tile::Id& tile_id)
{
    if (m_received_height_tiles.contains(tile_id) && m_received_ortho_tiles.contains(tile_id)) {
        m_pending_tile_requests.erase(tile_id);
        auto heightraster = tile_conversion::qImage2uint16Raster(tile_conversion::toQImage(*m_received_height_tiles[tile_id]));
        auto ortho = tile_conversion::toQImage(*m_received_ortho_tiles[tile_id]);
        const auto tile = std::make_shared<Tile>(tile_id, this->aabb_decorator()->aabb(tile_id), std::move(heightraster), std::move(ortho));
        m_received_ortho_tiles.erase(tile_id);
        m_received_height_tiles.erase(tile_id);

        m_gpu_tiles.insert(tile_id);
        emit tileReady(tile);
        if (!m_purge_timer.isActive())
            m_purge_timer.start();
    }
}

bool GpuCacheTileScheduler::enabled() const
{
    return m_enabled;
}

void GpuCacheTileScheduler::setEnabled(bool newEnabled)
{
    m_enabled = newEnabled;
}

void GpuCacheTileScheduler::remove_gpu_tiles(const std::vector<tile::Id>& tiles)
{
    for (const auto& id : tiles) {
        emit tileExpired(id);
        m_gpu_tiles.erase(id);
    }
}
