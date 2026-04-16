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
/// \file   DockContainerWidget.cpp
/// \author Uwe Kindler
/// \date   24.02.2017
/// \brief  Implementation of CDockContainerWidget class
//============================================================================


//============================================================================
//                                   INCLUDES
//============================================================================
#include "DockContainerWidget.h"

#include <QEvent>
#include <QList>
#include <QGridLayout>
#include <QPointer>
#include <QVariant>
#include <QDebug>
#include <QXmlStreamWriter>
#include <QAbstractButton>
#include <QLabel>
#include <QTimer>
#include <QMetaObject>
#include <QMetaType>
#include <QApplication>

#include "DockManager.h"
#include "DockAreaWidget.h"
#include "DockWidget.h"
#include "DockingStateReader.h"
#include "FloatingDockContainer.h"
#include "DockOverlay.h"
#include "ads_globals.h"
#include "DockSplitter.h"
#include "DockWidgetTab.h"
#include "DockAreaTitleBar.h"
#include "DockFocusController.h"
#include "AutoHideDockContainer.h"
#include "AutoHideSideBar.h"
#include "AutoHideTab.h"

#include <functional>
#include <iostream>

#if QT_VERSION < 0x050900

inline char toHexLower(uint value)
{
    return "0123456789abcdef"[value & 0xF];
}

QByteArray qByteArrayToHex(const QByteArray& src, char separator)
{
    if(src.size() == 0)
        return QByteArray();

    const int length = separator ? (src.size() * 3 - 1) : (src.size() * 2);
    QByteArray hex(length, Qt::Uninitialized);
    char *hexData = hex.data();
    const uchar *data = reinterpret_cast<const uchar *>(src.data());
    for (int i = 0, o = 0; i < src.size(); ++i) {
        hexData[o++] = toHexLower(data[i] >> 4);
        hexData[o++] = toHexLower(data[i] & 0xf);

        if ((separator) && (o < length))
            hexData[o++] = separator;
    }
    return hex;
}
#endif

namespace ads
{
static unsigned int zOrderCounter = 0;

enum eDropMode
{
	DropModeIntoArea,///< drop widget into a dock area
	DropModeIntoContainer,///< drop into container
	DropModeInvalid///< invalid mode - do not drop
};

/**
 * Converts dock area ID to an index for array access
 */
static int areaIdToIndex(DockWidgetArea area)
{
	switch (area)
	{
	case LeftDockWidgetArea: return 0;
	case RightDockWidgetArea: return 1;
	case TopDockWidgetArea: return 2;
	case BottomDockWidgetArea: return 3;
	case CenterDockWidgetArea: return 4;
	default:
		return 4;
	}
}

/**
 * Helper function to ease insertion of dock area into splitter
 */
static void insertWidgetIntoSplitter(QSplitter* Splitter, QWidget* widget, bool Append)
{
	if (Append)
	{
		Splitter->addWidget(widget);
	}
	else
	{
		Splitter->insertWidget(0, widget);
	}
}

/**
 * Private data class of CDockContainerWidget class (pimpl)
 */
class DockContainerWidgetPrivate
{
public:
	CDockContainerWidget* _this;
	QPointer<CDockManager> DockManager;
	unsigned int zOrderIndex = 0;
	QList<QPointer<CDockAreaWidget>> DockAreas;
	QList<CAutoHideDockContainer*> AutoHideWidgets;
	QMap<SideBarLocation, CAutoHideSideBar*> SideTabBarWidgets;
	QGridLayout* Layout = nullptr;
	CDockSplitter* RootSplitter = nullptr;
	bool isFloating = false;
	CDockAreaWidget* LastAddedAreaCache[5];
	int VisibleDockAreaCount = -1;
	CDockAreaWidget* TopLevelDockArea = nullptr;
	QTimer DelayedAutoHideTimer;
	CAutoHideTab* DelayedAutoHideTab;
	bool DelayedAutoHideShow = false;

	/**
	 * Private data constructor
	 */
	DockContainerWidgetPrivate(CDockContainerWidget* _public);

	/**
	 * Adds dock widget to container and returns the dock area that contains
	 * the inserted dock widget
	 */
	CDockAreaWidget* addDockWidgetToContainer(DockWidgetArea area, CDockWidget* Dockwidget);

	/**
	 * Adds dock widget to a existing DockWidgetArea
	 */
	CDockAreaWidget* addDockWidgetToDockArea(DockWidgetArea area, CDockWidget* Dockwidget,
		CDockAreaWidget* TargetDockArea, int Index = -1);

	/**
	 * Add dock area to this container
	 */
	void addDockArea(CDockAreaWidget* NewDockWidget, DockWidgetArea area = CenterDockWidgetArea);

	/**
	 * Drop floating widget into container
	 */
	void dropIntoContainer(CFloatingDockContainer* FloatingWidget, DockWidgetArea area);

	/**
	 * Drop floating widget into auto hide side bar
	 */
	void dropIntoAutoHideSideBar(CFloatingDockContainer* FloatingWidget, DockWidgetArea area);

	/**
	 * Creates a new tab for a widget dropped into the center of a section
	 */
	void dropIntoCenterOfSection(CFloatingDockContainer* FloatingWidget,
		CDockAreaWidget* TargetArea, int TabIndex = 0);

	/**
	 * Drop floating widget into dock area
	 */
	void dropIntoSection(CFloatingDockContainer* FloatingWidget,
		CDockAreaWidget* TargetArea, DockWidgetArea area, int TabIndex = 0);

	/**
	 * Moves the dock widget or dock area given in Widget parameter to a
	 * new dock widget area
	 */
	void moveToNewSection(QWidget* Widget, CDockAreaWidget* TargetArea, DockWidgetArea area,
		int TabIndex = 0);

	/**
	 * Moves the dock widget or dock area given in Widget parameter to a
	 * a dock area in container
	 */
	void moveToContainer(QWidget* Widgett, DockWidgetArea area);

	/**
	 * Creates a new tab for a widget dropped into the center of a section
	 */
	void moveIntoCenterOfSection(QWidget* Widget, CDockAreaWidget* TargetArea, int TabIndex = 0);

	/**
	 * Moves the dock widget or dock area given in Widget parameter to
	 * a auto hide sidebar area
	 */
	void moveToAutoHideSideBar(QWidget* Widget, DockWidgetArea area, int TabIndex = TabDefaultInsertIndex);


	/**
	 * Adds new dock areas to the internal dock area list
	 */
	void addDockAreasToList(const QList<CDockAreaWidget*> NewDockAreas);

	/**
	 * Wrapper function for DockAreas append, that ensures that dock area signals
	 * are properly connected to dock container slots
	 */
	void appendDockAreas(const QList<CDockAreaWidget*> NewDockAreas);

	/**
	 * Save state of child nodes
	 */
	void saveChildNodesState(QXmlStreamWriter& Stream, QWidget* Widget);

	/**
	 * Save state of auto hide widgets
	 */
    void saveAutoHideWidgetsState(QXmlStreamWriter& Stream);

    /**
	 * Restore state of child nodes.
	 * \param[in] Stream The data stream that contains the serialized state
	 * \param[out] CreatedWidget The widget created from parsed data or 0 if
	 * the parsed widget was an empty splitter
	 * \param[in] Testing If Testing is true, only the stream data is
	 * parsed without modifying anything.
	 */
	bool restoreChildNodes(CDockingStateReader& Stream, QWidget*& CreatedWidget,
		bool Testing);

	/**
	 * Restores a splitter.
	 * \see restoreChildNodes() for details
	 */
	bool restoreSplitter(CDockingStateReader& Stream, QWidget*& CreatedWidget,
		bool Testing);

	/**
	 * Restores a dock area.
	 * \see restoreChildNodes() for details
	 */
	bool restoreDockArea(CDockingStateReader& Stream, QWidget*& CreatedWidget,
		bool Testing);

    /**
     * Restores a auto hide side bar
     */
    bool restoreSideBar(CDockingStateReader& Stream, QWidget*& CreatedWidget,
        bool Testing);

	/**
	 * Helper function for recursive dumping of layout
	 */
	void dumpRecursive(int level, QWidget* widget);

	/**
	 * Calculate the drop mode from the given target position
	 */
	eDropMode getDropMode(const QPoint& TargetPos);

	/**
	 * Initializes the visible dock area count variable if it is not initialized
	 * yet
	 */
	void initVisibleDockAreaCount()
	{
		if (VisibleDockAreaCount > -1)
		{
			return;
		}

		VisibleDockAreaCount = 0;
		for (auto DockArea : DockAreas)
		{
			if (!DockArea)
			{
				continue;
			}
			VisibleDockAreaCount += (DockArea->isHidden() ? 0 : 1);
		}
	}

	/**
	 * Access function for the visible dock area counter
	 */
	int& visibleDockAreaCount()
	{
		// Lazy initialisation - we initialize the VisibleDockAreaCount variable
		// on first use
		initVisibleDockAreaCount();
		return VisibleDockAreaCount;
	}

