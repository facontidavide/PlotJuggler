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
/// \file   AutoHideTab.cpp
/// \author Syarif Fakhri
/// \date   05.09.2022
/// \brief  Implementation of CAutoHideTab class
//============================================================================

//============================================================================
//                                   INCLUDES
//============================================================================
#include "AutoHideTab.h"

#include <QBoxLayout>
#include <QApplication>
#include <QElapsedTimer>
#include <QMenu>
#include <QEvent>
#include <QTimer>

#include "AutoHideDockContainer.h"
#include "AutoHideSideBar.h"
#include "DockAreaWidget.h"
#include "DockManager.h"
#include "DockWidget.h"
#include "FloatingDragPreview.h"
#include "DockOverlay.h"
#include "ads_globals.h"

namespace ads
{

/**
 * Private data class of CDockWidgetTab class (pimpl)
 */
struct AutoHideTabPrivate
{
    CAutoHideTab* _this;
    CDockWidget* DockWidget = nullptr;
    CAutoHideSideBar* SideBar = nullptr;
	Qt::Orientation Orientation{Qt::Vertical};
	QElapsedTimer TimeSinceHoverMousePress;
	QTimer DragOverTimer;
	bool MousePressed = false;
	eDragState DragState = DraggingInactive;
	QPoint GlobalDragStartMousePosition;
	QPoint DragStartMousePosition;
	IFloatingWidget* FloatingWidget = nullptr;
	Qt::Orientation DragStartOrientation;

	/**
	 * Private data constructor
	 */
	AutoHideTabPrivate(CAutoHideTab* _public);

	/**
	 * Update the orientation, visibility and spacing based on the area of
	 * the side bar
	 */
	void updateOrientation();

	/**
	 * Convenience function to ease dock container access
	 */
	CDockContainerWidget* dockContainer() const
	{
		return DockWidget ? DockWidget->dockContainer() : nullptr;
	}

	/**
	 * Forward this event to the dock container
	 */
	void forwardEventToDockContainer(QEvent* event)
	{
		auto DockContainer = dockContainer();
		if (DockContainer)
		{
			DockContainer->handleAutoHideWidgetEvent(event, _this);
		}
	}

	/**
	 * Helper function to create and initialize the menu entries for
	 * the "Auto Hide Group To..." menu
	 */
	QAction* createAutoHideToAction(const QString& Title, SideBarLocation Location,
		QMenu* Menu)
	{
		auto Action = Menu->addAction(Title);
		Action->setProperty(internal::LocationProperty, Location);
		QObject::connect(Action, &QAction::triggered, _this, &CAutoHideTab::onAutoHideToActionClicked);
		Action->setEnabled(Location != _this->sideBarLocation());
		return Action;
	}

	/**
	 * Test function for current drag state
	 */
	bool isDraggingState(eDragState dragState) const
	{
		return this->DragState == dragState;
	}

	/**
	 * Saves the drag start position in global and local coordinates
	 */
	void saveDragStartMousePosition(const QPoint& GlobalPos)
	{
		GlobalDragStartMousePosition = GlobalPos;
		DragStartMousePosition = _this->mapFromGlobal(GlobalPos);
	}

	/**
	 * Starts floating of the dock widget that belongs to this title bar
	 * Returns true, if floating has been started and false if floating
	 * is not possible for any reason
	 */
	bool startFloating(eDragState DraggingState = DraggingFloatingWidget);

	template <typename T>
	IFloatingWidget* createFloatingWidget(T* Widget)
	{
		auto w = new CFloatingDragPreview(Widget);
		_this->connect(w, &CFloatingDragPreview::draggingCanceled, [this]()
		{
			DragState = DraggingInactive;
		});
		return w;
	}
}; // struct DockWidgetTabPrivate


//============================================================================
AutoHideTabPrivate::AutoHideTabPrivate(CAutoHideTab* _public) :
	_this(_public)
{

}


//============================================================================
void AutoHideTabPrivate::updateOrientation()
{
	bool IconOnly = CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideSideBarsIconOnly);
	if (IconOnly && !_this->icon().isNull())
	{
		_this->setText("");
		_this->setOrientation(Qt::Horizontal);
	}
	else
	{
		auto area = SideBar->sideBarLocation();
		_this->setOrientation((area == SideBarBottom || area == SideBarTop) ? Qt::Horizontal : Qt::Vertical);
	}
}


