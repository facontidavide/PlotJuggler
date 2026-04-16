#ifndef AutoHideTabH
#define AutoHideTabH
/*******************************************************************************
** Qt Advanced Docking System
** Copyright (C) 2017 Uwe Kindler
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


//============================================================================
/// \file   AutoHideTab.h
/// \author Syarif Fakhri
/// \date   05.09.2022
/// \brief  Declaration of CAutoHideTab class
//============================================================================

//============================================================================
//                                   INCLUDES
//============================================================================
#include "PushButton.h"

#include "ads_globals.h"

namespace ads
{
struct AutoHideTabPrivate;
class CDockWidget;
class CAutoHideSideBar;
class CDockWidgetTab;
class DockContainerWidgetPrivate;

/**
 * A dock widget Side tab that shows a title or an icon.
 * The dock widget tab is shown in the side tab bar to switch between
 * pinned dock widgets
 */
class ADS_EXPORT CAutoHideTab : public CPushButton
{
    Q_OBJECT

    Q_PROPERTY(int sideBarLocation READ sideBarLocation)
    Q_PROPERTY(Qt::Orientation orientation READ orientation)
	Q_PROPERTY(bool activeTab READ isActiveTab)
	Q_PROPERTY(bool iconOnly READ iconOnly)

private:
	AutoHideTabPrivate* d; ///< private data (pimpl)
    friend struct AutoHideTabPrivate;
	friend class CDockWidget;
	friend class CAutoHideDockContainer;
	friend class CAutoHideSideBar;
	friend class CDockAreaWidget;
	friend class CDockContainerWidget;
	friend DockContainerWidgetPrivate;

private Q_SLOTS:
	void onAutoHideToActionClicked();
	void onDragHoverDelayExpired();

protected:
	void setSideBar(CAutoHideSideBar *SideTabBar);
	void removeFromSideBar();
	virtual bool event(QEvent* event) override;
	virtual void contextMenuEvent(QContextMenuEvent* ev) override;
	virtual void mousePressEvent(QMouseEvent* ev) override;
	virtual void mouseReleaseEvent(QMouseEvent* ev) override;
	virtual void mouseMoveEvent(QMouseEvent* ev) override;
    virtual void dragEnterEvent(QDragEnterEvent* ev) override;
    virtual void dragLeaveEvent(QDragLeaveEvent* ev) override;

public:
    using Super = CPushButton;

	/**
	 * Default Constructor
	 * param[in] DockWidget The dock widget this title bar belongs to
	 * param[in] parent The parent widget of this title bar
	 */
	CAutoHideTab(QWidget* parent = nullptr);

	/**
	 * Virtual Destructor
	 */
    ~CAutoHideTab() override;

	/**
	 * Update stylesheet style if a property changes
	 */
	void updateStyle();

	/**
	 * Getter for side tab bar area property
	 */
	SideBarLocation sideBarLocation() const;

	/**
	 * Set orientation vertical or horizontal
	 */
	void setOrientation(Qt::Orientation Orientation);

	/**
	 * Returns the current orientation
	 */
	Qt::Orientation orientation() const;

	/**
	 * Returns true, if this is the active tab. The tab is active if the auto
	 * hide widget is visible
	 */
	bool isActiveTab() const;

	/**
	 * returns the dock widget this belongs to
	 */
	CDockWidget* dockWidget() const;

	/**
	 * Sets the dock widget that is controlled by this tab
	 */
	void setDockWidget(CDockWidget* DockWidget);

	/**
	 * Returns true if the auto hide config flag AutoHideSideBarsIconOnly
	 * is set and if the tab has an icon - that means the icon is not null
	 */
	bool iconOnly() const;

	/**
	 * Returns the side bar that contains this tab or a nullptr if the tab is
	 * not in a side bar
	 */
	CAutoHideSideBar* sideBar() const;

	/**
	 * Returns the index of this tab in the sideBar
	 */
	int tabIndex() const;

public Q_SLOTS:
	/**
	 * Set the dock widget floating, if it is floatable
	 */
	void setDockWidgetFloating();

	/**
	 * Unpin and dock the auto hide widget
	 */
	void unpinDockWidget();

	/**
	 * Calls the requestCloseDockWidget() function for the assigned dock widget
	 */
	void requestCloseDockWidget();
}; // class AutoHideTab
}
 // namespace ads
//-----------------------------------------------------------------------------

#endif
