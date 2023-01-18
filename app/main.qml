/****************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company.
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

import QtQuick 2.0
import MyRenderLibrary 42.0

Window{
    visible: true
    id: root_window

    Rectangle {
        id: root
        width: parent.width
        height: parent.height
        MeshRenderer {
            id: renderer
            width: parent.width
            height: parent.height
        }

        CameraControls {
            camera: renderer

            anchors.bottom: root.bottom
            anchors.horizontalCenter: root.horizontalCenter
        }
    }
}
