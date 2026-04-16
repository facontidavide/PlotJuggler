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
/// \file   DockAreaTitleBar.cpp
/// \author Uwe Kindler
/// \date   12.10.2018
/// \brief  Implementation of CDockAreaTitleBar class
//============================================================================

//============================================================================
//                                   INCLUDES
//============================================================================
#include "DockAreaTitleBar.h"

#include <QPushButton>
#include <QToolButton>
#include <QBoxLayout>
#include <QStyle>
#include <QMenu>
#include <QScrollArea>
#include <QMouseEvent>
#include <QDebug>
#include <QPointer>
#include <QApplication>

#include "DockAreaTitleBar_p.h"
#include "ads_globals.h"
#include "FloatingDockContainer.h"
#include "FloatingDragPreview.h"
#include "DockAreaWidget.h"
#include "DockOverlay.h"
#include "DockManager.h"
#include "DockWidget.h"
#include "DockWidgetTab.h"
#include "DockAreaTabBar.h"
#include "DockComponentsFactory.h"
#include "DockFocusController.h"
#include "ElidingLabel.h"
#include "AutoHideDockContainer.h"
#include "IconProvider.h"

#include <iostream>

namespace ads
{

/**
 * Private data class of CDockAreaTitleBar class (pimpl)
 */
struct DockAreaTitleBarPrivate
{
	CDockAreaTitleBar* _this;
	QPointer<CTitleBarButton> TabsMenuButton;
	QPointer<CTitleBarButton> AutoHideButton;
	QPointer<CTitleBarButton> UndockButton;
	QPointer<CTitleBarButton> CloseButton;
	QPointer<CTitleBarButton> MinimizeButton;
	QBoxLayout* Layout;
	CDockAreaWidget* DockArea;
	CDockAreaTabBar* TabBar;
	CElidingLabel* AutoHideTitleLabel = nullptr;
	bool MenuOutdated = true;
	QMenu* TabsMenu;
	QList<tTitleBarButton*> DockWidgetActionsButtons;

	QPoint DragStartMousePos;
	eDragState DragState = DraggingInactive;
	IFloatingWidget* FloatingWidget = nullptr;


	/**
	 * Private data constructor
	 */
	DockAreaTitleBarPrivate(CDockAreaTitleBar* _public);

	/**
	 * Creates the title bar close and menu buttons
	 */
	void createButtons();


	/**
	 * Creates the auto hide title label, only displayed when the dock area is overlayed
	 */
    void createAutoHideTitleLabel();

    /**
	 * Creates the internal TabBar
	 */
	void createTabBar();

	/**
	 * Convenience function for DockManager access
	 */
	CDockManager* dockManager() const
	{
		return DockArea->dockManager();
	}

	/**
	 * Convenience function for access to dock manager components factory
	 */
	QSharedPointer<ads::CDockComponentsFactory> componentsFactory() const
	{
        return dockManager()->componentsFactory();
    }

	/**
	 * Returns true if the given config flag is set
	 * Convenience function to ease config flag testing
	 */
	static bool testConfigFlag(CDockManager::eConfigFlag Flag)
	{
		return CDockManager::testConfigFlag(Flag);
	}

	/**
	 * Returns true if the given config flag is set
	 * Convenience function to ease config flag testing
	 */
	static bool testAutoHideConfigFlag(CDockManager::eAutoHideFlag Flag)
	{
		return CDockManager::testAutoHideConfigFlag(Flag);
	}

	/**
	 * Test function for current drag state
	 */
	bool isDraggingState(eDragState dragState) const
	{
		return this->DragState == dragState;
	}

	/**
	 * Starts floating
	 */
	void startFloating(const QPoint& Offset);

	/**
	 * Makes the dock area floating
	 */
	IFloatingWidget* makeAreaFloating(const QPoint& Offset, eDragState DragState);

