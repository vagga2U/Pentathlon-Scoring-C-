/****************************************************************************
** Meta object code from reading C++ file 'setuptab.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/setuptab.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'setuptab.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.7.3. It"
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
struct qt_meta_stringdata_CLASSSetupTabENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSSetupTabENDCLASS = QtMocHelpers::stringData(
    "SetupTab",
    "eventDetailsSubmitted",
    "",
    "EventDetails",
    "details",
    "importEntriesRequested",
    "csvPath",
    "manualCompetitorSubmitted",
    "Competitor",
    "competitor",
    "competitorPreviewEdited",
    "competitorIndex",
    "deleteCompetitorsRequested",
    "QList<int>",
    "competitorIndexes",
    "autoAssignNumbersRequested",
    "competitorIndexesInViewOrder",
    "startingNumber",
    "printParticipantSummaryRequested",
    "exportCsvRequested",
    "clearAllRequested",
    "chooseLogo",
    "chooseCsv",
    "submitDetails",
    "submitManualCompetitor",
    "handlePreviewCellChanged",
    "row",
    "column",
    "requestDeleteSelectedRows",
    "requestAutoAssignNumbers"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSSetupTabENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  110,    2, 0x06,    1 /* Public */,
       5,    1,  113,    2, 0x06,    3 /* Public */,
       7,    1,  116,    2, 0x06,    5 /* Public */,
      10,    2,  119,    2, 0x06,    7 /* Public */,
      12,    1,  124,    2, 0x06,   10 /* Public */,
      15,    2,  127,    2, 0x06,   12 /* Public */,
      18,    0,  132,    2, 0x06,   15 /* Public */,
      19,    0,  133,    2, 0x06,   16 /* Public */,
      20,    0,  134,    2, 0x06,   17 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      21,    0,  135,    2, 0x08,   18 /* Private */,
      22,    0,  136,    2, 0x08,   19 /* Private */,
      23,    0,  137,    2, 0x08,   20 /* Private */,
      24,    0,  138,    2, 0x08,   21 /* Private */,
      25,    2,  139,    2, 0x08,   22 /* Private */,
      28,    0,  144,    2, 0x08,   25 /* Private */,
      29,    0,  145,    2, 0x08,   26 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 8,   11,    9,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void, 0x80000000 | 13, QMetaType::Int,   16,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   26,   27,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject SetupTab::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSSetupTabENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSSetupTabENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSSetupTabENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SetupTab, std::true_type>,
        // method 'eventDetailsSubmitted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const EventDetails &, std::false_type>,
        // method 'importEntriesRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'manualCompetitorSubmitted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const Competitor &, std::false_type>,
        // method 'competitorPreviewEdited'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const Competitor &, std::false_type>,
        // method 'deleteCompetitorsRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<int> &, std::false_type>,
        // method 'autoAssignNumbersRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<int> &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'printParticipantSummaryRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'exportCsvRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearAllRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'chooseLogo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'chooseCsv'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'submitDetails'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'submitManualCompetitor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handlePreviewCellChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'requestDeleteSelectedRows'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestAutoAssignNumbers'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void SetupTab::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SetupTab *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->eventDetailsSubmitted((*reinterpret_cast< std::add_pointer_t<EventDetails>>(_a[1]))); break;
        case 1: _t->importEntriesRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->manualCompetitorSubmitted((*reinterpret_cast< std::add_pointer_t<Competitor>>(_a[1]))); break;
        case 3: _t->competitorPreviewEdited((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<Competitor>>(_a[2]))); break;
        case 4: _t->deleteCompetitorsRequested((*reinterpret_cast< std::add_pointer_t<QList<int>>>(_a[1]))); break;
        case 5: _t->autoAssignNumbersRequested((*reinterpret_cast< std::add_pointer_t<QList<int>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 6: _t->printParticipantSummaryRequested(); break;
        case 7: _t->exportCsvRequested(); break;
        case 8: _t->clearAllRequested(); break;
        case 9: _t->chooseLogo(); break;
        case 10: _t->chooseCsv(); break;
        case 11: _t->submitDetails(); break;
        case 12: _t->submitManualCompetitor(); break;
        case 13: _t->handlePreviewCellChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 14: _t->requestDeleteSelectedRows(); break;
        case 15: _t->requestAutoAssignNumbers(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<int> >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<int> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SetupTab::*)(const EventDetails & );
            if (_t _q_method = &SetupTab::eventDetailsSubmitted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)(const QString & );
            if (_t _q_method = &SetupTab::importEntriesRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)(const Competitor & );
            if (_t _q_method = &SetupTab::manualCompetitorSubmitted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)(int , const Competitor & );
            if (_t _q_method = &SetupTab::competitorPreviewEdited; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)(const QVector<int> & );
            if (_t _q_method = &SetupTab::deleteCompetitorsRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)(const QVector<int> & , int );
            if (_t _q_method = &SetupTab::autoAssignNumbersRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)();
            if (_t _q_method = &SetupTab::printParticipantSummaryRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)();
            if (_t _q_method = &SetupTab::exportCsvRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (SetupTab::*)();
            if (_t _q_method = &SetupTab::clearAllRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
    }
}

const QMetaObject *SetupTab::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SetupTab::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSSetupTabENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SetupTab::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void SetupTab::eventDetailsSubmitted(const EventDetails & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SetupTab::importEntriesRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SetupTab::manualCompetitorSubmitted(const Competitor & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SetupTab::competitorPreviewEdited(int _t1, const Competitor & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void SetupTab::deleteCompetitorsRequested(const QVector<int> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void SetupTab::autoAssignNumbersRequested(const QVector<int> & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void SetupTab::printParticipantSummaryRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void SetupTab::exportCsvRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void SetupTab::clearAllRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}
QT_WARNING_POP
