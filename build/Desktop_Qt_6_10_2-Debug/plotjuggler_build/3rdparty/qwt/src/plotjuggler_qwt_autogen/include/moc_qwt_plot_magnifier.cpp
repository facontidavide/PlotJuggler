/****************************************************************************
** Meta object code from reading C++ file 'qwt_plot_magnifier.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../../../../PlotJuggler/3rdparty/qwt/src/qwt_plot_magnifier.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qwt_plot_magnifier.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_QwtPlotMagnifier_t {
    QByteArrayData data[4];
    char stringdata0[33];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_QwtPlotMagnifier_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_QwtPlotMagnifier_t qt_meta_stringdata_QwtPlotMagnifier = {
    {
QT_MOC_LITERAL(0, 0, 16), // "QwtPlotMagnifier"
QT_MOC_LITERAL(1, 17, 7), // "rescale"
QT_MOC_LITERAL(2, 25, 0), // ""
QT_MOC_LITERAL(3, 26, 6) // "factor"

    },
    "QwtPlotMagnifier\0rescale\0\0factor"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_QwtPlotMagnifier[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   19,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::Double,    3,

       0        // eod
};

void QwtPlotMagnifier::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<QwtPlotMagnifier *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->rescale((*reinterpret_cast< double(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject QwtPlotMagnifier::staticMetaObject = { {
    QMetaObject::SuperData::link<QwtMagnifier::staticMetaObject>(),
    qt_meta_stringdata_QwtPlotMagnifier.data,
    qt_meta_data_QwtPlotMagnifier,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *QwtPlotMagnifier::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QwtPlotMagnifier::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_QwtPlotMagnifier.stringdata0))
        return static_cast<void*>(this);
    return QwtMagnifier::qt_metacast(_clname);
}

int QwtPlotMagnifier::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QwtMagnifier::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
