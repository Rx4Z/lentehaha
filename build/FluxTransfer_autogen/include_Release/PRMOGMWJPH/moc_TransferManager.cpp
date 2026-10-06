/****************************************************************************
** Meta object code from reading C++ file 'TransferManager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/core/TransferManager.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TransferManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.5.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSTransferManagerENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSTransferManagerENDCLASS = QtMocHelpers::stringData(
    "TransferManager",
    "itemAdded",
    "",
    "TransferItem",
    "item",
    "itemUpdated",
    "itemRemoved",
    "id",
    "itemStarted",
    "itemProgress",
    "bytesTransferred",
    "totalBytes",
    "speedBps",
    "itemCompleted",
    "success",
    "error",
    "allCompleted",
    "onWorkerFinished"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSTransferManagerENDCLASS_t {
    uint offsetsAndSizes[36];
    char stringdata0[16];
    char stringdata1[10];
    char stringdata2[1];
    char stringdata3[13];
    char stringdata4[5];
    char stringdata5[12];
    char stringdata6[12];
    char stringdata7[3];
    char stringdata8[12];
    char stringdata9[13];
    char stringdata10[17];
    char stringdata11[11];
    char stringdata12[9];
    char stringdata13[14];
    char stringdata14[8];
    char stringdata15[6];
    char stringdata16[13];
    char stringdata17[17];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSTransferManagerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSTransferManagerENDCLASS_t qt_meta_stringdata_CLASSTransferManagerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 15),  // "TransferManager"
        QT_MOC_LITERAL(16, 9),  // "itemAdded"
        QT_MOC_LITERAL(26, 0),  // ""
        QT_MOC_LITERAL(27, 12),  // "TransferItem"
        QT_MOC_LITERAL(40, 4),  // "item"
        QT_MOC_LITERAL(45, 11),  // "itemUpdated"
        QT_MOC_LITERAL(57, 11),  // "itemRemoved"
        QT_MOC_LITERAL(69, 2),  // "id"
        QT_MOC_LITERAL(72, 11),  // "itemStarted"
        QT_MOC_LITERAL(84, 12),  // "itemProgress"
        QT_MOC_LITERAL(97, 16),  // "bytesTransferred"
        QT_MOC_LITERAL(114, 10),  // "totalBytes"
        QT_MOC_LITERAL(125, 8),  // "speedBps"
        QT_MOC_LITERAL(134, 13),  // "itemCompleted"
        QT_MOC_LITERAL(148, 7),  // "success"
        QT_MOC_LITERAL(156, 5),  // "error"
        QT_MOC_LITERAL(162, 12),  // "allCompleted"
        QT_MOC_LITERAL(175, 16)   // "onWorkerFinished"
    },
    "TransferManager",
    "itemAdded",
    "",
    "TransferItem",
    "item",
    "itemUpdated",
    "itemRemoved",
    "id",
    "itemStarted",
    "itemProgress",
    "bytesTransferred",
    "totalBytes",
    "speedBps",
    "itemCompleted",
    "success",
    "error",
    "allCompleted",
    "onWorkerFinished"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSTransferManagerENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   62,    2, 0x06,    1 /* Public */,
       5,    1,   65,    2, 0x06,    3 /* Public */,
       6,    1,   68,    2, 0x06,    5 /* Public */,
       8,    1,   71,    2, 0x06,    7 /* Public */,
       9,    4,   74,    2, 0x06,    9 /* Public */,
      13,    3,   83,    2, 0x06,   14 /* Public */,
      16,    0,   90,    2, 0x06,   18 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      17,    3,   91,    2, 0x08,   19 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QUuid,    7,
    QMetaType::Void, QMetaType::QUuid,    7,
    QMetaType::Void, QMetaType::QUuid, QMetaType::LongLong, QMetaType::LongLong, QMetaType::Double,    7,   10,   11,   12,
    QMetaType::Void, QMetaType::QUuid, QMetaType::Bool, QMetaType::QString,    7,   14,   15,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QUuid, QMetaType::Bool, QMetaType::QString,    7,   14,   15,

       0        // eod
};

Q_CONSTINIT const QMetaObject TransferManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSTransferManagerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSTransferManagerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSTransferManagerENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TransferManager, std::true_type>,
        // method 'itemAdded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TransferItem &, std::false_type>,
        // method 'itemUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TransferItem &, std::false_type>,
        // method 'itemRemoved'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUuid &, std::false_type>,
        // method 'itemStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUuid &, std::false_type>,
        // method 'itemProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUuid &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'itemCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUuid &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'allCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onWorkerFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUuid &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void TransferManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TransferManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->itemAdded((*reinterpret_cast< std::add_pointer_t<TransferItem>>(_a[1]))); break;
        case 1: _t->itemUpdated((*reinterpret_cast< std::add_pointer_t<TransferItem>>(_a[1]))); break;
        case 2: _t->itemRemoved((*reinterpret_cast< std::add_pointer_t<QUuid>>(_a[1]))); break;
        case 3: _t->itemStarted((*reinterpret_cast< std::add_pointer_t<QUuid>>(_a[1]))); break;
        case 4: _t->itemProgress((*reinterpret_cast< std::add_pointer_t<QUuid>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[4]))); break;
        case 5: _t->itemCompleted((*reinterpret_cast< std::add_pointer_t<QUuid>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 6: _t->allCompleted(); break;
        case 7: _t->onWorkerFinished((*reinterpret_cast< std::add_pointer_t<QUuid>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TransferItem >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TransferItem >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TransferManager::*)(const TransferItem & );
            if (_t _q_method = &TransferManager::itemAdded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TransferManager::*)(const TransferItem & );
            if (_t _q_method = &TransferManager::itemUpdated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (TransferManager::*)(const QUuid & );
            if (_t _q_method = &TransferManager::itemRemoved; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (TransferManager::*)(const QUuid & );
            if (_t _q_method = &TransferManager::itemStarted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (TransferManager::*)(const QUuid & , qint64 , qint64 , double );
            if (_t _q_method = &TransferManager::itemProgress; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (TransferManager::*)(const QUuid & , bool , const QString & );
            if (_t _q_method = &TransferManager::itemCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (TransferManager::*)();
            if (_t _q_method = &TransferManager::allCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
    }
}

const QMetaObject *TransferManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TransferManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSTransferManagerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int TransferManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void TransferManager::itemAdded(const TransferItem & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TransferManager::itemUpdated(const TransferItem & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void TransferManager::itemRemoved(const QUuid & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void TransferManager::itemStarted(const QUuid & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void TransferManager::itemProgress(const QUuid & _t1, qint64 _t2, qint64 _t3, double _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void TransferManager::itemCompleted(const QUuid & _t1, bool _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void TransferManager::allCompleted()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
