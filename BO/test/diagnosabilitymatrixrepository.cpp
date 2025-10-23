#include "BO/test/diagnosabilitymatrixrepository.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>

DiagnosabilityMatrixRepository::DiagnosabilityMatrixRepository(const QSqlDatabase &db)
    : m_db(db)
{
}

DiagnosabilityMatrix DiagnosabilityMatrixRepository::loadLatestForContainer(int containerId) const
{
    DiagnosabilityMatrix matrix;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT matrix_id, version, notes FROM diagnosis_matrix WHERE container_id = ? ORDER BY matrix_id DESC LIMIT 1"));
    query.bindValue(0, containerId);
    if (!query.exec())
        return matrix;
    if (!query.next())
        return matrix;

    matrix.matrixId = query.value(0).toInt();
    matrix.containerId = containerId;
    matrix.version = query.value(1).toString();
    matrix.notes = query.value(2).toString();

    matrix = loadByMatrixId(matrix.matrixId);
    matrix.containerId = containerId;
    matrix.version = query.value(1).toString();
    matrix.notes = query.value(2).toString();
    return matrix;
}

DiagnosabilityMatrix DiagnosabilityMatrixRepository::loadByMatrixId(int matrixId) const
{
    DiagnosabilityMatrix matrix;
    matrix.matrixId = matrixId;

    populateTests(matrix);
    populateFaults(matrix);
    populateEntries(matrix);

    return matrix;
}

void DiagnosabilityMatrixRepository::populateTests(DiagnosabilityMatrix &matrix) const
{
    if (matrix.matrixId <= 0)
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT DISTINCT t.diagnosis_test_id, t.code, t.name, t.description, t.test_type, t.scope, t.metadata_json "
        "FROM diagnosis_test t "
        "INNER JOIN diagnosis_matrix_entry e ON e.diagnosis_test_id = t.diagnosis_test_id "
        "WHERE e.matrix_id = ? "
        "ORDER BY t.diagnosis_test_id"));
    query.bindValue(0, matrix.matrixId);
    if (!query.exec())
        return;

    while (query.next()) {
        DiagnosabilityTest test;
        test.id = query.value(0).toInt();
        test.code = query.value(1).toString();
        test.name = query.value(2).toString();
        test.description = query.value(3).toString();
        test.type = query.value(4).toString();
        test.scope = query.value(5).toString();
        const QString metadataStr = query.value(6).toString();
        if (!metadataStr.isEmpty())
            test.metadata = QJsonDocument::fromJson(metadataStr.toUtf8()).object();
        matrix.testIndexMap.insert(test.id, matrix.tests.size());
        matrix.tests.append(test);
    }
}

void DiagnosabilityMatrixRepository::populateFaults(DiagnosabilityMatrix &matrix) const
{
    if (matrix.matrixId <= 0)
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT DISTINCT f.fault_id, f.state_id, f.code, f.name, f.description, f.category, f.severity, f.metadata_json "
        "FROM diagnosis_fault f "
        "INNER JOIN diagnosis_matrix_entry e ON e.fault_id = f.fault_id "
        "WHERE e.matrix_id = ? "
        "ORDER BY f.fault_id"));
    query.bindValue(0, matrix.matrixId);
    if (!query.exec())
        return;

    while (query.next()) {
        DiagnosabilityFault fault;
        fault.id = query.value(0).toInt();
        fault.stateId = query.value(1).toInt();
        fault.code = query.value(2).toString();
        fault.name = query.value(3).toString();
        fault.description = query.value(4).toString();
        fault.category = query.value(5).toString();
        const QString severity = query.value(6).toString();
        fault.metadata.insert(QStringLiteral("severity"), severity);
        const QString metadataStr = query.value(7).toString();
        if (!metadataStr.isEmpty()) {
            const QJsonObject extra = QJsonDocument::fromJson(metadataStr.toUtf8()).object();
            for (auto it = extra.begin(); it != extra.end(); ++it)
                fault.metadata.insert(it.key(), it.value());
        }
        matrix.faultIndexMap.insert(fault.id, matrix.faults.size());
        matrix.faults.append(fault);
    }
}

void DiagnosabilityMatrixRepository::populateEntries(DiagnosabilityMatrix &matrix) const
{
    if (matrix.matrixId <= 0)
        return;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT matrix_id, diagnosis_test_id, fault_id, effect, weight, evidence_type, notes FROM diagnosis_matrix_entry WHERE matrix_id = ?"));
    query.bindValue(0, matrix.matrixId);
    if (!query.exec())
        return;

    while (query.next()) {
        DiagnosabilityCell cell;
        cell.testId = query.value(1).toInt();
        cell.faultId = query.value(2).toInt();
        cell.effect = query.value(3).toString();
        cell.weight = query.value(4).toDouble();
        cell.evidenceType = query.value(5).toString();
        cell.notes = query.value(6).toString();

        matrix.columnMap[cell.testId].append(cell);
        matrix.rowMap[cell.faultId].append(cell);
    }
}
