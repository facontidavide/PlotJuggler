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
/// \file   DockWidget.cpp
/// \author Uwe Kindler
/// \date   26.02.2017
/// \brief  Implementation of CDockWidget class
//============================================================================


//============================================================================
//                                   INCLUDES
//============================================================================
#include <AutoHideDockContainer.h>
#include <AutoHideSideBar.h>
#include <AutoHideTab.h>
#include "DockWidgetTab.h"
#include "DockWidget.h"

#include <iostream>

#include <QBoxLayout>
#include <QAction>
#include <QSplitter>
#include <QStack>
#include <QScrollArea>
#include <QTextStream>
#include <QPointer>
#include <QEvent>
#include <QDebug>
#include <QToolBar>
#include <QXmlStreamWriter>
#include <QWindow>

#include <QGuiApplication>
#include <QScreen>
#include <QWindow>

#include "DockContainerWidget.h"
#include "DockAreaWidget.h"
#include "DockManager.h"
#include "FloatingDockContainer.h"
#include "DockSplitter.h"
#include "DockComponentsFactory.h"
#include "ads_globals.h"


namespace ads
{
/**
 * Private data class of CDockWidget class (pimpl)
 */
struct DockWidgetPrivate
{
	struct WidgetFactory 
	{
		CDockWidget::FactoryFunc createWidget;
		CDockWidget::eInsertMode insertMode;
	};	
	
	CDockWidget* _this = nullptr;
	QBoxLayout* Layout = nullptr;
	QWidget* Widget = nullptr;
	CDockWidgetTab* TabWidget = nullptr;
	CDockWidget::DockWidgetFeatures Features = CDockWidget::DefaultDockWidgetFeatures;
	QPointer<CDockManager> DockManager;
	QPointer<CDockAreaWidget> DockArea;
	QAction* ToggleViewAction = nullptr;
	bool Closed = false;
	QScrollArea* ScrollArea = nullptr;
	QToolBar* ToolBar = nullptr;
	Qt::ToolButtonStyle ToolBarStyleDocked = Qt::ToolButtonIconOnly;
	Qt::ToolButtonStyle ToolBarStyleFloating = Qt::ToolButtonTextUnderIcon;
	QSize ToolBarIconSizeDocked = QSize(16, 16);
	QSize ToolBarIconSizeFloating = QSize(24, 24);
	bool IsFloatingTopLevel = false;
	QList<QAction*> TitleBarActions;
	CDockWidget::eMinimumSizeHintMode MinimumSizeHintMode = CDockWidget::MinimumSizeHintFromDockWidget;
	WidgetFactory* Factory = nullptr;
	QPointer<CAutoHideTab> SideTabWidget;
	CDockWidget::eToolBarStyleSource ToolBarStyleSource = CDockWidget::ToolBarStyleFromDockManager;
	
	/**
	 * Private data constructor
	 */
	DockWidgetPrivate(CDockWidget* _public);

	/**
	 * Convenience function to ease components factory access
	 */
	QSharedPointer<ads::CDockComponentsFactory> componentsFactory() const
	{
        return DockManager ? DockManager->componentsFactory() : CDockComponentsFactory::factory();
    }

	/**
	 * Show dock widget
	 */
	void showDockWidget();

	/**
	 * Hide dock widget.
	 */
	void hideDockWidget();

	/**
	 * Hides a dock area if all dock widgets in the area are closed.
	 * This function updates the current selected tab and hides the parent
	 * dock area if it is empty
	 */
	void updateParentDockArea();

	/**
	 * Closes all auto hide dock widgets if there are no more opened dock areas
	 * This prevents the auto hide dock widgets from being pinned to an empty dock area
	 */
	void closeAutoHideDockWidgetsIfNeeded();

	/**
	 * Setup the top tool bar
	 */
	void setupToolBar();

	/**
	 * Setup the main scroll area
	 */
	void setupScrollArea();
	
	/**
	 * Creates the content widget with the registered widget factory and
	 * returns true on success.
	 */
	bool createWidgetFromFactory();