	/**
	 * The visible dock area count changes, if dock areas are remove, added or
	 * when its view is toggled
	 */
	void onVisibleDockAreaCountChanged();

	void emitDockAreasRemoved()
	{
		onVisibleDockAreaCountChanged();
		Q_EMIT _this->dockAreasRemoved();
	}

	void emitDockAreasAdded()
	{
		onVisibleDockAreaCountChanged();
		Q_EMIT _this->dockAreasAdded();
	}

	/**
	 * Helper function for creation of new splitter
	 */
	CDockSplitter* newSplitter(Qt::Orientation orientation, QWidget* parent = nullptr)
	{
		CDockSplitter* s = new CDockSplitter(orientation, parent);
		s->setOpaqueResize(CDockManager::testConfigFlag(CDockManager::OpaqueSplitterResize));
		s->setChildrenCollapsible(false);
		return s;
	}

	/**
	 * Ensures equal distribution of the sizes of a splitter if an dock widget
	 * is inserted from code
	 */
	void adjustSplitterSizesOnInsertion(QSplitter* Splitter, qreal LastRatio = 1.0)
	{
		int AreaSize = (Splitter->orientation() == Qt::Horizontal) ? Splitter->width() : Splitter->height();
		auto SplitterSizes = Splitter->sizes();

		qreal TotRatio = SplitterSizes.size() - 1.0 + LastRatio;
		for(int i = 0; i < SplitterSizes.size() -1; i++)
		{
			SplitterSizes[i] = AreaSize / TotRatio;
		}
		SplitterSizes.back() = AreaSize * LastRatio / TotRatio;
		Splitter->setSizes(SplitterSizes);
	}

    /**
     * This function forces the dock container widget to update handles of splitters
     * based if a central widget exists.
     */
    void updateSplitterHandles(QSplitter* splitter);

