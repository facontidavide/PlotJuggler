#ifndef AutoHideSideBarH
#define AutoHideSideBarH
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
/// \file   AutoHideSideBar.h
/// \author Syarif Fakhri
/// \date   05.09.2022
/// \brief  Declaration of CAutoHideSideBar class
//============================================================================

//============================================================================
//                                   INCLUDES
//============================================================================
#include <QScrollArea>
#include "ads_globals.h"
#include "AutoHideTab.h"

QT_FORWARD_DECLARE_CLASS(QXmlStreamWriter)

namespace ads
{
struct AutoHideSideBarPrivate;
class DockContainerWidgetPrivate;
class CDockContainerWidget;
class CAutoHideTab;
class CAutoHideDockContainer;
class CDockingStateReader;

/**
 * Side tab bar widget that is shown at the edges of a dock container.
 * The tab bar is only visible, if it contains visible content, that means if
 * it contains visible tabs. If it is empty or all tabs are hidden, then the
 * side bar is also hidden. As soon as one single tab becomes visible, this
 * tab bar will be shown.
 * The CAutoHideSideBar uses a QScrollArea here, to enable proper resizing.
 * If the side bar contains many tabs, then the tabs are simply clipped - this
 * is the same like in visual studio
 */
class ADS_EXPORT CAutoHideSideBar : public QScrollArea
{
    Q_OBJECT
    Q_PROPERTY(int sideBarLocation READ sideBarLocation)
    Q_PROPERTY(Qt::Orientation orientation READ orientation)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing)

private:
    AutoHideSideBarPrivate* d; ///< private data (pimpl)
	friend struct AutoHideSideBarPrivate;
	friend class DockWidgetSideTab;
	friend DockContainerWidgetPrivate;
	friend CDockContainerWidget;

protected:
	virtual bool eventFilter(QObject *watched, QEvent *event) override;

	/**
	 * Saves the state into the given stream
	 */
	void saveState(QXmlStreamWriter& Stream) const;

	/**
	 * Inserts the given dock widget tab at the given position.
	 * An Index value of -1 appends the side tab at the end.
	 */
	void insertTab(int Index, CAutoHideTab* SideTab);

public:
    using Super = QScrollArea;

	/**
	 * Default Constructor
	 */
    CAutoHideSideBar(CDockContainerWidget* parent, SideBarLocation area);

	/**
	 * Virtual Destructor
	 */
	virtual ~CAutoHideSideBar();

	/**
	 * Removes the given DockWidgetSideTab from the tabbar
	 */
	void removeTab(CAutoHideTab* SideTab);

	/**
	 * Insert dock widget into the side bar.
	 * The function creates the auto hide dock container, inserts the
	 * auto hide tab
	 */
	CAutoHideDockContainer* insertDockWidget(int Index, CDockWidget* DockWidget);

	/**
	 * Removes the auto hide widget from this side bar
	 */
	void removeAutoHideWidget(CAutoHideDockContainer* AutoHideWidget);

	/**
	 * Adds the given AutoHideWidget to this sidebar.
	 * If the AutoHideWidget is in another sidebar, then it will be removed
	 * from this sidebar.
	 */
	void addAutoHideWidget(CAutoHideDockContainer* AutoHideWidget, int Index = TabDefaultInsertIndex);

	/**
	 * Returns orientation of side tab.
	 */
	Qt::Orientation orientation() const;

	/*
	 * Get the side tab widget at position, returns nullptr if it's out of bounds
	 */
	CAutoHideTab* tab(int index) const;

	/**
	 * Returns the tab at the given position.
	 * Returns -1 if the position is left of the first tab and count() if the
	 * position is right of the last tab. Returns InvalidTabIndex (-2) to
	 * indicate an invalid value.
	 */
	int tabAt(const QPoint& Pos) const;

	/**
	 * Returns the tab insertion index for the given mouse cursor position
	 */
	int tabInsertIndexAt(const QPoint& Pos) const;

	/**
	 * Returns the index of the given tab
	 */
	int indexOfTab(const CAutoHideTab& Tab) const;

	/*
	 * Gets the count of the tab widgets
	 */
	int count() const;

	/**
	 * Returns the number of visible tabs to its parent widget.
	 */
	int visibleTabCount() const;

	/**
	 * Returns true, if the sidebar contains visible tabs to its parent widget.
	 * The function returns as soon as it finds the first visible tab.
	 * That means, if you just want to find out if there are visible tabs
	 * then this function is quicker than visibleTabCount()
	 */
	bool hasVisibleTabs() const;

	/**
	 * Getter for side tab bar area property
	 */
	SideBarLocation sideBarLocation() const;

	/**
	 * Overrides the minimumSizeHint() function of QScrollArea
	 * The minimumSizeHint() is bigger than the sizeHint () for the scroll
	 * area because even if the scrollbars are invisible, the required space
	 * is reserved in the minimumSizeHint(). This override simply returns
	 * sizeHint();
	 */
	virtual QSize minimumSizeHint() const override;

	/**
	 * The function provides a sizeHint that matches the height of the
	 * internal viewport.
	 */
	virtual QSize sizeHint() const override;

	/**
	 * Getter for spacing property - returns the spacing of the tabs
	 */
	int spacing() const;

	/**
	 * Setter for spacing property - sets the spacing
	 */
	void setSpacing(int Spacing);

	/**
	 * Returns the dock container that hosts this sideBar()
	 */
	CDockContainerWidget* dockContainer() const;
};
} // namespace ads
//-----------------------------------------------------------------------------
#endif