	/**
	 * Helper function to create and initialize the menu entries for
	 * the "Auto Hide Group To..." menu
	 */
	QAction* createAutoHideToAction(const QString& Title, SideBarLocation Location,
		QMenu* Menu)
	{
		auto Action = Menu->addAction(Title);
		Action->setProperty(internal::LocationProperty, Location);
		QObject::connect(Action, &QAction::triggered, _this, &CDockAreaTitleBar::onAutoHideToActionClicked);
		return Action;
	}
};// struct DockAreaTitleBarPrivate

//============================================================================
DockAreaTitleBarPrivate::DockAreaTitleBarPrivate(CDockAreaTitleBar* _public) :
	_this(_public)
{

}


//============================================================================
void DockAreaTitleBarPrivate::createButtons()
{
	QSizePolicy ButtonSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

	// Tabs menu button
	TabsMenuButton = new CTitleBarButton(testConfigFlag(CDockManager::DockAreaHasTabsMenuButton),
		false, TitleBarButtonTabsMenu);
	TabsMenuButton->setObjectName("tabsMenuButton");
	TabsMenuButton->setAutoRaise(true);
	TabsMenuButton->setPopupMode(QToolButton::InstantPopup);
	internal::setButtonIcon(TabsMenuButton, QStyle::SP_TitleBarUnshadeButton, ads::DockAreaMenuIcon);
	QMenu* TabsMenu = new QMenu(TabsMenuButton);
#ifndef QT_NO_TOOLTIP
	TabsMenu->setToolTipsVisible(true);
#endif
	_this->connect(TabsMenu, SIGNAL(aboutToShow()), SLOT(onTabsMenuAboutToShow()));
	TabsMenuButton->setMenu(TabsMenu);
	internal::setToolTip(TabsMenuButton, QObject::tr("List All Tabs"));
	TabsMenuButton->setSizePolicy(ButtonSizePolicy);
	Layout->addWidget(TabsMenuButton, 0);
	_this->connect(TabsMenuButton->menu(), SIGNAL(triggered(QAction*)),
		SLOT(onTabsMenuActionTriggered(QAction*)));

	// Undock button
	UndockButton = new CTitleBarButton(testConfigFlag(CDockManager::DockAreaHasUndockButton),
		true, TitleBarButtonUndock);
	UndockButton->setObjectName("detachGroupButton");
	UndockButton->setAutoRaise(true);
	internal::setToolTip(UndockButton, QObject::tr("Detach Group"));
	internal::setButtonIcon(UndockButton, QStyle::SP_TitleBarNormalButton, ads::DockAreaUndockIcon);
	UndockButton->setSizePolicy(ButtonSizePolicy);
	Layout->addWidget(UndockButton, 0);
	_this->connect(UndockButton, SIGNAL(clicked()), SLOT(onUndockButtonClicked()));

	// AutoHide Button
	const auto autoHideEnabled = testAutoHideConfigFlag(CDockManager::AutoHideFeatureEnabled);
	AutoHideButton = new CTitleBarButton(testAutoHideConfigFlag(CDockManager::DockAreaHasAutoHideButton) && autoHideEnabled,
		true, TitleBarButtonAutoHide);
	AutoHideButton->setObjectName("dockAreaAutoHideButton");
	AutoHideButton->setAutoRaise(true);
	internal::setToolTip(AutoHideButton, _this->titleBarButtonToolTip(TitleBarButtonAutoHide));
	internal::setButtonIcon(AutoHideButton, QStyle::SP_DialogOkButton, ads::AutoHideIcon);
	AutoHideButton->setSizePolicy(ButtonSizePolicy);
	AutoHideButton->setCheckable(testAutoHideConfigFlag(CDockManager::AutoHideButtonCheckable));
	AutoHideButton->setChecked(false);
	Layout->addWidget(AutoHideButton, 0);
	_this->connect(AutoHideButton, SIGNAL(clicked()),  SLOT(onAutoHideButtonClicked()));

	// Minimize button
	MinimizeButton = new CTitleBarButton(testAutoHideConfigFlag(CDockManager::AutoHideHasMinimizeButton),
		false, TitleBarButtonMinimize);
	MinimizeButton->setObjectName("dockAreaMinimizeButton");
	MinimizeButton->setAutoRaise(true);
	MinimizeButton->setVisible(false);
	internal::setButtonIcon(MinimizeButton, QStyle::SP_TitleBarMinButton, ads::DockAreaMinimizeIcon);
	internal::setToolTip(MinimizeButton, QObject::tr("Minimize"));
	MinimizeButton->setSizePolicy(ButtonSizePolicy);
	Layout->addWidget(MinimizeButton, 0);
	_this->connect(MinimizeButton, SIGNAL(clicked()), SLOT(minimizeAutoHideContainer()));

	// Close button
	CloseButton = new CTitleBarButton(testConfigFlag(CDockManager::DockAreaHasCloseButton),
		true, TitleBarButtonClose);
	CloseButton->setObjectName("dockAreaCloseButton");
	CloseButton->setAutoRaise(true);
	internal::setButtonIcon(CloseButton, QStyle::SP_TitleBarCloseButton, ads::DockAreaCloseIcon);
    internal::setToolTip(CloseButton, _this->titleBarButtonToolTip(TitleBarButtonClose));
	CloseButton->setSizePolicy(ButtonSizePolicy);
	CloseButton->setIconSize(QSize(16, 16));
	Layout->addWidget(CloseButton, 0);
	_this->connect(CloseButton, SIGNAL(clicked()), SLOT(onCloseButtonClicked()));
}


//============================================================================
void DockAreaTitleBarPrivate::createAutoHideTitleLabel()
{
	AutoHideTitleLabel = new CElidingLabel("");
	AutoHideTitleLabel->setObjectName("autoHideTitleLabel");
	// When the tabs are at the top, they will be at position 0, insert the label behind them, and hide it.
	Layout->addWidget(AutoHideTitleLabel);
	AutoHideTitleLabel->setVisible(CDockManager::testConfigFlag(CDockManager::TabsAtBottom));
	Layout->addWidget(new CSpacerWidget(_this));
}


//============================================================================
void DockAreaTitleBarPrivate::createTabBar()
{
	TabBar = componentsFactory()->createDockAreaTabBar(DockArea);
    TabBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
	if (!CDockManager::testConfigFlag(CDockManager::TabsAtBottom))
		Layout->addWidget(TabBar);

	_this->connect(TabBar, SIGNAL(tabClosed(int)), SLOT(markTabsMenuOutdated()));
	_this->connect(TabBar, SIGNAL(tabOpened(int)), SLOT(markTabsMenuOutdated()));
	_this->connect(TabBar, SIGNAL(tabInserted(int)), SLOT(markTabsMenuOutdated()));
	_this->connect(TabBar, SIGNAL(removingTab(int)), SLOT(markTabsMenuOutdated()));
	_this->connect(TabBar, SIGNAL(tabMoved(int, int)), SLOT(markTabsMenuOutdated()));
	_this->connect(TabBar, SIGNAL(currentChanged(int)), SLOT(onCurrentTabChanged(int)));
	_this->connect(TabBar, SIGNAL(tabBarClicked(int)), SIGNAL(tabBarClicked(int)));
	_this->connect(TabBar, SIGNAL(elidedChanged(bool)), SLOT(markTabsMenuOutdated()));
}


//============================================================================
IFloatingWidget* DockAreaTitleBarPrivate::makeAreaFloating(const QPoint& Offset, eDragState DragState)
{
	QSize Size = DockArea->size();
	this->DragState = DragState;
	bool CreateFloatingDockContainer = (DraggingFloatingWidget != DragState);
	CFloatingDockContainer* FloatingDockContainer = nullptr;
	IFloatingWidget* FloatingWidget;
	if (CreateFloatingDockContainer)
	{
		if (DockArea->autoHideDockContainer())
        {
            DockArea->autoHideDockContainer()->cleanupAndDelete();
        }
		FloatingWidget = FloatingDockContainer = new CFloatingDockContainer(DockArea);
	}
	else
	{
		auto w = new CFloatingDragPreview(DockArea);
		QObject::connect(w, &CFloatingDragPreview::draggingCanceled, [this]()
		{
			this->DragState = DraggingInactive;
		});
		FloatingWidget = w;
	}

    FloatingWidget->startFloating(Offset, Size, DragState, nullptr);
    if (FloatingDockContainer)
    {
		auto TopLevelDockWidget = FloatingDockContainer->topLevelDockWidget();
		if (TopLevelDockWidget)
		{
			TopLevelDockWidget->emitTopLevelChanged(true);
		}
    }

	return FloatingWidget;
}


//============================================================================
void DockAreaTitleBarPrivate::startFloating(const QPoint& Offset)
{
	if (DockArea->autoHideDockContainer())
	{
		DockArea->autoHideDockContainer()->hide();
	}
	FloatingWidget = makeAreaFloating(Offset, DraggingFloatingWidget);
	qApp->postEvent(DockArea, new QEvent((QEvent::Type)internal::DockedWidgetDragStartEvent));
}


//============================================================================
CDockAreaTitleBar::CDockAreaTitleBar(CDockAreaWidget* parent) :
	QFrame(parent),
	d(new DockAreaTitleBarPrivate(this))
{
	d->DockArea = parent;

	setObjectName("dockAreaTitleBar");
	d->Layout = new QBoxLayout(QBoxLayout::LeftToRight);
	d->Layout->setContentsMargins(0, 0, 0, 0);
	d->Layout->setSpacing(0);
	setLayout(d->Layout);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	d->createTabBar();
	d->createAutoHideTitleLabel();
	d->createButtons();

    setFocusPolicy(Qt::NoFocus);
}


//============================================================================
CDockAreaTitleBar::~CDockAreaTitleBar()
{
	if (!d->CloseButton.isNull())
	{
		delete d->CloseButton;
	}

	if (!d->TabsMenuButton.isNull())
	{
		delete d->TabsMenuButton;
	}

	if (!d->UndockButton.isNull())
	{
		delete d->UndockButton;
	}
	delete d;
}


//============================================================================
CDockAreaTabBar* CDockAreaTitleBar::tabBar() const
{
	return d->TabBar;
}


//============================================================================
void CDockAreaTitleBar::resizeEvent(QResizeEvent *event)
{
	Super::resizeEvent(event);
	if (CDockManager::testConfigFlag(CDockManager::DockAreaDynamicTabsMenuButtonVisibility)
	 && CDockManager::testConfigFlag(CDockManager::DisableTabTextEliding))
	{
		// Use queued connection to ensure that the resizing and relayouting has
		// finished to ensure that the d->TabBar->areTabsOverflowing() function
		// returns the correct value
		QMetaObject::invokeMethod(this, "markTabsMenuOutdated", Qt::QueuedConnection);
	}
}


//============================================================================
void CDockAreaTitleBar::markTabsMenuOutdated()
{
	if (CDockManager::testConfigFlag(CDockManager::DockAreaDynamicTabsMenuButtonVisibility))
	{
		bool TabsMenuButtonVisible = false;
		if (CDockManager::testConfigFlag(CDockManager::DisableTabTextEliding))
		{
			TabsMenuButtonVisible = d->TabBar->areTabsOverflowing();
		}
		else
		{
			bool hasElidedTabTitle = false;
			for (int i = 0; i < d->TabBar->count(); ++i)
			{
				if (!d->TabBar->isTabOpen(i))
				{
					continue;
				}
				CDockWidgetTab* Tab = d->TabBar->tab(i);
				if(Tab->isTitleElided())
				{
					hasElidedTabTitle = true;
					break;
				}
			}
			TabsMenuButtonVisible = (hasElidedTabTitle && (d->TabBar->count() > 1));
		}
		QMetaObject::invokeMethod(d->TabsMenuButton, "setVisible", Qt::QueuedConnection, Q_ARG(bool, TabsMenuButtonVisible));
	}
	d->MenuOutdated = true;
}

//============================================================================
void CDockAreaTitleBar::onTabsMenuAboutToShow()
{
	if (!d->MenuOutdated)
	{
		return;
	}

	QMenu* menu = d->TabsMenuButton->menu();
	menu->clear();
	for (int i = 0; i < d->TabBar->count(); ++i)
	{
		if (!d->TabBar->isTabOpen(i))
		{
			continue;
		}
		auto Tab = d->TabBar->tab(i);
		QAction* Action = menu->addAction(Tab->icon(), Tab->text());
		internal::setToolTip(Action, Tab->toolTip());
		Action->setData(i);
	}

	d->MenuOutdated = false;
}


//============================================================================
void CDockAreaTitleBar::onCloseButtonClicked()
{
    ADS_PRINT("CDockAreaTitleBar::onCloseButtonClicked");
	if (CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideCloseButtonCollapsesDock) &&
		d->DockArea->autoHideDockContainer())
	{
		d->DockArea->autoHideDockContainer()->collapseView(true);
	}
	else if (d->testConfigFlag(CDockManager::DockAreaCloseButtonClosesTab))
	{
		d->TabBar->closeTab(d->TabBar->currentIndex());
	}
	else
	{
		d->DockArea->closeArea();
	}
}


