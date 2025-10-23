#ifndef DIAGNOSABILITYMATRIXMODEL_H
#define DIAGNOSABILITYMATRIXMODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QMap>
#include <QPointer>

#include "DO/diagnosis/diagnosabilitymatrix.h"

class DiagnosabilityMatrixModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit DiagnosabilityMatrixModel(QObject *parent = nullptr);

    void setMatrix(const DiagnosabilityMatrix &matrix);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    const DiagnosabilityMatrix &matrix() const { return m_matrix; }

private:
    QString effectShortCode(const QString &effect) const;
    QVariant cellTooltip(const DiagnosabilityCell &cell, const DiagnosabilityTest &test, const DiagnosabilityFault &fault) const;

    DiagnosabilityMatrix m_matrix;
    QHash<int, QHash<int, DiagnosabilityCell>> m_cells; // testId -> (faultId -> cell)
};

#endif // DIAGNOSABILITYMATRIXMODEL_H