	/**
	 * Use the dock manager toolbar style and icon size for the different states
	 */
	void setToolBarStyleFromDockManager();
};
// struct DockWidgetPrivate

//============================================================================
DockWidgetPrivate::DockWidgetPrivate(CDockWidget* _public) :
	_this(_public)
{

}


//============================================================================
void DockWidgetPrivate::showDockWidget()
{
	if (!Widget)
	{
		if (!createWidgetFromFactory())
		{
			Q_ASSERT(!Features.testFlag(CDockWidget::DeleteContentOnClose)
					 && "DeleteContentOnClose flag was set, but the widget "
						"factory is missing or it doesn't return a valid QWidget.");
			return;	
		}
	}
	
	if (!DockArea)
	{
		CFloatingDockContainer* FloatingWidget = new CFloatingDockContainer(_this);
		// We use the size hint of the content widget to provide a good
		// initial size
		FloatingWidget->resize(Widget ? Widget->sizeHint() : _this->sizeHint());
		TabWidget->show();
		FloatingWidget->show();
	}
	else
	{
		DockArea->setCurrentDockWidget(_this);
		DockArea->toggleView(true);
		TabWidget->show();
		auto Splitter = DockArea->parentSplitter();
		while (Splitter && !Splitter->isVisible() && !DockArea->isAutoHide())
		{
			Splitter->show();
			Splitter = internal::findParent<CDockSplitter*>(Splitter);
		}

		CDockContainerWidget* Container = DockArea->dockContainer();
		if (Container->isFloating())
		{
			CFloatingDockContainer* FloatingWidget = internal::findParent<
					CFloatingDockContainer*>(Container);
			FloatingWidget->show();
		}

        // If this widget is pinned and there are no opened dock widgets, unpin the auto hide widget by moving it's contents to parent container
		// While restoring state, opened dock widgets are not valid
		if (Container->openedDockWidgets().count() == 0 && DockArea->isAutoHide() && !DockManager->isRestoringState())
		{
			DockArea->autoHideDockContainer()->moveContentsToParent();
		}
	}
}


//============================================================================
void DockWidgetPrivate::hideDockWidget()
{
	TabWidget->hide();
	updateParentDockArea();

	closeAutoHideDockWidgetsIfNeeded();

	if (Features.testFlag(CDockWidget::DeleteContentOnClose))
	{
		if (ScrollArea)
		{
			ScrollArea->takeWidget();
			delete ScrollArea;
			ScrollArea = nullptr;
		}
		Widget->deleteLater();
		Widget = nullptr;
	}
}


//============================================================================
void DockWidgetPrivate::updateParentDockArea()
{
	if (!DockArea)
	{
		return;
	}

	// we don't need to change the current tab if the
	// current dock widget is not the one being closed
	if (DockArea->currentDockWidget() != _this){
		return;
	}

	auto NextDockWidget = DockArea->nextOpenDockWidget(_this);
	if (NextDockWidget)
	{
		DockArea->setCurrentDockWidget(NextDockWidget);
	}
	else
	{
		DockArea->hideAreaWithNoVisibleContent();
	}
}


//============================================================================
void DockWidgetPrivate::closeAutoHideDockWidgetsIfNeeded()
{
	auto DockContainer = _this->dockContainer();
	if (!DockContainer)
	{
		return;
	}

	if (_this->dockManager()->isRestoringState())
	{
		return;
	}

	// If the dock container is the dock manager, or if it is not empty, then we
	// don't need to do anything
	if ((DockContainer == _this->dockManager())
	 || !DockContainer->openedDockWidgets().isEmpty())
	{
		return;
	}

	for (auto autoHideWidget : DockContainer->autoHideWidgets())
	{
		auto DockWidget = autoHideWidget->dockWidget();
		if (DockWidget == _this)
		{
			continue;
		}

		DockWidget->toggleView(false);
	}
}


//============================================================================
void DockWidgetPrivate::setupToolBar()
{
	ToolBar = new QToolBar(_this);
	ToolBar->setObjectName("dockWidgetToolBar");
	Layout->insertWidget(0, ToolBar);
	ToolBar->setIconSize(QSize(16, 16));
	ToolBar->toggleViewAction()->setEnabled(false);
	ToolBar->toggleViewAction()->setVisible(false);
	_this->connect(_this, SIGNAL(topLevelChanged(bool)), SLOT(setToolbarFloatingStyle(bool)));
}



//============================================================================
void DockWidgetPrivate::setupScrollArea()
{
	ScrollArea = new QScrollArea(_this);
	ScrollArea->setObjectName("dockWidgetScrollArea");
	ScrollArea->setWidgetResizable(true);
	Layout->addWidget(ScrollArea);
}


//============================================================================
bool DockWidgetPrivate::createWidgetFromFactory()
{
	if (!Features.testFlag(CDockWidget::DeleteContentOnClose)) 
	{
		return false;
	}
	
	if (!Factory)
	{
		return false;
	}
	
	QWidget* w = Factory->createWidget(_this);
	if (!w)
	{
		return false;
	}
	
	_this->setWidget(w, Factory->insertMode);
	return true;
}


//============================================================================
void DockWidgetPrivate::setToolBarStyleFromDockManager()
{
	if (!DockManager)
	{
		return;
	}
	auto State = CDockWidget::StateDocked;
	_this->setToolBarIconSize(DockManager->dockWidgetToolBarIconSize(State), State);
	_this->setToolBarStyle(DockManager->dockWidgetToolBarStyle(State), State);
	State = CDockWidget::StateFloating;
	_this->setToolBarIconSize(DockManager->dockWidgetToolBarIconSize(State), State);
	_this->setToolBarStyle(DockManager->dockWidgetToolBarStyle(State), State);
}


//============================================================================
CDockWidget::CDockWidget(const QString &title, QWidget *parent) :
      CDockWidget(nullptr, title, parent)
{
}


//============================================================================
CDockWidget::CDockWidget(CDockManager *manager, const QString &title, QWidget* parent)
	: QFrame(parent),
	  d(new DockWidgetPrivate(this))
{
	d->DockManager = manager;
	d->Layout = new QBoxLayout(QBoxLayout::TopToBottom);
	d->Layout->setContentsMargins(0, 0, 0, 0);
	d->Layout->setSpacing(0);
	setLayout(d->Layout);
	setWindowTitle(title);
	setObjectName(title);

	d->TabWidget = d->componentsFactory()->createDockWidgetTab(this);

	d->ToggleViewAction = new QAction(title, this);
	d->ToggleViewAction->setCheckable(true);
	connect(d->ToggleViewAction, SIGNAL(triggered(bool)), this,
		SLOT(toggleView(bool)));
	setToolbarFloatingStyle(false);

	if (CDockManager::testConfigFlag(CDockManager::FocusHighlighting))
	{
		setFocusPolicy(Qt::ClickFocus);
	}
}


//============================================================================
CDockWidget::~CDockWidget()
{
    ADS_PRINT("~CDockWidget(): " << this->windowTitle());
	delete d;
}

//============================================================================
void CDockWidget::setToggleViewAction(QAction* action)
{
	if (!action)
	{
		return;
	}

	d->ToggleViewAction->setParent(nullptr);
	delete d->ToggleViewAction;
	d->ToggleViewAction = action;
	d->ToggleViewAction->setParent(this);
	connect(d->ToggleViewAction, &QAction::triggered, this, &CDockWidget::toggleView);
}

//============================================================================
void CDockWidget::setToggleViewActionChecked(bool Checked)
{
	QAction* Action = d->ToggleViewAction;
	Action->blockSignals(true);
	Action->setChecked(Checked);
	Action->blockSignals(false);
}


//============================================================================
void CDockWidget::setWidget(QWidget* widget, eInsertMode InsertMode)
{
	if (d->Widget)
	{
		takeWidget();
	}

	auto ScrollAreaWidget = qobject_cast<QAbstractScrollArea*>(widget);
	if (ScrollAreaWidget || ForceNoScrollArea == InsertMode)
	{
		d->Layout->addWidget(widget);
		if (ScrollAreaWidget && ScrollAreaWidget->viewport())
		{
			ScrollAreaWidget->viewport()->setProperty("dockWidgetContent", true);
		}
	}
	else
	{
		d->setupScrollArea();
		d->ScrollArea->setWidget(widget);
	}

	d->Widget = widget;
	d->Widget->setProperty("dockWidgetContent", true);
}

//============================================================================
void CDockWidget::setWidgetFactory(FactoryFunc createWidget, eInsertMode insertMode)
{
	if (d->Factory)
	{
		delete d->Factory;
	}

	d->Factory = new DockWidgetPrivate::WidgetFactory { createWidget, insertMode };
}


//============================================================================
QWidget* CDockWidget::takeWidget()
{
	QWidget* w = nullptr;
	if (d->ScrollArea)
	{
		d->Layout->removeWidget(d->ScrollArea);
		w = d->ScrollArea->takeWidget();
		delete d->ScrollArea;
		d->ScrollArea = nullptr;
		d->Widget = nullptr;
	}
	else if (d->Widget)
	{
		d->Layout->removeWidget(d->Widget);
		w = d->Widget;
		d->Widget = nullptr;
	}

	if (w)
	{
		w->setParent(nullptr);
	}
	return w;
}


//============================================================================
QWidget* CDockWidget::widget() const
{
	return d->Widget;
}


//============================================================================
CDockWidgetTab* CDockWidget::tabWidget() const
{
	return d->TabWidget;
}


//============================================================================
CAutoHideDockContainer* CDockWidget::autoHideDockContainer() const
{
	if (!d->DockArea)
	{
		return nullptr;
	}

	return d->DockArea->autoHideDockContainer();
}


//============================================================================
void CDockWidget::setFeatures(DockWidgetFeatures features)
{
	if (d->Features == features)
	{
		return;
	}
	d->Features = features;
	notifyFeaturesChanged();
}


//============================================================================
void CDockWidget::notifyFeaturesChanged()
{
	Q_EMIT featuresChanged(d->Features);
	d->TabWidget->onDockWidgetFeaturesChanged();
	if(CDockAreaWidget* DockArea = dockAreaWidget())
	{
		DockArea->onDockWidgetFeaturesChanged();
	}
}


//============================================================================
void CDockWidget::setFeature(DockWidgetFeature flag, bool on)
{
	auto Features = features();
	internal::setFlag(Features, flag, on);
	setFeatures(Features);
}


//============================================================================
CDockWidget::DockWidgetFeatures CDockWidget::features() const
{
	if (d->DockManager)
	{
		return d->Features &~ d->DockManager->globallyLockedDockWidgetFeatures();
	}
	else
	{
		return d->Features;
	}
}


//============================================================================
CDockManager* CDockWidget::dockManager() const
{
	return d->DockManager;
}


//============================================================================
void CDockWidget::setDockManager(CDockManager* DockManager)
{
	d->DockManager = DockManager;
	if (!DockManager)
	{
		return;
	}

	if (ToolBarStyleFromDockManager == d->ToolBarStyleSource)
	{
		d->setToolBarStyleFromDockManager();
	}
}


//============================================================================
CDockContainerWidget* CDockWidget::dockContainer() const
{
	if (d->DockArea)
	{
		return d->DockArea->dockContainer();
	}
	else
	{
		return nullptr;
	}
}


//============================================================================
CFloatingDockContainer* CDockWidget::floatingDockContainer() const
{
	auto DockContainer = dockContainer();
	return DockContainer ? DockContainer->floatingWidget() : nullptr;
}


//============================================================================
CDockAreaWidget* CDockWidget::dockAreaWidget() const
{
	return d->DockArea;
}

//============================================================================
CAutoHideTab* CDockWidget::sideTabWidget() const
{
	return d->SideTabWidget;
}


//============================================================================
void CDockWidget::setSideTabWidget(CAutoHideTab* SideTab) const
{
	d->SideTabWidget = SideTab;
}


//============================================================================
bool CDockWidget::isAutoHide() const
{
	return !d->SideTabWidget.isNull();
}


//============================================================================
SideBarLocation CDockWidget::autoHideLocation() const
{
	return isAutoHide() ? autoHideDockContainer()->sideBarLocation() : SideBarNone;
}


//============================================================================
bool CDockWidget::isFloating() const
{
	if (!isInFloatingContainer())
	{
		return false;
	}

	return dockContainer()->topLevelDockWidget() == this;
}


//============================================================================
bool CDockWidget::isInFloatingContainer() const
{
	auto Container = dockContainer();
	if (!Container)
	{
		return false;
	}

	if (!Container->isFloating())
	{
		return false;
	}

	return true;
}


//============================================================================
bool CDockWidget::isClosed() const
{
	return d->Closed;
}


//============================================================================
QAction* CDockWidget::toggleViewAction() const
{
	return d->ToggleViewAction;
}


//============================================================================
void CDockWidget::setToggleViewActionMode(eToggleViewActionMode Mode)
{
	if (ActionModeToggle == Mode)
	{
		d->ToggleViewAction->setCheckable(true);
		d->ToggleViewAction->setIcon(QIcon());
	}
	else
	{
		d->ToggleViewAction->setCheckable(false);
		d->ToggleViewAction->setIcon(d->TabWidget->icon());
	}
}


//============================================================================
void CDockWidget::setMinimumSizeHintMode(eMinimumSizeHintMode Mode)
{
	d->MinimumSizeHintMode = Mode;
}


//============================================================================
CDockWidget::eMinimumSizeHintMode CDockWidget::minimumSizeHintMode() const
{
	return d->MinimumSizeHintMode;
}


//============================================================================
bool CDockWidget::isCentralWidget() const
{
	return dockManager()->centralWidget() == this;
}


//============================================================================
void CDockWidget::toggleView(bool Open)
{
	// If the toggle view action mode is ActionModeShow, then Open is always
	// true if the sender is the toggle view action
	QAction* Sender = qobject_cast<QAction*>(sender());
	if (Sender == d->ToggleViewAction && !d->ToggleViewAction->isCheckable())
	{
		Open = true;
	}

	// If the dock widget state is different, then we really need to toggle
	// the state. If we are in the right state, then we simply make this
	// dock widget the current dock widget
	auto AutoHideContainer = autoHideDockContainer();
	if (d->Closed != !Open)
	{
		toggleViewInternal(Open);
	}
	else if (Open && d->DockArea && !AutoHideContainer)
	{
		raise();
	}

	if (Open && AutoHideContainer)
	{
		AutoHideContainer->collapseView(false);
	}
}


//============================================================================
void CDockWidget::toggleViewInternal(bool Open)
{
	CDockContainerWidget* DockContainer = dockContainer();
	CDockWidget* TopLevelDockWidgetBefore = DockContainer
		? DockContainer->topLevelDockWidget() : nullptr;

	d->Closed = !Open;

	if (Open)
	{
		d->showDockWidget();
	}
	else
	{
		d->hideDockWidget();
	}

	d->ToggleViewAction->blockSignals(true);
	d->ToggleViewAction->setChecked(Open);
	d->ToggleViewAction->blockSignals(false);
	if (d->DockArea)
	{
		d->DockArea->toggleDockWidgetView(this, Open);
		if (d->DockArea->isAutoHide())
		{
			d->DockArea->autoHideDockContainer()->toggleView(Open);
		}
	}

	if (Open && TopLevelDockWidgetBefore)
	{
		CDockWidget::emitTopLevelEventForWidget(TopLevelDockWidgetBefore, false);
	}

	// Here we need to call the dockContainer() function again, because if
	// this dock widget was unassigned before the call to showDockWidget() then
	// it has a dock container now
	DockContainer = dockContainer();
	CDockWidget* TopLevelDockWidgetAfter = DockContainer
		? DockContainer->topLevelDockWidget() : nullptr;
	CDockWidget::emitTopLevelEventForWidget(TopLevelDockWidgetAfter, true);
	CFloatingDockContainer* FloatingContainer = DockContainer
		? DockContainer->floatingWidget() : nullptr;
	if (FloatingContainer)
	{
		FloatingContainer->updateWindowTitle();
	}

	if (!Open)
	{
		Q_EMIT closed();
	}
	Q_EMIT viewToggled(Open);
}


//============================================================================
void CDockWidget::setDockArea(CDockAreaWidget* DockArea)
{
	d->DockArea = DockArea;
	d->ToggleViewAction->setChecked(DockArea != nullptr && !this->isClosed());
	setParent(DockArea);
}


//============================================================================
void CDockWidget::saveState(QXmlStreamWriter& s) const
{
	s.writeStartElement("Widget");
	s.writeAttribute("Name", objectName());
	s.writeAttribute("Closed", QString::number(d->Closed ? 1 : 0));
	s.writeEndElement();
}


//============================================================================
void CDockWidget::flagAsUnassigned()
{
	d->Closed = true;
	setVisible(false);
	setDockArea(nullptr);
	setParent(d->DockManager);
	tabWidget()->setParent(this);
}


//============================================================================
bool CDockWidget::event(QEvent *e)
{
	switch (e->type())
	{
	case QEvent::Hide:
		Q_EMIT visibilityChanged(false);
		break;

	case QEvent::Show:
		Q_EMIT visibilityChanged(geometry().right() >= 0 && geometry().bottom() >= 0);
		break;

	case QEvent::WindowTitleChange :
		{
			const auto title = windowTitle();
			if (d->TabWidget)
			{
				d->TabWidget->setText(title);
			}
			if (d->SideTabWidget)
			{
				d->SideTabWidget->setText(title);
			}
			if (d->ToggleViewAction)
			{
				d->ToggleViewAction->setText(title);
			}
			if (d->DockArea)
			{
				d->DockArea->markTitleBarMenuOutdated();//update tabs menu
			}

			auto FloatingWidget = floatingDockContainer();
			if (FloatingWidget)
			{
				FloatingWidget->updateWindowTitle();
			}
			Q_EMIT titleChanged(title);
		}
		break;

	default:
		break;
	}

	return Super::event(e);
}


#ifndef QT_NO_TOOLTIP
//============================================================================
void CDockWidget::setTabToolTip(const QString &text)
{
	if (d->TabWidget)
	{
		d->TabWidget->setToolTip(text);
	}
	if (d->ToggleViewAction)
	{
		d->ToggleViewAction->setToolTip(text);
	}
	if (d->DockArea)
	{
		d->DockArea->markTitleBarMenuOutdated();//update tabs menu
	}
}
#endif


//============================================================================
void CDockWidget::setIcon(const QIcon& Icon)
{
	d->TabWidget->setIcon(Icon);

	if (d->SideTabWidget)
	{
		d->SideTabWidget->setIcon(Icon);
	}

	if (!d->ToggleViewAction->isCheckable())
	{
		d->ToggleViewAction->setIcon(Icon);
	}
}


//============================================================================
QIcon CDockWidget::icon() const
{
	return d->TabWidget->icon();
}


//============================================================================
QToolBar* CDockWidget::toolBar() const
{
	return d->ToolBar;
}


//============================================================================
QToolBar* CDockWidget::createDefaultToolBar()
{
	if (!d->ToolBar)
	{
		d->setupToolBar();
	}

	return d->ToolBar;
}


//============================================================================
void CDockWidget::setToolBar(QToolBar* ToolBar)
{
	if (d->ToolBar)
	{
		delete d->ToolBar;
	}

	d->ToolBar = ToolBar;
	d->Layout->insertWidget(0, d->ToolBar);
	this->connect(this, SIGNAL(topLevelChanged(bool)), SLOT(setToolbarFloatingStyle(bool)));
	setToolbarFloatingStyle(isFloating());
}


//============================================================================
void CDockWidget::setToolBarStyle(Qt::ToolButtonStyle Style, eState State)
{
	if (StateFloating == State)
	{
		d->ToolBarStyleFloating = Style;
	}
	else
	{
		d->ToolBarStyleDocked = Style;
	}

	setToolbarFloatingStyle(isFloating());
}


//============================================================================
Qt::ToolButtonStyle CDockWidget::toolBarStyle(eState State) const
{
	if (StateFloating == State)
	{
		return d->ToolBarStyleFloating;
	}
	else
	{
		return d->ToolBarStyleDocked;
	}
}


//============================================================================
void CDockWidget::setToolBarIconSize(const QSize& IconSize, eState State)
{
	if (StateFloating == State)
	{
		d->ToolBarIconSizeFloating = IconSize;
	}
	else
	{
		d->ToolBarIconSizeDocked = IconSize;
	}

	setToolbarFloatingStyle(isFloating());
}


//============================================================================
QSize CDockWidget::toolBarIconSize(eState State) const
{
	if (StateFloating == State)
	{
		return d->ToolBarIconSizeFloating;
	}
	else
	{
		return d->ToolBarIconSizeDocked;
	}
}


//============================================================================
void CDockWidget::setToolbarFloatingStyle(bool Floating)
{
	if (!d->ToolBar)
	{
		return;
	}

	auto IconSize = Floating ? d->ToolBarIconSizeFloating : d->ToolBarIconSizeDocked;
	if (IconSize != d->ToolBar->iconSize())
	{
		d->ToolBar->setIconSize(IconSize);
	}

	auto ButtonStyle = Floating ? d->ToolBarStyleFloating : d->ToolBarStyleDocked;
	if (ButtonStyle != d->ToolBar->toolButtonStyle())
	{
		d->ToolBar->setToolButtonStyle(ButtonStyle);
	}
}


//============================================================================
void CDockWidget::emitTopLevelEventForWidget(CDockWidget* TopLevelDockWidget, bool Floating)
{
	if (TopLevelDockWidget)
	{
		TopLevelDockWidget->dockAreaWidget()->updateTitleBarVisibility();
		TopLevelDockWidget->emitTopLevelChanged(Floating);
	}
}


//============================================================================
void CDockWidget::emitTopLevelChanged(bool Floating)
{
	if (Floating != d->IsFloatingTopLevel)
	{
		d->IsFloatingTopLevel = Floating;
		Q_EMIT topLevelChanged(d->IsFloatingTopLevel);
	}
}


//============================================================================
void CDockWidget::setClosedState(bool Closed)
{
	d->Closed = Closed;
}


//============================================================================
QSize CDockWidget::minimumSizeHint() const
{
	if (!d->Widget)
	{
		return QSize(60, 40);
	}

	switch (d->MinimumSizeHintMode)
	{
		case MinimumSizeHintFromDockWidget: return QSize(60, 40);
    	case MinimumSizeHintFromContent: return d->Widget->minimumSizeHint();
    	case MinimumSizeHintFromDockWidgetMinimumSize: return minimumSize();
    	case MinimumSizeHintFromContentMinimumSize: return d->Widget->minimumSize();
	}

	return d->Widget->minimumSizeHint();
}


//============================================================================
void CDockWidget::setFloating()
{
	if (isClosed())
	{
		return;
	}

	if (this->isAutoHide())
	{
		dockAreaWidget()->setFloating();
	}
	else
	{
		d->TabWidget->detachDockWidget();
	}
}


//============================================================================
void CDockWidget::deleteDockWidget()
{
	auto manager=dockManager();
	if(manager){
		manager->removeDockWidget(this);
	}
	deleteLater();
	d->Closed = true;
}


//============================================================================
void CDockWidget::closeDockWidget()
{
	closeDockWidgetInternal(true);
}



//============================================================================
void CDockWidget::requestCloseDockWidget()
{
    if (features().testFlag(CDockWidget::DockWidgetDeleteOnClose)
     || features().testFlag(CDockWidget::CustomCloseHandling))
    {
    	closeDockWidgetInternal(false);
    }
    else
    {
    	toggleView(false);
    }
}


//============================================================================
bool CDockWidget::closeDockWidgetInternal(bool ForceClose)
{
	if (!ForceClose)
	{
		Q_EMIT closeRequested();
	}

	if (!ForceClose && features().testFlag(CDockWidget::CustomCloseHandling))
	{
		return false;
	}

	if (features().testFlag(CDockWidget::DockWidgetDeleteOnClose))
	{
		// If the dock widget is floating, then we check if we also need to
		// delete the floating widget
		if (isFloating())
		{
			CFloatingDockContainer* FloatingWidget = internal::findParent<
					CFloatingDockContainer*>(this);
			if (FloatingWidget->dockWidgets().count() == 1)
			{
				FloatingWidget->deleteLater();
			}
			else
			{
				FloatingWidget->hide();
			}
		}
		if (d->DockArea && d->DockArea->isAutoHide())
		{
			d->DockArea->autoHideDockContainer()->cleanupAndDelete();
		}
		deleteDockWidget();
		Q_EMIT closed();
	}
	else
	{
		toggleView(false);
	}

	return true;
}


//============================================================================
void CDockWidget::setTitleBarActions(QList<QAction*> actions)
{
	d->TitleBarActions = actions;
}


//============================================================================
QList<QAction*> CDockWidget::titleBarActions() const
{
	return d->TitleBarActions;
}


//============================================================================
void CDockWidget::showFullScreen()
{
	if (isFloating())
	{
		dockContainer()->floatingWidget()->showFullScreen();
	}
	else
	{
		Super::showFullScreen();
	}
}


//============================================================================
void CDockWidget::showNormal()
{
	if (isFloating())
	{
		dockContainer()->floatingWidget()->showNormal();
	}
	else
	{
		Super::showNormal();
	}
}


//============================================================================
bool CDockWidget::isFullScreen() const
{
	if (isFloating())
	{
		return dockContainer()->floatingWidget()->isFullScreen();
	}
	else
	{
		return Super::isFullScreen();
	}
}


//============================================================================
void CDockWidget::setAsCurrentTab()
{
	if (d->DockArea && !isClosed())
	{
		d->DockArea->setCurrentDockWidget(this);
	}
}


//============================================================================
bool CDockWidget::isTabbed() const
{
	return d->DockArea && (d->DockArea->openDockWidgetsCount() > 1);
}



//============================================================================
bool CDockWidget::isCurrentTab() const
{
	return d->DockArea && (d->DockArea->currentDockWidget() == this);
}


//============================================================================
void CDockWidget::raise()
{
	if (isClosed())
	{
		return;
	}

	setAsCurrentTab();
	if (isInFloatingContainer())
	{
		auto FloatingWindow = window();
		FloatingWindow->raise();
		FloatingWindow->activateWindow();
	}
}


//============================================================================
void CDockWidget::setAutoHide(bool Enable, SideBarLocation Location, int TabIndex)
{
	if (!CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideFeatureEnabled))
	{
		return;
	}

