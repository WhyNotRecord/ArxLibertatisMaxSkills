/*
 * Copyright 2011-2021 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "game/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include "graphics/Math.h"
#include "graphics/Renderer.h"


Entity * g_cameraEntity;
PreparedCamera g_preparedCamera;
Camera * g_camera = nullptr;
Camera g_playerCamera;
Vec3f g_playerCameraStablePos;

Anglef Camera::getLookAtAngle(const Vec3f & origin, const Vec3f & target) {
	
	Anglef angle;
	
	if(origin != target) {
		float targetz = origin.z + glm::distance(Vec2f(origin.x, origin.z), Vec2f(target.x, target.z));
		angle.setPitch(MAKEANGLE(-glm::degrees(getAngle(origin.y, origin.z, target.y, targetz))));
		angle.setYaw(MAKEANGLE(glm::degrees(getAngle(origin.x, origin.z, target.x, target.z))));
		angle.setRoll(0.f);
	}
	
	return angle;
}

static glm::mat4x4 createProjectionMatrix(const Vec2f & size, const Vec2f & center, Camera * cam) {
	
	float fov = cam->fov();
	
	const float nearDist = 1.f;
	const float farDist = cam->cdepth;
	const float frustumDepth = farDist - nearDist;
	
	float aspectw = (size.y < size.x) ? size.y / size.x : 1.f;
	float aspecth = (size.y < size.x) ? 1.f : size.x / size.y;
	float w = aspectw * (glm::cos(fov / 2) / glm::sin(fov / 2));
	float h = aspecth * (glm::cos(fov / 2) / glm::sin(fov / 2));
	float Q = farDist / frustumDepth;
	
	glm::mat4x4 projectionMatrix(1.f);
	projectionMatrix[0][0] = w;
	projectionMatrix[1][1] = -h;
	projectionMatrix[2][2] = Q;
	projectionMatrix[3][2] = -Q * nearDist;
	projectionMatrix[2][3] = 1.f;
	projectionMatrix[3][3] = 0.f;
	
	glm::mat4x4 centerShift(1.f);
	centerShift = glm::translate(centerShift, Vec3f(2.f * center.x / size.x - 1.f,
	                                                1.f - 2.f * center.y / size.y,
	                                                0.f));
	
	return centerShift * projectionMatrix;
}

void PrepareCamera(Camera * cam, const Rect & viewport, const Vec2i & projectionCenter) {
	
	SetActiveCamera(cam);
	
	glm::mat4x4 rotation = toRotationMatrix(cam->angle);
	g_preparedCamera.m_worldToView = glm::translate(rotation, -cam->m_pos);
	GRenderer->SetViewMatrix(g_preparedCamera.m_worldToView);
	
	const Vec2f center = Vec2f(projectionCenter - viewport.topLeft());
	g_preparedCamera.m_viewToClip = createProjectionMatrix(Vec2f(viewport.size()), center, cam);
	GRenderer->SetProjectionMatrix(g_preparedCamera.m_viewToClip);
	
	// Change coordinate system from [-1, 1] x [-1, 1] to [0, width] x [0, height] and flip the y axis
	glm::mat4x4 ndcToScreen(1.f);
	ndcToScreen = glm::scale(ndcToScreen, Vec3f(Vec2f(viewport.size()) / 2.f, 1.f));
	ndcToScreen = glm::translate(ndcToScreen, Vec3f(1.f, 1.f, 0.f));
	ndcToScreen = glm::scale(ndcToScreen, Vec3f(1.f, -1.f, 1.f));
	g_preparedCamera.m_viewToScreen = ndcToScreen * g_preparedCamera.m_viewToClip;
	
	g_preparedCamera.m_worldToScreen = g_preparedCamera.m_viewToScreen * g_preparedCamera.m_worldToView;
	
	GRenderer->SetViewport(viewport);
	
	// Equivalent to glm::inverse(g_preparedCamera.m_worldToView) but more stable and efficient
	g_preparedCamera.m_viewToWorld = glm::translate(glm::mat4x4(1.f), cam->m_pos) * glm::transpose(rotation);
	
	// Equivalent to glm::inverse(g_preparedCamera.m_viewToScreen)
	g_preparedCamera.m_center = Vec2f(projectionCenter);
	Vec2f defov = Vec2f(g_preparedCamera.m_viewToClip[0][0], -g_preparedCamera.m_viewToClip[1][1]);
	g_preparedCamera.m_screenToView = 2.f / (Vec2f(viewport.size()) * defov);
	
}

void SetActiveCamera(Camera * cam) {
	g_camera = cam;
}

Vec3f screenToWorldSpace(Vec2f pos, float depth) {
	/*
	 * Convert from screen to view-space coordinates. Equivalent to:
	 * Vec4f d = g_preparedCamera.m_viewToScreen * Vec4f(0.f, 0.f, depth, 1.f);
	 * view = glm::inverse(g_preparedCamera.m_viewToScreen) * Vec4f(pos * d.w, d.z, d.w))
	 */
	Vec4f view(
		(pos - g_preparedCamera.m_center) * g_preparedCamera.m_screenToView * depth,
		depth,
		1.f
	);
	/*
	 * Convert from view to world-space coordinates. Equivalent to:
	 * return Vec3f(glm::inverse(g_preparedCamera.m_worldToView) * view);
	 */
	return Vec3f(g_preparedCamera.m_viewToWorld * view);
}