    /**
     * If no central widget exists, the widgets resize with the container.
     * If a central widget exists, the widgets surrounding the central widget
     * do not resize its height or width.
     */
    bool widgetResizesWithContainer(QWidget* widget);

// private slots: ------------------------------------------------------------
	void onDockAreaViewToggled(bool Visible)
	{
		CDockAreaWidget* DockArea = qobject_cast<CDockAreaWidget*>(_this->sender());
		VisibleDockAreaCount += Visible ? 1 : -1;
		onVisibleDockAreaCountChanged();
		Q_EMIT _this->dockAreaViewToggled(DockArea, Visible);
	}
}; // struct DockContainerWidgetPrivate


//============================================================================
DockContainerWidgetPrivate::DockContainerWidgetPrivate(CDockContainerWidget* _public) :
	_this(_public)
{
	std::fill(std::begin(LastAddedAreaCache),std::end(LastAddedAreaCache), nullptr);
	DelayedAutoHideTimer.setSingleShot(true);
	DelayedAutoHideTimer.setInterval(500);
	QObject::connect(&DelayedAutoHideTimer, &QTimer::timeout, [this](){
		auto GlobalPos = DelayedAutoHideTab->mapToGlobal(QPoint(0, 0));
		qApp->sendEvent(DelayedAutoHideTab, new QMouseEvent(QEvent::MouseButtonPress,
				QPoint(0, 0), GlobalPos, Qt::LeftButton, {Qt::LeftButton}, Qt::NoModifier));
	});
}


//============================================================================
eDropMode DockContainerWidgetPrivate::getDropMode(const QPoint& TargetPos)
{
	CDockAreaWidget* DockArea = _this->dockAreaAt(TargetPos);
	auto dropArea = InvalidDockWidgetArea;
	auto ContainerDropArea = DockManager->containerOverlay()->dropAreaUnderCursor();

	if (DockArea)
	{
		auto dropOverlay = DockManager->dockAreaOverlay();
		dropOverlay->setAllowedAreas(DockArea->allowedAreas());
		dropArea = dropOverlay->showOverlay(DockArea);
		if (ContainerDropArea != InvalidDockWidgetArea &&
			ContainerDropArea != dropArea)
		{
			dropArea = InvalidDockWidgetArea;
		}

		if (dropArea != InvalidDockWidgetArea)
		{
            ADS_PRINT("Dock Area Drop Content: " << dropArea);
            return DropModeIntoArea;
		}
	}

	// mouse is over container
	if (InvalidDockWidgetArea == dropArea)
	{
		dropArea = ContainerDropArea;
        ADS_PRINT("Container Drop Content: " << dropArea);
		if (dropArea != InvalidDockWidgetArea)
		{
			return DropModeIntoContainer;
		}
	}

	return DropModeInvalid;
}


//============================================================================
void DockContainerWidgetPrivate::onVisibleDockAreaCountChanged()
{
	auto TopLevelDockArea = _this->topLevelDockArea();

	if (TopLevelDockArea)
	{
		this->TopLevelDockArea = TopLevelDockArea;
		TopLevelDockArea->updateTitleBarButtonVisibility(true);
	}
	else if (this->TopLevelDockArea)
	{
		this->TopLevelDockArea->updateTitleBarButtonVisibility(false);
		this->TopLevelDockArea = nullptr;
	}
}


//============================================================================
void DockContainerWidgetPrivate::dropIntoContainer(CFloatingDockContainer* FloatingWidget,
	DockWidgetArea area)
{
	auto InsertParam = internal::dockAreaInsertParameters(area);
	CDockContainerWidget* FloatingDockContainer = FloatingWidget->dockContainer();
	auto NewDockAreas = FloatingDockContainer->findChildren<CDockAreaWidget*>(
		QString(), Qt::FindChildrenRecursively);
	auto Splitter = RootSplitter;

	if (DockAreas.count() <= 1)
	{
		Splitter->setOrientation(InsertParam.orientation());
	}
	else if (Splitter->orientation() != InsertParam.orientation())
	{
		auto NewSplitter = newSplitter(InsertParam.orientation());
		QLayoutItem* li = Layout->replaceWidget(Splitter, NewSplitter);
		NewSplitter->addWidget(Splitter);
        updateSplitterHandles(NewSplitter);
        Splitter = NewSplitter;
		delete li;
	}

	// Now we can insert the floating widget content into this container
	auto FloatingSplitter = FloatingDockContainer->rootSplitter();
	if (FloatingSplitter->count() == 1)
	{
		insertWidgetIntoSplitter(Splitter, FloatingSplitter->widget(0), InsertParam.append());
        updateSplitterHandles(Splitter);
    }
	else if (FloatingSplitter->orientation() == InsertParam.orientation())
	{
        int InsertIndex = InsertParam.append() ? Splitter->count() : 0;
		while (FloatingSplitter->count())
		{
            Splitter->insertWidget(InsertIndex++, FloatingSplitter->widget(0));
            updateSplitterHandles(Splitter);
        }
    }
	else
	{
		insertWidgetIntoSplitter(Splitter, FloatingSplitter, InsertParam.append());
    }

	RootSplitter = Splitter;
	addDockAreasToList(NewDockAreas);

	// If we dropped the floating widget into the main dock container that does
	// not contain any dock widgets, then splitter is invisible and we need to
	// show it to display the docked widgets
	if (!Splitter->isVisible())
	{
		Splitter->show();
    }
	_this->dumpLayout();
}


//============================================================================
void DockContainerWidgetPrivate::dropIntoAutoHideSideBar(CFloatingDockContainer* FloatingWidget, DockWidgetArea area)
{
	auto SideBarLocation = internal::toSideBarLocation(area);
	auto NewDockAreas = FloatingWidget->findChildren<CDockAreaWidget*>(
		QString(), Qt::FindChildrenRecursively);
	int TabIndex = DockManager->containerOverlay()->tabIndexUnderCursor();
	for (auto DockArea : NewDockAreas)
	{
		auto DockWidgets = DockArea->dockWidgets();
		for (auto DockWidget : DockWidgets)
		{
			_this->createAndSetupAutoHideContainer(SideBarLocation, DockWidget, TabIndex++);
		}
	}
}


//============================================================================
void DockContainerWidgetPrivate::dropIntoCenterOfSection(
	CFloatingDockContainer* FloatingWidget, CDockAreaWidget* TargetArea, int TabIndex)
{
	CDockContainerWidget* FloatingContainer = FloatingWidget->dockContainer();
	auto NewDockWidgets = FloatingContainer->dockWidgets();
	auto TopLevelDockArea = FloatingContainer->topLevelDockArea();
	int NewCurrentIndex = -1;
	TabIndex = qMax(0, TabIndex);

	// If the floating widget contains only one single dock are, then the
	// current dock widget of the dock area will also be the future current
	// dock widget in the drop area.
	if (TopLevelDockArea)
	{
		NewCurrentIndex = TopLevelDockArea->currentIndex();
	}

	for (int i = 0; i < NewDockWidgets.count(); ++i)
	{
		CDockWidget* DockWidget = NewDockWidgets[i];
		TargetArea->insertDockWidget(TabIndex + i, DockWidget, false);
		// If the floating widget contains multiple visible dock areas, then we
		// simply pick the first visible open dock widget and make it
		// the current one.
		if (NewCurrentIndex < 0 && !DockWidget->isClosed())
		{
			NewCurrentIndex = i;
		}
	}
	TargetArea->setCurrentIndex(NewCurrentIndex + TabIndex);
	TargetArea->updateTitleBarVisibility();
	return;
}


//============================================================================
void DockContainerWidgetPrivate::dropIntoSection(CFloatingDockContainer* FloatingWidget,
		CDockAreaWidget* TargetArea, DockWidgetArea area, int TabIndex)
{
	// Dropping into center means all dock widgets in the dropped floating
	// widget will become tabs of the drop area
	if (CenterDockWidgetArea == area)
	{
		dropIntoCenterOfSection(FloatingWidget, TargetArea, TabIndex);
		return;
	}

	CDockContainerWidget* FloatingContainer = FloatingWidget->dockContainer();
	auto InsertParam = internal::dockAreaInsertParameters(area);
	auto NewDockAreas = FloatingContainer->findChildren<CDockAreaWidget*>(
		QString(), Qt::FindChildrenRecursively);
	auto TargetAreaSplitter = TargetArea->parentSplitter();
	int AreaIndex = TargetAreaSplitter->indexOf(TargetArea);
	auto FloatingSplitter = FloatingContainer->rootSplitter();
	if (TargetAreaSplitter->orientation() == InsertParam.orientation())
	{
		auto Sizes = TargetAreaSplitter->sizes();
		int TargetAreaSize = (InsertParam.orientation() == Qt::Horizontal) ? TargetArea->width() : TargetArea->height();
		bool AdjustSplitterSizes = true;
		if ((FloatingSplitter->orientation() != InsertParam.orientation()) && FloatingSplitter->count() > 1)
		{
			TargetAreaSplitter->insertWidget(AreaIndex + InsertParam.insertOffset(), FloatingSplitter);
            updateSplitterHandles(TargetAreaSplitter);
        }
		else
		{
			AdjustSplitterSizes = (FloatingSplitter->count() == 1);
			int InsertIndex = AreaIndex + InsertParam.insertOffset();
			while (FloatingSplitter->count())
			{
				TargetAreaSplitter->insertWidget(InsertIndex++, FloatingSplitter->widget(0));
                updateSplitterHandles(TargetAreaSplitter);
            }
        }

		if (AdjustSplitterSizes)
		{
			int Size = (TargetAreaSize - TargetAreaSplitter->handleWidth()) / 2;
			Sizes[AreaIndex] = Size;
			Sizes.insert(AreaIndex, Size);
			TargetAreaSplitter->setSizes(Sizes);
		}
	}
	else
	{
		QSplitter* NewSplitter = newSplitter(InsertParam.orientation());
		int TargetAreaSize = (InsertParam.orientation() == Qt::Horizontal) ? TargetArea->width() : TargetArea->height();
		bool AdjustSplitterSizes = true;
		if ((FloatingSplitter->orientation() != InsertParam.orientation()) && FloatingSplitter->count() > 1)
		{
			NewSplitter->addWidget(FloatingSplitter);
            updateSplitterHandles(NewSplitter);
        }
		else
		{
			AdjustSplitterSizes = (FloatingSplitter->count() == 1);
			while (FloatingSplitter->count())
			{
				NewSplitter->addWidget(FloatingSplitter->widget(0));
                updateSplitterHandles(NewSplitter);
            }
        }

		// Save the sizes before insertion and restore it later to prevent
		// shrinking of existing area
		auto Sizes = TargetAreaSplitter->sizes();
		insertWidgetIntoSplitter(NewSplitter, TargetArea, !InsertParam.append());
        updateSplitterHandles(NewSplitter);
        if (AdjustSplitterSizes)
		{
			int Size = TargetAreaSize / 2;
			NewSplitter->setSizes({Size, Size});
		}
		TargetAreaSplitter->insertWidget(AreaIndex, NewSplitter);
		TargetAreaSplitter->setSizes(Sizes);
        updateSplitterHandles(TargetAreaSplitter);
    }

	addDockAreasToList(NewDockAreas);
	_this->dumpLayout();
}


//============================================================================
void DockContainerWidgetPrivate::moveIntoCenterOfSection(QWidget* Widget, CDockAreaWidget* TargetArea,
	int TabIndex)
{
	auto DroppedDockWidget = qobject_cast<CDockWidget*>(Widget);
	auto DroppedArea = qobject_cast<CDockAreaWidget*>(Widget);

	TabIndex = qMax(0, TabIndex);
	if (DroppedDockWidget)
	{
		CDockAreaWidget* OldDockArea = DroppedDockWidget->dockAreaWidget();
		if (OldDockArea == TargetArea)
		{
			return;
		}

		if (OldDockArea)
		{
			OldDockArea->removeDockWidget(DroppedDockWidget);
		}
		TargetArea->insertDockWidget(TabIndex, DroppedDockWidget, true);
	}
	else
	{
		QList<CDockWidget*> NewDockWidgets = DroppedArea->dockWidgets();
		int NewCurrentIndex = DroppedArea->currentIndex();
		for (int i = 0; i < NewDockWidgets.count(); ++i)
		{
			CDockWidget* DockWidget = NewDockWidgets[i];
			TargetArea->insertDockWidget(TabIndex + i, DockWidget, false);
		}
		TargetArea->setCurrentIndex(TabIndex + NewCurrentIndex);
		DroppedArea->dockContainer()->removeDockArea(DroppedArea);
		DroppedArea->deleteLater();
	}

	TargetArea->updateTitleBarVisibility();
	return;
}


//============================================================================
void DockContainerWidgetPrivate::moveToNewSection(QWidget* Widget, CDockAreaWidget* TargetArea, DockWidgetArea area,
	int TabIndex)
{
	// Dropping into center means all dock widgets in the dropped floating
	// widget will become tabs of the drop area
	if (CenterDockWidgetArea == area)
	{
		moveIntoCenterOfSection(Widget, TargetArea, TabIndex);
		return;
	}


	CDockWidget* DroppedDockWidget = qobject_cast<CDockWidget*>(Widget);
	CDockAreaWidget* DroppedDockArea = qobject_cast<CDockAreaWidget*>(Widget);
	CDockAreaWidget* NewDockArea;
	if (DroppedDockWidget)
	{
		NewDockArea = new CDockAreaWidget(DockManager, _this);
		CDockAreaWidget* OldDockArea = DroppedDockWidget->dockAreaWidget();
		if (OldDockArea)
		{
			OldDockArea->removeDockWidget(DroppedDockWidget);
		}
		NewDockArea->addDockWidget(DroppedDockWidget);
	}
	else
	{
		DroppedDockArea->dockContainer()->removeDockArea(DroppedDockArea);
		NewDockArea = DroppedDockArea;
	}

	auto InsertParam = internal::dockAreaInsertParameters(area);
	auto TargetAreaSplitter = TargetArea->parentSplitter();
	int AreaIndex = TargetAreaSplitter->indexOf(TargetArea);
	auto Sizes = TargetAreaSplitter->sizes();
	if (TargetAreaSplitter->orientation() == InsertParam.orientation())
	{
		int TargetAreaSize = (InsertParam.orientation() == Qt::Horizontal) ? TargetArea->width() : TargetArea->height();
		TargetAreaSplitter->insertWidget(AreaIndex + InsertParam.insertOffset(), NewDockArea);
        updateSplitterHandles(TargetAreaSplitter);
        int Size = (TargetAreaSize - TargetAreaSplitter->handleWidth()) / 2;
		Sizes[AreaIndex] = Size;
		Sizes.insert(AreaIndex, Size);
	}
	else
	{
		int TargetAreaSize = (InsertParam.orientation() == Qt::Horizontal) ? TargetArea->width() : TargetArea->height();
		QSplitter* NewSplitter = newSplitter(InsertParam.orientation());
		NewSplitter->addWidget(TargetArea);
		insertWidgetIntoSplitter(NewSplitter, NewDockArea, InsertParam.append());
        updateSplitterHandles(NewSplitter);
        int Size = TargetAreaSize / 2;
		NewSplitter->setSizes({Size, Size});
		TargetAreaSplitter->insertWidget(AreaIndex, NewSplitter);
        updateSplitterHandles(TargetAreaSplitter);
    }
	TargetAreaSplitter->setSizes(Sizes);

	addDockAreasToList({NewDockArea});
}


//============================================================================
void DockContainerWidgetPrivate::moveToAutoHideSideBar(QWidget* Widget, DockWidgetArea area, int TabIndex)
{
	CDockWidget* DroppedDockWidget = qobject_cast<CDockWidget*>(Widget);
	CDockAreaWidget* DroppedDockArea = qobject_cast<CDockAreaWidget*>(Widget);
	auto SideBarLocation = internal::toSideBarLocation(area);

	if (DroppedDockWidget)
	{
		if (_this == DroppedDockWidget->dockContainer())
		{
			DroppedDockWidget->setAutoHide(true, SideBarLocation, TabIndex);
		}
		else
		{
			_this->createAndSetupAutoHideContainer(SideBarLocation, DroppedDockWidget, TabIndex);
		}
	}
	else
	{
		if (_this == DroppedDockArea->dockContainer())
		{
			DroppedDockArea->setAutoHide(true, SideBarLocation, TabIndex);
		}
		else
		{
			for (const auto DockWidget : DroppedDockArea->openedDockWidgets())
			{
				if (!DockWidget->features().testFlag(
				    CDockWidget::DockWidgetPinnable))
				{
					continue;
				}

				_this->createAndSetupAutoHideContainer(SideBarLocation,
				    DockWidget, TabIndex++);
			}
		}
	}
}


//============================================================================
void DockContainerWidgetPrivate::updateSplitterHandles( QSplitter* splitter )
{
	if (!DockManager->centralWidget() || !splitter)
	{
		return;
	}

	for (int i = 0; i < splitter->count(); ++i)
    {
		splitter->setStretchFactor(i, widgetResizesWithContainer(splitter->widget(i)) ? 1 : 0);
    }
}


//============================================================================
bool DockContainerWidgetPrivate::widgetResizesWithContainer(QWidget* widget)
{
    if (!DockManager->centralWidget())
    {
        return true;
    }

    auto Area = qobject_cast<CDockAreaWidget*>(widget);
    if(Area)
    {
        return Area->isCentralWidgetArea();
    }

    auto innerSplitter = qobject_cast<CDockSplitter*>(widget);
    if (innerSplitter)
    {
        return innerSplitter->isResizingWithContainer();
    }

    return false;
}



//============================================================================
void DockContainerWidgetPrivate::moveToContainer(QWidget* Widget, DockWidgetArea area)
{
	CDockWidget* DroppedDockWidget = qobject_cast<CDockWidget*>(Widget);
	CDockAreaWidget* DroppedDockArea = qobject_cast<CDockAreaWidget*>(Widget);
	CDockAreaWidget* NewDockArea;

	if (DroppedDockWidget)
	{
		NewDockArea = new CDockAreaWidget(DockManager, _this);
		CDockAreaWidget* OldDockArea = DroppedDockWidget->dockAreaWidget();
		if (OldDockArea)
		{
			OldDockArea->removeDockWidget(DroppedDockWidget);
		}
		NewDockArea->addDockWidget(DroppedDockWidget);
	}
	else
	{
		// We check, if we insert the dropped widget into the same place that
		// it already has and do nothing, if it is the same place. It would
		// also work without this check, but it looks nicer with the check
		// because there will be no layout updates
		auto Splitter = DroppedDockArea->parentSplitter();
		auto InsertParam = internal::dockAreaInsertParameters(area);
		if (Splitter == RootSplitter && InsertParam.orientation() == Splitter->orientation())
		{
			if (InsertParam.append() && Splitter->lastWidget() == DroppedDockArea)
			{
				return;
			}
			else if (!InsertParam.append() && Splitter->firstWidget() == DroppedDockArea)
			{
				return;
			}
		}
		DroppedDockArea->dockContainer()->removeDockArea(DroppedDockArea);
		NewDockArea = DroppedDockArea;
	}

	addDockArea(NewDockArea, area);
	LastAddedAreaCache[areaIdToIndex(area)] = NewDockArea;
}


//============================================================================
void DockContainerWidgetPrivate::addDockAreasToList(const QList<CDockAreaWidget*> NewDockAreas)
{
	int CountBefore = DockAreas.count();
	int NewAreaCount = NewDockAreas.count();
	appendDockAreas(NewDockAreas);
	// If the user dropped a floating widget that contains only one single
	// visible dock area, then its title bar button TitleBarButtonUndock is
	// likely hidden. We need to ensure, that it is visible
	for (auto DockArea : NewDockAreas)
	{
		DockArea->titleBarButton(TitleBarButtonClose)->setVisible(true);
		DockArea->titleBarButton(TitleBarButtonAutoHide)->setVisible(true);
	}

	// We need to ensure, that the dock area title bar is visible. The title bar
	// is invisible, if the dock are is a single dock area in a floating widget.
	if (1 == CountBefore)
	{
		DockAreas.at(0)->updateTitleBarVisibility();
	}

	if (1 == NewAreaCount)
	{
		DockAreas.last()->updateTitleBarVisibility();
	}

	emitDockAreasAdded();
}


//============================================================================
void DockContainerWidgetPrivate::appendDockAreas(const QList<CDockAreaWidget*> NewDockAreas)
{
	for (auto *newDockArea : NewDockAreas)
	{
		DockAreas.append(newDockArea);
	}
	for (auto DockArea : NewDockAreas)
	{
		QObject::connect(DockArea,
			&CDockAreaWidget::viewToggled,
			_this,
			std::bind(&DockContainerWidgetPrivate::onDockAreaViewToggled, this, std::placeholders::_1));
	}
}


//============================================================================
void DockContainerWidgetPrivate::saveChildNodesState(QXmlStreamWriter& s, QWidget* Widget)
{
	QSplitter* Splitter = qobject_cast<QSplitter*>(Widget);
	if (Splitter)
	{
		s.writeStartElement("Splitter");
		s.writeAttribute("Orientation", (Splitter->orientation() == Qt::Horizontal) ? "|" : "-");
		s.writeAttribute("Count", QString::number(Splitter->count()));
        ADS_PRINT("NodeSplitter orient: " << Splitter->orientation()
            << " WidgetCont: " << Splitter->count());
			for (int i = 0; i < Splitter->count(); ++i)
			{
				saveChildNodesState(s, Splitter->widget(i));
			}

			s.writeStartElement("Sizes");
			for (auto Size : Splitter->sizes())
			{
				s.writeCharacters(QString::number(Size) + " ");
			}
			s.writeEndElement();
		s.writeEndElement();
	}
	else
	{
		CDockAreaWidget* DockArea = qobject_cast<CDockAreaWidget*>(Widget);
		if (DockArea)
		{
			DockArea->saveState(s);
		}
	}
}


//============================================================================
void DockContainerWidgetPrivate::saveAutoHideWidgetsState(QXmlStreamWriter& s)
{
	for (const auto sideTabBar : SideTabBarWidgets.values())
    {
		if (!sideTabBar->count())
		{
			continue;
		}

		sideTabBar->saveState(s);
    }
}


//============================================================================
bool DockContainerWidgetPrivate::restoreSplitter(CDockingStateReader& s,
	QWidget*& CreatedWidget, bool Testing)
{
	bool Ok;
	QString OrientationStr = s.attributes().value("Orientation").toString();

	// Check if the orientation string is right
	if (!OrientationStr.startsWith("|") && !OrientationStr.startsWith("-"))
	{
		return false;
	}

	// The "|" shall indicate a vertical splitter handle which in turn means
	// a Horizontal orientation of the splitter layout.
	bool HorizontalSplitter = OrientationStr.startsWith("|");
	// In version 0 we had a small bug. The "|" indicated a vertical orientation,
	// but this is wrong, because only the splitter handle is vertical, the
	// layout of the splitter is a horizontal layout. We fix this here
	if (s.fileVersion() == 0)
	{
		HorizontalSplitter = !HorizontalSplitter;
	}

	int Orientation = HorizontalSplitter ? Qt::Horizontal : Qt::Vertical;
	int WidgetCount = s.attributes().value("Count").toInt(&Ok);
	if (!Ok)
	{
		return false;
	}
    ADS_PRINT("Restore NodeSplitter Orientation: " <<  Orientation <<
            " WidgetCount: " << WidgetCount);
	QSplitter* Splitter = nullptr;
	if (!Testing)
	{
		Splitter = newSplitter(static_cast<Qt::Orientation>(Orientation));
	}
	bool Visible = false;
	QList<int> Sizes;
	while (s.readNextStartElement())
	{
		QWidget* ChildNode = nullptr;
		bool Result = true;
        if (s.name() == QLatin1String("Splitter"))
		{
			Result = restoreSplitter(s, ChildNode, Testing);
		}
        else if (s.name() == QLatin1String("Area"))
		{
			Result = restoreDockArea(s, ChildNode, Testing);
		}
        else if (s.name() == QLatin1String("Sizes"))
		{
			QString sSizes = s.readElementText().trimmed();
            ADS_PRINT("Sizes: " << sSizes);
			QTextStream TextStream(&sSizes);
			while (!TextStream.atEnd())
			{
				int value;
				TextStream >> value;
				Sizes.append(value);
			}
		}
		else
		{
			s.skipCurrentElement();
		}

		if (!Result)
		{
			return false;
		}

		if (Testing || !ChildNode)
		{
			continue;
		}

        ADS_PRINT("ChildNode isVisible " << ChildNode->isVisible()
            << " isVisibleTo " << ChildNode->isVisibleTo(Splitter));
		Splitter->addWidget(ChildNode);
		Visible |= ChildNode->isVisibleTo(Splitter);
	}
    if(!Testing)
    {
       updateSplitterHandles(Splitter);
    }

	if (Sizes.count() != WidgetCount)
	{
		return false;
	}

	if (!Testing)
	{
		if (!Splitter->count())
		{
			delete Splitter;
			Splitter = nullptr;
		}
		else
		{
			Splitter->setSizes(Sizes);
			Splitter->setVisible(Visible);
		}
		CreatedWidget = Splitter;
	}
	else
	{
		CreatedWidget = nullptr;
	}

	return true;
}


//============================================================================
bool DockContainerWidgetPrivate::restoreDockArea(CDockingStateReader& s,
	QWidget*& CreatedWidget, bool Testing)
{
	CDockAreaWidget* DockArea = nullptr;
	auto Result = CDockAreaWidget::restoreState(s, DockArea, Testing, _this);
	if (Result && DockArea)
	{
		appendDockAreas({DockArea});
	}
	CreatedWidget = DockArea;
	return Result;
}


//============================================================================
bool DockContainerWidgetPrivate::restoreSideBar(CDockingStateReader& s,
	QWidget*& CreatedWidget, bool Testing)
{
	Q_UNUSED(CreatedWidget)
	// Simply ignore side bar auto hide widgets from saved state if
	// auto hide support is disabled
	if (!CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideFeatureEnabled))
	{
		return true;
	}

