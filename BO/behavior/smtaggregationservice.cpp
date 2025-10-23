#include "BO/behavior/smtaggregationservice.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QSet>

namespace {
QString fetchInterfaceName(const QSqlDatabase &db, int interfaceId, QString *errorMessage)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT name FROM container_interface WHERE interface_id = ?"));
    query.bindValue(0, interfaceId);
    if (!query.exec()) {
        if (errorMessage)
            *errorMessage = query.lastError().text();
        return QString();
    }
    if (query.next())
        return query.value(0).toString();
    return QString();
}
}

QList<ComponentSmtDefinition> SmtAggregationService::loadComponentDefinitions(const QSqlDatabase &db,
                                                                              const QList<int> &equipmentIds,
                                                                              QString *errorMessage)
{
    QList<ComponentSmtDefinition> result;
    for (int equipmentId : equipmentIds) {
        ComponentSmtDefinition definition = loadSingleComponent(db, equipmentId, errorMessage);
        if (!definition.componentCode.isEmpty())
            result.push_back(definition);
    }
    return result;
}

QStringList SmtAggregationService::buildConnectionConstraints(const QSqlDatabase &db, int containerId,
                                                              QString *errorMessage)
{
    QStringList constraints;
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT from_interface_id, to_interface_id FROM container_interface_link "
                                 "WHERE parent_container_id = ?"));
    query.bindValue(0, containerId);
    if (!query.exec()) {
        if (errorMessage)
            *errorMessage = query.lastError().text();
        return constraints;
    }

    while (query.next()) {
        const int fromId = query.value(0).toInt();
        const int toId = query.value(1).toInt();
        const QString fromName = fetchInterfaceName(db, fromId, errorMessage);
        const QString toName = fetchInterfaceName(db, toId, errorMessage);
        if (fromName.isEmpty() || toName.isEmpty())
            continue;
        constraints.append(QStringLiteral("(assert (= %1 %2))").arg(fromName, toName));
    }
    return constraints;
}

QString SmtAggregationService::composeAggregatedStateScript(const QList<ComponentSmtDefinition> &components,
                                                            const AggregatedStateRequest &request,
                                                            const QStringList &connectionConstraints)
{
    QStringList scriptLines;
    scriptLines.append(QStringLiteral("; Auto-generated aggregated state %1").arg(request.parentStateId));

    for (const QString &ifaceName : request.containerInterfaceNames) {
        if (!ifaceName.trimmed().isEmpty())
            scriptLines.append(QStringLiteral("(declare-fun %1 () Real)").arg(ifaceName));
    }

    QSet<QString> emittedVariables;
    for (const ComponentSmtDefinition &component : components) {
        if (component.variableBlock.isEmpty())
            continue;
        const QString trimmed = component.variableBlock.trimmed();
        if (!trimmed.isEmpty() && !emittedVariables.contains(trimmed)) {
            scriptLines.append(trimmed);
            emittedVariables.insert(trimmed);
        }
    }

    for (const QString &constraint : connectionConstraints)
        scriptLines.append(constraint);

    for (const ComponentSmtDefinition &component : components) {
        const QString stateCode = request.childStateCodes.value(component.componentId);
        if (stateCode.isEmpty())
            continue;
        const QString stateScript = findStateScript(component, stateCode);
        if (!stateScript.isEmpty())
            scriptLines.append(stateScript);
    }

    return scriptLines.join(QStringLiteral("\n"));
}

ComponentSmtDefinition SmtAggregationService::loadSingleComponent(const QSqlDatabase &db, int equipmentId,
                                                                  QString *errorMessage)
{
    ComponentSmtDefinition definition;
    definition.componentId = equipmentId;

    QSqlQuery equipmentQuery(db);
    equipmentQuery.prepare(QStringLiteral("SELECT DT, Type, TVariable, TVariableLegacy FROM Equipment WHERE Equipment_ID = ?"));
    equipmentQuery.bindValue(0, equipmentId);
    if (!equipmentQuery.exec()) {
        if (errorMessage)
            *errorMessage = equipmentQuery.lastError().text();
        return definition;
    }
    if (!equipmentQuery.next())
        return definition;

    definition.componentCode = equipmentQuery.value(0).toString();
    definition.componentType = equipmentQuery.value(1).toString();
    definition.variableBlock = equipmentQuery.value(2).toString();
    if (definition.variableBlock.isEmpty())
        definition.variableBlock = equipmentQuery.value(3).toString();

    QSqlQuery smtQuery(db);
    smtQuery.prepare(QStringLiteral("SELECT state_code, display_name, smt_script, metadata_json "
                                    "FROM component_smt WHERE component_id = ?"));
    smtQuery.bindValue(0, equipmentId);
    if (!smtQuery.exec()) {
        if (errorMessage)
            *errorMessage = smtQuery.lastError().text();
        return definition;
    }
    while (smtQuery.next()) {
        ComponentSmtState state;
        state.stateCode = smtQuery.value(0).toString();
        state.displayName = smtQuery.value(1).toString();
        state.smtScript = smtQuery.value(2).toString();
        definition.states.append(state);
    }
    return definition;
}

QString SmtAggregationService::findStateScript(const ComponentSmtDefinition &definition,
                                               const QString &stateCode)
{
    for (const ComponentSmtState &state : definition.states) {
        if (state.stateCode.compare(stateCode, Qt::CaseInsensitive) == 0)
            return state.smtScript;
    }
    return QString();
}