//============================================================================
bool AutoHideTabPrivate::startFloating(eDragState DraggingState)
{
	auto DockArea = DockWidget->dockAreaWidget();
    ADS_PRINT("isFloating " << dockContainer()->isFloating());

    ADS_PRINT("startFloating");
	DragState = DraggingState;
	IFloatingWidget* FloatingWidget = nullptr;
	FloatingWidget = createFloatingWidget(DockArea);
	auto Size = DockArea->size();
	auto StartPos = DragStartMousePosition;
	auto AutoHideContainer = DockWidget->autoHideDockContainer();
	DragStartOrientation = AutoHideContainer->orientation();
	switch (SideBar->sideBarLocation())
	{
	case SideBarLeft:
		 StartPos.rx() = AutoHideContainer->rect().left() + 10;
		 break;

	case SideBarRight:
		 StartPos.rx() = AutoHideContainer->rect().right() - 10;
		 break;

	case SideBarTop:
		 StartPos.ry() = AutoHideContainer->rect().top() + 10;
		 break;

	case SideBarBottom:
		 StartPos.ry() = AutoHideContainer->rect().bottom() - 10;
		 break;

	case SideBarNone:
		 return false;
	}
	FloatingWidget->startFloating(StartPos, Size, DraggingFloatingWidget, _this);
	auto DockManager = DockWidget->dockManager();
	auto Overlay = DockManager->containerOverlay();
	Overlay->setAllowedAreas(OuterDockAreas);
	this->FloatingWidget = FloatingWidget;
	qApp->postEvent(DockWidget, new QEvent((QEvent::Type)internal::DockedWidgetDragStartEvent));

	return true;
}



//============================================================================
void CAutoHideTab::setSideBar(CAutoHideSideBar* SideTabBar)
{
	d->SideBar = SideTabBar;
	if (d->SideBar)
	{
		d->updateOrientation();
	}
}


//============================================================================
CAutoHideSideBar* CAutoHideTab::sideBar() const
{
	return d->SideBar;
}


//============================================================================
void CAutoHideTab::removeFromSideBar()
{
	if (d->SideBar == nullptr)
	{
		return;
	}
	d->SideBar->removeTab(this);
    setSideBar(nullptr);
}

//============================================================================
CAutoHideTab::CAutoHideTab(QWidget* parent) :
	CPushButton(parent),
	d(new AutoHideTabPrivate(this))
{
	setAttribute(Qt::WA_NoMousePropagation);
	setFocusPolicy(Qt::NoFocus);
    if (CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideOpenOnDragHover)) {
        setAcceptDrops(true);
    }

    d->DragOverTimer.setInterval(CDockManager::configParam(
    	CDockManager::AutoHideOpenOnDragHoverDelay_ms, 500).toInt());
    d->DragOverTimer.setSingleShot(true);
	connect(&d->DragOverTimer, &QTimer::timeout, this, &CAutoHideTab::onDragHoverDelayExpired);
}


//============================================================================
CAutoHideTab::~CAutoHideTab()
{
	ADS_PRINT("~CDockWidgetSideTab()");
	delete d;
}


//============================================================================
void CAutoHideTab::updateStyle()
{
    internal::repolishStyle(this, internal::RepolishDirectChildren);
	update();
}


//============================================================================
SideBarLocation CAutoHideTab::sideBarLocation() const
{
    if (d->SideBar)
	{
        return d->SideBar->sideBarLocation();
	}

	return SideBarLeft;
}


//============================================================================
void CAutoHideTab::setOrientation(Qt::Orientation Orientation)
{
	d->Orientation = Orientation;
	if (orientation() == Qt::Horizontal)
	{
		setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	}
	else
	{
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	}
	CPushButton::setButtonOrientation((Qt::Horizontal == Orientation)
		? CPushButton::Horizontal : CPushButton::VerticalTopToBottom);
	updateStyle();
}


//============================================================================
Qt::Orientation CAutoHideTab::orientation() const
{
	return d->Orientation;
}