//============================================================================
void CDockAreaTitleBar::onAutoHideCloseActionTriggered()
{
	d->DockArea->closeArea();
}


//============================================================================
void CDockAreaTitleBar::minimizeAutoHideContainer()
{
	auto AutoHideContainer = d->DockArea->autoHideDockContainer();
	if (AutoHideContainer)
	{
		AutoHideContainer->collapseView(true);
	}
}


//============================================================================
void CDockAreaTitleBar::onUndockButtonClicked()
{
	if (d->DockArea->features().testFlag(CDockWidget::DockWidgetFloatable))
	{
		d->makeAreaFloating(mapFromGlobal(QCursor::pos()), DraggingInactive);
	}
}


//============================================================================
void CDockAreaTitleBar::onTabsMenuActionTriggered(QAction* Action)
{
	int Index = Action->data().toInt();
	d->TabBar->setCurrentIndex(Index);
	Q_EMIT tabBarClicked(Index);
}


//============================================================================
void CDockAreaTitleBar::updateDockWidgetActionsButtons()
{
	auto Tab = d->TabBar->currentTab();
	if (!Tab)
	{
		return;
	}

	CDockWidget* DockWidget = Tab->dockWidget();
	if (!d->DockWidgetActionsButtons.isEmpty())
	{
		for (auto Button : d->DockWidgetActionsButtons)
		{
			d->Layout->removeWidget(Button);
			delete Button;
		}
		d->DockWidgetActionsButtons.clear();
	}

	auto Actions = DockWidget->titleBarActions();
	if (Actions.isEmpty())
	{
		return;
	}

	int InsertIndex = indexOf(d->TabsMenuButton);
	for (auto Action : Actions)
	{
		auto Button = new CTitleBarButton(true, false, TitleBarButtonTabsMenu, this);
		Button->setDefaultAction(Action);
		Button->setAutoRaise(true);
		Button->setPopupMode(QToolButton::InstantPopup);
		Button->setObjectName(Action->objectName());
		d->Layout->insertWidget(InsertIndex++, Button, 0);
		d->DockWidgetActionsButtons.append(Button);
	}
}


