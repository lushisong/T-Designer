﻿#ifndef DIAGNOSTICMATRIXBUILDER_H
#define DIAGNOSTICMATRIXBUILDER_H

#include <memory>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QVector>

#include "BO/container/containerdata.h"
#include "BO/test/testdefinition.h"

struct MatrixEntry {
    QString testId;
    QString faultId;
    bool detects = false;
    bool isolates = false;
};

struct CoverageStats {
    int totalTests = 0;
    int totalFaults = 0;
    QSet<QString> detectedFaults;
    QSet<QString> isolatableFaults;
    QHash<QString, double> detectionRateByTest;
};

struct CoverageSummary {
    int totalFaults = 0;
    int detectedFaults = 0;
    int isolatableFaults = 0;
    double detectionRate = 0.0;
    double isolationRate = 0.0;
};

struct DecisionNode {
    QString testId;
    QString faultId;
    bool isLeaf = false;
    QVector<std::shared_ptr<DecisionNode>> children;
};

class DiagnosticMatrixBuilder
{
public:
    DiagnosticMatrixBuilder();

    void rebuild(const ContainerData &container);

    const QVector<MatrixEntry> &entries() const { return m_entries; }
    CoverageStats coverageStats() const;
    CoverageSummary coverageSummary(const QStringList &testIds) const;

    QStringList candidateTests(double minDetectionRate) const;
    QStringList candidateTests(const QStringList &availableTests, double minDetectionRate) const;

    std::shared_ptr<DecisionNode> buildDecisionTree() const;
    std::shared_ptr<DecisionNode> buildDecisionTree(const QStringList &testIds) const;

private:
    std::shared_ptr<DecisionNode> buildTreeRecursive(const QVector<GeneratedTest> &tests,
                                                     const QSet<QString> &faults,
                                                     QSet<QString> usedTests) const;

    QVector<MatrixEntry> m_entries;
    QVector<GeneratedTest> m_tests;
    QSet<QString> m_faultIds;
    QHash<QString, QSet<QString>> m_detectionMap;
    QHash<QString, QSet<QString>> m_isolationMap;
};

#endif // DIAGNOSTICMATRIXBUILDER_H
