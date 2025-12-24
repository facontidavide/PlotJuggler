/****************************************************************************
** Meta object code from reading C++ file 'color_wheel.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../3rdparty/color_widgets/include/color_wheel.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'color_wheel.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_color_widgets__ColorWheel_t {
    QByteArrayData data[39];
    char stringdata0[431];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_color_widgets__ColorWheel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_color_widgets__ColorWheel_t qt_meta_stringdata_color_widgets__ColorWheel = {
    {
QT_MOC_LITERAL(0, 0, 25), // "color_widgets::ColorWheel"
QT_MOC_LITERAL(1, 26, 12), // "colorChanged"
QT_MOC_LITERAL(2, 39, 0), // ""
QT_MOC_LITERAL(3, 40, 13), // "colorSelected"
QT_MOC_LITERAL(4, 54, 19), // "displayFlagsChanged"
QT_MOC_LITERAL(5, 74, 24), // "ColorWheel::DisplayFlags"
QT_MOC_LITERAL(6, 99, 5), // "flags"
QT_MOC_LITERAL(7, 105, 8), // "setColor"
QT_MOC_LITERAL(8, 114, 1), // "c"
QT_MOC_LITERAL(9, 116, 6), // "setHue"
QT_MOC_LITERAL(10, 123, 1), // "h"
QT_MOC_LITERAL(11, 125, 13), // "setSaturation"
QT_MOC_LITERAL(12, 139, 1), // "s"
QT_MOC_LITERAL(13, 141, 8), // "setValue"
QT_MOC_LITERAL(14, 150, 1), // "v"
QT_MOC_LITERAL(15, 152, 15), // "setDisplayFlags"
QT_MOC_LITERAL(16, 168, 5), // "color"
QT_MOC_LITERAL(17, 174, 3), // "hue"
QT_MOC_LITERAL(18, 178, 10), // "saturation"
QT_MOC_LITERAL(19, 189, 5), // "value"
QT_MOC_LITERAL(20, 195, 10), // "wheelWidth"
QT_MOC_LITERAL(21, 206, 12), // "displayFlags"
QT_MOC_LITERAL(22, 219, 12), // "DisplayFlags"
QT_MOC_LITERAL(23, 232, 11), // "DisplayEnum"
QT_MOC_LITERAL(24, 244, 13), // "SHAPE_DEFAULT"
QT_MOC_LITERAL(25, 258, 14), // "SHAPE_TRIANGLE"
QT_MOC_LITERAL(26, 273, 12), // "SHAPE_SQUARE"
QT_MOC_LITERAL(27, 286, 11), // "SHAPE_FLAGS"
QT_MOC_LITERAL(28, 298, 13), // "ANGLE_DEFAULT"
QT_MOC_LITERAL(29, 312, 11), // "ANGLE_FIXED"
QT_MOC_LITERAL(30, 324, 14), // "ANGLE_ROTATING"
QT_MOC_LITERAL(31, 339, 11), // "ANGLE_FLAGS"
QT_MOC_LITERAL(32, 351, 13), // "COLOR_DEFAULT"
QT_MOC_LITERAL(33, 365, 9), // "COLOR_HSV"
QT_MOC_LITERAL(34, 375, 9), // "COLOR_HSL"
QT_MOC_LITERAL(35, 385, 9), // "COLOR_LCH"
QT_MOC_LITERAL(36, 395, 11), // "COLOR_FLAGS"
QT_MOC_LITERAL(37, 407, 13), // "FLAGS_DEFAULT"
QT_MOC_LITERAL(38, 421, 9) // "FLAGS_ALL"

    },
    "color_widgets::ColorWheel\0colorChanged\0"
    "\0colorSelected\0displayFlagsChanged\0"
    "ColorWheel::DisplayFlags\0flags\0setColor\0"
    "c\0setHue\0h\0setSaturation\0s\0setValue\0"
    "v\0setDisplayFlags\0color\0hue\0saturation\0"
    "value\0wheelWidth\0displayFlags\0"
    "DisplayFlags\0DisplayEnum\0SHAPE_DEFAULT\0"
    "SHAPE_TRIANGLE\0SHAPE_SQUARE\0SHAPE_FLAGS\0"
    "ANGLE_DEFAULT\0ANGLE_FIXED\0ANGLE_ROTATING\0"
    "ANGLE_FLAGS\0COLOR_DEFAULT\0COLOR_HSV\0"
    "COLOR_HSL\0COLOR_LCH\0COLOR_FLAGS\0"
    "FLAGS_DEFAULT\0FLAGS_ALL"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_color_widgets__ColorWheel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       6,   78, // properties
       1,  102, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x06 /* Public */,
       3,    1,   57,    2, 0x06 /* Public */,
       4,    1,   60,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    1,   63,    2, 0x0a /* Public */,
       9,    1,   66,    2, 0x0a /* Public */,
      11,    1,   69,    2, 0x0a /* Public */,
      13,    1,   72,    2, 0x0a /* Public */,
      15,    1,   75,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QColor,    2,
    QMetaType::Void, QMetaType::QColor,    2,
    QMetaType::Void, 0x80000000 | 5,    6,

 // slots: parameters
    QMetaType::Void, QMetaType::QColor,    8,
    QMetaType::Void, QMetaType::QReal,   10,
    QMetaType::Void, QMetaType::QReal,   12,
    QMetaType::Void, QMetaType::QReal,   14,
    QMetaType::Void, 0x80000000 | 5,    6,

 // properties: name, type, flags
      16, QMetaType::QColor, 0x00485103,
      17, QMetaType::QReal, 0x00094103,
      18, QMetaType::QReal, 0x00094103,
      19, QMetaType::QReal, 0x00094103,
      20, QMetaType::UInt, 0x00095103,
      21, 0x80000000 | 22, 0x0049510b,

 // properties: notify_signal_id
       0,
       0,
       0,
       0,
       0,
       2,

 // enums: name, alias, flags, count, data
      22,   23, 0x1,   15,  107,

 // enum data: key, value
      24, uint(color_widgets::ColorWheel::SHAPE_DEFAULT),
      25, uint(color_widgets::ColorWheel::SHAPE_TRIANGLE),
      26, uint(color_widgets::ColorWheel::SHAPE_SQUARE),
      27, uint(color_widgets::ColorWheel::SHAPE_FLAGS),
      28, uint(color_widgets::ColorWheel::ANGLE_DEFAULT),
      29, uint(color_widgets::ColorWheel::ANGLE_FIXED),
      30, uint(color_widgets::ColorWheel::ANGLE_ROTATING),
      31, uint(color_widgets::ColorWheel::ANGLE_FLAGS),
      32, uint(color_widgets::ColorWheel::COLOR_DEFAULT),
      33, uint(color_widgets::ColorWheel::COLOR_HSV),
      34, uint(color_widgets::ColorWheel::COLOR_HSL),
      35, uint(color_widgets::ColorWheel::COLOR_LCH),
      36, uint(color_widgets::ColorWheel::COLOR_FLAGS),
      37, uint(color_widgets::ColorWheel::FLAGS_DEFAULT),
      38, uint(color_widgets::ColorWheel::FLAGS_ALL),

       0        // eod
};