//============================================================================
bool CAutoHideTab::isActiveTab() const
{
	if (d->DockWidget && d->DockWidget->autoHideDockContainer())
	{
		return d->DockWidget->autoHideDockContainer()->isVisible();
	}

	return false;
}


//============================================================================
CDockWidget* CAutoHideTab::dockWidget() const
{
	return d->DockWidget;
}


//============================================================================
void CAutoHideTab::setDockWidget(CDockWidget* DockWidget)
{
	if (!DockWidget)
	{
		return;
	}
	d->DockWidget = DockWidget;
	setText(DockWidget->windowTitle());
	setIcon(d->DockWidget->icon());
	setToolTip(DockWidget->windowTitle());
}


//============================================================================
bool CAutoHideTab::event(QEvent* event)
{
	if (!CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideShowOnMouseOver))
	{
		return Super::event(event);
	}

	switch (event->type())
	{
	case QEvent::Enter:
	case QEvent::Leave:
		 d->forwardEventToDockContainer(event);
		 break;
	default:
		break;
	}
	return Super::event(event);
}


//============================================================================
bool CAutoHideTab::iconOnly() const
{
	return CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideSideBarsIconOnly) && !icon().isNull();
}


//============================================================================
void CAutoHideTab::contextMenuEvent(QContextMenuEvent* ev)
{
	ev->accept();
	d->saveDragStartMousePosition(ev->globalPos());

    const bool isFloatable = d->DockWidget->features().testFlag(CDockWidget::DockWidgetFloatable);
	QAction* Action;
	QMenu Menu(this);

	Action = Menu.addAction(tr("Detach"), this, SLOT(setDockWidgetFloating()));
	Action->setEnabled(isFloatable);
	auto IsPinnable = d->DockWidget->features().testFlag(CDockWidget::DockWidgetPinnable);
	Action->setEnabled(IsPinnable);

	auto menu = Menu.addMenu(tr("Pin To..."));
	menu->setEnabled(IsPinnable);
	d->createAutoHideToAction(tr("Top"), SideBarTop, menu);
	d->createAutoHideToAction(tr("Left"), SideBarLeft, menu);
	d->createAutoHideToAction(tr("Right"), SideBarRight, menu);
	d->createAutoHideToAction(tr("Bottom"), SideBarBottom, menu);

	Action = Menu.addAction(tr("Unpin (Dock)"), this, SLOT(unpinDockWidget()));
	Menu.addSeparator();
	Action = Menu.addAction(tr("Close"), this, SLOT(requestCloseDockWidget()));
	Action->setEnabled(d->DockWidget->features().testFlag(CDockWidget::DockWidgetClosable));

	Menu.exec(ev->globalPos());
}


//============================================================================
void CAutoHideTab::setDockWidgetFloating()
{
	d->DockWidget->setFloating();
}


//============================================================================
void CAutoHideTab::unpinDockWidget()
{
	d->DockWidget->setAutoHide(false);
}


//===========================================================================
void CAutoHideTab::onAutoHideToActionClicked()
{
	int Location = sender()->property(internal::LocationProperty).toInt();
	d->DockWidget->setAutoHide(true, (SideBarLocation)Location);
}


//============================================================================
void CAutoHideTab::mousePressEvent(QMouseEvent* ev)
{
	 // If AutoHideShowOnMouseOver is active, then the showing is triggered
	 // by a MousePressEvent sent to this tab. To prevent accidental hiding
	 // of the tab by a mouse click, we wait at least 500 ms before we accept
	 // the mouse click
	 if (!ev->spontaneous())
	 {
		 d->TimeSinceHoverMousePress.restart();
		 d->forwardEventToDockContainer(ev);
	 }
	 else if (d->TimeSinceHoverMousePress.hasExpired(500))
	 {
		 d->forwardEventToDockContainer(ev);
	 }

	if (ev->button() == Qt::LeftButton)
	{
		ev->accept();
		d->MousePressed = true;
        d->saveDragStartMousePosition(internal::globalPositionOf(ev));
        d->DragState = DraggingMousePressed;
	}
	Super::mousePressEvent(ev);
}



