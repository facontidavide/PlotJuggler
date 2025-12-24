/****************************************************************************
** Meta object code from reading C++ file 'qwt_picker.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../3rdparty/qwt/src/qwt_picker.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qwt_picker.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_QwtPicker_t {
    QByteArrayData data[36];
    char stringdata0[386];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_QwtPicker_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_QwtPicker_t qt_meta_stringdata_QwtPicker = {
    {
QT_MOC_LITERAL(0, 0, 9), // "QwtPicker"
QT_MOC_LITERAL(1, 10, 9), // "activated"
QT_MOC_LITERAL(2, 20, 0), // ""
QT_MOC_LITERAL(3, 21, 2), // "on"
QT_MOC_LITERAL(4, 24, 8), // "selected"
QT_MOC_LITERAL(5, 33, 7), // "polygon"
QT_MOC_LITERAL(6, 41, 8), // "appended"
QT_MOC_LITERAL(7, 50, 3), // "pos"
QT_MOC_LITERAL(8, 54, 5), // "moved"
QT_MOC_LITERAL(9, 60, 7), // "removed"
QT_MOC_LITERAL(10, 68, 7), // "changed"
QT_MOC_LITERAL(11, 76, 9), // "selection"
QT_MOC_LITERAL(12, 86, 10), // "setEnabled"
QT_MOC_LITERAL(13, 97, 9), // "isEnabled"
QT_MOC_LITERAL(14, 107, 10), // "resizeMode"
QT_MOC_LITERAL(15, 118, 10), // "ResizeMode"
QT_MOC_LITERAL(16, 129, 11), // "trackerMode"
QT_MOC_LITERAL(17, 141, 11), // "DisplayMode"
QT_MOC_LITERAL(18, 153, 10), // "trackerPen"
QT_MOC_LITERAL(19, 164, 11), // "trackerFont"
QT_MOC_LITERAL(20, 176, 10), // "rubberBand"
QT_MOC_LITERAL(21, 187, 10), // "RubberBand"
QT_MOC_LITERAL(22, 198, 13), // "rubberBandPen"
QT_MOC_LITERAL(23, 212, 12), // "NoRubberBand"
QT_MOC_LITERAL(24, 225, 15), // "HLineRubberBand"
QT_MOC_LITERAL(25, 241, 15), // "VLineRubberBand"
QT_MOC_LITERAL(26, 257, 15), // "CrossRubberBand"
QT_MOC_LITERAL(27, 273, 14), // "RectRubberBand"
QT_MOC_LITERAL(28, 288, 17), // "EllipseRubberBand"
QT_MOC_LITERAL(29, 306, 17), // "PolygonRubberBand"
QT_MOC_LITERAL(30, 324, 14), // "UserRubberBand"
QT_MOC_LITERAL(31, 339, 9), // "AlwaysOff"
QT_MOC_LITERAL(32, 349, 8), // "AlwaysOn"
QT_MOC_LITERAL(33, 358, 10), // "ActiveOnly"
QT_MOC_LITERAL(34, 369, 7), // "Stretch"
QT_MOC_LITERAL(35, 377, 8) // "KeepSize"

    },
    "QwtPicker\0activated\0\0on\0selected\0"
    "polygon\0appended\0pos\0moved\0removed\0"
    "changed\0selection\0setEnabled\0isEnabled\0"
    "resizeMode\0ResizeMode\0trackerMode\0"
    "DisplayMode\0trackerPen\0trackerFont\0"
    "rubberBand\0RubberBand\0rubberBandPen\0"
    "NoRubberBand\0HLineRubberBand\0"
    "VLineRubberBand\0CrossRubberBand\0"
    "RectRubberBand\0EllipseRubberBand\0"
    "PolygonRubberBand\0UserRubberBand\0"
    "AlwaysOff\0AlwaysOn\0ActiveOnly\0Stretch\0"
    "KeepSize"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_QwtPicker[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       7,   70, // properties
       3,   91, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,
       4,    1,   52,    2, 0x06 /* Public */,
       6,    1,   55,    2, 0x06 /* Public */,
       8,    1,   58,    2, 0x06 /* Public */,
       9,    1,   61,    2, 0x06 /* Public */,
      10,    1,   64,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    1,   67,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, QMetaType::QPolygon,    5,
    QMetaType::Void, QMetaType::QPoint,    7,
    QMetaType::Void, QMetaType::QPoint,    7,
    QMetaType::Void, QMetaType::QPoint,    7,
    QMetaType::Void, QMetaType::QPolygon,   11,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    2,

 // properties: name, type, flags
      13, QMetaType::Bool, 0x00095003,
      14, 0x80000000 | 15, 0x0009510b,
      16, 0x80000000 | 17, 0x0009510b,
      18, QMetaType::QPen, 0x00095103,
      19, QMetaType::QFont, 0x00095103,
      20, 0x80000000 | 21, 0x0009510b,
      22, QMetaType::QPen, 0x00095103,

 // enums: name, alias, flags, count, data
      21,   21, 0x0,    8,  106,
      17,   17, 0x0,    3,  122,
      15,   15, 0x0,    2,  128,

 // enum data: key, value
      23, uint(QwtPicker::NoRubberBand),
      24, uint(QwtPicker::HLineRubberBand),
      25, uint(QwtPicker::VLineRubberBand),
      26, uint(QwtPicker::CrossRubberBand),
      27, uint(QwtPicker::RectRubberBand),
      28, uint(QwtPicker::EllipseRubberBand),
      29, uint(QwtPicker::PolygonRubberBand),
      30, uint(QwtPicker::UserRubberBand),
      31, uint(QwtPicker::AlwaysOff),
      32, uint(QwtPicker::AlwaysOn),
      33, uint(QwtPicker::ActiveOnly),
      34, uint(QwtPicker::Stretch),
      35, uint(QwtPicker::KeepSize),

       0        // eod
};

