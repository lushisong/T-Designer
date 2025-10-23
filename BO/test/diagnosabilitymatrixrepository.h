#ifndef DIAGNOSABILITYMATRIXREPOSITORY_H
#define DIAGNOSABILITYMATRIXREPOSITORY_H

#include <QSqlDatabase>
#include "DO/diagnosis/diagnosabilitymatrix.h"

class DiagnosabilityMatrixRepository
{
public:
    explicit DiagnosabilityMatrixRepository(const QSqlDatabase &db = QSqlDatabase::database());

    DiagnosabilityMatrix loadLatestForContainer(int containerId) const;

private:
    DiagnosabilityMatrix loadByMatrixId(int matrixId) const;
    void populateTests(DiagnosabilityMatrix &matrix) const;
    void populateFaults(DiagnosabilityMatrix &matrix) const;
    void populateEntries(DiagnosabilityMatrix &matrix) const;

    QSqlDatabase m_db;
};

#endif // DIAGNOSABILITYMATRIXREPOSITORY_H