	bool Ok;
	auto Area = (ads::SideBarLocation)s.attributes().value("Area").toInt(&Ok);
	if (!Ok)
	{
		return false;
	}

	while (s.readNextStartElement())
	{
		if (s.name() != QLatin1String("Widget"))
		{
			continue;
		}

		auto Name = s.attributes().value("Name");
		if (Name.isEmpty())
		{
			return false;
		}

		bool Ok;
		bool Closed = s.attributes().value("Closed").toInt(&Ok);
		if (!Ok)
		{
			return false;
		}

		int Size = s.attributes().value("Size").toInt(&Ok);
		if (!Ok)
		{
			return false;
		}

		s.skipCurrentElement();
		CDockWidget* DockWidget = DockManager->findDockWidget(Name.toString());
		if (!DockWidget || Testing)
		{
			continue;
		}

		auto SideBar = _this->autoHideSideBar(Area);
		CAutoHideDockContainer* AutoHideContainer;
		if (DockWidget->isAutoHide())
		{
			AutoHideContainer = DockWidget->autoHideDockContainer();
			if (AutoHideContainer->autoHideSideBar() != SideBar)
			{
				SideBar->addAutoHideWidget(AutoHideContainer);
			}
		}
		else
		{
			AutoHideContainer = SideBar->insertDockWidget(-1, DockWidget);
		}
		AutoHideContainer->setSize(Size);
        DockWidget->setProperty(internal::ClosedProperty, Closed);
		DockWidget->setProperty(internal::DirtyProperty, false);
	}