void QwtPicker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<QwtPicker *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->activated((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->selected((*reinterpret_cast< const QPolygon(*)>(_a[1]))); break;
        case 2: _t->appended((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 3: _t->moved((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 4: _t->removed((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 5: _t->changed((*reinterpret_cast< const QPolygon(*)>(_a[1]))); break;
        case 6: _t->setEnabled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (QwtPicker::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&QwtPicker::activated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (QwtPicker::*)(const QPolygon & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&QwtPicker::selected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (QwtPicker::*)(const QPoint & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&QwtPicker::appended)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (QwtPicker::*)(const QPoint & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&QwtPicker::moved)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (QwtPicker::*)(const QPoint & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&QwtPicker::removed)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (QwtPicker::*)(const QPolygon & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&QwtPicker::changed)) {
                *result = 5;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<QwtPicker *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isEnabled(); break;
        case 1: *reinterpret_cast< ResizeMode*>(_v) = _t->resizeMode(); break;
        case 2: *reinterpret_cast< DisplayMode*>(_v) = _t->trackerMode(); break;
        case 3: *reinterpret_cast< QPen*>(_v) = _t->trackerPen(); break;
        case 4: *reinterpret_cast< QFont*>(_v) = _t->trackerFont(); break;
        case 5: *reinterpret_cast< RubberBand*>(_v) = _t->rubberBand(); break;
        case 6: *reinterpret_cast< QPen*>(_v) = _t->rubberBandPen(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<QwtPicker *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 1: _t->setResizeMode(*reinterpret_cast< ResizeMode*>(_v)); break;
        case 2: _t->setTrackerMode(*reinterpret_cast< DisplayMode*>(_v)); break;
        case 3: _t->setTrackerPen(*reinterpret_cast< QPen*>(_v)); break;
        case 4: _t->setTrackerFont(*reinterpret_cast< QFont*>(_v)); break;
        case 5: _t->setRubberBand(*reinterpret_cast< RubberBand*>(_v)); break;
        case 6: _t->setRubberBandPen(*reinterpret_cast< QPen*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject QwtPicker::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_QwtPicker.data,
    qt_meta_data_QwtPicker,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *QwtPicker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QwtPicker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_QwtPicker.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QwtEventPattern"))
        return static_cast< QwtEventPattern*>(this);
    return QObject::qt_metacast(_clname);
}

int QwtPicker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
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

// SIGNAL 0
void QwtPicker::activated(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void QwtPicker::selected(const QPolygon & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void QwtPicker::appended(const QPoint & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void QwtPicker::moved(const QPoint & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void QwtPicker::removed(const QPoint & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void QwtPicker::changed(const QPolygon & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
