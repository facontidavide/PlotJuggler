#ifndef ResizeHandleH
#define ResizeHandleH
//============================================================================
/// \file   ResizeHandle.h
/// \author Uwe Kindler
/// \date   24.10.2022
/// \brief  Declaration of CResizeHandle class
//============================================================================

//============================================================================
//                                   INCLUDES
//============================================================================
#include "ads_globals.h"
#include <QFrame>

namespace ads
{
struct ResizeHandlePrivate;

/**
 * Resize handle for resizing its parent widget
 */
class ADS_EXPORT CResizeHandle : public QFrame
{
	Q_OBJECT
	Q_DISABLE_COPY(CResizeHandle)
    Q_PROPERTY(bool opaqueResize READ opaqueResize WRITE setOpaqueResize)
private:
	ResizeHandlePrivate* d; ///< private data (pimpl)
	friend struct ResizeHandlePrivate;

protected:
	void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

public:
	using Super = QFrame;

	/**
	 * Default Constructor
	 */
	CResizeHandle(Qt::Edge HandlePosition, QWidget* parent);

	/**
	 * Virtual Destructor
	 */
	virtual ~CResizeHandle();

	/**
	 * Sets the handle position
	 */
	void setHandlePosition(Qt::Edge HandlePosition);

	/**
	 * Returns the handle position
	 */
	Qt::Edge handlePostion() const;

	/**
	 * Returns the orientation of this resize handle
	 */
	Qt::Orientation orientation() const;

	/**
	 * Returns the size hint
	 */
	QSize sizeHint() const override;

	/**
	 * Returns true, if resizing is active
	 */
	bool isResizing() const;

	/**
	 * Sets the minimum size for the widget that is going to be resized.
	 * The resize handle will not resize the target widget to a size smaller
	 * than this value
	 */
	void setMinResizeSize(int MinSize);

	/**
	 * Sets the maximum size for the widget that is going to be resized
	 * The resize handle will not resize the target widget to a size bigger
	 * than this value
	 */
	void setMaxResizeSize(int MaxSize);

	/**
	 * Enable / disable opaque resizing
	 */
	void setOpaqueResize(bool opaque = true);

	/**
	 * Returns true if widgets are resized dynamically (opaquely) while
	 * interactively moving the resize handle. Otherwise returns false.
	 */
	bool opaqueResize() const;
}; // class name
} // namespace ads
//-----------------------------------------------------------------------------
#endif // ResizeHandleH