void color_widgets::ColorWheel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ColorWheel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->colorChanged((*reinterpret_cast< QColor(*)>(_a[1]))); break;
        case 1: _t->colorSelected((*reinterpret_cast< QColor(*)>(_a[1]))); break;
        case 2: _t->displayFlagsChanged((*reinterpret_cast< ColorWheel::DisplayFlags(*)>(_a[1]))); break;
        case 3: _t->setColor((*reinterpret_cast< QColor(*)>(_a[1]))); break;
        case 4: _t->setHue((*reinterpret_cast< qreal(*)>(_a[1]))); break;
        case 5: _t->setSaturation((*reinterpret_cast< qreal(*)>(_a[1]))); break;
        case 6: _t->setValue((*reinterpret_cast< qreal(*)>(_a[1]))); break;
        case 7: _t->setDisplayFlags((*reinterpret_cast< ColorWheel::DisplayFlags(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ColorWheel::*)(QColor );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ColorWheel::colorChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ColorWheel::*)(QColor );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ColorWheel::colorSelected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ColorWheel::*)(ColorWheel::DisplayFlags );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ColorWheel::displayFlagsChanged)) {
                *result = 2;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<ColorWheel *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QColor*>(_v) = _t->color(); break;
        case 1: *reinterpret_cast< qreal*>(_v) = _t->hue(); break;
        case 2: *reinterpret_cast< qreal*>(_v) = _t->saturation(); break;
        case 3: *reinterpret_cast< qreal*>(_v) = _t->value(); break;
        case 4: *reinterpret_cast< uint*>(_v) = _t->wheelWidth(); break;
        case 5: *reinterpret_cast<int*>(_v) = QFlag(_t->displayFlags()); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<ColorWheel *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setColor(*reinterpret_cast< QColor*>(_v)); break;
        case 1: _t->setHue(*reinterpret_cast< qreal*>(_v)); break;
        case 2: _t->setSaturation(*reinterpret_cast< qreal*>(_v)); break;
        case 3: _t->setValue(*reinterpret_cast< qreal*>(_v)); break;
        case 4: _t->setWheelWidth(*reinterpret_cast< uint*>(_v)); break;
        case 5: _t->setDisplayFlags(QFlag(*reinterpret_cast<int*>(_v))); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject color_widgets::ColorWheel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_color_widgets__ColorWheel.data,
    qt_meta_data_color_widgets__ColorWheel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *color_widgets::ColorWheel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *color_widgets::ColorWheel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_color_widgets__ColorWheel.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int color_widgets::ColorWheel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 6;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 6;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 6;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 6;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 6;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void color_widgets::ColorWheel::colorChanged(QColor _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void color_widgets::ColorWheel::colorSelected(QColor _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void color_widgets::ColorWheel::displayFlagsChanged(ColorWheel::DisplayFlags _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
