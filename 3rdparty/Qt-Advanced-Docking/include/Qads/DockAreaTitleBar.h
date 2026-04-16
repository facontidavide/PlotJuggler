#ifndef DockAreaTitleBarH
#define DockAreaTitleBarH
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
/// \file   DockAreaTitleBar.h
/// \author Uwe Kindler
/// \date   12.10.2018
/// \brief  Declaration of CDockAreaTitleBar class
//============================================================================


//============================================================================
//                                   INCLUDES
//============================================================================
#include <QToolButton>
#include <QFrame>

#include "ads_globals.h"

QT_FORWARD_DECLARE_CLASS(QAbstractButton)

namespace ads
{
class CDockAreaTabBar;
class CDockAreaWidget;
struct DockAreaTitleBarPrivate;
class CElidingLabel;
class CDockAreaTitleBar;

using tTitleBarButton = QToolButton;

/**
* Title bar button of a dock area that customizes tTitleBarButton appearance/behaviour
* according to various config flags such as:
* CDockManager::DockAreaHas_xxx_Button - if set to 'false' keeps the button always invisible
* CDockManager::DockAreaHideDisabledButtons - if set to 'true' hides button when it is disabled
*/
class CTitleBarButton : public tTitleBarButton
{
	Q_OBJECT

private:
	bool ShowInTitleBar = true;
	bool HideWhenDisabled = false;
	TitleBarButton TitleBarButtonId;

public:
	using Super = tTitleBarButton;
	CTitleBarButton(bool ShowInTitleBar, bool HideWhenDisabled, TitleBarButton ButtonId,
		QWidget* parent = nullptr);

	/**
	* Adjust this visibility change request with our internal settings:
	*/
	virtual void setVisible(bool visible) override;

	/**
	 * Configures, if the title bar button should be shown in title bar
	 */
	void setShowInTitleBar(bool Show);

	/**
	 * Identifier for the title bar button
	 */
	TitleBarButton buttonId() const {return TitleBarButtonId;}

	/**
	 * Return the title bar that contains this button
     */
	CDockAreaTitleBar* titleBar() const;

	/**
	 * Returns true, if the button is in a title bar in an auto hide area
	 */
	bool isInAutoHideArea() const;


protected:
	/**
	* Handle EnabledChanged signal to set button invisible if the configured
	*/
	bool event(QEvent *ev) override;
};


/**
 * Title bar of a dock area.
 * The title bar contains a tabbar with all tabs for a dock widget group and
 * with a tabs menu button, a undock button and a close button.
 */
class ADS_EXPORT CDockAreaTitleBar : public QFrame
{
	Q_OBJECT
private:
	DockAreaTitleBarPrivate* d; ///< private data (pimpl)
	friend struct DockAreaTitleBarPrivate;

private Q_SLOTS:
	void onTabsMenuAboutToShow();
	void onCloseButtonClicked();
	void onAutoHideCloseActionTriggered();
	void minimizeAutoHideContainer();
	void onUndockButtonClicked();
	void onTabsMenuActionTriggered(QAction* Action);
	void onCurrentTabChanged(int Index);
	void onAutoHideButtonClicked();
	void onAutoHideDockAreaActionClicked();
	void onAutoHideToActionClicked();

protected:
    /**
	 * Stores mouse position to detect dragging
	 */
	virtual void mousePressEvent(QMouseEvent* ev) override;

	/**
	 * Stores mouse position to detect dragging
	 */
	virtual void mouseReleaseEvent(QMouseEvent* ev) override;

	/**
	 * Starts floating the complete docking area including all dock widgets,
	 * if it is not the last dock area in a floating widget
	 */
	virtual void mouseMoveEvent(QMouseEvent* ev) override;

	/**
	 * Double clicking the title bar also starts floating of the complete area
	 */
	virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

	/**
	 * Show context menu
	 */
	virtual void contextMenuEvent(QContextMenuEvent *event) override;

	/**
	 * Handle resize events
	 */
	virtual void resizeEvent(QResizeEvent *event) override;

public Q_SLOTS:
	/**
	 * Call this slot to tell the title bar that it should update the tabs menu
	 * the next time it is shown.
	 */
	void markTabsMenuOutdated();


public:
	using Super = QFrame;

	/**
	 * Default Constructor
	 */
	CDockAreaTitleBar(CDockAreaWidget* parent);

	/**
	 * Virtual Destructor
	 */
	virtual ~CDockAreaTitleBar();

	/**
	 * Returns the pointer to the tabBar()
	 */
	CDockAreaTabBar* tabBar() const;

	/**
	 * Returns the button corresponding to the given title bar button identifier
	 */
	CTitleBarButton* button(TitleBarButton which) const;

	/**
	 * Returns the auto hide title label, used when the dock area is expanded and auto hidden
	 */
	CElidingLabel* autoHideTitleLabel() const;

	/**
     * Returns the dock area widget that contains this title bar
     */
	CDockAreaWidget* dockAreaWidget() const;

	/**
	 * Updates the visibility of the dock widget actions in the title bar
	 */
	void updateDockWidgetActionsButtons();

	/**
	 * Marks the tabs menu outdated before it calls its base class
	 * implementation
	 */
	virtual void setVisible(bool Visible) override;

	/**
	 * Inserts a custom widget at position index into this title bar.
	 * If index is negative, the widget is added at the end.
	 * You can use this function to insert custom widgets into the title bar.
	 */
	void insertWidget(int index, QWidget *widget);

	/**
	 * Searches for widget widget in this title bar.
	 * You can use this function, to get the position of the default
	 * widget in the tile bar.
	 * \code
	 * int tabBarIndex = TitleBar->indexOf(TitleBar->tabBar());
	 * int closeButtonIndex = TitleBar->indexOf(TitleBar->button(TitleBarButtonClose));
	 * \endcode
	 */
	int indexOf(QWidget *widget) const;

	/**
	 * Close group tool tip based on the current state
	 * Auto hide widgets can only have one dock widget so it does not make sense for the tooltip to show close group
	 */
	QString titleBarButtonToolTip(TitleBarButton Button) const;

	/**
	 * Moves the dock area into its own floating widget if the area
	 * DockWidgetFloatable flag is true
	 */
	void setAreaFloating();

	/**
	 * Call this function, to create all the required auto hide controls
	 */
	void showAutoHideControls(bool Show);

	/**
	 * Returns true, if the auto hide controls are visible
	 */
	bool isAutoHide() const;

    /**
     * Fills the provided menu with standard entries. If a nullptr is passed, a
     * new menu is created and filled with standard entries.
     * This function is called from the actual version of contextMenuEvent, but
     * can be called from any code. Caller is responsible of deleting the created
     * object.
     *
     * @param menu The QMenu to fill with standard entries. If nullptr, a new
     * QMenu will be created.
     * @return The filled QMenu, either the provided one or a newly created one if
     * nullptr was passed.
     */
    virtual QMenu *buildContextMenu(QMenu *);

Q_SIGNALS:
	/**
	 * This signal is emitted if a tab in the tab bar is clicked by the user
	 * or if the user clicks on a tab item in the title bar tab menu.
	 */
	void tabBarClicked(int index);
}; // class name

}
 // namespace ads
//-----------------------------------------------------------------------------
#endif // DockAreaTitleBarH
