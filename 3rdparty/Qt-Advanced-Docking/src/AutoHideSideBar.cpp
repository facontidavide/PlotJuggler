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
/// \file   AutoHideSideBar.cpp
/// \author Syarif Fakhri
/// \date   05.09.2022
/// \brief  Implementation of CAutoHideSideBar class
//============================================================================


//============================================================================
//                                   INCLUDES
//============================================================================
#include "AutoHideSideBar.h"

#include <QBoxLayout>
#include <QStyleOption>
#include <QPainter>
#include <QXmlStreamWriter>

#include "DockContainerWidget.h"
#include "DockWidgetTab.h"
#include "DockFocusController.h"
#include "AutoHideDockContainer.h"
#include "DockAreaWidget.h"
#include "DockingStateReader.h"
#include "AutoHideTab.h"

namespace ads
{
class CTabsWidget;

/**
 * Private data class of CSideTabBar class (pimpl)
 */
struct AutoHideSideBarPrivate
{
	/**
	 * Private data constructor
	 */
	AutoHideSideBarPrivate(CAutoHideSideBar* _public);

    CAutoHideSideBar* _this;
    CDockContainerWidget* ContainerWidget;
    CTabsWidget* TabsContainerWidget;
    QBoxLayout* TabsLayout;
    Qt::Orientation Orientation;
    SideBarLocation SideTabArea = SideBarLocation::SideBarLeft;

    /**
     * Convenience function to check if this is a horizontal side bar
     */
    bool isHorizontal() const
    {
    	return Qt::Horizontal == Orientation;
    }

    /**
     * Called from viewport to forward event handling to this
     */
    void handleViewportEvent(QEvent* e);
}; // struct AutoHideSideBarPrivate


/**
 * This widget stores the tab buttons
 */
class CTabsWidget : public QWidget
{
public:
	using QWidget::QWidget;
	using Super = QWidget;
	AutoHideSideBarPrivate* EventHandler;

	/**
	 * Returns the size hint as minimum size hint
	 */
	virtual QSize minimumSizeHint() const override
	{
		return Super::sizeHint();
	}

