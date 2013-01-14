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

#ifndef __TrenchBroom__MoveObjectsTool__
#define __TrenchBroom__MoveObjectsTool__

#include "Controller/Tool.h"
#include "Controller/MoveHandle.h"
#include "Model/Picker.h"
#include "Utility/VecMath.h"

using namespace TrenchBroom::Math;

namespace TrenchBroom {
    namespace Renderer {
        class MovementIndicator;
        class Vbo;
        class RenderContext;
    }
    
    namespace Controller {
        class MoveObjectsTool : public PlaneDragTool {
        protected:
            typedef enum {
                LeftRight,
                Horizontal,
                Vertical
            } Direction;
            
            Vec3f m_totalDelta;
            Direction m_direction;
            Renderer::MovementIndicator* m_indicator;
            
            void handleRender(InputState& inputState, Renderer::Vbo& vbo, Renderer::RenderContext& renderContext);
            void handleFreeRenderResources();

            bool handleStartPlaneDrag(InputState& inputState, Plane& plane, Vec3f& initialPoint);
            bool handlePlaneDrag(InputState& inputState, const Vec3f& lastPoint, const Vec3f& curPoint, Vec3f& refPoint);
            void handleEndPlaneDrag(InputState& inputState);

            void handleObjectsChange(InputState& inputState);
            void handleEditStateChange(InputState& inputState, const Model::EditStateChangeSet& changeSet);
            void handleGridChange(InputState& inputState);
        public:
            MoveObjectsTool(View::DocumentViewHolder& documentViewHolder, InputController& inputController, float axisLength, float planeRadius);
        };
    }
}

#endif /* defined(__TrenchBroom__MoveObjectsTool__) */