	return true;
}


//============================================================================
bool DockContainerWidgetPrivate::restoreChildNodes(CDockingStateReader& s,
	QWidget*& CreatedWidget, bool Testing)
{
	bool Result = true;
	while (s.readNextStartElement())
	{
        if (s.name() == QLatin1String("Splitter"))
		{
			Result = restoreSplitter(s, CreatedWidget, Testing);
            ADS_PRINT("Splitter");
		}
        else if (s.name() == QLatin1String("Area"))
		{
			Result = restoreDockArea(s, CreatedWidget, Testing);
            ADS_PRINT("DockAreaWidget");
		}
        else if (s.name() == QLatin1String("SideBar"))
        {
        	Result = restoreSideBar(s, CreatedWidget, Testing);
        	ADS_PRINT("SideBar");
        }
		else
		{
			s.skipCurrentElement();
            ADS_PRINT("Unknown element");
		}
	}

	return Result;
}


//============================================================================
CDockAreaWidget* DockContainerWidgetPrivate::addDockWidgetToContainer(DockWidgetArea area,
	CDockWidget* Dockwidget)
{
	CDockAreaWidget* NewDockArea = new CDockAreaWidget(DockManager, _this);
	NewDockArea->addDockWidget(Dockwidget);
	addDockArea(NewDockArea, area);
	NewDockArea->updateTitleBarVisibility();
	LastAddedAreaCache[areaIdToIndex(area)] = NewDockArea;
	return NewDockArea;
}


//============================================================================
void DockContainerWidgetPrivate::addDockArea(CDockAreaWidget* NewDockArea, DockWidgetArea area)
{
	auto InsertParam = internal::dockAreaInsertParameters(area);
	// As long as we have only one dock area in the splitter we can adjust
	// its orientation
	if (DockAreas.count() <= 1)
	{
		RootSplitter->setOrientation(InsertParam.orientation());
	}

	QSplitter* Splitter = RootSplitter;
	if (Splitter->orientation() == InsertParam.orientation())
	{
		insertWidgetIntoSplitter(Splitter, NewDockArea, InsertParam.append());
        updateSplitterHandles(Splitter);
        if (Splitter->isHidden())
		{
			Splitter->show();
		}
	}
	else
	{
		auto NewSplitter = newSplitter(InsertParam.orientation());
		if (InsertParam.append())
		{
			QLayoutItem* li = Layout->replaceWidget(Splitter, NewSplitter);
			NewSplitter->addWidget(Splitter);
			NewSplitter->addWidget(NewDockArea);
            updateSplitterHandles(NewSplitter);
            delete li;
		}
		else
		{
			NewSplitter->addWidget(NewDockArea);
			QLayoutItem* li = Layout->replaceWidget(Splitter, NewSplitter);
			NewSplitter->addWidget(Splitter);
            updateSplitterHandles(NewSplitter);
            delete li;
		}
		RootSplitter = NewSplitter;
	}

	addDockAreasToList({NewDockArea});
}


//============================================================================
void DockContainerWidgetPrivate::dumpRecursive(int level, QWidget* widget)
{
#if defined(QT_DEBUG)
	QSplitter* Splitter = qobject_cast<QSplitter*>(widget);
	QByteArray buf;
    buf.fill(' ', level * 4);
	if (Splitter)
	{
#ifdef ADS_DEBUG_PRINT
		qDebug("%sSplitter %s v: %s c: %s",
			(const char*)buf,
			(Splitter->orientation() == Qt::Vertical) ? "--" : "|",
			 Splitter->isHidden() ? " " : "v",
			 QString::number(Splitter->count()).toStdString().c_str());
        std::cout << (const char*)buf << "Splitter "
            << ((Splitter->orientation() == Qt::Vertical) ? "--" : "|") << " "
            << (Splitter->isHidden() ? " " : "v") << " "
            << QString::number(Splitter->count()).toStdString() << std::endl;
#endif
		for (int i = 0; i < Splitter->count(); ++i)
		{
			dumpRecursive(level + 1, Splitter->widget(i));
		}
	}
	else
	{
		CDockAreaWidget* DockArea = qobject_cast<CDockAreaWidget*>(widget);
		if (!DockArea)
		{
			return;
		}
#ifdef ADS_DEBUG_PRINT
		qDebug("%sDockArea", (const char*)buf);
		std::cout << (const char*)buf
			<< (DockArea->isHidden() ? " " : "v")
			<< (DockArea->openDockWidgetsCount() > 0 ? " " : "c")
			<< " DockArea " << "[hs: " << DockArea->sizePolicy().horizontalStretch() << ", vs: " <<  DockArea->sizePolicy().verticalStretch() << "]" << std::endl;
		buf.fill(' ', (level + 1) * 4);
		for (int i = 0; i < DockArea->dockWidgetsCount(); ++i)
		{
			std::cout << (const char*)buf << (i == DockArea->currentIndex() ? "*" : " ");
			CDockWidget* DockWidget = DockArea->dockWidget(i);
			std::cout << (DockWidget->isHidden() ? " " : "v");
			std::cout << (DockWidget->isClosed() ? "c" : " ") << " ";
			std::cout << DockWidget->windowTitle().toStdString() << std::endl;
        }
#endif
	}
#else
	Q_UNUSED(level);
	Q_UNUSED(widget);
#endif
}