	/**
	 * Forward event handling to EventHandler
	 */
	virtual bool event(QEvent* e) override
	{
		EventHandler->handleViewportEvent(e);
		return Super::event(e);
	}
};


//============================================================================
AutoHideSideBarPrivate::AutoHideSideBarPrivate(CAutoHideSideBar* _public) :
    _this(_public)
{
}


//============================================================================
void AutoHideSideBarPrivate::handleViewportEvent(QEvent* e)
{
	switch (e->type())
	{
	case QEvent::ChildRemoved:
		if (TabsLayout->isEmpty())
		{
			_this->hide();
		}
		break;

	default:
		break;
	}
}


//============================================================================
CAutoHideSideBar::CAutoHideSideBar(CDockContainerWidget* parent, SideBarLocation area) :
    Super(parent),
    d(new AutoHideSideBarPrivate(this))
{
	d->SideTabArea = area;
    d->ContainerWidget = parent;
    d->Orientation = (area == SideBarLocation::SideBarBottom || area == SideBarLocation::SideBarTop)
    	? Qt::Horizontal : Qt::Vertical;

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setFrameStyle(QFrame::NoFrame);
	setWidgetResizable(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	d->TabsContainerWidget = new CTabsWidget();
	d->TabsContainerWidget->EventHandler = d;
	d->TabsContainerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	d->TabsContainerWidget->setObjectName("sideTabsContainerWidget");


    d->TabsLayout = new QBoxLayout(d->Orientation == Qt::Vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    d->TabsLayout->setContentsMargins(0, 0, 0, 0);
    d->TabsLayout->setSpacing(12);
    d->TabsLayout->addStretch(1);
    d->TabsContainerWidget->setLayout(d->TabsLayout);
	setWidget(d->TabsContainerWidget);

    setFocusPolicy(Qt::NoFocus);
	if (d->isHorizontal())
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	}
	else
	{
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	}

	hide();
}


//============================================================================
CAutoHideSideBar::~CAutoHideSideBar() 
{
	ADS_PRINT("~CSideTabBar()");
	// The SideTabeBar is not the owner of the tabs and to prevent deletion
	// we set the parent here to nullptr to remove it from the children
	auto Tabs = findChildren<CAutoHideTab*>(QString(), Qt::FindDirectChildrenOnly);
	for (auto Tab : Tabs)
	{
		Tab->setParent(nullptr);
	}
	delete d;
}


//============================================================================
void CAutoHideSideBar::insertTab(int Index, CAutoHideTab* SideTab)
{
    SideTab->setSideBar(this);
	SideTab->installEventFilter(this);
	// Default insertion is append
    if (Index < 0)
    {
    	d->TabsLayout->insertWidget(d->TabsLayout->count() - 1, SideTab);
    }
    else
    {
    	d->TabsLayout->insertWidget(Index, SideTab);
    }
    show();
}


//============================================================================
CAutoHideDockContainer* CAutoHideSideBar::insertDockWidget(int Index, CDockWidget* DockWidget)
{
	auto AutoHideContainer = new CAutoHideDockContainer(DockWidget, d->SideTabArea, d->ContainerWidget);
	DockWidget->dockManager()->dockFocusController()->clearDockWidgetFocus(DockWidget);
	auto Tab = AutoHideContainer->autoHideTab();
	DockWidget->setSideTabWidget(Tab);
	insertTab(Index, Tab);
	return AutoHideContainer;
}


//============================================================================
void CAutoHideSideBar::removeAutoHideWidget(CAutoHideDockContainer* AutoHideWidget)
{
	AutoHideWidget->autoHideTab()->removeFromSideBar();
	auto DockContainer = AutoHideWidget->dockContainer();
	if (DockContainer)
	{
		DockContainer->removeAutoHideWidget(AutoHideWidget);
	}
	AutoHideWidget->setParent(nullptr);
}

//============================================================================
void CAutoHideSideBar::addAutoHideWidget(CAutoHideDockContainer* AutoHideWidget,
	int TabIndex)
{
	auto SideBar = AutoHideWidget->autoHideTab()->sideBar();
	if (SideBar == this)
	{
		// If we move to the same tab index or if we insert before the next
		// tab index, then we will end at the same tab position and can leave
		if (AutoHideWidget->tabIndex() == TabIndex || (AutoHideWidget->tabIndex() + 1) == TabIndex)
		{
			return;
		}

		// We remove this auto hide widget from the sidebar in the code below
		// and therefore need to correct the TabIndex here
		if (AutoHideWidget->tabIndex() < TabIndex)
		{
			--TabIndex;
		}
	}

	if (SideBar)
	{
		SideBar->removeAutoHideWidget(AutoHideWidget);
	}
	AutoHideWidget->setParent(d->ContainerWidget);
	AutoHideWidget->setSideBarLocation(d->SideTabArea);
	d->ContainerWidget->registerAutoHideWidget(AutoHideWidget);
	insertTab(TabIndex, AutoHideWidget->autoHideTab());
}


//============================================================================
void CAutoHideSideBar::removeTab(CAutoHideTab* SideTab)
{
	SideTab->removeEventFilter(this);
    d->TabsLayout->removeWidget(SideTab);
    if (d->TabsLayout->isEmpty())
    {
    	hide();
    }
}


//============================================================================
bool CAutoHideSideBar::eventFilter(QObject *watched, QEvent *event)
{
	auto Tab = qobject_cast<CAutoHideTab*>(watched);
	if (!Tab)
	{
		return false;
	}

	switch (event->type())
	{
	case QEvent::ShowToParent:
		 show();
	     break;

	case QEvent::HideToParent:
		 if (!hasVisibleTabs())
		 {
			 hide();
		 }
		 break;

	default:
		break;
	}

	return false;
}


//============================================================================
Qt::Orientation CAutoHideSideBar::orientation() const
{
    return d->Orientation;
}


//============================================================================
CAutoHideTab* CAutoHideSideBar::tab(int index) const
{
    return qobject_cast<CAutoHideTab*>(d->TabsLayout->itemAt(index)->widget());
}


//============================================================================
int CAutoHideSideBar::count() const
{
    return d->TabsLayout->count() - 1;
}


//============================================================================
int CAutoHideSideBar::visibleTabCount() const
{
	int VisibleTabCount = 0;
	auto ParentWidget = parentWidget();
	for (auto i = 0; i < count(); i++)
	{
		if (tab(i)->isVisibleTo(ParentWidget))
		{
			VisibleTabCount++;
		}
	}

	return VisibleTabCount;
}


//============================================================================
bool CAutoHideSideBar::hasVisibleTabs() const
{
	auto ParentWidget = parentWidget();
	for (auto i = 0; i < count(); i++)
	{
		if (tab(i)->isVisibleTo(ParentWidget))
		{
			return true;
		}
	}

	return false;
}


//============================================================================
int CAutoHideSideBar::indexOfTab(const CAutoHideTab& Tab) const
{
	for (auto i = 0; i < count(); i++)
	{
		if (tab(i) == &Tab)
		{
			return i;
		}
	}

	return -1;
}


//============================================================================
SideBarLocation CAutoHideSideBar::sideBarLocation() const
{
	return d->SideTabArea;
}


//============================================================================
void CAutoHideSideBar::saveState(QXmlStreamWriter& s) const
{
	if (!count())
	{
		return;
	}

	s.writeStartElement("SideBar");
	s.writeAttribute("Area", QString::number(sideBarLocation()));
	s.writeAttribute("Tabs", QString::number(count()));

	for (auto i = 0; i < count(); ++i)
	{
		auto Tab = tab(i);
		if (!Tab)
		{
			continue;
		}

		Tab->dockWidget()->autoHideDockContainer()->saveState(s);
	}

	s.writeEndElement();
}

//===========================================================================
QSize CAutoHideSideBar::minimumSizeHint() const
{
	QSize Size = sizeHint();
	if (d->isHorizontal())
	{
		Size.setWidth(0);
	}
	else
	{
		Size.setHeight(0);
	}
	return Size;
}


//===========================================================================
QSize CAutoHideSideBar::sizeHint() const
{
	return d->TabsContainerWidget->sizeHint();
}


//===========================================================================
int CAutoHideSideBar::spacing() const
{
	return d->TabsLayout->spacing();
}


//===========================================================================
void CAutoHideSideBar::setSpacing(int Spacing)
{
	d->TabsLayout->setSpacing(Spacing);
}


//===========================================================================
CDockContainerWidget* CAutoHideSideBar::dockContainer() const
{
	return d->ContainerWidget;
}


//===========================================================================
int CAutoHideSideBar::tabAt(const QPoint& Pos) const
{
	if (!isVisible())
	{
		return TabInvalidIndex;
	}

	if (orientation() == Qt::Horizontal)
	{
		if (Pos.x() < tab(0)->geometry().x())
		{
			return -1;
		}
	}
	else
	{
		if (Pos.y() < tab(0)->geometry().y())
		{
			return -1;
		}
	}


	for (int i = 0; i < count(); ++i)
	{
		if (tab(i)->geometry().contains(Pos))
		{
			return i;
		}
	}

	return count();
}


//===========================================================================
int CAutoHideSideBar::tabInsertIndexAt(const QPoint& Pos) const
{
	int Index = tabAt(Pos);
	if (Index == TabInvalidIndex)
	{
		return TabDefaultInsertIndex;
	}
	else
	{
		return (Index < 0) ? 0 : Index;
	}
}

} // namespace ads

