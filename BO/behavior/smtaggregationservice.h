#ifndef SMTAGGREGATIONSERVICE_H
#define SMTAGGREGATIONSERVICE_H

#include <QHash>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QSqlDatabase>

#include "DO/componentsmtdefinition.h"

struct AggregatedStateRequest
{
    int parentStateId = 0;
    QMap<int, QString> childStateCodes; // key: equipment_id, value: state_code
    QStringList containerInterfaceNames;
};

class SmtAggregationService
{
public:
    static QList<ComponentSmtDefinition> loadComponentDefinitions(const QSqlDatabase &db,
                                                                  const QList<int> &equipmentIds,
                                                                  QString *errorMessage = nullptr);

    static QStringList buildConnectionConstraints(const QSqlDatabase &db, int containerId,
                                                  QString *errorMessage = nullptr);

    static QString composeAggregatedStateScript(const QList<ComponentSmtDefinition> &components,
                                                const AggregatedStateRequest &request,
                                                const QStringList &connectionConstraints);

private:
    static ComponentSmtDefinition loadSingleComponent(const QSqlDatabase &db, int equipmentId,
                                                      QString *errorMessage);
    static QString findStateScript(const ComponentSmtDefinition &definition,
                                   const QString &stateCode);
};

#endif // SMTAGGREGATIONSERVICE_H