	// Do nothing if nothing changes
	if (Enable == isAutoHide() && Location == autoHideLocation())
	{
		return;
	}

	auto DockArea = dockAreaWidget();

	if (!Enable)
	{
		DockArea->setAutoHide(false);
	}
	else if (isAutoHide())
	{
		autoHideDockContainer()->moveToNewSideBarLocation(Location);
	}
	else
	{
		auto area = (SideBarNone == Location) ? DockArea->calculateSideTabBarArea() : Location;
		dockContainer()->createAndSetupAutoHideContainer(area, this, TabIndex);
	}
}


//============================================================================
void CDockWidget::toggleAutoHide(SideBarLocation Location)
{
	if (!CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideFeatureEnabled))
	{
		return;
	}

	setAutoHide(!isAutoHide(), Location);
}


//============================================================================
void CDockWidget::setToolBarStyleSource(eToolBarStyleSource Source)
{
	d->ToolBarStyleSource = Source;
	if (ToolBarStyleFromDockManager == d->ToolBarStyleSource)
	{
		d->setToolBarStyleFromDockManager();
	}
}


//============================================================================
CDockWidget::eToolBarStyleSource CDockWidget::toolBarStyleSource() const
{
	return d->ToolBarStyleSource;
}


} // namespace ads

//---------------------------------------------------------------------------
// EOF DockWidget.cpp