//============================================================================
void CDockAreaTitleBar::onCurrentTabChanged(int Index)
{
	if (Index < 0)
	{
		return;
	}

	if (d->testConfigFlag(CDockManager::DockAreaCloseButtonClosesTab))
	{
		CDockWidget* DockWidget = d->TabBar->tab(Index)->dockWidget();
		d->CloseButton->setEnabled(DockWidget->features().testFlag(CDockWidget::DockWidgetClosable));
	}

	updateDockWidgetActionsButtons();
}


//============================================================================
void CDockAreaTitleBar::onAutoHideButtonClicked()
{
	if (CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideButtonTogglesArea)
	 || qApp->keyboardModifiers().testFlag(Qt::ControlModifier))
	{
		d->DockArea->toggleAutoHide();
	}
	else
	{
		d->DockArea->currentDockWidget()->toggleAutoHide();
	}
}


//============================================================================
void CDockAreaTitleBar::onAutoHideDockAreaActionClicked()
{
	d->DockArea->toggleAutoHide();
}


//============================================================================
void CDockAreaTitleBar::onAutoHideToActionClicked()
{
	int Location = sender()->property(internal::LocationProperty).toInt();
	d->DockArea->toggleAutoHide((SideBarLocation)Location);
}


//============================================================================
CTitleBarButton* CDockAreaTitleBar::button(TitleBarButton which) const
{
	switch (which)
	{
	case TitleBarButtonTabsMenu: return d->TabsMenuButton;
	case TitleBarButtonUndock: return d->UndockButton;
	case TitleBarButtonClose: return d->CloseButton;
	case TitleBarButtonAutoHide: return d->AutoHideButton;
	case TitleBarButtonMinimize: return d->MinimizeButton;
	default:
		return nullptr;
	}
}


