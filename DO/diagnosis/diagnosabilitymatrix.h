#ifndef DIAGNOSABILITYMATRIX_H
#define DIAGNOSABILITYMATRIX_H

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QVector>
#include <QString>

struct DiagnosabilityTest
{
    int id = 0;            // surrogate key within diagnosis_test
    QString code;          // compact label, e.g. t05
    QString name;
    QString description;
    QString type;          // signal/function/fault-mode
    QString scope;         // container level or context identifier
    QJsonObject metadata;
};

struct DiagnosabilityFault
{
    int id = 0;            // surrogate key within diagnosis_fault
    int stateId = 0;       // linked container_state_id
    QString code;          // compact label, e.g. f12
    QString name;
    QString description;
    QString category;
    QJsonObject metadata;
};

struct DiagnosabilityCell
{
    int testId = 0;
    int faultId = 0;
    QString effect;        // none/detect/isolate/...
    double weight = 0.0;   // detection probability or ranking score
    QString evidenceType;  // e.g. simulated/analysis
    QString notes;
};

class DiagnosabilityMatrix
{
public:
    int matrixId = 0;
    int containerId = 0;
    QString version;
    QString notes;

    QVector<DiagnosabilityTest> tests;
    QVector<DiagnosabilityFault> faults;

    // Sparse storage keyed by testId/faultId for quick lookup
    QHash<int, QVector<DiagnosabilityCell>> columnMap;
    QHash<int, QVector<DiagnosabilityCell>> rowMap;

    QHash<int, int> testIndexMap;   // testId -> column index
    QHash<int, int> faultIndexMap;  // faultId -> row index
};

#endif // DIAGNOSABILITYMATRIX_H
