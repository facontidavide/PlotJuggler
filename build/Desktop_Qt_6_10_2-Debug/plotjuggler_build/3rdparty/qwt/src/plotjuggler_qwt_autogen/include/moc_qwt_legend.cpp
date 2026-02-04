/****************************************************************************
** Meta object code from reading C++ file 'qwt_legend.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../../../../PlotJuggler/3rdparty/qwt/src/qwt_legend.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qwt_legend.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_QwtLegend_t {
    QByteArrayData data[11];
    char stringdata0[103];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_QwtLegend_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_QwtLegend_t qt_meta_stringdata_QwtLegend = {
    {
QT_MOC_LITERAL(0, 0, 9), // "QwtLegend"
QT_MOC_LITERAL(1, 10, 7), // "clicked"
QT_MOC_LITERAL(2, 18, 0), // ""
QT_MOC_LITERAL(3, 19, 8), // "itemInfo"
QT_MOC_LITERAL(4, 28, 5), // "index"
QT_MOC_LITERAL(5, 34, 7), // "checked"
QT_MOC_LITERAL(6, 42, 2), // "on"
QT_MOC_LITERAL(7, 45, 12), // "updateLegend"
QT_MOC_LITERAL(8, 58, 20), // "QList<QwtLegendData>"
QT_MOC_LITERAL(9, 79, 11), // "itemClicked"
QT_MOC_LITERAL(10, 91, 11) // "itemChecked"

    },
    "QwtLegend\0clicked\0\0itemInfo\0index\0"
    "checked\0on\0updateLegend\0QList<QwtLegendData>\0"
    "itemClicked\0itemChecked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_QwtLegend[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   39,    2, 0x06 /* Public */,
       5,    3,   44,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    2,   51,    2, 0x0a /* Public */,
       9,    0,   56,    2, 0x09 /* Protected */,
      10,    1,   57,    2, 0x09 /* Protected */,

 // signals: parameters
    QMetaType::Void, QMetaType::QVariant, QMetaType::Int,    3,    4,
    QMetaType::Void, QMetaType::QVariant, QMetaType::Bool, QMetaType::Int,    3,    6,    4,

 // slots: parameters
    QMetaType::Void, QMetaType::QVariant, 0x80000000 | 8,    2,    2,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    2,

       0        // eod
};

void QwtLegend::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<QwtLegend *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->clicked((*reinterpret_cast< const QVariant(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: _t->checked((*reinterpret_cast< const QVariant(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 2: _t->updateLegend((*reinterpret_cast< const QVariant(*)>(_a[1])),(*reinterpret_cast< const QList<QwtLegendData>(*)>(_a[2]))); break;
        case 3: _t->itemClicked(); break;
        case 4: _t->itemChecked((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (QwtLegend::*)(const QVariant & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&QwtLegend::clicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (QwtLegend::*)(const QVariant & , bool , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&QwtLegend::checked)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject QwtLegend::staticMetaObject = { {
    QMetaObject::SuperData::link<QwtAbstractLegend::staticMetaObject>(),
    qt_meta_stringdata_QwtLegend.data,
    qt_meta_data_QwtLegend,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *QwtLegend::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QwtLegend::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_QwtLegend.stringdata0))
        return static_cast<void*>(this);
    return QwtAbstractLegend::qt_metacast(_clname);
}

int QwtLegend::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QwtAbstractLegend::qt_metacall(_c, _id, _a);
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
void QwtLegend::clicked(const QVariant & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void QwtLegend::checked(const QVariant & _t1, bool _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