//============================================================================
CElidingLabel* CDockAreaTitleBar::autoHideTitleLabel() const
{
	return d->AutoHideTitleLabel;
}


//============================================================================
void CDockAreaTitleBar::setVisible(bool Visible)
{
	Super::setVisible(Visible);
	markTabsMenuOutdated();
}


//============================================================================
void CDockAreaTitleBar::mousePressEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		ev->accept();
		d->DragStartMousePos = ev->pos();
		d->DragState = DraggingMousePressed;

		if (CDockManager::testConfigFlag(CDockManager::FocusHighlighting))
		{
			d->dockManager()->dockFocusController()->setDockWidgetTabFocused(d->TabBar->currentTab());
		}
		return;
	}
	Super::mousePressEvent(ev);
}


//============================================================================
void CDockAreaTitleBar::mouseReleaseEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
	{
        ADS_PRINT("CDockAreaTitleBar::mouseReleaseEvent");
		ev->accept();
		auto CurrentDragState = d->DragState;
		d->DragStartMousePos = QPoint();
		d->DragState = DraggingInactive;
		if (DraggingFloatingWidget == CurrentDragState)
		{
			d->FloatingWidget->finishDragging();
		}

		return;
	}
	Super::mouseReleaseEvent(ev);
}


//============================================================================
void CDockAreaTitleBar::mouseMoveEvent(QMouseEvent* ev)
{
	Super::mouseMoveEvent(ev);
	if (!(ev->buttons() & Qt::LeftButton) || d->isDraggingState(DraggingInactive))
	{
		d->DragState = DraggingInactive;
		return;
	}

    // move floating window
    if (d->isDraggingState(DraggingFloatingWidget))
    {
        d->FloatingWidget->moveFloating();
        return;
    }

	// If this is the last dock area in a floating dock container it does not make
	// sense to move it to a new floating widget and leave this one
	// empty
	if (d->DockArea->dockContainer()->isFloating()
	 && d->DockArea->dockContainer()->visibleDockAreaCount() == 1 
     && !d->DockArea->isAutoHide())
	{
		return;
	}

	// If one single dock widget in this area is not floatable then the whole
	// area is not floatable
	// We can create the floating drag preview if the dock widget is movable
	auto Features = d->DockArea->features();
    if (!Features.testFlag(CDockWidget::DockWidgetFloatable) && !(Features.testFlag(CDockWidget::DockWidgetMovable)))
	{
		return;
	}

	int DragDistance = (d->DragStartMousePos - ev->pos()).manhattanLength();
	if (DragDistance >= CDockManager::startDragDistance())
	{
        ADS_PRINT("CDockAreaTitleBar::startFloating");
		d->startFloating(d->DragStartMousePos);
		auto Overlay = d->DockArea->dockManager()->containerOverlay();
		Overlay->setAllowedAreas(OuterDockAreas);
	}

	return;
}


