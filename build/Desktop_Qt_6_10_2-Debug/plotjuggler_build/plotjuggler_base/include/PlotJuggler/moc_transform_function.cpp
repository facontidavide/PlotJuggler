/****************************************************************************
** Meta object code from reading C++ file 'transform_function.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../../../PlotJuggler/plotjuggler_base/include/PlotJuggler/transform_function.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'transform_function.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_PJ__TransformFunction_t {
    QByteArrayData data[3];
    char stringdata0[41];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_PJ__TransformFunction_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_PJ__TransformFunction_t qt_meta_stringdata_PJ__TransformFunction = {
    {
QT_MOC_LITERAL(0, 0, 21), // "PJ::TransformFunction"
QT_MOC_LITERAL(1, 22, 17), // "parametersChanged"
QT_MOC_LITERAL(2, 40, 0) // ""

    },
    "PJ::TransformFunction\0parametersChanged\0"
    ""
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PJ__TransformFunction[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,

       0        // eod
};

void PJ::TransformFunction::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TransformFunction *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->parametersChanged(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TransformFunction::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TransformFunction::parametersChanged)) {
                *result = 0;
                return;
            }
        }
    }
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject PJ::TransformFunction::staticMetaObject = { {
    QMetaObject::SuperData::link<PlotJugglerPlugin::staticMetaObject>(),
    qt_meta_stringdata_PJ__TransformFunction.data,
    qt_meta_data_PJ__TransformFunction,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *PJ::TransformFunction::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PJ::TransformFunction::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PJ__TransformFunction.stringdata0))
        return static_cast<void*>(this);
    return PlotJugglerPlugin::qt_metacast(_clname);
}

int PJ::TransformFunction::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = PlotJugglerPlugin::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void PJ::TransformFunction::parametersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
struct qt_meta_stringdata_PJ__TransformFunction_SISO_t {
    QByteArrayData data[1];
    char stringdata0[27];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_PJ__TransformFunction_SISO_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_PJ__TransformFunction_SISO_t qt_meta_stringdata_PJ__TransformFunction_SISO = {
    {
QT_MOC_LITERAL(0, 0, 26) // "PJ::TransformFunction_SISO"

    },
    "PJ::TransformFunction_SISO"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PJ__TransformFunction_SISO[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void PJ::TransformFunction_SISO::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject PJ::TransformFunction_SISO::staticMetaObject = { {
    QMetaObject::SuperData::link<TransformFunction::staticMetaObject>(),
    qt_meta_stringdata_PJ__TransformFunction_SISO.data,
    qt_meta_data_PJ__TransformFunction_SISO,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *PJ::TransformFunction_SISO::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PJ::TransformFunction_SISO::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PJ__TransformFunction_SISO.stringdata0))
        return static_cast<void*>(this);
    return TransformFunction::qt_metacast(_clname);
}

int PJ::TransformFunction_SISO::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = TransformFunction::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
