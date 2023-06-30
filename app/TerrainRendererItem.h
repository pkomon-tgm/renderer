/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company.
** Author: Giuseppe D'Angelo
** Contact: info@kdab.com
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef TERRAINRENDERERITEM_H
#define TERRAINRENDERERITEM_H

#include "nucleus/camera/Definition.h"
#include "nucleus/event_parameter.h"
#include "gl_engine/UniformBufferObjects.h"
#include "TimerFrontendManager.h"
#include <QQuickFramebufferObject>
#include <QTimer>
#include <map>

class TerrainRendererItem : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(int frame_limit READ frame_limit WRITE set_frame_limit NOTIFY frame_limit_changed)
    Q_PROPERTY(nucleus::camera::Definition camera READ camera NOTIFY camera_changed)
    Q_PROPERTY(int camera_width READ camera_width NOTIFY camera_width_changed)
    Q_PROPERTY(int camera_height READ camera_height NOTIFY camera_height_changed)
    Q_PROPERTY(float field_of_view READ field_of_view WRITE set_field_of_view NOTIFY field_of_view_changed)
    Q_PROPERTY(float camera_rotation_from_north READ camera_rotation_from_north NOTIFY camera_rotation_from_north_changed)
    Q_PROPERTY(QPointF camera_operation_centre READ camera_operation_centre NOTIFY camera_operation_centre_changed)
    Q_PROPERTY(bool camera_operation_centre_visibility READ camera_operation_centre_visibility NOTIFY camera_operation_centre_visibility_changed)
    Q_PROPERTY(float camera_operation_centre_distance READ camera_operation_centre_distance NOTIFY camera_operation_centre_distance_changed)
    Q_PROPERTY(float render_quality READ render_quality WRITE set_render_quality NOTIFY render_quality_changed)    
    Q_PROPERTY(gl_engine::uboSharedConfig shared_config MEMBER m_shared_config NOTIFY shared_config_changed)
    Q_PROPERTY(TimerFrontendManager* timer_manager MEMBER m_timer_manager)

public:
    explicit TerrainRendererItem(QQuickItem* parent = 0);
    ~TerrainRendererItem() override;
    Renderer *createRenderer() const Q_DECL_OVERRIDE;

signals:

    void frame_limit_changed();

    void mouse_pressed(const nucleus::event_parameter::Mouse&) const;
    void mouse_moved(const nucleus::event_parameter::Mouse&) const;
    void wheel_turned(const nucleus::event_parameter::Wheel&) const;
    void touch_made(const nucleus::event_parameter::Touch&) const;
    void key_pressed(const QKeyCombination&) const;
    void key_released(const QKeyCombination&) const;
    void update_camera_requested() const;
    //    void viewport_changed(const glm::uvec2& new_viewport) const;
    void position_set_by_user(double new_latitude, double new_longitude);

    void shared_config_changed(gl_engine::uboSharedConfig new_shared_config);

    void camera_changed();
    void camera_width_changed();
    void camera_height_changed();
    void field_of_view_changed();
    void camera_rotation_from_north_changed();
    void camera_operation_centre_changed();
    void camera_operation_centre_visibility_changed();
    void camera_operation_centre_distance_changed();
    void render_quality_changed(float new_render_quality);

protected:
    void touchEvent(QTouchEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;

public slots:
    void set_position(double latitude, double longitude);
    void rotate_north();

private slots:
    void schedule_update();
    void update_camera_request();

public:
    [[nodiscard]] int frame_limit() const;
    void set_frame_limit(int new_frame_limit);

    [[nodiscard]] nucleus::camera::Definition camera() const;
    void set_read_only_camera(const nucleus::camera::Definition& new_camera); // implementation detail

    int camera_width() const;
    void set_read_only_camera_width(int new_camera_width);

    int camera_height() const;
    void set_read_only_camera_height(int new_camera_height);

    float field_of_view() const;
    void set_field_of_view(float new_field_of_view);

    float camera_rotation_from_north() const;
    void set_camera_rotation_from_north(float new_camera_rotation_from_north);

    QPointF camera_operation_centre() const;
    void set_camera_operation_centre(QPointF new_camera_operation_centre);

    bool camera_operation_centre_visibility() const;
    void set_camera_operation_centre_visibility(bool new_camera_operation_centre_visibility);

    float camera_operation_centre_distance() const;
    void set_camera_operation_centre_distance(float new_camera_operation_centre_distance);

    float render_quality() const;
    void set_render_quality(float new_render_quality);

private:
    float m_camera_rotation_from_north = 0;
    QPointF m_camera_operation_centre;
    bool m_camera_operation_centre_visibility = false;
    float m_camera_operation_centre_distance = 1;
    float m_field_of_view = 75;
    int m_frame_limit = 60;
    float m_render_quality = 0.5f;

    gl_engine::uboSharedConfig m_shared_config;


    QTimer* m_update_timer = nullptr;
    nucleus::camera::Definition m_camera;
    int m_camera_width = 0;
    int m_camera_height = 0;

    TimerFrontendManager* m_timer_manager;
};

#endif // TERRAINRENDERERITEM_H
