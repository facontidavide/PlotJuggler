//============================================================================
/// \file   DockFocusController.cpp
/// \author Uwe Kindler
/// \date   05.06.2020
/// \brief  Implementation of CDockFocusController class
//============================================================================

//============================================================================
//                                   INCLUDES
//============================================================================
#include <AutoHideTab.h>
#include "DockFocusController.h"

#include <algorithm>
#include <iostream>

#include <QPointer>
#include <QApplication>
#include <QAbstractButton>
#include <QWindow>

#include "DockWidget.h"
#include "DockAreaWidget.h"
#include "DockWidgetTab.h"
#include "DockContainerWidget.h"
#include "FloatingDockContainer.h"
#include "DockManager.h"
#include "DockAreaTitleBar.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include "linux/FloatingWidgetTitleBar.h"
#endif

namespace ads
{
static const char* const FocusedDockWidgetProperty = "FocusedDockWidget";

/**
 * Private data class of CDockFocusController class (pimpl)
 */
struct DockFocusControllerPrivate
{
	CDockFocusController *_this;
	QPointer<CDockWidget> FocusedDockWidget = nullptr;
	QPointer<CDockAreaWidget> FocusedArea = nullptr;
	QPointer<CDockWidget> OldFocusedDockWidget = nullptr;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
     QPointer<CFloatingDockContainer> FloatingWidget = nullptr;
#endif
	CDockManager* DockManager;
    bool ForceFocusChangedSignal = false;
    bool TabPressed = false;

	/**
	 * Private data constructor
	 */
	DockFocusControllerPrivate(CDockFocusController *_public);

