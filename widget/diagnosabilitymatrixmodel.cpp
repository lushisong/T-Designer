#include "widget/diagnosabilitymatrixmodel.h"

#include <QBrush>
#include <QColor>
#include <QStringList>

DiagnosabilityMatrixModel::DiagnosabilityMatrixModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void DiagnosabilityMatrixModel::setMatrix(const DiagnosabilityMatrix &matrix)
{
    beginResetModel();
    m_matrix = matrix;
    m_cells.clear();
    for (auto it = matrix.columnMap.constBegin(); it != matrix.columnMap.constEnd(); ++it) {
        const int testId = it.key();
        const QVector<DiagnosabilityCell> &cells = it.value();
        for (const DiagnosabilityCell &cell : cells)
            m_cells[testId].insert(cell.faultId, cell);
    }
    endResetModel();
}

int DiagnosabilityMatrixModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_matrix.faults.size();
}

int DiagnosabilityMatrixModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_matrix.tests.size();
}

QVariant DiagnosabilityMatrixModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const int row = index.row();
    const int column = index.column();
    if (row < 0 || row >= m_matrix.faults.size() || column < 0 || column >= m_matrix.tests.size())
        return QVariant();

    const DiagnosabilityFault &fault = m_matrix.faults.at(row);
    const DiagnosabilityTest &test = m_matrix.tests.at(column);
    const QHash<int, DiagnosabilityCell> &cellHash = m_cells.value(test.id);
    const DiagnosabilityCell cell = cellHash.value(fault.id);

    if (role == Qt::DisplayRole) {
        if (!cell.effect.isEmpty())
            return effectShortCode(cell.effect);
        return QVariant();
    }
    if (role == Qt::ToolTipRole) {
        if (!cell.effect.isEmpty())
            return cellTooltip(cell, test, fault);
        return QStringLiteral("%1 → %2 无覆盖信息").arg(test.code, fault.code);
    }
    if (role == Qt::BackgroundRole) {
        if (cell.effect.compare(QStringLiteral("detect"), Qt::CaseInsensitive) == 0)
            return QBrush(QColor(220, 245, 255));
        if (cell.effect.compare(QStringLiteral("isolate"), Qt::CaseInsensitive) == 0)
            return QBrush(QColor(220, 255, 220));
    }
    if (role == Qt::TextAlignmentRole)
        return Qt::AlignCenter;

    return QVariant();
}

QVariant DiagnosabilityMatrixModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (section < 0 || section >= m_matrix.tests.size())
            return QVariant();
        const DiagnosabilityTest &test = m_matrix.tests.at(section);
        if (role == Qt::DisplayRole)
            return test.code;
        if (role == Qt::ToolTipRole)
            return QStringLiteral("%1\n%2").arg(test.name, test.description);
    } else {
        if (section < 0 || section >= m_matrix.faults.size())
            return QVariant();
        const DiagnosabilityFault &fault = m_matrix.faults.at(section);
        if (role == Qt::DisplayRole)
            return fault.code;
        if (role == Qt::ToolTipRole)
            return QStringLiteral("%1\n%2").arg(fault.name, fault.description);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags DiagnosabilityMatrixModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QString DiagnosabilityMatrixModel::effectShortCode(const QString &effect) const
{
    if (effect.compare(QStringLiteral("detect"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("D");
    if (effect.compare(QStringLiteral("isolate"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("I");
    if (effect.compare(QStringLiteral("ambiguity"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("A");
    return effect;
}

QVariant DiagnosabilityMatrixModel::cellTooltip(const DiagnosabilityCell &cell, const DiagnosabilityTest &test, const DiagnosabilityFault &fault) const
{
    QStringList lines;
    lines << QStringLiteral("测试 %1 — %2").arg(test.code, test.name);
    lines << QStringLiteral("故障 %1 — %2").arg(fault.code, fault.name);
    lines << QStringLiteral("效应: %1").arg(cell.effect);
    if (cell.weight > 0.0)
        lines << QStringLiteral("置信度: %1").arg(cell.weight, 0, 'f', 2);
    if (!cell.evidenceType.isEmpty())
        lines << QStringLiteral("证据类型: %1").arg(cell.evidenceType);
    if (!cell.notes.isEmpty())
        lines << cell.notes;
    return lines.join(QStringLiteral("\n"));
}
