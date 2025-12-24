/****************************************************************************
** Meta object code from reading C++ file 'qwt_plot_opengl_canvas.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../3rdparty/qwt/src/qwt_plot_opengl_canvas.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qwt_plot_opengl_canvas.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_QwtPlotOpenGLCanvas_t {
    QByteArrayData data[15];
    char stringdata0[184];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_QwtPlotOpenGLCanvas_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_QwtPlotOpenGLCanvas_t qt_meta_stringdata_QwtPlotOpenGLCanvas = {
    {
QT_MOC_LITERAL(0, 0, 19), // "QwtPlotOpenGLCanvas"
QT_MOC_LITERAL(1, 20, 6), // "replot"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 22), // "invalidateBackingStore"
QT_MOC_LITERAL(4, 51, 10), // "borderPath"
QT_MOC_LITERAL(5, 62, 12), // "QPainterPath"
QT_MOC_LITERAL(6, 75, 11), // "frameShadow"
QT_MOC_LITERAL(7, 87, 14), // "QFrame::Shadow"
QT_MOC_LITERAL(8, 102, 10), // "frameShape"
QT_MOC_LITERAL(9, 113, 13), // "QFrame::Shape"
QT_MOC_LITERAL(10, 127, 9), // "lineWidth"
QT_MOC_LITERAL(11, 137, 12), // "midLineWidth"
QT_MOC_LITERAL(12, 150, 10), // "frameWidth"
QT_MOC_LITERAL(13, 161, 9), // "frameRect"
QT_MOC_LITERAL(14, 171, 12) // "borderRadius"

    },
    "QwtPlotOpenGLCanvas\0replot\0\0"
    "invalidateBackingStore\0borderPath\0"
    "QPainterPath\0frameShadow\0QFrame::Shadow\0"
    "frameShape\0QFrame::Shape\0lineWidth\0"
    "midLineWidth\0frameWidth\0frameRect\0"
    "borderRadius"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_QwtPlotOpenGLCanvas[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       7,   34, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x0a /* Public */,

 // methods: name, argc, parameters, tag, flags
       3,    0,   30,    2, 0x02 /* Public */,
       4,    1,   31,    2, 0x02 /* Public */,

 // slots: parameters
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void,
    0x80000000 | 5, QMetaType::QRect,    2,

 // properties: name, type, flags
       6, 0x80000000 | 7, 0x0009510b,
       8, 0x80000000 | 9, 0x0009510b,
      10, QMetaType::Int, 0x00095103,
      11, QMetaType::Int, 0x00095103,
      12, QMetaType::Int, 0x00095001,
      13, QMetaType::QRect, 0x00094001,
      14, QMetaType::Double, 0x00095103,

       0        // eod
};

void QwtPlotOpenGLCanvas::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<QwtPlotOpenGLCanvas *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->replot(); break;
        case 1: _t->invalidateBackingStore(); break;
        case 2: { QPainterPath _r = _t->borderPath((*reinterpret_cast< const QRect(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QPainterPath*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<QwtPlotOpenGLCanvas *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QFrame::Shadow*>(_v) = _t->frameShadow(); break;
        case 1: *reinterpret_cast< QFrame::Shape*>(_v) = _t->frameShape(); break;
        case 2: *reinterpret_cast< int*>(_v) = _t->lineWidth(); break;
        case 3: *reinterpret_cast< int*>(_v) = _t->midLineWidth(); break;
        case 4: *reinterpret_cast< int*>(_v) = _t->frameWidth(); break;
        case 5: *reinterpret_cast< QRect*>(_v) = _t->frameRect(); break;
        case 6: *reinterpret_cast< double*>(_v) = _t->borderRadius(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<QwtPlotOpenGLCanvas *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setFrameShadow(*reinterpret_cast< QFrame::Shadow*>(_v)); break;
        case 1: _t->setFrameShape(*reinterpret_cast< QFrame::Shape*>(_v)); break;
        case 2: _t->setLineWidth(*reinterpret_cast< int*>(_v)); break;
        case 3: _t->setMidLineWidth(*reinterpret_cast< int*>(_v)); break;
        case 6: _t->setBorderRadius(*reinterpret_cast< double*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

static const QMetaObject::SuperData qt_meta_extradata_QwtPlotOpenGLCanvas[] = {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    nullptr
};

QT_INIT_METAOBJECT const QMetaObject QwtPlotOpenGLCanvas::staticMetaObject = { {
    QMetaObject::SuperData::link<QOpenGLWidget::staticMetaObject>(),
    qt_meta_stringdata_QwtPlotOpenGLCanvas.data,
    qt_meta_data_QwtPlotOpenGLCanvas,
    qt_static_metacall,
    qt_meta_extradata_QwtPlotOpenGLCanvas,
    nullptr
} };


const QMetaObject *QwtPlotOpenGLCanvas::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QwtPlotOpenGLCanvas::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_QwtPlotOpenGLCanvas.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QwtPlotAbstractGLCanvas"))
        return static_cast< QwtPlotAbstractGLCanvas*>(this);
    return QOpenGLWidget::qt_metacast(_clname);
}

int QwtPlotOpenGLCanvas::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QOpenGLWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 7;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 7;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 7;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 7;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 7;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
