﻿#include "BO/container/behavioraggregator.h"

#include <algorithm>
#include <QSet>

BehaviorAggregator::BehaviorAggregator(Loader loader, ChildrenFetcher childrenFetcher)
    : m_loader(std::move(loader))
    , m_childrenFetcher(std::move(childrenFetcher))
{
}

AggregationResult BehaviorAggregator::aggregate(int containerId, const AggregationOptions &options) const
{
    if (!m_loader) return {};
    ContainerEntity entity = m_loader(containerId);
    if (entity.id() == 0) return {};
    return aggregate(entity, options);
}

AggregationResult BehaviorAggregator::aggregate(const ContainerEntity &entity, const AggregationOptions &options) const
{
    AggregationResult result;
    const QList<ContainerEntity> children = m_childrenFetcher ? m_childrenFetcher(entity.id()) : QList<ContainerEntity>();
    if (children.isEmpty()) {
        result.container = ContainerData(entity);
        return result;
    }

    QList<AggregationResult> childResults;
    childResults.reserve(children.size());
    for (const ContainerEntity &child : children)
        childResults.append(aggregate(child, options));

    return combine(entity, children, childResults, options);
}

AggregationResult BehaviorAggregator::combine(const ContainerEntity &entity,
                                              const QList<ContainerEntity> &children,
                                              const QList<AggregationResult> &childResults,
                                              const AggregationOptions &options) const
{
    AggregationResult result;
    ContainerData aggregated(entity);

    if (children.size() == 1 && options.inheritSingleChild) {
        const AggregationResult &childResult = childResults.first();
        aggregated.setPorts(childResult.container.ports());
        aggregated.setBehavior(childResult.container.behavior());
        aggregated.setBehaviorSmt(childResult.container.behaviorSmt());
        result.container = aggregated;
        result.contributions = childResult.contributions;
        FaultContribution contribution;
        contribution.sourceContainerId = children.first().id();
        contribution.sourceName = children.first().name();
        for (const BehaviorMode &mode : childResult.container.behavior().faultModes)
            contribution.propagatedModes.append(mode.modeId);
        result.contributions.append(contribution);
        result.warnings = childResult.warnings;
        return result;
    }

    QVector<ContainerPort> ports;
    ports.reserve(children.size() * 4);
    QStringList smtClauses;
    QStringList warnings;
    QVector<FaultContribution> contributions;
    contributions.reserve(children.size());

    BehaviorSpec behaviorSpec;
    behaviorSpec.normalMode.modeType = BehaviorModeType::Normal;
    behaviorSpec.normalMode.modeId = entity.name().isEmpty()
            ? QStringLiteral("%1.normal").arg(entity.id())
            : entity.name() + QStringLiteral(".normal");
    behaviorSpec.normalMode.displayName = entity.name().isEmpty()
            ? QStringLiteral("Container %1 正常").arg(entity.id())
            : entity.name() + QStringLiteral(" 正常");
    behaviorSpec.normalMode.sourceContainers.append(entity.id());

    QSet<QString> seenPortNames;

    for (int index = 0; index < children.size(); ++index) {
        const ContainerEntity &childEntity = children.at(index);
        const AggregationResult &childResult = childResults.at(index);

        for (const ContainerPort &childPort : childResult.container.ports()) {
            ContainerPort parentPort = childPort;
            parentPort.sourceContainerId = childEntity.id();
            parentPort.name = qualifiedPortName(childEntity, childPort, options.prefixChildPortNames);
            if (seenPortNames.contains(parentPort.name)) {
                warnings.append(QStringLiteral("端口名称冲突: %1").arg(parentPort.name));
                continue;
            }
            seenPortNames.insert(parentPort.name);
            ports.append(parentPort);
        }

        const BehaviorSpec &childBehavior = childResult.container.behavior();
        for (const QString &constraint : childBehavior.normalMode.constraints) {
            if (constraint.trimmed().isEmpty()) continue;
            if (childEntity.name().isEmpty())
                behaviorSpec.normalMode.constraints.append(constraint);
            else
                behaviorSpec.normalMode.constraints.append(QStringLiteral("[%1] %2").arg(childEntity.name(), constraint));
        }

        FaultContribution contribution;
        contribution.sourceContainerId = childEntity.id();
        contribution.sourceName = childEntity.name();

        for (const BehaviorMode &mode : childBehavior.faultModes) {
            BehaviorMode derived = mode;
            derived.sourceContainers.prepend(childEntity.id());
            if (childEntity.name().isEmpty()) {
                if (derived.modeId.isEmpty())
                    derived.modeId = QStringLiteral("fault-%1").arg(childEntity.id());
                if (derived.displayName.isEmpty())
                    derived.displayName = QStringLiteral("容器%1故障").arg(childEntity.id());
            } else {
                if (derived.modeId.isEmpty())
                    derived.modeId = childEntity.name() + QStringLiteral(".fault");
                else
                    derived.modeId = QStringLiteral("%1/%2").arg(childEntity.name(), derived.modeId);
                if (derived.displayName.isEmpty())
                    derived.displayName = childEntity.name() + QStringLiteral(" 故障");
                else
                    derived.displayName = childEntity.name() + QStringLiteral(".") + derived.displayName;
            }
            behaviorSpec.faultModes.append(derived);
            contribution.propagatedModes.append(derived.modeId);
        }

        contributions.append(contribution);

        const QString clause = childResult.container.behaviorSmt().trimmed();
        if (!clause.isEmpty())
            smtClauses.append(clause);

        result.warnings << childResult.warnings;
    }

    aggregated.setPorts(ports);
    aggregated.setBehavior(behaviorSpec);
    if (smtClauses.size() == 1)
        aggregated.setBehaviorSmt(smtClauses.first());
    else if (smtClauses.size() > 1)
        aggregated.setBehaviorSmt(QStringLiteral("(and %1)").arg(smtClauses.join(QChar(' '))));
    else
        aggregated.setBehaviorSmt(QString());

    result.container = aggregated;
    result.contributions = contributions;
    result.warnings << warnings;
    result.warnings.erase(std::remove_if(result.warnings.begin(), result.warnings.end(),
                                         [](const QString &value) { return value.trimmed().isEmpty(); }),
                          result.warnings.end());

    return result;
}

QString BehaviorAggregator::qualifiedPortName(const ContainerEntity &child,
                                              const ContainerPort &port,
                                              bool prefixChildName)
{
    if (!prefixChildName || child.name().isEmpty() || port.name.isEmpty())
        return port.name;
    return child.name() + QStringLiteral(".") + port.name;
}
