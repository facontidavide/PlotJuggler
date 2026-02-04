/****************************************************************************
** Meta object code from reading C++ file 'qwt_plot_zoomer.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../../../../PlotJuggler/3rdparty/qwt/src/qwt_plot_zoomer.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qwt_plot_zoomer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_QwtPlotZoomer_t {
    QByteArrayData data[10];
    char stringdata0[59];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_QwtPlotZoomer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_QwtPlotZoomer_t qt_meta_stringdata_QwtPlotZoomer = {
    {
QT_MOC_LITERAL(0, 0, 13), // "QwtPlotZoomer"
QT_MOC_LITERAL(1, 14, 6), // "zoomed"
QT_MOC_LITERAL(2, 21, 0), // ""
QT_MOC_LITERAL(3, 22, 4), // "rect"
QT_MOC_LITERAL(4, 27, 6), // "moveBy"
QT_MOC_LITERAL(5, 34, 2), // "dx"
QT_MOC_LITERAL(6, 37, 2), // "dy"
QT_MOC_LITERAL(7, 40, 6), // "moveTo"
QT_MOC_LITERAL(8, 47, 4), // "zoom"
QT_MOC_LITERAL(9, 52, 6) // "offset"

    },
    "QwtPlotZoomer\0zoomed\0\0rect\0moveBy\0dx\0"
    "dy\0moveTo\0zoom\0offset"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_QwtPlotZoomer[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   39,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    2,   42,    2, 0x0a /* Public */,
       7,    1,   47,    2, 0x0a /* Public */,
       8,    1,   50,    2, 0x0a /* Public */,
       8,    1,   53,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QRectF,    3,

 // slots: parameters
    QMetaType::Void, QMetaType::Double, QMetaType::Double,    5,    6,
    QMetaType::Void, QMetaType::QPointF,    2,
    QMetaType::Void, QMetaType::QRectF,    2,
    QMetaType::Void, QMetaType::Int,    9,

       0        // eod
};

void QwtPlotZoomer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<QwtPlotZoomer *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->zoomed((*reinterpret_cast< const QRectF(*)>(_a[1]))); break;
        case 1: _t->moveBy((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 2: _t->moveTo((*reinterpret_cast< const QPointF(*)>(_a[1]))); break;
        case 3: _t->zoom((*reinterpret_cast< const QRectF(*)>(_a[1]))); break;
        case 4: _t->zoom((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (QwtPlotZoomer::*)(const QRectF & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&QwtPlotZoomer::zoomed)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject QwtPlotZoomer::staticMetaObject = { {
    QMetaObject::SuperData::link<QwtPlotPicker::staticMetaObject>(),
    qt_meta_stringdata_QwtPlotZoomer.data,
    qt_meta_data_QwtPlotZoomer,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *QwtPlotZoomer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QwtPlotZoomer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_QwtPlotZoomer.stringdata0))
        return static_cast<void*>(this);
    return QwtPlotPicker::qt_metacast(_clname);
}

int QwtPlotZoomer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QwtPlotPicker::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void QwtPlotZoomer::zoomed(const QRectF & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