//============================================================================
CDockAreaWidget* DockContainerWidgetPrivate::addDockWidgetToDockArea(DockWidgetArea area,
	CDockWidget* Dockwidget, CDockAreaWidget* TargetDockArea, int Index)
{
	if (CenterDockWidgetArea == area)
	{
		TargetDockArea->insertDockWidget(Index, Dockwidget);
		TargetDockArea->updateTitleBarVisibility();
		return TargetDockArea;
	}

	CDockAreaWidget* NewDockArea = new CDockAreaWidget(DockManager, _this);
	NewDockArea->addDockWidget(Dockwidget);
	auto InsertParam = internal::dockAreaInsertParameters(area);

	auto TargetAreaSplitter = TargetDockArea->parentSplitter();
	int index = TargetAreaSplitter ->indexOf(TargetDockArea);
	if (TargetAreaSplitter->orientation() == InsertParam.orientation())
	{
		ADS_PRINT("TargetAreaSplitter->orientation() == InsertParam.orientation()");
		TargetAreaSplitter->insertWidget(index + InsertParam.insertOffset(), NewDockArea);
        updateSplitterHandles(TargetAreaSplitter);
        // do nothing, if flag is not enabled
		if (CDockManager::testConfigFlag(CDockManager::EqualSplitOnInsertion))
		{
			adjustSplitterSizesOnInsertion(TargetAreaSplitter);
		}
	}
	else
	{
		ADS_PRINT("TargetAreaSplitter->orientation() != InsertParam.orientation()");
		auto TargetAreaSizes = TargetAreaSplitter->sizes();
		auto NewSplitter = newSplitter(InsertParam.orientation());
		NewSplitter->addWidget(TargetDockArea);

		insertWidgetIntoSplitter(NewSplitter, NewDockArea, InsertParam.append());
        updateSplitterHandles(NewSplitter);
        TargetAreaSplitter->insertWidget(index, NewSplitter);
        updateSplitterHandles(TargetAreaSplitter);
        if (CDockManager::testConfigFlag(CDockManager::EqualSplitOnInsertion))
        {
			TargetAreaSplitter->setSizes(TargetAreaSizes);
			adjustSplitterSizesOnInsertion(NewSplitter);
		}
	}

	addDockAreasToList({NewDockArea});
	return NewDockArea;
}


//============================================================================
CDockContainerWidget::CDockContainerWidget(CDockManager* DockManager, QWidget *parent) :
	QFrame(parent),
	d(new DockContainerWidgetPrivate(this))
{
	d->DockManager = DockManager;
	d->isFloating = floatingWidget() != nullptr;

	d->Layout = new QGridLayout();
	d->Layout->setContentsMargins(0, 0, 0, 0);
	d->Layout->setSpacing(0);
	d->Layout->setColumnStretch(1, 1);
	d->Layout->setRowStretch(1, 1);
	setLayout(d->Layout);

	// The function d->newSplitter() accesses the config flags from dock
	// manager which in turn requires a properly constructed dock manager.
	// If this dock container is the dock manager, then it is not properly
	// constructed yet because this base class constructor is called before
	// the constructor of the DockManager private class
	if (DockManager != this)
	{
		d->DockManager->registerDockContainer(this);
		createRootSplitter();
		createSideTabBarWidgets();
	}
}


//============================================================================
CDockContainerWidget::~CDockContainerWidget()
{
	if (d->DockManager)
	{
		d->DockManager->removeDockContainer(this);
	}

	delete d;
}


//============================================================================
CDockAreaWidget* CDockContainerWidget::addDockWidget(DockWidgetArea area, CDockWidget* Dockwidget,
	CDockAreaWidget* DockAreaWidget, int Index)
{
	auto TopLevelDockWidget = topLevelDockWidget();
	CDockAreaWidget* OldDockArea = Dockwidget->dockAreaWidget();
	if (OldDockArea)
	{
		OldDockArea->removeDockWidget(Dockwidget);
	}

	Dockwidget->setDockManager(d->DockManager);
	CDockAreaWidget* DockArea;
	if (DockAreaWidget)
	{
		DockArea = d->addDockWidgetToDockArea(area, Dockwidget, DockAreaWidget, Index);
	}
	else
	{
		DockArea = d->addDockWidgetToContainer(area, Dockwidget);
	}

	if (TopLevelDockWidget)
	{
		auto NewTopLevelDockWidget = topLevelDockWidget();
		// If the container contained only one visible dock widget, the we need
		// to emit a top level event for this widget because it is not the one and
		// only visible docked widget anymore
		if (!NewTopLevelDockWidget)
		{
			CDockWidget::emitTopLevelEventForWidget(TopLevelDockWidget, false);
		}
	}
	return DockArea;
}


//============================================================================
CAutoHideDockContainer* CDockContainerWidget::createAndSetupAutoHideContainer(
	SideBarLocation area, CDockWidget* DockWidget, int TabIndex)
{
	if (!CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideFeatureEnabled))
	{
		Q_ASSERT_X(false, "CDockContainerWidget::createAndInitializeDockWidgetOverlayContainer",
			"Requested area does not exist in config");
		return nullptr;
	}
	if (d->DockManager != DockWidget->dockManager())
	{
        DockWidget->setDockManager(d->DockManager); // Auto hide Dock Container needs a valid dock manager
	}

	return autoHideSideBar(area)->insertDockWidget(TabIndex, DockWidget);
}


//============================================================================
void CDockContainerWidget::removeDockWidget(CDockWidget* Dockwidget)
{
	CDockAreaWidget* Area = Dockwidget->dockAreaWidget();
	if (Area)
	{
		Area->removeDockWidget(Dockwidget);
	}
}

//============================================================================
unsigned int CDockContainerWidget::zOrderIndex() const
{
	return d->zOrderIndex;
}


//============================================================================
bool CDockContainerWidget::isInFrontOf(CDockContainerWidget* Other) const
{
	return this->zOrderIndex() > Other->zOrderIndex();
}


//============================================================================
bool CDockContainerWidget::event(QEvent *e)
{
	bool Result = QWidget::event(e);
	if (e->type() == QEvent::WindowActivate)
    {
        d->zOrderIndex = ++zOrderCounter;
    }
	else if (e->type() == QEvent::Show && !d->zOrderIndex)
	{
		d->zOrderIndex = ++zOrderCounter;
	}

	return Result;
}


//============================================================================
QList<CAutoHideDockContainer*> CDockContainerWidget::autoHideWidgets() const
{
	return d->AutoHideWidgets;
}


//============================================================================
void CDockContainerWidget::addDockArea(CDockAreaWidget* DockAreaWidget,
	DockWidgetArea area)
{
	CDockContainerWidget* Container = DockAreaWidget->dockContainer();
	if (Container && Container != this)
	{
		Container->removeDockArea(DockAreaWidget);
	}

	d->addDockArea(DockAreaWidget, area);
}


//============================================================================
void CDockContainerWidget::removeDockArea(CDockAreaWidget* area)
{
    ADS_PRINT("CDockContainerWidget::removeDockArea");
    // If it is an auto hide area, then there is nothing much to do
	if (area->isAutoHide())
	{
        area->setAutoHideDockContainer(nullptr);
		return;
	}

	area->disconnect(this);
	d->DockAreas.removeAll(area);
	auto Splitter = area->parentSplitter();

	// Remove are from parent splitter and recursively hide tree of parent
	// splitters if it has no visible content
	area->setParent(nullptr);
	internal::hideEmptyParentSplitters(Splitter);

	// Remove this area from cached areas
	auto p = std::find(std::begin(d->LastAddedAreaCache), std::end(d->LastAddedAreaCache), area);
	if (p != std::end(d->LastAddedAreaCache)) {
		*p = nullptr;
	}

	// If splitter has more than 1 widgets, we are finished and can leave
	if (Splitter->count() >  1)
	{
		goto emitAndExit;
	}

	// If this is the RootSplitter we need to remove empty splitters to
	// avoid too many empty splitters
	if (Splitter == d->RootSplitter)
	{
        ADS_PRINT("Removed from RootSplitter");
		// If splitter is empty, we are finished
		if (!Splitter->count())
		{
			Splitter->hide();
			goto emitAndExit;
		}

		QWidget* widget = Splitter->widget(0);
		auto ChildSplitter = qobject_cast<CDockSplitter*>(widget);
		// If the one and only content widget of the splitter is not a splitter
		// then we are finished
		if (!ChildSplitter)
		{
			goto emitAndExit;
		}

		// We replace the superfluous RootSplitter with the ChildSplitter
		ChildSplitter->setParent(nullptr);
		QLayoutItem* li = d->Layout->replaceWidget(Splitter, ChildSplitter);
		d->RootSplitter = ChildSplitter;
		delete li;
        ADS_PRINT("RootSplitter replaced by child splitter");
	}
	else if (Splitter->count() == 1)
	{
        ADS_PRINT("Replacing splitter with content");
		QSplitter* ParentSplitter = internal::findParent<QSplitter*>(Splitter);
		auto Sizes = ParentSplitter->sizes();
		QWidget* widget = Splitter->widget(0);
		widget->setParent(this);
		internal::replaceSplitterWidget(ParentSplitter, Splitter, widget);
		ParentSplitter->setSizes(Sizes);
	}

	delete Splitter;
    Splitter = nullptr;

emitAndExit:
    updateSplitterHandles(Splitter);
    CDockWidget* TopLevelWidget = topLevelDockWidget();

	// Updated the title bar visibility of the dock widget if there is only
    // one single visible dock widget
	CDockWidget::emitTopLevelEventForWidget(TopLevelWidget, true);
	dumpLayout();
	d->emitDockAreasRemoved();
}