//============================================================================
void CAutoHideTab::mouseReleaseEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		d->MousePressed = false;
		auto CurrentDragState = d->DragState;
		d->GlobalDragStartMousePosition = QPoint();
		d->DragStartMousePosition = QPoint();
		d->DragState = DraggingInactive;

		switch (CurrentDragState)
		{
		case DraggingTab:
			// End of tab moving, emit signal
			/*if (d->DockArea)
			{
				ev->accept();
                Q_EMIT moved(internal::globalPositionOf(ev));
			}*/
			break;

		case DraggingFloatingWidget:
			 ev->accept();
			 d->FloatingWidget->finishDragging();
			 if (d->DockWidget->autoHideDockContainer() && d->DragStartOrientation != orientation())
			 {
				 d->DockWidget->autoHideDockContainer()->resetToInitialDockWidgetSize();
			 }
			 break;

		default:
			break; // do nothing
		}
	}

	Super::mouseReleaseEvent(ev);
}


//============================================================================
void CAutoHideTab::mouseMoveEvent(QMouseEvent *ev)
{
	if (!(ev->buttons() & Qt::LeftButton)
	    || d->isDraggingState(DraggingInactive))
	{
		d->DragState = DraggingInactive;
		Super::mouseMoveEvent(ev);
		return;
	}

	// move floating window
	if (d->isDraggingState(DraggingFloatingWidget))
	{
		d->FloatingWidget->moveFloating();
		Super::mouseMoveEvent(ev);
		return;
	}

	// move tab
	if (d->isDraggingState(DraggingTab))
	{
		// Moving the tab is always allowed because it does not mean moving the
		// dock widget around
		//d->moveTab(ev);
	}

	auto MappedPos = mapToParent(ev->pos());
	bool MouseOutsideBar = (MappedPos.x() < 0)
	    || (MappedPos.x() > parentWidget()->rect().right());
	// Maybe a fixed drag distance is better here ?
	int DragDistanceY = qAbs(
	    d->GlobalDragStartMousePosition.y()
	        - internal::globalPositionOf(ev).y());
	if (DragDistanceY >= CDockManager::startDragDistance() || MouseOutsideBar)
	{
		// Floating is only allowed for widgets that are floatable
		// We can create the drag preview if the widget is movable.
		auto Features = d->DockWidget->features();
		if (Features.testFlag(CDockWidget::DockWidgetFloatable)
		    || (Features.testFlag(CDockWidget::DockWidgetMovable)))
		{
			d->startFloating();
		}
		return;
	}

	Super::mouseMoveEvent(ev);
}

//============================================================================
void CAutoHideTab::dragEnterEvent(QDragEnterEvent *ev)
{
	Q_UNUSED(ev);
	if (CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideOpenOnDragHover))
	{
		d->DragOverTimer.start();
		ev->accept();
	}
}

//============================================================================
void CAutoHideTab::dragLeaveEvent(QDragLeaveEvent *ev)
{
	Q_UNUSED(ev);
	if (CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideOpenOnDragHover))
	{
		d->DragOverTimer.stop();
	}
}


//============================================================================
void CAutoHideTab::requestCloseDockWidget()
{
	d->DockWidget->requestCloseDockWidget();
}

//============================================================================
int CAutoHideTab::tabIndex() const
{
	if (!d->SideBar)
	{
		return -1;
	}

	return d->SideBar->indexOfTab(*this);
}


//============================================================================
void CAutoHideTab::onDragHoverDelayExpired()
{
	static const char* const PropertyId = "ActiveDragOverAutoHideContainer";

	// First we check if there is an active auto hide container that is visible
	// In this case, we collapse it before we open the new one
	auto v = d->DockWidget->dockManager()->property(PropertyId);
	if (v.isValid())
	{
		auto ActiveAutoHideContainer = v.value<QPointer<CAutoHideDockContainer>>();
		if (ActiveAutoHideContainer)
		{
			ActiveAutoHideContainer->collapseView(true);
		}
	}

	auto AutoHideContainer = d->DockWidget->autoHideDockContainer();
	AutoHideContainer->collapseView(false);
	d->DockWidget->dockManager()->setProperty(PropertyId,
        QVariant::fromValue(QPointer<CAutoHideDockContainer>(AutoHideContainer)));
}


}
