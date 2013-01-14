/*
 Copyright (C) 2010-2012 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MoveObjectsTool.h"

#include "Controller/Command.h"
#include "Controller/MoveObjectsCommand.h"
#include "Model/EditStateManager.h"
#include "Model/MapDocument.h"
#include "Model/MapObject.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderContext.h"
#include "Renderer/MovementIndicator.h"
#include "Utility/Grid.h"

#include <cassert>

namespace TrenchBroom {
    namespace Controller {
        void MoveObjectsTool::handleRender(InputState& inputState, Renderer::Vbo& vbo, Renderer::RenderContext& renderContext) {
            if (inputState.mouseButtons() != MouseButtons::MBNone ||
                (inputState.modifierKeys() != ModifierKeys::MKNone &&
                 inputState.modifierKeys() != ModifierKeys::MKAlt))
                return;
            
            if (dragType() != DTNone)
                return;
            
            Model::EditStateManager& editStateManager = document().editStateManager();
            const Model::EntityList& entities = editStateManager.selectedEntities();
            const Model::BrushList& brushes = editStateManager.selectedBrushes();
            if (entities.empty() && brushes.empty())
                return;
            
            Model::ObjectHit* hit = static_cast<Model::ObjectHit*>(inputState.pickResult().first(Model::HitType::ObjectHit, false, view().filter()));
            if (hit == NULL || !hit->object().selected())
                return;

            if (m_indicator == NULL)
                m_indicator = new Renderer::MovementIndicator();
            
            if (inputState.modifierKeys() == ModifierKeys::MKAlt) {
                m_indicator->setDirection(Renderer::MovementIndicator::Vertical);
            } else {
                if (std::abs(inputState.pickRay().direction.z) < 0.3f)
                    m_indicator->setDirection(Renderer::MovementIndicator::LeftRight);
                else
                    m_indicator->setDirection(Renderer::MovementIndicator::Horizontal);
            }

            Vec3f position = renderContext.camera().defaultPoint(inputState.x() + 20.0f, inputState.y() + 20.0f);
            m_indicator->setPosition(position);
            m_indicator->render(vbo, renderContext);
        }
        
        void MoveObjectsTool::handleFreeRenderResources() {
            delete m_indicator;
            m_indicator = NULL;
        }

        bool MoveObjectsTool::handleStartPlaneDrag(InputState& inputState, Plane& plane, Vec3f& initialPoint) {
            if (inputState.mouseButtons() != MouseButtons::MBLeft ||
                (inputState.modifierKeys() != ModifierKeys::MKNone &&
                 inputState.modifierKeys() != ModifierKeys::MKAlt))
                return false;
            
            Model::EditStateManager& editStateManager = document().editStateManager();
            const Model::EntityList& entities = editStateManager.selectedEntities();
            const Model::BrushList& brushes = editStateManager.selectedBrushes();
            if (entities.empty() && brushes.empty())
                return false;
            
            Model::ObjectHit* hit = static_cast<Model::ObjectHit*>(inputState.pickResult().first(Model::HitType::ObjectHit, false, view().filter()));
            if (hit == NULL || !hit->object().selected())
                return false;
            
            initialPoint = hit->hitPoint();
            m_totalDelta = Vec3f::Null;

            if (inputState.modifierKeys() == ModifierKeys::MKAlt) {
                plane = Plane::verticalDragPlane(initialPoint, inputState.camera().direction());
                m_direction = Vertical;
            } else {
                if (std::abs(inputState.pickRay().direction.z) < 0.3f) {
                    plane = Plane::verticalDragPlane(initialPoint, inputState.camera().direction());
                    m_direction = LeftRight;
                } else {
                    plane = Plane::horizontalDragPlane(initialPoint);
                    m_direction = Horizontal;
                }
            }
            
            beginCommandGroup(Controller::Command::makeObjectActionName(wxT("Move"), entities, brushes));
            
            return true;
        }
        
        bool MoveObjectsTool::handlePlaneDrag(InputState& inputState, const Vec3f& lastPoint, const Vec3f& curPoint, Vec3f& refPoint) {
            Vec3f delta = curPoint - refPoint;
            if (m_direction == Vertical) {
                delta = Vec3f::PosZ * delta.dot(Vec3f::PosZ);
            } else if (m_direction == LeftRight) {
                Vec3f axis = Vec3f::PosZ.crossed(dragPlane().normal);
                delta = axis * delta.dot(axis);
            }
            
            Utility::Grid& grid = document().grid();
            delta = grid.snap(delta);
            if (delta.null())
                return true;
            
            Model::EditStateManager& editStateManager = document().editStateManager();
            const Model::EntityList& entities = editStateManager.selectedEntities();
            const Model::BrushList& brushes = editStateManager.selectedBrushes();
            Controller::MoveObjectsCommand* command = Controller::MoveObjectsCommand::moveObjects(document(), entities, brushes, delta, document().textureLock());
            submitCommand(command);
            
            refPoint += delta;
            m_totalDelta += delta;
            return true;
        }
        
        void MoveObjectsTool::handleEndPlaneDrag(InputState& inputState) {
            if (m_totalDelta.null())
                discardCommandGroup();
            else
                endCommandGroup();
        }
        
        void MoveObjectsTool::handleObjectsChange(InputState& inputState) {
        }
        
        void MoveObjectsTool::handleEditStateChange(InputState& inputState, const Model::EditStateChangeSet& changeSet) {
        }

        void MoveObjectsTool::handleGridChange(InputState& inputState) {
        }
        
        MoveObjectsTool::MoveObjectsTool(View::DocumentViewHolder& documentViewHolder, InputController& inputController, float axisLength, float planeRadius) :
        PlaneDragTool(documentViewHolder, inputController, true),
        m_indicator(NULL) {}
    }
}
