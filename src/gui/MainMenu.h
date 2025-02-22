/*
 * Copyright 2014-2021 Arx Libertatis Team (see the AUTHORS file)
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

#ifndef ARX_GUI_MAINMENU_H
#define ARX_GUI_MAINMENU_H

#include "gui/MenuWidgets.h"
#include "gui/widget/Widget.h"
#include "gui/widget/WidgetContainer.h"
#include "math/Types.h"

class MainMenu {
	
public:
	
	bool bReInitAll;
	
	MenuWindow * m_window;
	
	MainMenu(const MainMenu &) = delete;
	MainMenu & operator=(const MainMenu &) = delete;
	
	explicit MainMenu();
	virtual ~MainMenu();
	
	void init();
	void initWindowPages();
	
	void update();
	void render();
	
	void requestPage(MENUSTATE page) {
		m_requestedPage = page;
	}
	
	MENUSTATE requestedPage() const {
		return m_requestedPage;
	}
	
private:
	
	MENUSTATE m_requestedPage;
	
	TextureContainer * m_background;
	WidgetContainer * m_widgets;
	
	TextWidget * m_resumeGame;
	
	Widget * m_selected;
	
};

extern MainMenu * g_mainMenu;

#endif // ARX_GUI_MAINMENU_H
