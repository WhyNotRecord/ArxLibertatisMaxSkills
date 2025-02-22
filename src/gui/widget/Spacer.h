/*
 * Copyright 2018-2021 Arx Libertatis Team (see the AUTHORS file)
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

#ifndef ARX_GUI_WIDGET_SPACER_H
#define ARX_GUI_WIDGET_SPACER_H

#include <string>

#include "gui/widget/Widget.h"
#include "math/Rectangle.h"
#include "platform/Platform.h"

class Spacer final : public Widget {
	
public:
	
	explicit Spacer(int height) {
		m_rect = Rectf(0, float(height));
		setEnabled(false);
	}
	
	void render(bool mouseOver = false) override {
		ARX_UNUSED(mouseOver);
	}
	
	WidgetType type() const override {
		return WidgetType_Spacer;
	}
	
};

#endif // ARX_GUI_WIDGET_SPACER_H
