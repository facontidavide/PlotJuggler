#ifndef DockFocusControllerH
#define DockFocusControllerH
//============================================================================
/// \file   DockFocusController.h
/// \author Uwe Kindler
/// \date   05.06.2020
/// \brief  Declaration of CDockFocusController class
//============================================================================

//============================================================================
//                                   INCLUDES
//============================================================================
#include <QObject>
#include "ads_globals.h"
#include "DockManager.h"

namespace ads
{
struct DockFocusControllerPrivate;
class CDockManager;
class CFloatingDockContainer;

/**
 * Manages focus styling of dock widgets and handling of focus changes
 */
class ADS_EXPORT CDockFocusController : public QObject
{
	Q_OBJECT
private:
	DockFocusControllerPrivate* d; ///< private data (pimpl)
    friend struct DockFocusControllerPrivate;

private Q_SLOTS:
	void onApplicationFocusChanged(QWidget *old, QWidget *now);
	void onFocusWindowChanged(QWindow *focusWindow);
	void onFocusedDockAreaViewToggled(bool Open);
	void onStateRestored();
	void onDockWidgetVisibilityChanged(bool Visible);

public:
	using Super = QObject;
	/**
	 * Default Constructor
	 */
	CDockFocusController(CDockManager* DockManager);

	/**
	 * Virtual Destructor
	 */
	virtual ~CDockFocusController();

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
	 * Returns the dock widget that has focus style in the ui or a nullptr if
	 * not dock widget is painted focused.
	 */
	CDockWidget* focusedDockWidget() const;

    /**
     * Returns the dock area that contains the focusedDockWidget() or nullptr if
     * the focused dock widget is not in this area.
     */
    CDockAreaWidget* focusedDockArea() const;

	/**
	 * Request focus highlighting for the given dock widget assigned to the tab
	 * given in Tab parameter
	 */
	void setDockWidgetTabFocused(CDockWidgetTab* Tab);

	/*
	 * Request clear focus for a dock widget
	 */
	void clearDockWidgetFocus(CDockWidget* dockWidget);

	/**
	 * Notifies the dock focus controller, that a the mouse is pressed or
	 * released
	 */
	void setDockWidgetTabPressed(bool Value);

public Q_SLOTS:
	/**
	 * Request a focus change to the given dock widget
	 */
	void setDockWidgetFocused(CDockWidget* focusedNow);
}; // class DockFocusController
}
 // namespace ads
//-----------------------------------------------------------------------------
#endif // DockFocusControllerH