//============================================================================
void CDockAreaTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
	// If this is the last dock area in a dock container it does not make
	// sense to move it to a new floating widget and leave this one
	// empty
	if (d->DockArea->dockContainer()->isFloating() && d->DockArea->dockContainer()->dockAreaCount() == 1)
	{
		return;
	}

	if (!d->DockArea->features().testFlag(CDockWidget::DockWidgetFloatable))
	{
		return;
	}

	if (!CDockManager::testConfigFlag(CDockManager::DoubleClickUndocksWidget))
	{
		return;
	}

	d->makeAreaFloating(event->pos(), DraggingInactive);
}


//============================================================================
void CDockAreaTitleBar::setAreaFloating()
{
	// If this is the last dock area in a dock container it does not make
	// sense to move it to a new floating widget and leave this one
	// empty.
	auto DockContainer = d->DockArea->dockContainer();
	if (DockContainer->isFloating() && DockContainer->dockAreaCount() == 1
	 && !d->DockArea->isAutoHide())
	{
		return;
	}

	if (!d->DockArea->features().testFlag(CDockWidget::DockWidgetFloatable))
	{
		return;
	}

	d->makeAreaFloating(mapFromGlobal(QCursor::pos()), DraggingInactive);
}


//============================================================================
void CDockAreaTitleBar::contextMenuEvent(QContextMenuEvent* ev)
{
	ev->accept();
	if (d->isDraggingState(DraggingFloatingWidget))
	{
		return;
	}

    auto Menu = buildContextMenu(nullptr);
	Menu->exec(ev->globalPos());
    delete Menu;
}