	/**
	 * This function updates the focus style of the given dock widget and
	 * the dock area that it belongs to
	 */
	void updateDockWidgetFocus(CDockWidget* DockWidget);
}; // struct DockFocusControllerPrivate



//===========================================================================
static void updateDockWidgetFocusStyle(CDockWidget* DockWidget, bool Focused)
{
	DockWidget->setProperty("focused", Focused);
	DockWidget->tabWidget()->setProperty("focused", Focused);
	DockWidget->tabWidget()->updateStyle();
	internal::repolishStyle(DockWidget);
}


//===========================================================================
static void updateDockAreaFocusStyle(CDockAreaWidget* DockArea, bool Focused)
{
	DockArea->setProperty("focused", Focused);
	internal::repolishStyle(DockArea);
	internal::repolishStyle(DockArea->titleBar());
}


//===========================================================================
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
static void updateFloatingWidgetFocusStyle(CFloatingDockContainer* FloatingWidget, bool Focused)
{
	if (FloatingWidget->hasNativeTitleBar())
	{
		return;
	}
    auto TitleBar = qobject_cast<CFloatingWidgetTitleBar*>(FloatingWidget->titleBarWidget());
    if (!TitleBar)
    {
        return;
    }
    TitleBar->setProperty("focused", Focused);
    TitleBar->updateStyle();
}
#endif


//============================================================================
DockFocusControllerPrivate::DockFocusControllerPrivate(
    CDockFocusController *_public) :
	_this(_public)
{

}


//============================================================================
void DockFocusControllerPrivate::updateDockWidgetFocus(CDockWidget* DockWidget)
{
	if (!DockWidget) return;
	if (!DockWidget->features().testFlag(CDockWidget::DockWidgetFocusable))
	{
		return;
	}

	QWindow* Window = nullptr;
	auto DockContainer = DockWidget->dockContainer();
	if (DockContainer)
	{
		Window = DockContainer->window()->windowHandle();
	}

	if (Window)
	{
        Window->setProperty(FocusedDockWidgetProperty, QVariant::fromValue(QPointer<CDockWidget>(DockWidget)));
	}
	CDockAreaWidget* NewFocusedDockArea = nullptr;
	if (FocusedDockWidget)
	{
		updateDockWidgetFocusStyle(FocusedDockWidget, false);
	}

	CDockWidget* old = FocusedDockWidget;
	FocusedDockWidget = DockWidget;
	updateDockWidgetFocusStyle(FocusedDockWidget, true);
	NewFocusedDockArea = FocusedDockWidget->dockAreaWidget();
	if (NewFocusedDockArea && (FocusedArea != NewFocusedDockArea))
	{
		if (FocusedArea)
		{
			QObject::disconnect(FocusedArea, SIGNAL(viewToggled(bool)), _this, SLOT(onFocusedDockAreaViewToggled(bool)));
			updateDockAreaFocusStyle(FocusedArea, false);
		}

		FocusedArea = NewFocusedDockArea;
		updateDockAreaFocusStyle(FocusedArea, true);
		QObject::connect(FocusedArea, SIGNAL(viewToggled(bool)), _this, SLOT(onFocusedDockAreaViewToggled(bool)));
	}



    CFloatingDockContainer* NewFloatingWidget = nullptr;
    DockContainer = FocusedDockWidget->dockContainer();
    if (DockContainer)
    {
    	NewFloatingWidget = DockContainer->floatingWidget();
    }

    if (NewFloatingWidget)
    {
        NewFloatingWidget->setProperty(FocusedDockWidgetProperty, QVariant::fromValue(QPointer<CDockWidget>(DockWidget)));
    }


#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
	// This code is required for styling the floating widget titlebar for linux
	// depending on the current focus state
	if (FloatingWidget != NewFloatingWidget)
	{
		if (FloatingWidget)
		{
			updateFloatingWidgetFocusStyle(FloatingWidget, false);
		}
		FloatingWidget = NewFloatingWidget;

		if (FloatingWidget)
		{
			updateFloatingWidgetFocusStyle(FloatingWidget, true);
		}
	}
#endif

	if (old == DockWidget && !ForceFocusChangedSignal)
    {
    	return;
    }

    ForceFocusChangedSignal = false;
    if (DockWidget->isVisible())
    {
    	Q_EMIT DockManager->focusedDockWidgetChanged(old, DockWidget);
    }
    else
    {
    	OldFocusedDockWidget = old;
    	QObject::connect(DockWidget, SIGNAL(visibilityChanged(bool)), _this, SLOT(onDockWidgetVisibilityChanged(bool)));
    }
}



//============================================================================
void CDockFocusController::onDockWidgetVisibilityChanged(bool Visible)
{
    auto Sender = sender();
    auto DockWidget = qobject_cast<ads::CDockWidget*>(Sender);
    disconnect(Sender, SIGNAL(visibilityChanged(bool)), this, SLOT(onDockWidgetVisibilityChanged(bool)));
    if (DockWidget && Visible)
	{
        Q_EMIT d->DockManager->focusedDockWidgetChanged(d->OldFocusedDockWidget, DockWidget);
	}
}


//============================================================================
CDockFocusController::CDockFocusController(CDockManager* DockManager) :
	Super(DockManager),
	d(new DockFocusControllerPrivate(this))
{
	d->DockManager = DockManager;
	connect(QApplication::instance(), SIGNAL(focusChanged(QWidget*, QWidget*)),
			this, SLOT(onApplicationFocusChanged(QWidget*, QWidget*)));
	connect(QApplication::instance(), SIGNAL(focusWindowChanged(QWindow*)),
			this, SLOT(onFocusWindowChanged(QWindow*)));
	connect(d->DockManager, SIGNAL(stateRestored()), SLOT(onStateRestored()));
}

//============================================================================
CDockFocusController::~CDockFocusController()
{
	delete d;
}


//============================================================================
void CDockFocusController::onFocusWindowChanged(QWindow *focusWindow)
{
	if (!focusWindow)
	{
		return;
	}

    auto vDockWidget = focusWindow->property(FocusedDockWidgetProperty);
	if (!vDockWidget.isValid())
	{
		return;
	}

    auto DockWidget = vDockWidget.value<QPointer<CDockWidget>>();
	if (!DockWidget)
	{
		return;
	}

	d->updateDockWidgetFocus(DockWidget);
}


//===========================================================================
void CDockFocusController::onApplicationFocusChanged(QWidget* focusedOld, QWidget* focusedNow)
{
    Q_UNUSED(focusedOld);

    // Ignore focus changes if we are restoring state, or if user clicked
    // a tab which in turn caused the focus change
	if (d->DockManager->isRestoringState() || d->TabPressed)
	{
		return;
	}

    ADS_PRINT("CDockFocusController::onApplicationFocusChanged "
    	<< " old: " << focusedOld << " new: " << focusedNow);
	if (!focusedNow)
	{
		return;
	}

    CDockWidget* DockWidget = qobject_cast<CDockWidget*>(focusedNow);
	if (!DockWidget)
	{
		DockWidget = internal::findParent<CDockWidget*>(focusedNow);
	}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    if (!DockWidget)
	{
		return;
	}
#else
    if (!DockWidget || DockWidget->tabWidget()->isHidden())
	{
    	return;
	}
#endif

	d->updateDockWidgetFocus(DockWidget);
}


//===========================================================================
void CDockFocusController::setDockWidgetTabFocused(CDockWidgetTab* Tab)
{
	auto DockWidget = Tab->dockWidget();
	if (DockWidget)
	{
		d->updateDockWidgetFocus(DockWidget);
	}
}


//===========================================================================
void CDockFocusController::clearDockWidgetFocus(CDockWidget* dockWidget)
{
	dockWidget->clearFocus();
	updateDockWidgetFocusStyle(dockWidget, false);
}


//===========================================================================
void CDockFocusController::setDockWidgetFocused(CDockWidget* focusedNow)
{
	d->updateDockWidgetFocus(focusedNow);
}


//===========================================================================
void CDockFocusController::onFocusedDockAreaViewToggled(bool Open)
{
	if (d->DockManager->isRestoringState())
	{
		return;
	}

	CDockAreaWidget* DockArea = qobject_cast<CDockAreaWidget*>(sender());
	if (!DockArea || Open)
	{
		return;
	}
	auto Container = DockArea->dockContainer();
	auto OpenedDockAreas = Container->openedDockAreas();
	if (OpenedDockAreas.isEmpty())
	{
		return;
	}

	d->updateDockWidgetFocus(OpenedDockAreas[0]->currentDockWidget());
}


//===========================================================================
void CDockFocusController::notifyWidgetOrAreaRelocation(QWidget* DroppedWidget)
{
	if (d->DockManager->isRestoringState())
	{
		return;
	}

	CDockWidget* DockWidget = qobject_cast<CDockWidget*>(DroppedWidget);
    if (!DockWidget)
    {
        CDockAreaWidget* DockArea = qobject_cast<CDockAreaWidget*>(DroppedWidget);
        if (DockArea)
        {
            DockWidget = DockArea->currentDockWidget();
        }
    }

    if (!DockWidget)
    {
        return;
    }

    d->ForceFocusChangedSignal = true;
    CDockManager::setWidgetFocus(DockWidget);
}


//===========================================================================
void CDockFocusController::notifyFloatingWidgetDrop(CFloatingDockContainer* FloatingWidget)
{
	if (!FloatingWidget || d->DockManager->isRestoringState())
	{
		return;
	}

    auto vDockWidget = FloatingWidget->property(FocusedDockWidgetProperty);
	if (!vDockWidget.isValid())
	{
		return;
	}

    auto DockWidget = vDockWidget.value<QPointer<CDockWidget>>();
	if (DockWidget)
	{
		DockWidget->dockAreaWidget()->setCurrentDockWidget(DockWidget);
		CDockManager::setWidgetFocus(DockWidget);
	}
}


//==========================================================================
void CDockFocusController::onStateRestored()
{
	if (d->FocusedDockWidget)
	{
		updateDockWidgetFocusStyle(d->FocusedDockWidget, false);
	}
}


//==========================================================================
CDockWidget* CDockFocusController::focusedDockWidget() const
{
	return d->FocusedDockWidget.data();
}


//==========================================================================
CDockAreaWidget* CDockFocusController::focusedDockArea() const
{
    return d->FocusedArea.data();
}


//==========================================================================
void CDockFocusController::setDockWidgetTabPressed(bool Value)
{
	d->TabPressed = Value;
}
} // namespace ads

//---------------------------------------------------------------------------
// EOF DockFocusController.cpp
