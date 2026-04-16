#ifndef DockManagerH
#define DockManagerH
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
/// \file   DockManager.h
/// \author Uwe Kindler
/// \date   26.02.2017
/// \brief  Declaration of CDockManager class
//============================================================================


//============================================================================
//                                   INCLUDES
//============================================================================
#include "ads_globals.h"
#include "DockContainerWidget.h"
#include "DockWidget.h"
#include "FloatingDockContainer.h"


QT_FORWARD_DECLARE_CLASS(QSettings)
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace ads
{
struct DockManagerPrivate;
class CFloatingDockContainer;
struct FloatingDockContainerPrivate;
class CDockContainerWidget;
class DockContainerWidgetPrivate;
class CDockOverlay;
class CDockAreaTabBar;
class CDockWidgetTab;
struct DockWidgetTabPrivate;
struct DockAreaWidgetPrivate;
class CIconProvider;
class CDockComponentsFactory;
class CDockFocusController;
class CAutoHideSideBar;
class CAutoHideTab;
struct AutoHideTabPrivate;

/**
 * The central dock manager that maintains the complete docking system.
 * With the configuration flags you can globally control the functionality
 * of the docking system. The dock manager uses an internal stylesheet to
 * style its components like splitters, tabs and buttons. If you want to
 * disable this stylesheet because your application uses its own,
 * just call the function for settings the stylesheet with an empty
 * string.
 * \code
 * DockManager->setStyleSheet("");
 * \endcode
 **/
class ADS_EXPORT CDockManager : public CDockContainerWidget
{
	Q_OBJECT
private:
	DockManagerPrivate* d; ///< private data (pimpl)
	friend struct DockManagerPrivate;
	friend class CFloatingDockContainer;
	friend struct FloatingDockContainerPrivate;
	friend class CDockContainerWidget;
	friend class DockContainerWidgetPrivate;
	friend class CDockAreaTabBar;
	friend class CDockWidgetTab;
	friend struct DockAreaWidgetPrivate;
	friend struct DockWidgetTabPrivate;
	friend class CFloatingDragPreview;
	friend struct FloatingDragPreviewPrivate;
	friend class CDockAreaTitleBar;
	friend class CAutoHideDockContainer;
	friend CAutoHideSideBar;
	friend CAutoHideTab;
	friend AutoHideTabPrivate;

public Q_SLOTS:
	/**
	 * Ends the isRestoringFromMinimizedState
	 */
	void endLeavingMinimizedState();


protected:
	/**
	 * Registers the given floating widget in the internal list of
	 * floating widgets
	 */
	void registerFloatingWidget(CFloatingDockContainer* FloatingWidget);

	/**
	 * Remove the given floating widget from the list of registered floating
	 * widgets
	 */
	void removeFloatingWidget(CFloatingDockContainer* FloatingWidget);

	/**
	 * Registers the given dock container widget
	 */
	void registerDockContainer(CDockContainerWidget* DockContainer);

	/**
	 * Remove dock container from the internal list of registered dock
	 * containers
	 */
	void removeDockContainer(CDockContainerWidget* DockContainer);

	/**
	 * Overlay for containers
	 */
	CDockOverlay* containerOverlay() const;

	/**
	 * Overlay for dock areas
	 */
	CDockOverlay* dockAreaOverlay() const;


	/**
	 * A container needs to call this function if a widget has been dropped
	 * into it
	 */
	void notifyWidgetOrAreaRelocation(QWidget* RelocatedWidget);

	/**
	 * This function is called, if a floating widget has been dropped into
	 * an new position.
	 * When this function is called, all dock widgets of the FloatingWidget
	 * are already inserted into its new position
	 */
	void notifyFloatingWidgetDrop(CFloatingDockContainer* FloatingWidget);

	/**
	 * Show the floating widgets that has been created floating
	 */
	virtual void showEvent(QShowEvent *event) override;

	/**
	 * Access for the internal dock focus controller.
	 * This function only returns a valid object, if the FocusHighlighting
	 * flag is set.
	 */
	CDockFocusController* dockFocusController() const;

    /**
     * Restore floating widgets hidden by an earlier call to hideManagerAndFloatingWidgets.
     */
    void restoreHiddenFloatingWidgets();

public:
	using Super = CDockContainerWidget;

	enum eViewMenuInsertionOrder
	{
		MenuSortedByInsertion,
		MenuAlphabeticallySorted
	};

	/**
	 * These global configuration flags configure some global dock manager
	 * settings.
	 * Set the dock manager flags, before you create the dock manager instance.
	 */
	enum eConfigFlag
	{
		ActiveTabHasCloseButton = 0x0001,    //!< If this flag is set, the active tab in a tab area has a close button
		DockAreaHasCloseButton = 0x0002,     //!< If the flag is set each dock area has a close button
		DockAreaCloseButtonClosesTab = 0x0004,//!< If the flag is set, the dock area close button closes the active tab, if not set, it closes the complete dock area
		OpaqueSplitterResize = 0x0008, //!< See QSplitter::setOpaqueResize() documentation
		XmlAutoFormattingEnabled = 0x0010,//!< If enabled, the XML writer automatically adds line-breaks and indentation to empty sections between elements (ignorable whitespace).
		XmlCompressionEnabled = 0x0020,//!< If enabled, the XML output will be compressed and is not human readable anymore
		TabCloseButtonIsToolButton = 0x0040,//! If enabled the tab close buttons will be QToolButtons instead of QPushButtons - disabled by default
		AllTabsHaveCloseButton = 0x0080, //!< if this flag is set, then all tabs that are closable show a close button
		RetainTabSizeWhenCloseButtonHidden = 0x0100, //!< if this flag is set, the space for the close button is reserved even if the close button is not visible
		DragPreviewIsDynamic = 0x0400,///< If opaque undocking is disabled, this flag defines the behavior of the drag preview window, if this flag is enabled, the preview will be adjusted dynamically to the drop area
		DragPreviewShowsContentPixmap = 0x0800,///< If opaque undocking is disabled, the created drag preview window shows a copy of the content of the dock widget / dock are that is dragged
		DragPreviewHasWindowFrame = 0x1000,///< If opaque undocking is disabled, then this flag configures if the drag preview is frameless or looks like a real window
		AlwaysShowTabs = 0x2000,///< If this option is enabled, the tab of a dock widget is always displayed - even if it is the only visible dock widget in a floating widget.
		DockAreaHasUndockButton = 0x4000,     //!< If the flag is set each dock area has an undock button
		DockAreaHasTabsMenuButton = 0x8000,     //!< If the flag is set each dock area has a tabs menu button
		DockAreaHideDisabledButtons = 0x10000,    //!< If the flag is set disabled dock area buttons will not appear on the toolbar at all (enabling them will bring them back)
		DockAreaDynamicTabsMenuButtonVisibility = 0x20000, //!< If the flag is set, the tabs menu button will be shown only when it is required - that means, if the tabs are elided. If the tabs are not elided, it is hidden
		FloatingContainerHasWidgetTitle = 0x40000, //!< If set, the Floating Widget window title reflects the title of the current dock widget otherwise it displays the title set with `CDockManager::setFloatingContainersTitle` or application name as window title
		FloatingContainerHasWidgetIcon = 0x80000, //!< If set, the Floating Widget icon reflects the icon of the current dock widget otherwise it displays application icon
		HideSingleCentralWidgetTitleBar = 0x100000, //!< If there is only one single visible dock widget in the main dock container (the dock manager) and if this flag is set, then the titlebar of this dock widget will be hidden
		                                            //!< this only makes sense for non draggable and non floatable widgets and enables the creation of some kind of "central" widget

		FocusHighlighting = 0x200000, //!< enables styling of focused dock widget tabs or floating widget titlebar
		EqualSplitOnInsertion = 0x400000, ///!< if enabled, the space is equally distributed to all widgets in a  splitter

		FloatingContainerForceNativeTitleBar = 0x800000, //!< Linux only ! Forces all FloatingContainer to use the native title bar. This might break docking for FloatinContainer on some Window Managers (like Kwin/KDE).
														 //!< If neither this nor FloatingContainerForceCustomTitleBar is set (the default) native titlebars are used except on known bad systems.
														 //! Users can overwrite this by setting the environment variable ADS_UseNativeTitle to "1" or "0".
        FloatingContainerForceQWidgetTitleBar = 0x1000000,//!< Linux only ! Forces all FloatingContainer to use a QWidget based title bar.
														 //!< If neither this nor FloatingContainerForceNativeTitleBar is set (the default) native titlebars are used except on known bad systems.
														 //! Users can overwrite this by setting the environment variable ADS_UseNativeTitle to "1" or "0".
		MiddleMouseButtonClosesTab = 0x2000000, //! If the flag is set, the user can use the mouse middle button to close the tab under the mouse
		DisableTabTextEliding =      0x4000000, //! Set this flag to disable eliding of tab texts in dock area tabs
		ShowTabTextOnlyForActiveTab =0x8000000, //! Set this flag to show label texts in dock area tabs only for active tabs
		DoubleClickUndocksWidget = 0x10000000, //!< If the flag is set, a double click on a tab undocks the widget
		TabsAtBottom = 0x20000000, //!< If the flag is set, tabs will be shown at the bottom instead of in the title bar.


        DefaultDockAreaButtons = DockAreaHasCloseButton
							   | DockAreaHasUndockButton
		                       | DockAreaHasTabsMenuButton, ///< default configuration of dock area title bar buttons

		DefaultBaseConfig = DefaultDockAreaButtons
		                  | ActiveTabHasCloseButton
		                  | XmlCompressionEnabled
		                  | FloatingContainerHasWidgetTitle
		                  | DoubleClickUndocksWidget, ///< default base configuration settings

        DefaultOpaqueConfig = DefaultBaseConfig
		                    | OpaqueSplitterResize
		                    | DragPreviewShowsContentPixmap, ///< the default configuration for non opaque operations

		DefaultNonOpaqueConfig = DefaultBaseConfig
		              | DragPreviewShowsContentPixmap, ///< the default configuration for non opaque operations

		NonOpaqueWithWindowFrame = DefaultNonOpaqueConfig
		              | DragPreviewHasWindowFrame ///< the default configuration for non opaque operations that show a real window with frame
	};
	Q_DECLARE_FLAGS(ConfigFlags, eConfigFlag)


	/**
	 * These global configuration flags configure some dock manager auto hide
	 * settings
	 * Set the dock manager flags, before you create the dock manager instance.
	 */
    enum eAutoHideFlag
	{
		AutoHideFeatureEnabled = 0x01, //!< enables / disables auto hide feature
		DockAreaHasAutoHideButton = 0x02,     //!< If the flag is set each dock area has a auto hide menu button
		AutoHideButtonTogglesArea = 0x04, //!< If the flag is set, the auto hide button enables auto hiding for all dock widgets in an area, if disabled, only the current dock widget will be toggled
		AutoHideButtonCheckable = 0x08, //!< If the flag is set, the auto hide button will be checked and unchecked depending on the auto hide state. Mainly for styling purposes.
		AutoHideSideBarsIconOnly = 0x10,///< show only icons in auto hide side tab - if a tab has no icon, then the text will be shown
		AutoHideShowOnMouseOver = 0x20, ///< show the auto hide window on mouse over tab and hide it if mouse leaves auto hide container
		AutoHideCloseButtonCollapsesDock = 0x40, ///< Close button of an auto hide container collapses the dock instead of hiding it completely
		AutoHideHasCloseButton = 0x80, //< If the flag is set an auto hide title bar has a close button
		AutoHideHasMinimizeButton = 0x100, ///< if this flag is set, the auto hide title bar has a minimize button to collapse the dock widget
        AutoHideOpenOnDragHover = 0x200,  ///< if this flag is set, dragging hover the tab bar will open the dock
        AutoHideCloseOnOutsideMouseClick = 0x400, ///< if this flag is set, the auto hide dock container will collapse if the user clicks outside of the container, if not set, the auto hide container can be closed only via click on sidebar tab

		DefaultAutoHideConfig = AutoHideFeatureEnabled
			                  | DockAreaHasAutoHideButton
			                  | AutoHideHasMinimizeButton
			                  | AutoHideCloseOnOutsideMouseClick

	};
    Q_DECLARE_FLAGS(AutoHideFlags, eAutoHideFlag)

	/**
	 * Global configuration parameters that you can set via setConfigParam()
	 */
	enum eConfigParam
	{
    	AutoHideOpenOnDragHoverDelay_ms, ///< Delay in ms before the dock opens on drag hover if AutoHideOpenOnDragHover flag is set
    	ConfigParamCount // just a delimiter to count number of config params
	};


	/**
	 * Default Constructor.
	 * If the given parent is a QMainWindow, the dock manager sets itself as the
	 * central widget.
	 * Before you create any dock widgets, you should properly setup the
	 * configuration flags via setConfigFlags().
	 */
	CDockManager(QWidget* parent = nullptr);

	/**
	 * Virtual Destructor
	 */
	virtual ~CDockManager() override;

    /**
     * Creates a new dock widget with the specified title and optional parent
     * widget.
     *
     * The new dock widget will be managed by the dock manager, and its lifetime
     * will be tied to the dock manager. If a parent widget is provided, the dock
     * widget will be created as a child of the parent widget. If no parent widget
     * is provided, the dock widget will be created as a top-level widget.
     *
     * @param title The title of the dock widget.
     * @param parent The parent widget, if any. Defaults to nullptr.
     * @return Returns a pointer to the created CDockWidget.
     */
    CDockWidget *createDockWidget(const QString &title, QWidget* parent = nullptr);

	/**
	 * Returns the dock manager specific factory for creating components of
	 * fock widgets
	 */
    QSharedPointer<ads::CDockComponentsFactory> componentsFactory() const;

    /**
     * Sets a custom factory for creating components of dock widgets.
     * The pointer is stored internally into a shared pointer so you should not
     * delete the given factory object as long as it is used by the dock manager.
     */
    void setComponentsFactory(ads::CDockComponentsFactory* Factory);

    /**
     * Sets a custom factory for creating components of dock widgets.
     */
    void setComponentsFactory(QSharedPointer<ads::CDockComponentsFactory>);

	/**
	 * This function returns the global configuration flags
	 */
	static ConfigFlags configFlags();

	/**
	 * This function returns the auto hide configuration flags
	 */
	static AutoHideFlags autoHideConfigFlags();

	/**
	 * Sets the global configuration flags for the whole docking system.
	 * Call this function before you create the dock manager and before
	 * your create the first dock widget.
	 */
	static void setConfigFlags(const ConfigFlags Flags);

	/**
	 * Sets the global configuration flags for the whole docking system.
	 * Call this function before you create the dock manager and before
	 * your create the first dock widget.
	 */
	static void setAutoHideConfigFlags(const AutoHideFlags Flags);

	/**
	 * Set a certain config flag.
	 * \see setConfigFlags()
	 */
	static void setConfigFlag(eConfigFlag Flag, bool On = true);

	/**
	 * Set a certain overlay config flag.
	 * \see setConfigFlags()
	 */
	static void setAutoHideConfigFlag(eAutoHideFlag Flag, bool On = true);

	/**
	 * Returns true if the given config flag is set
	 */
	static bool testConfigFlag(eConfigFlag Flag);

	/**
	 * Returns true if the given overlay config flag is set
	 */
	static bool testAutoHideConfigFlag(eAutoHideFlag Flag);

	/**
	 * Sets the value for the given config parameter
	 */
	static void setConfigParam(eConfigParam Param, QVariant Value);

	/**
	 * Returns the value for the given config parameter or the default value
	 * if the parameter is not set.
	 */
	static QVariant configParam(eConfigParam Param, QVariant Default);

	/**
	 * Returns the global icon provider.
	 * The icon provider enables the use of custom icons in case using
	 * styleheets for icons is not an option.
	 */
	static CIconProvider& iconProvider();

	/**
	 * Adds dockwidget into the given area.
	 * If DockAreaWidget is not null, then the area parameter indicates the area
	 * into the DockAreaWidget. If DockAreaWidget is null, the Dockwidget will
	 * be dropped into the container. If you would like to add a dock widget
	 * tabified, then you need to add it to an existing dock area object
	 * into the CenterDockWidgetArea. The following code shows this:
	 * \code
	 * DockManager->addDockWidget(ads::CenterDockWidgetArea, NewDockWidget,
	 * 	   ExisitingDockArea);
	 * \endcode
	 * \return Returns the dock area widget that contains the new DockWidget
	 */
	CDockAreaWidget* addDockWidget(DockWidgetArea area, CDockWidget* Dockwidget,
		CDockAreaWidget* DockAreaWidget = nullptr, int Index = -1);

	/**
	 * Adds dockwidget into the given container.
	 * This allows you to place the dock widget into a container, even if that
	 * container does not yet contain a DockAreaWidget.
	 * \return Returns the dock area widget that contains the new DockWidget
	 */
	CDockAreaWidget* addDockWidgetToContainer(DockWidgetArea area, CDockWidget* Dockwidget,
		CDockContainerWidget* DockContainerWidget);

	/**
	 * Adds an Auto-Hide widget to the dock manager container pinned to
	 * the given side bar location.
	 * \return Returns the CAutoHideDockContainer that contains the new DockWidget
	 */
	CAutoHideDockContainer* addAutoHideDockWidget(SideBarLocation Location, CDockWidget* Dockwidget);

	/**
	 * Adds an Auto-Hide widget to the given DockContainerWidget pinned to
	 * the given side bar location in this container.
	 * \return Returns the CAutoHideDockContainer that contains the new DockWidget
	 */
	CAutoHideDockContainer* addAutoHideDockWidgetToContainer(SideBarLocation Location,
		CDockWidget* Dockwidget, CDockContainerWidget* DockContainerWidget);

	/**
	 * This function will add the given Dockwidget to the given dock area as
	 * a new tab.
	 * If no dock area widget exists for the given area identifier, a new
	 * dock area widget is created.
	 */
	CDockAreaWidget* addDockWidgetTab(DockWidgetArea area,
		CDockWidget* Dockwidget);

	/**
	 * This function will add the given Dockwidget to the given DockAreaWidget
	 * as a new tab.
	 * If index is out of range, the tab is simply appended. Otherwise it is
	 * inserted at the specified position.
	 */
	CDockAreaWidget* addDockWidgetTabToArea(CDockWidget* Dockwidget,
		CDockAreaWidget* DockAreaWidget, int Index = -1);

	/**
	 * Adds the given DockWidget floating and returns the created
	 * CFloatingDockContainer instance.
	 */
	CFloatingDockContainer* addDockWidgetFloating(CDockWidget* Dockwidget);

	/**
	 * Searches for a registered doc widget with the given ObjectName
	 * \return Return the found dock widget or nullptr if a dock widget with the
	 * given name is not registered
	 */
	CDockWidget* findDockWidget(const QString& ObjectName) const;

	/**
	 * Remove the given Dock from the dock manager
	 */
	void removeDockWidget(CDockWidget* Dockwidget);

	/**
	 * This function returns a readable reference to the internal dock
	 * widgets map so that it is possible to iterate over all dock widgets
	 */
	QMap<QString, CDockWidget*> dockWidgetsMap() const;

	/**
	 * Returns the list of all active and visible dock containers
	 * Dock containers are the main dock manager and all floating widgets
	 */
	const QList<CDockContainerWidget*> dockContainers() const;

	/**
	 * Returns the list of all floating widgets
	 */
	const QList<CFloatingDockContainer*> floatingWidgets() const;

	/**
	 * This function always return 0 because the main window is always behind
	 * any floating widget
	 */
	unsigned int zOrderIndex() const override;

	/**
	 * Saves the current state of the dockmanger and all its dock widgets
	 * into the returned QByteArray.
	 * The XmlMode enables / disables the auto formatting for the XmlStreamWriter.
	 * If auto formatting is enabled, the output is intended and line wrapped.
	 * The XmlMode XmlAutoFormattingDisabled is better if you would like to have
	 * a more compact XML output - i.e. for storage in ini files.
	 * The version number is stored as part of the data.
	 * To restore the saved state, pass the return value and version number
	 * to restoreState().
	 * \see restoreState()
	 */
	QByteArray saveState(int version = 0) const;

	/**
	 * Restores the state of this dockmanagers dockwidgets.
	 * The version number is compared with that stored in state. If they do
	 * not match, the dockmanager's state is left unchanged, and this function
	 * returns false; otherwise, the state is restored, and this function
	 * returns true.
	 * \see saveState()
	 */
	bool restoreState(const QByteArray &state, int version = 0);

	/**
	 * Saves the current perspective to the internal list of perspectives.
	 * A perspective is the current state of the dock manager assigned
	 * with a certain name. This makes it possible for the user,
	 * to switch between different perspectives quickly.
	 * If a perspective with the given name already exists, then
	 * it will be overwritten with the new state.
	 */
	void addPerspective(const QString& UniquePrespectiveName);

	/**
	 * Removes the perspective with the given name from the list of perspectives
	 */
	void removePerspective(const QString& Name);

	/**
	 * Removes the given perspectives from the dock manager
	 */
	void removePerspectives(const QStringList& Names);

	/**
	 * Returns the names of all available perspectives
	 */
	QStringList perspectiveNames() const;

	/**
	 * Saves the perspectives to the given settings file.
	 */
	void savePerspectives(QSettings& Settings) const;

	/**
	 * Loads the perspectives from the given settings file
	 */
	void loadPerspectives(QSettings& Settings);

    /**
     * This function returns managers central widget or nullptr if no central widget is set.
     */
    CDockWidget* centralWidget() const;

    /**
     * Adds dockwidget widget into the central area and marks it as central widget.
     * If central widget is set, it will be the only dock widget
     * that will resize with the dock container. A central widget if not
     * movable, floatable or closable and the titlebar of the central
     * dock area is not visible.
     * If the given widget could be set as central widget, the function returns
     * the created dock area. If the widget could not be set, because there
     * is already a central widget, this function returns a nullptr.
     * To clear the central widget, pass a nullptr to the function.
     * \note Setting a central widget is only possible if no other dock widgets
     * have been registered before. That means, this function should be the
     * first function that you call before you add other dock widgets.
     * \retval != 0 The dock area that contains the central widget
     * \retval nullptr Indicates that the given widget can not be set as central
     *         widget because there is already a central widget.
     */
    CDockAreaWidget* setCentralWidget(CDockWidget* widget);

    /**
	 * Adds a toggle view action to the the internal view menu.
	 * You can either manage the insertion of the toggle view actions in your
	 * application or you can add the actions to the internal view menu and
	 * then simply insert the menu object into your.
	 * \param[in] ToggleViewAction The action to insert. If no group is provided
	 *            the action is directly inserted into the menu. If a group
	 *            is provided, the action is inserted into the group and the
	 *            group is inserted into the menu if it is not existing yet.
	 * \param[in] Group This is the text used for the group menu item
	 * \param[in] GroupIcon The icon used for grouping the workbenches in the
	 *            view menu. I.e. if there is a workbench for each device
	 *            like for spectrometer devices, it is good to group all these
	 *            workbenches under a menu item
	 * \return If Group is not empty, this function returns the GroupAction
	 *         for this group. If the group is empty, the function returns
	 *         the given ToggleViewAction.
	 */
	QAction* addToggleViewActionToMenu(QAction* ToggleViewAction,
		const QString& Group = QString(), const QIcon& GroupIcon = QIcon());

	/**
	 * This function returns the internal view menu.
	 * To fill the view menu, you can use the addToggleViewActionToMenu()
	 * function.
	 */
	QMenu* viewMenu() const;

	/**
	 * Define the insertion order for toggle view menu items.
	 * The order defines how the actions are added to the view menu.
	 * The default insertion order is MenuAlphabeticallySorted to make it
	 * easier for users to find the menu entry for a certain dock widget.
	 * You need to call this function before you insert the first menu item
	 * into the view menu.
	 */
	void setViewMenuInsertionOrder(eViewMenuInsertionOrder Order);

	/**
	 * This function returns true between the restoringState() and
	 * stateRestored() signals.
	 */
	bool isRestoringState() const;

	/**
	 * This function returns true, if the DockManager window is restoring from
	 * minimized state.
	 * The DockManager is in this state starting from the QWindowStateChangeEvent
	 * that signals the state change from minimized to normal until
	 * endLeavingMinimizedState() function is called.
	 */
	bool isLeavingMinimizedState() const;

	/**
	 * The distance the user needs to move the mouse with the left button
	 * hold down before a dock widget start floating
	 */
	static int startDragDistance();

	/**
	 * Helper function to set focus depending on the configuration of the
	 * FocusStyling flag
	 */
	template <class QWidgetPtr>
	static void setWidgetFocus(QWidgetPtr widget)
	{
		if (!CDockManager::testConfigFlag(CDockManager::FocusHighlighting))
		{
			return;
		}

		widget->setFocus(Qt::OtherFocusReason);
	}

	bool eventFilter(QObject *obj, QEvent *e) override;

	/**
	 * Returns the dock widget that has focus style in the ui or a nullptr if
	 * not dock widget is painted focused.
	 * If the flag FocusHighlighting is disabled, this function always returns
	 * nullptr.
	 */
	CDockWidget* focusedDockWidget() const;

    /**
     * Returns the sizes of the splitter that contains the dock area.
     *
     * If there is no splitter that contains the area, an empty list will be
     * returned.
     */
    QList<int> splitterSizes(CDockAreaWidget *ContainedArea) const;

    /**
     * Update the sizes of a splitter
     * Programmatically updates the sizes of a given splitter by calling
     * QSplitter::setSizes(). The splitter will be the splitter that
     * contains the supplied dock area widget. If there is not splitter
     * that contains the dock area, or the sizes supplied does not match
     * the number of children of the splitter, this method will have no
     * effect.
     */
    void setSplitterSizes(CDockAreaWidget *ContainedArea, const QList<int>& sizes);

	/**
	 * Set a custom title for all FloatingContainer that does not reflect
	 * the title of the current dock widget.
	 */
	static void setFloatingContainersTitle(const QString& Title);

	/**
	 * Returns the title used by all FloatingContainer that does not
	 * reflect the title of the current dock widget.
	 *
	 * If not title was set with setFloatingContainersTitle(), it returns
	 * QGuiApplication::applicationDisplayName().
	 */
	static QString floatingContainersTitle();

    /**
     * This function sets the tool button style for the given dock widget state.
     * It is possible to switch the tool button style depending on the state.
     * If a dock widget is floating, then here are more space and it is
     * possible to select a style that requires more space like
     * Qt::ToolButtonTextUnderIcon. For the docked state Qt::ToolButtonIconOnly
     * might be better.
     */
    void setDockWidgetToolBarStyle(Qt::ToolButtonStyle Style, CDockWidget::eState State);

    /**
     * Returns the tool button style for the given docking state.
     * \see setToolBarStyle()
     */
    Qt::ToolButtonStyle dockWidgetToolBarStyle(CDockWidget::eState State) const;

    /**
     * This function sets the tool button icon size for the given state.
     * If a dock widget is floating, there is more space and increasing the
     * icon size is possible. For docked widgets, small icon sizes, eg. 16 x 16
     * might be better.
     */
    void setDockWidgetToolBarIconSize(const QSize& IconSize, CDockWidget::eState State);

    /**
     * Returns the icon size for a given docking state.
     * \see setToolBarIconSize()
     */
    QSize dockWidgetToolBarIconSize(CDockWidget::eState State) const;

    /**
     * Returns all dock widget features that are globally locked by the dock
     * manager.
     * Globally locked features are removed from the features of all dock
     * widgets.
     */
    CDockWidget::DockWidgetFeatures globallyLockedDockWidgetFeatures() const;

    /**
     * Globally Lock features of all dock widgets to "freeze" the current
     * workspace layout.
     * For example, it is now possible to lock the workspace to avoid
     * accidentally dragging a docked view. Locking wasn’t possible before.
     * So, users had to manually dock it back to the desired place after
     * each accidental undock.
     * You can use a combination of the following feature flags:
     * - CDockWidget::DockWidgetClosable
     * - CDockWidget::DockWidgetMovable
     * - CDockWidget::DockWidgetFloatable
     * - CDockWidget::DockWidgetPinable
     *
     * To clear the locked features, you can use CDockWidget::NoDockWidgetFeatures
     * The following code shows how to lock and unlock dock widget features
     * globally.
     *
     * \code
     * DockManager->lockDockWidgetFeaturesGlobally();
     * DockManager->lockDockWidgetFeaturesGlobally(CDockWidget::NoDockWidgetFeatures);
     * \code
     */
    void lockDockWidgetFeaturesGlobally(CDockWidget::DockWidgetFeatures Features = CDockWidget::GloballyLockableFeatures);

public Q_SLOTS:
	/**
	 * Opens the perspective with the given name.
	 */
	void openPerspective(const QString& PerspectiveName);

	/**
	 * Request a focus change to the given dock widget.
	 * This function only has an effect, if the flag CDockManager::FocusStyling
	 * is enabled
	 */
	void setDockWidgetFocused(CDockWidget* DockWidget);

    /**
     * hide CDockManager and all floating widgets (See Issue #380). Calling regular QWidget::hide()
     * hides the CDockManager but not the floating widgets;
     */
    void hideManagerAndFloatingWidgets();

    /**
     * Calls raise() for the widget that hosts this dock manager.
     * This will bring the widget in front of any other application that is running
     */
    void raise();

Q_SIGNALS:
	/**
	 * This signal is emitted if the list of perspectives changed.
	 * The list of perspectives changes if perspectives are added, removed
	 * or if the perspective list has been loaded
	 */
	void perspectiveListChanged();

	/**
	 * This signal is emitted if the perspective list has been loaded
	 */
	void perspectiveListLoaded();

	/**
	 * This signal is emitted if perspectives have been removed
	 */
	void perspectivesRemoved();

	/**
	 * This signal is emitted, if the restore function is called, just before
	 * the dock manager starts restoring the state.
	 * If this function is called, nothing has changed yet
	 */
	void restoringState();

    /**
     * This signal is emitted if the state changed in restoreState.
     * The signal is emitted if the restoreState() function is called or
     * if the openPerspective() function is called
     */
    void stateRestored();

    /**
     * This signal is emitted, if the dock manager starts opening a
     * perspective.
     * Opening a perspective may take more than a second if there are
     * many complex widgets. The application may use this signal
     * to show some progress indicator or to change the mouse cursor
     * into a busy cursor.
     */
    void openingPerspective(const QString& PerspectiveName);

    /**
     * This signal is emitted if the dock manager finished opening a
     * perspective.
     */
    void perspectiveOpened(const QString& PerspectiveName);

	/**
	 * This signal is emitted, if a new floating widget has been created.
	 * An application can use this signal to e.g. subscribe to events of
	 * the newly created window.
	 */
	void floatingWidgetCreated(ads::CFloatingDockContainer* FloatingWidget);

    /**
     * This signal is emitted, if a new DockArea has been created.
     * An application can use this signal to set custom icons or custom
     * tooltips for the DockArea buttons.
     */
    void dockAreaCreated(ads::CDockAreaWidget* DockArea);

    /**
     * This signal is emitted if a dock widget has been added to this
     * dock manager instance.
     */
    void dockWidgetAdded(ads::CDockWidget* DockWidget);

    /**
     * This signal is emitted just before the given dock widget is removed
     * from the dock manager
     */
    void dockWidgetAboutToBeRemoved(ads::CDockWidget* DockWidget);

    /**
     * This signal is emitted if a dock widget has been removed with the remove
     * removeDockWidget() function.
     * If this signal is emitted, the dock widget has been removed from the
     * docking system but it is not deleted yet.
     */
    void dockWidgetRemoved(ads::CDockWidget* DockWidget);

    /**
     * This signal is emitted if the focused dock widget changed.
     * Both old and now can be nullptr.
     * The focused dock widget is the one that is highlighted in the GUI
     */
    void focusedDockWidgetChanged(ads::CDockWidget* old, ads::CDockWidget* now);
}; // class DockManager
} // namespace ads

Q_DECLARE_OPERATORS_FOR_FLAGS(ads::CDockManager::ConfigFlags)
//-----------------------------------------------------------------------------
#endif // DockManagerH