//============================================================================
QList<QPointer<CDockAreaWidget>> CDockContainerWidget::removeAllDockAreas()
{
	auto Result = d->DockAreas;
	d->DockAreas.clear();
	return Result;
}


//============================================================================
CDockAreaWidget* CDockContainerWidget::dockAreaAt(const QPoint& GlobalPos) const
{
	for (const auto& DockArea : d->DockAreas)
	{
		if (DockArea && DockArea->isVisible() && DockArea->rect().contains(DockArea->mapFromGlobal(GlobalPos)))
		{
			return DockArea;
		}
	}

	return nullptr;
}


//============================================================================
CDockAreaWidget* CDockContainerWidget::dockArea(int Index) const
{
	return (Index < dockAreaCount()) ? d->DockAreas[Index] : nullptr;
}


//============================================================================
bool CDockContainerWidget::isFloating() const
{
	return d->isFloating;
}


//============================================================================
int CDockContainerWidget::dockAreaCount() const
{
	return d->DockAreas.count();
}


//============================================================================
int CDockContainerWidget::visibleDockAreaCount() const
{
	int Result = 0;
	for (auto DockArea : d->DockAreas)
	{
		Result += (!DockArea || DockArea->isHidden()) ? 0 : 1;
	}

	return Result;

	// TODO Cache or precalculate this to speed it up because it is used during
	// movement of floating widget
	//return d->visibleDockAreaCount();
}


//============================================================================
void CDockContainerWidget::dropFloatingWidget(CFloatingDockContainer* FloatingWidget,
	const QPoint& TargetPos)
{
    ADS_PRINT("CDockContainerWidget::dropFloatingWidget");
	CDockWidget* SingleDroppedDockWidget = FloatingWidget->topLevelDockWidget();
	CDockWidget* SingleDockWidget = topLevelDockWidget();
	auto dropArea = InvalidDockWidgetArea;
	auto ContainerDropArea = d->DockManager->containerOverlay()->dropAreaUnderCursor();
	bool Dropped = false;

	CDockAreaWidget* DockArea = dockAreaAt(TargetPos);
	// mouse is over dock area
	if (DockArea)
	{
		auto dropOverlay = d->DockManager->dockAreaOverlay();
		dropOverlay->setAllowedAreas(DockArea->allowedAreas());
		dropArea = dropOverlay->showOverlay(DockArea);
		if (ContainerDropArea != InvalidDockWidgetArea &&
			ContainerDropArea != dropArea)
		{
			dropArea = InvalidDockWidgetArea;
		}

		if (dropArea != InvalidDockWidgetArea)
		{
            ADS_PRINT("Dock Area Drop Content: " << dropArea);
            int TabIndex = d->DockManager->dockAreaOverlay()->tabIndexUnderCursor();
			d->dropIntoSection(FloatingWidget, DockArea, dropArea, TabIndex);
			Dropped = true;
		}
	}

	// mouse is over container or auto hide side bar
	if (InvalidDockWidgetArea == dropArea && InvalidDockWidgetArea != ContainerDropArea)
	{
        if (internal::isSideBarArea(ContainerDropArea))
        {
        	ADS_PRINT("Container Drop Content: " << ContainerDropArea);
        	d->dropIntoAutoHideSideBar(FloatingWidget, ContainerDropArea);
        }
        else
        {
        	ADS_PRINT("Container Drop Content: " << ContainerDropArea);
        	d->dropIntoContainer(FloatingWidget, ContainerDropArea);
        }
		Dropped = true;
	}

    // Remove the auto hide widgets from the FloatingWidget and insert
	// them into this widget
	for (auto AutohideWidget : FloatingWidget->dockContainer()->autoHideWidgets())
	{
		auto SideBar = autoHideSideBar(AutohideWidget->sideBarLocation());
		SideBar->addAutoHideWidget(AutohideWidget);
	}

	if (Dropped)
	{ 
		// Fix https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/issues/351
		FloatingWidget->finishDropOperation();

		// If we dropped a floating widget with only one single dock widget, then we
		// drop a top level widget that changes from floating to docked now
		CDockWidget::emitTopLevelEventForWidget(SingleDroppedDockWidget, false);

		// If there was a top level widget before the drop, then it is not top
		// level widget anymore
		CDockWidget::emitTopLevelEventForWidget(SingleDockWidget, false);
	}

	window()->activateWindow();
	if (SingleDroppedDockWidget)
	{
		d->DockManager->notifyWidgetOrAreaRelocation(SingleDroppedDockWidget);
	}
	d->DockManager->notifyFloatingWidgetDrop(FloatingWidget);
}


//============================================================================
void CDockContainerWidget::dropWidget(QWidget* Widget, DockWidgetArea DropArea, CDockAreaWidget* TargetAreaWidget,
	int TabIndex)
{
    CDockWidget* SingleDockWidget = topLevelDockWidget();
	if (TargetAreaWidget)
	{
		d->moveToNewSection(Widget, TargetAreaWidget, DropArea, TabIndex);
	}
	else if (internal::isSideBarArea(DropArea))
	{
		d->moveToAutoHideSideBar(Widget, DropArea, TabIndex);
	}
	else
	{
		d->moveToContainer(Widget, DropArea);
	}

	// If there was a top level widget before the drop, then it is not top
	// level widget anymore
	CDockWidget::emitTopLevelEventForWidget(SingleDockWidget, false);

	window()->activateWindow();
	d->DockManager->notifyWidgetOrAreaRelocation(Widget);
}


//============================================================================
QList<CDockAreaWidget*> CDockContainerWidget::openedDockAreas() const
{
	QList<CDockAreaWidget*> Result;
	for (auto DockArea : d->DockAreas)
	{
		if (DockArea && !DockArea->isHidden())
		{
			Result.append(DockArea);
		}
	}

	return Result;
}


//============================================================================
QList<CDockWidget*> CDockContainerWidget::openedDockWidgets() const
{
	QList<CDockWidget*> DockWidgetList;
	for (auto DockArea : d->DockAreas)
	{
		if (DockArea && !DockArea->isHidden())
		{
			DockWidgetList.append(DockArea->openedDockWidgets());
		}
	}

	return DockWidgetList;
}


//============================================================================
bool CDockContainerWidget::hasOpenDockAreas() const
{
	for (auto DockArea : d->DockAreas)
	{
		if (DockArea && !DockArea->isHidden())
		{
			return true;
		}
	}

	return false;
}


//============================================================================
void CDockContainerWidget::saveState(QXmlStreamWriter& s) const
{
    ADS_PRINT("CDockContainerWidget::saveState isFloating "
        << isFloating());

	s.writeStartElement("Container");
	s.writeAttribute("Floating", QString::number(isFloating() ? 1 : 0));
	if (isFloating())
	{
		CFloatingDockContainer* FloatingWidget = floatingWidget();
		QByteArray Geometry = FloatingWidget->saveGeometry();
#if QT_VERSION < 0x050900
        s.writeTextElement("Geometry", qByteArrayToHex(Geometry, ' '));
#else
		s.writeTextElement("Geometry", Geometry.toHex(' '));
#endif
	}
	d->saveChildNodesState(s, d->RootSplitter);
	d->saveAutoHideWidgetsState(s);
	s.writeEndElement();
}


//============================================================================
bool CDockContainerWidget::restoreState(CDockingStateReader& s, bool Testing)
{
	bool IsFloating = s.attributes().value("Floating").toInt();
    ADS_PRINT("Restore CDockContainerWidget Floating" << IsFloating);

	QWidget* NewRootSplitter {};
	if (!Testing)
	{
		d->VisibleDockAreaCount = -1;// invalidate the dock area count
		d->DockAreas.clear();
		std::fill(std::begin(d->LastAddedAreaCache),std::end(d->LastAddedAreaCache), nullptr);
	}

	if (IsFloating)
	{
        ADS_PRINT("Restore floating widget");
        if (!s.readNextStartElement() || s.name() != QLatin1String("Geometry"))
		{
			return false;
		}

		QByteArray GeometryString = s.readElementText(CDockingStateReader::ErrorOnUnexpectedElement).toLocal8Bit();
		QByteArray Geometry = QByteArray::fromHex(GeometryString);
		if (Geometry.isEmpty())
		{
			return false;
		}

		if (!Testing)
		{
			CFloatingDockContainer* FloatingWidget = floatingWidget();
			if (FloatingWidget)
			{
				FloatingWidget->restoreGeometry(Geometry);
			}
		}
	}

	if (!d->restoreChildNodes(s, NewRootSplitter, Testing))
	{
		return false;
	}

	if (Testing)
	{
		return true;
	}

	// If the root splitter is empty, rostoreChildNodes returns a 0 pointer
	// and we need to create a new empty root splitter
	if (!NewRootSplitter)
	{
		NewRootSplitter = d->newSplitter(Qt::Horizontal);
	}

	QLayoutItem* li = d->Layout->replaceWidget(d->RootSplitter, NewRootSplitter);
	auto OldRoot = d->RootSplitter;
	d->RootSplitter = qobject_cast<CDockSplitter*>(NewRootSplitter);
	OldRoot->deleteLater();
	delete li;

	return true;
}


