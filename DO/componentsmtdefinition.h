#ifndef COMPONENTSMTDEFINITION_H
#define COMPONENTSMTDEFINITION_H

#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

struct ComponentSmtState
{
    QString stateCode;          // e.g. "normal", "fault:open"
    QString displayName;        // human readable name
    QString smtScript;          // SMT-LIB snippet
    QJsonObject metadata;       // failure probability, notes, etc.
};

struct ComponentSmtDefinition
{
    int componentId = 0;        // Equipment_ID reference
    QString componentCode;      // schematic mark (e.g. "PSU-1")
    QString componentType;      // library type (e.g. "DC24VSource")
    QString variableBlock;      // SMT variable declarations
    QJsonObject parameterMap;   // resolved parameter name/value pairs
    QList<ComponentSmtState> states;

    QStringList referencedInterfaces; // interface/port identifiers used by SMT
};

#endif // COMPONENTSMTDEFINITION_H
