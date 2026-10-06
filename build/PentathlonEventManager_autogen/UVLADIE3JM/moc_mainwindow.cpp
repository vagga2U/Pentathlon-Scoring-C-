/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSMainWindowENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSMainWindowENDCLASS = QtMocHelpers::stringData(
    "MainWindow",
    "applyEventDetails",
    "",
    "EventDetails",
    "details",
    "importEntries",
    "csvPath",
    "addManualCompetitor",
    "Competitor",
    "competitor",
    "updateCompetitorFromPreview",
    "competitorIndex",
    "deleteCompetitorsFromPreview",
    "QList<int>",
    "competitorIndexes",
    "autoAssignNumbersFromPreview",
    "competitorIndexesInViewOrder",
    "startingNumber",
    "assignHeats",
    "DisciplineType",
    "discipline",
    "lanesPerHeat",
    "autoAssignFencingGroups",
    "numberPistes",
    "updateLaneAssignment",
    "heat",
    "lane",
    "updateRawResult",
    "rawResult",
    "updatePenalty",
    "penalty",
    "recalculateDiscipline",
    "printParticipantSummaryPdf",
    "printBlankSheet",
    "printDisciplineResults",
    "printFinalResults",
    "exportEventCsv",
    "clearEventData",
    "refreshViews"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSMainWindowENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  128,    2, 0x08,    1 /* Private */,
       5,    1,  131,    2, 0x08,    3 /* Private */,
       7,    1,  134,    2, 0x08,    5 /* Private */,
      10,    2,  137,    2, 0x08,    7 /* Private */,
      12,    1,  142,    2, 0x08,   10 /* Private */,
      15,    2,  145,    2, 0x08,   12 /* Private */,
      18,    2,  150,    2, 0x08,   15 /* Private */,
      22,    1,  155,    2, 0x08,   18 /* Private */,
      24,    4,  158,    2, 0x08,   20 /* Private */,
      27,    3,  167,    2, 0x08,   25 /* Private */,
      29,    3,  174,    2, 0x08,   29 /* Private */,
      31,    1,  181,    2, 0x08,   33 /* Private */,
      32,    0,  184,    2, 0x08,   35 /* Private */,
      33,    1,  185,    2, 0x08,   36 /* Private */,
      34,    1,  188,    2, 0x08,   38 /* Private */,
      35,    0,  191,    2, 0x08,   40 /* Private */,
      36,    0,  192,    2, 0x08,   41 /* Private */,
      37,    0,  193,    2, 0x08,   42 /* Private */,
      38,    0,  194,    2, 0x08,   43 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 8,   11,    9,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void, 0x80000000 | 13, QMetaType::Int,   16,   17,
    QMetaType::Void, 0x80000000 | 19, QMetaType::Int,   20,   21,
    QMetaType::Void, QMetaType::Int,   23,
    QMetaType::Void, 0x80000000 | 19, QMetaType::Int, QMetaType::Int, QMetaType::Int,   20,   11,   25,   26,
    QMetaType::Void, 0x80000000 | 19, QMetaType::Int, QMetaType::QString,   20,   11,   28,
    QMetaType::Void, 0x80000000 | 19, QMetaType::Int, QMetaType::QString,   20,   11,   30,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_CLASSMainWindowENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSMainWindowENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSMainWindowENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'applyEventDetails'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const EventDetails &, std::false_type>,
        // method 'importEntries'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'addManualCompetitor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const Competitor &, std::false_type>,
        // method 'updateCompetitorFromPreview'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const Competitor &, std::false_type>,
        // method 'deleteCompetitorsFromPreview'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<int> &, std::false_type>,
        // method 'autoAssignNumbersFromPreview'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<int> &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'assignHeats'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<DisciplineType, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'autoAssignFencingGroups'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'updateLaneAssignment'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<DisciplineType, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'updateRawResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<DisciplineType, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updatePenalty'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<DisciplineType, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'recalculateDiscipline'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<DisciplineType, std::false_type>,
        // method 'printParticipantSummaryPdf'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'printBlankSheet'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<DisciplineType, std::false_type>,
        // method 'printDisciplineResults'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<DisciplineType, std::false_type>,
        // method 'printFinalResults'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'exportEventCsv'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearEventData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshViews'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->applyEventDetails((*reinterpret_cast< std::add_pointer_t<EventDetails>>(_a[1]))); break;
        case 1: _t->importEntries((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->addManualCompetitor((*reinterpret_cast< std::add_pointer_t<Competitor>>(_a[1]))); break;
        case 3: _t->updateCompetitorFromPreview((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<Competitor>>(_a[2]))); break;
        case 4: _t->deleteCompetitorsFromPreview((*reinterpret_cast< std::add_pointer_t<QList<int>>>(_a[1]))); break;
        case 5: _t->autoAssignNumbersFromPreview((*reinterpret_cast< std::add_pointer_t<QList<int>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 6: _t->assignHeats((*reinterpret_cast< std::add_pointer_t<DisciplineType>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 7: _t->autoAssignFencingGroups((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->updateLaneAssignment((*reinterpret_cast< std::add_pointer_t<DisciplineType>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        case 9: _t->updateRawResult((*reinterpret_cast< std::add_pointer_t<DisciplineType>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 10: _t->updatePenalty((*reinterpret_cast< std::add_pointer_t<DisciplineType>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 11: _t->recalculateDiscipline((*reinterpret_cast< std::add_pointer_t<DisciplineType>>(_a[1]))); break;
        case 12: _t->printParticipantSummaryPdf(); break;
        case 13: _t->printBlankSheet((*reinterpret_cast< std::add_pointer_t<DisciplineType>>(_a[1]))); break;
        case 14: _t->printDisciplineResults((*reinterpret_cast< std::add_pointer_t<DisciplineType>>(_a[1]))); break;
        case 15: _t->printFinalResults(); break;
        case 16: _t->exportEventCsv(); break;
        case 17: _t->clearEventData(); break;
        case 18: _t->refreshViews(); break;
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
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSMainWindowENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    return _id;
}
QT_WARNING_POP