//============================================================================
CDockSplitter* CDockContainerWidget::rootSplitter() const
{
	return d->RootSplitter;
}


//============================================================================
void CDockContainerWidget::createRootSplitter()
{
	if (d->RootSplitter)
	{
		return;
	}
	d->RootSplitter = d->newSplitter(Qt::Horizontal);
	d->Layout->addWidget(d->RootSplitter, 1, 1); // Add it to the center - the 0 and 2 indexes are used for the SideTabBar widgets
}


//============================================================================
void CDockContainerWidget::createSideTabBarWidgets()
{
	if (!CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideFeatureEnabled))
	{
		return;
	}

	{
		auto Area = SideBarLocation::SideBarLeft;
        d->SideTabBarWidgets[Area] = new CAutoHideSideBar(this, Area);
		d->Layout->addWidget(d->SideTabBarWidgets[Area], 1, 0);
	}

	{
		auto Area = SideBarLocation::SideBarRight;
        d->SideTabBarWidgets[Area] = new CAutoHideSideBar(this, Area);
		d->Layout->addWidget(d->SideTabBarWidgets[Area], 1, 2);
	}

	{
		auto Area = SideBarLocation::SideBarBottom;
        d->SideTabBarWidgets[Area] = new CAutoHideSideBar(this, Area);
        d->Layout->addWidget(d->SideTabBarWidgets[Area], 2, 1);
	}

	{
		auto Area = SideBarLocation::SideBarTop;
        d->SideTabBarWidgets[Area] = new CAutoHideSideBar(this, Area);
        d->Layout->addWidget(d->SideTabBarWidgets[Area], 0, 1);
	}
}


//============================================================================
void CDockContainerWidget::dumpLayout()
{
#if (ADS_DEBUG_LEVEL > 0)
	qDebug("\n\nDumping layout --------------------------");
	std::cout << "\n\nDumping layout --------------------------" << std::endl;
	d->dumpRecursive(0, d->RootSplitter);
	qDebug("--------------------------\n\n");
	std::cout << "--------------------------\n\n" << std::endl;
#endif
}


//============================================================================
CDockAreaWidget* CDockContainerWidget::lastAddedDockAreaWidget(DockWidgetArea area) const
{
	return d->LastAddedAreaCache[areaIdToIndex(area)];
}


//============================================================================
bool CDockContainerWidget::hasTopLevelDockWidget() const
{
	auto DockAreas = openedDockAreas();
	if (DockAreas.count() != 1)
	{
		return false;
	}

	return DockAreas[0]->openDockWidgetsCount() == 1;
}


//============================================================================
CDockWidget* CDockContainerWidget::topLevelDockWidget() const
{
	auto TopLevelDockArea = topLevelDockArea();
	if (!TopLevelDockArea)
	{
		return nullptr;
	}

	auto DockWidgets = TopLevelDockArea->openedDockWidgets();
	if (DockWidgets.count() != 1)
	{
		return nullptr;
	}

	return DockWidgets[0];

}


//============================================================================
CDockAreaWidget* CDockContainerWidget::topLevelDockArea() const
{
	auto DockAreas = openedDockAreas();
	if (DockAreas.count() != 1)
	{
		return nullptr;
	}

	return DockAreas[0];
}


//============================================================================
QList<CDockWidget*> CDockContainerWidget::dockWidgets() const
{
	QList<CDockWidget*> Result;
    for (const auto& DockArea : d->DockAreas)
	{
		if (!DockArea)
		{
			continue;
		}
		Result.append(DockArea->dockWidgets());
	}

	return Result;
}

//============================================================================
void CDockContainerWidget::updateSplitterHandles(QSplitter* splitter)
{
    d->updateSplitterHandles(splitter);
}

//============================================================================
void CDockContainerWidget::registerAutoHideWidget(CAutoHideDockContainer* AutohideWidget)
{
	d->AutoHideWidgets.append(AutohideWidget);
	Q_EMIT autoHideWidgetCreated(AutohideWidget);
    ADS_PRINT("d->AutoHideWidgets.count() " << d->AutoHideWidgets.count());
}

//============================================================================
void CDockContainerWidget::removeAutoHideWidget(CAutoHideDockContainer* AutohideWidget)
{
	d->AutoHideWidgets.removeAll(AutohideWidget);
}

//============================================================================
CDockWidget::DockWidgetFeatures CDockContainerWidget::features() const
{
	CDockWidget::DockWidgetFeatures Features(CDockWidget::AllDockWidgetFeatures);
    for (const auto& DockArea : d->DockAreas)
	{
		if (!DockArea)
		{
			continue;
		}
		Features &= DockArea->features();
	}

	return Features;
}


//============================================================================
CFloatingDockContainer* CDockContainerWidget::floatingWidget() const
{
	return internal::findParent<CFloatingDockContainer*>(this);
}


//============================================================================
void CDockContainerWidget::closeOtherAreas(CDockAreaWidget* KeepOpenArea)
{
    for (const auto& DockArea : d->DockAreas)
	{
		if (!DockArea || DockArea == KeepOpenArea)
		{
			continue;
		}

		if (!DockArea->features(BitwiseAnd).testFlag(CDockWidget::DockWidgetClosable))
		{
			continue;
		}

		// We do not close areas with widgets with custom close handling
		if (DockArea->features(BitwiseOr).testFlag(CDockWidget::CustomCloseHandling))
		{
			continue;
		}

		DockArea->closeArea();
	}
}

//============================================================================
CAutoHideSideBar* CDockContainerWidget::autoHideSideBar(SideBarLocation area) const
{
	return d->SideTabBarWidgets[area];
}


//============================================================================
QRect CDockContainerWidget::contentRect() const
{
	if (!d->RootSplitter)
	{
		return QRect();
	}

	if (d->RootSplitter->hasVisibleContent())
	{
		return d->RootSplitter->geometry();
	}
	else
	{
		auto ContentRect = this->rect();
		ContentRect.adjust(
			autoHideSideBar(SideBarLeft)->sizeHint().width(),
			autoHideSideBar(SideBarTop)->sizeHint().height(),
			-autoHideSideBar(SideBarRight)->sizeHint().width(),
			-autoHideSideBar(SideBarBottom)->sizeHint().height());

		return ContentRect;
	}
}


//===========================================================================
QRect CDockContainerWidget::contentRectGlobal() const
{
	if (!d->RootSplitter)
	{
		return QRect();
	}
	return internal::globalGeometry(d->RootSplitter);
}


//===========================================================================
CDockManager* CDockContainerWidget::dockManager() const
{
	return d->DockManager;
}


//===========================================================================
void CDockContainerWidget::handleAutoHideWidgetEvent(QEvent* e, QWidget* w)
{
	if (!CDockManager::testAutoHideConfigFlag(CDockManager::AutoHideShowOnMouseOver))
	{
		return;
	}

	if (dockManager()->isRestoringState())
	{
		return;
	}

	auto AutoHideTab = qobject_cast<CAutoHideTab*>(w);
	if (AutoHideTab)
	{
		switch (e->type())
		{
		case QEvent::Enter:
			 if (!AutoHideTab->dockWidget()->isVisible())
			 {
				 d->DelayedAutoHideTab = AutoHideTab;
				 d->DelayedAutoHideShow = true;
				 d->DelayedAutoHideTimer.start();
			 }
			 else
			 {
				 d->DelayedAutoHideTimer.stop();
			 }
			 break;

		case QEvent::MouseButtonPress:
			 d->DelayedAutoHideTimer.stop();
			 break;

		case QEvent::Leave:
			 if (AutoHideTab->dockWidget()->isVisible())
			 {
				 d->DelayedAutoHideTab = AutoHideTab;
				 d->DelayedAutoHideShow = false;
				 d->DelayedAutoHideTimer.start();
			 }
			 else
			 {
				 d->DelayedAutoHideTimer.stop();
			 }
			 break;

		default:
			break;
		}
		return;
	}

	auto AutoHideContainer = qobject_cast<CAutoHideDockContainer*>(w);
	if (AutoHideContainer)
	{
		switch (e->type())
		{
		case QEvent::Enter:
		case QEvent::Hide:
			 d->DelayedAutoHideTimer.stop();
			 break;

		case QEvent::Leave:
			 if (AutoHideContainer->isVisible())
			 {
				 d->DelayedAutoHideTab = AutoHideContainer->autoHideTab();
				 d->DelayedAutoHideShow = false;
				 d->DelayedAutoHideTimer.start();
			 }
			 break;

		default:
			break;
		}
		return;
		return;
	}
}
} // namespace ads

//---------------------------------------------------------------------------
// EOF DockContainerWidget.cpp