QMenu* CDockAreaTitleBar::buildContextMenu(QMenu *Menu)
{
    const bool isAutoHide = d->DockArea->isAutoHide();
	const bool isTopLevelArea = d->DockArea->isTopLevelArea();
	QAction* Action;
    if (Menu == nullptr)
    {
        Menu = new QMenu(this);
    }
    
    if (!isTopLevelArea)
	{
		Action = Menu->addAction(isAutoHide ? tr("Detach") : tr("Detach Group"),
			this, SLOT(onUndockButtonClicked()));
		Action->setEnabled(d->DockArea->features().testFlag(CDockWidget::DockWidgetFloatable));
		if (CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideFeatureEnabled))
		{
			Action = Menu->addAction(isAutoHide ? tr("Unpin (Dock)") : tr("Pin Group"), this, SLOT(onAutoHideDockAreaActionClicked()));
			auto AreaIsPinnable = d->DockArea->features().testFlag(CDockWidget::DockWidgetPinnable);
			Action->setEnabled(AreaIsPinnable);

			if (!isAutoHide)
			{
				auto menu = Menu->addMenu(tr("Pin Group To..."));
				menu->setEnabled(AreaIsPinnable);
				d->createAutoHideToAction(tr("Top"), SideBarTop, menu);
				d->createAutoHideToAction(tr("Left"), SideBarLeft, menu);
				d->createAutoHideToAction(tr("Right"), SideBarRight, menu);
				d->createAutoHideToAction(tr("Bottom"), SideBarBottom, menu);
			}
		}
		Menu->addSeparator();
	}

	if (isAutoHide)
	{
		Action = Menu->addAction(tr("Minimize"), this, SLOT(minimizeAutoHideContainer()));
		Action = Menu->addAction(tr("Close"), this, SLOT(onAutoHideCloseActionTriggered()));
	}
	else
	{
		Action = Menu->addAction(isAutoHide ? tr("Close") : tr("Close Group"), this, SLOT(onCloseButtonClicked()));
	}

	Action->setEnabled(d->DockArea->features().testFlag(CDockWidget::DockWidgetClosable));
	if (!isAutoHide && !isTopLevelArea)
	{
		Action = Menu->addAction(tr("Close Other Groups"), d->DockArea, SLOT(closeOtherAreas()));
	}
    return Menu;
}

//============================================================================
void CDockAreaTitleBar::insertWidget(int index, QWidget *widget)
{
	d->Layout->insertWidget(index, widget);
}


//============================================================================
int CDockAreaTitleBar::indexOf(QWidget *widget) const
{
	return d->Layout->indexOf(widget);
}

//============================================================================
QString CDockAreaTitleBar::titleBarButtonToolTip(TitleBarButton Button) const
{
	switch (Button)
	{
	case TitleBarButtonAutoHide:
		 if (d->DockArea->isAutoHide())
		 {
			 return tr("Unpin (Dock)");
		 }

		 if (CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideButtonTogglesArea))
		 {
			 return tr("Pin Group");
		 }
		 else
		 {
			 return tr("Pin Active Tab (Press Ctrl to Pin Group)");
		 }
		 break;

	case TitleBarButtonClose:
		if (d->DockArea->isAutoHide())
		{
			bool Minimize = CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideCloseButtonCollapsesDock);
			return Minimize ? tr("Minimize") : tr("Close");
		}

		if (CDockManager::testConfigFlag(CDockManager::DockAreaCloseButtonClosesTab))
		{
			return tr("Close Active Tab");
		}
		else
		{
			return tr("Close Group");
		}
	    break;

	default:
		break;
	}

	return QString();
}


//============================================================================
void CDockAreaTitleBar::showAutoHideControls(bool Show)
{
	if (Show)
		d->TabBar->setVisible(false); // Auto hide toolbar never has tabs

	d->MinimizeButton->setVisible(Show);
	if (!CDockManager::testConfigFlag(CDockManager::TabsAtBottom))
		d->AutoHideTitleLabel->setVisible(Show);
}


//============================================================================
bool CDockAreaTitleBar::isAutoHide() const
{
	return d->DockArea && d->DockArea->isAutoHide();
}


//============================================================================
CDockAreaWidget* CDockAreaTitleBar::dockAreaWidget() const
{
	return d->DockArea;
}


//============================================================================
CTitleBarButton::CTitleBarButton(bool showInTitleBar, bool hideWhenDisabled, TitleBarButton ButtonId, QWidget* parent)
	: tTitleBarButton(parent),
	  ShowInTitleBar(showInTitleBar),
	  HideWhenDisabled(CDockManager::testConfigFlag(CDockManager::DockAreaHideDisabledButtons) && hideWhenDisabled),
	  TitleBarButtonId(ButtonId)
{
    setFocusPolicy(Qt::NoFocus);
}

//============================================================================
void CTitleBarButton::setVisible(bool visible)
{
	// 'visible' can stay 'true' if and only if this button is configured to generally visible:
	visible = visible && this->ShowInTitleBar;

	// 'visible' can stay 'true' unless: this button is configured to be invisible when it is disabled and it is currently disabled:
	if (visible && HideWhenDisabled)
	{
		visible = isEnabled();
	}

	Super::setVisible(visible);
}


//============================================================================
void CTitleBarButton::setShowInTitleBar(bool Show)
{
	this->ShowInTitleBar = Show;
	if (!Show)
	{
		setVisible(false);
	}
}


//============================================================================
bool CTitleBarButton::event(QEvent *ev)
{
	if (QEvent::EnabledChange != ev->type() || !HideWhenDisabled || !ShowInTitleBar)
	{
		return Super::event(ev);
	}

	bool Show = true;
	if (isInAutoHideArea())
	{
		switch (TitleBarButtonId)
        {
		case TitleBarButtonClose: Show = CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideHasCloseButton); break;
		case TitleBarButtonUndock: Show = false; break;
		default:
			break;
        }
	}

	// force setVisible() call - Calling setVisible() directly here doesn't
	// work well when button is expected to be shown first time
	QMetaObject::invokeMethod(this, "setVisible", Qt::QueuedConnection,
		Q_ARG(bool, isEnabledTo(this->parentWidget()) & Show));

	return Super::event(ev);
}


//============================================================================
CDockAreaTitleBar* CTitleBarButton::titleBar() const
{
	return qobject_cast<CDockAreaTitleBar*>(parentWidget());
}


//============================================================================
bool CTitleBarButton::isInAutoHideArea() const
{
	auto TitleBar = titleBar();
    return TitleBar && TitleBar->isAutoHide();
}


//============================================================================
CSpacerWidget::CSpacerWidget(QWidget* Parent /*= 0*/) : Super(Parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setStyleSheet("border: none; background: none;");
}


} // namespace ads

//---------------------------------------------------------------------------
// EOF DockAreaTitleBar.cpp
