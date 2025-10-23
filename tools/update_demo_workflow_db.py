#!/usr/bin/env python3
"""Upgrade a project database with DemoWorkflow SMT and diagnosability data."""

import argparse
import json
import sqlite3
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple
from datetime import datetime


def compact_json(value) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def coil_base_tmodel() -> str:
    return (
        "class KA_xq {\n"
        "\n"
        "    ModeType mode;\n"
        "    onOffState cmdIn;\n"
        "    Resistance Res;\n"
        "    onOffState xqActivatedLed;\n"
        "    onOffState cmdOut;\n"
        "\n"
        "    enum ModeType {nominal, malFunction, unknownFault};\n"
        "\n"
        "    stateVector [mode];\n"
        "\n"
        "    {\n"
        "        if (cmdIn = on) {\n"
        "            xqActivatedLed = on;\n"
        "        }\n"
        "        if (cmdIn = off) {\n"
        "            xqActivatedLed = off;\n"
        "        }\n"
        "        switch (mode) {\n"
        "            case nominal:\n"
        "                if (cmdIn = on) {\n"
        "                    cmdOut = on;\n"
        "                    Res = nominal;\n"
        "                }\n"
        "                if (cmdIn = off) {\n"
        "                    cmdOut = off;\n"
        "                    Res = nominal;\n"
        "                }\n"
        "            case malFunction:\n"
        "                cmdOut = off;\n"
        "                Res != nominal;\n"
        "            case unknownFault:\n"
        "        }\n"
        "    }\n"
        "\n"
        "    failure toMalFunction(*, malFunction, 2.0e-5) {\n"
        "    }\n"
        "    failure toUnknownFault(*, unknownFault, 1.0e-7) {\n"
        "    }\n"
        "}\n"
    )


def coil_tmodel() -> str:
    return (
        "class NewKA_xq {\n"
        "\n"
        "    ModeType mode;\n"
        "    onOffState xqActivatedLed;\n"
        "    onOffState cmdOut;\n"
        "    resistance innerR;\n"
        "    elecPort ePort_in;\n"
        "\n"
        "    enum ModeType {nominal, blown, shorted, unknownFault};\n"
        "\n"
        "    stateVector [mode];\n"
        "\n"
        "    {\n"
        "        ePort_in.R = innerR;\n"
        "        ePort_in.appliance_U_I();\n"
        "        if (ePort_in.I = middle) {\n"
        "            xqActivatedLed = on;\n"
        "        }\n"
        "        if (ePort_in.I = none) {\n"
        "            xqActivatedLed = off;\n"
        "        }\n"
        "        switch (mode) {\n"
        "            case nominal:\n"
        "                innerR = middle;\n"
        "                if (ePort_in.I = middle) {\n"
        "                    cmdOut = on;\n"
        "                }\n"
        "                if (ePort_in.I != middle) {\n"
        "                    cmdOut = off;\n"
        "                }\n"
        "            case blown:\n"
        "                innerR = infinite;\n"
        "                cmdOut = off;\n"
        "            case shorted:\n"
        "                innerR = none;\n"
        "                cmdOut = off;\n"
        "            case unknownFault:\n"
        "        }\n"
        "    }\n"
        "\n"
        "    failure toBlown(*, blown, 2.0e-5) {\n"
        "    }\n"
        "    failure toShorted(*, shorted, 1.0e-5) {\n"
        "    }\n"
        "    failure toUnknownFault(*, unknownFault, 1.0e-8) {\n"
        "    }\n"
        "}\n"
    )


def elec_port_tmodel() -> str:
    return (
        "class elecPort {\n"
        "\n"
        "    current I;\n"
        "    voltage U;\n"
        "    resistance R;\n"
        "\n"
        "    relation ePort_init() {\n"
        "        I = none;\n"
        "        U = none;\n"
        "    }\n"
        "    relation highRes_U_I() {\n"
        "        appliance_U_I();\n"
        "    }\n"
        "    relation appliance_U_I() {\n"
        "        if (R = none) {\n"
        "            U = none;\n"
        "        }\n"
        "        if (R = infinite) {\n"
        "            I = none;\n"
        "        }\n"
        "        if ((U = none &\n"
        "            R != none)) {\n"
        "            I = none;\n"
        "        }\n"
        "        if ((U = middle &\n"
        "            R = middle)) {\n"
        "            I = middle;\n"
        "        }\n"
        "        if ((U = middle &\n"
        "            R = high)) {\n"
        "            I = low;\n"
        "        }\n"
        "        if ((U = middle &\n"
        "            R = low)) {\n"
        "            I = high;\n"
        "        }\n"
        "        if ((U = low &\n"
        "            R = middle)) {\n"
        "            I = low;\n"
        "        }\n"
        "        if ((U = low &\n"
        "            R = high)) {\n"
        "            I = low;\n"
        "        }\n"
        "        if ((U = high &\n"
        "            R = low)) {\n"
        "            I = high;\n"
        "        }\n"
        "        if ((U = high &\n"
        "            R = middle)) {\n"
        "            I = high;\n"
        "        }\n"
        "    }\n"
        "}\n"
    )


def psu_tmodel() -> str:
    return (
        "class DC24VSource {\n"
        "\n"
        "    ModeType mode;\n"
        "    supplyState supplyStatus;\n"
        "    elecPort port_pos;\n"
        "    elecPort port_neg;\n"
        "\n"
        "    enum ModeType {nominal, openCircuit, shortCircuit, unknownFault};\n"
        "    enum supplyState {energized, deEnergized, shorted};\n"
        "\n"
        "    stateVector [mode];\n"
        "\n"
        "    {\n"
        "        port_pos.appliance_U_I();\n"
        "        port_neg.appliance_U_I();\n"
        "        switch (mode) {\n"
        "            case nominal:\n"
        "                supplyStatus = energized;\n"
        "            case openCircuit:\n"
        "                supplyStatus = deEnergized;\n"
        "            case shortCircuit:\n"
        "                supplyStatus = shorted;\n"
        "            case unknownFault:\n"
        "        }\n"
        "    }\n"
        "\n"
        "    failure toOpenCircuit(*, openCircuit, 1.0e-5) {\n"
        "    }\n"
        "    failure toShortCircuit(*, shortCircuit, 1.0e-5) {\n"
        "    }\n"
        "    failure toUnknownFault(*, unknownFault, 1.0e-8) {\n"
        "    }\n"
        "}\n"
    )


def psu_smt_variables() -> str:
    return (
        "(declare-fun PSU-1.Vout () Real)\n"
        "(declare-fun PSU-1.InputVoltage () Real)"
    )


def psu_normal_smt() -> str:
    return (
        "(assert (>= PSU-1.InputVoltage 22))\n"
        "(assert (= PSU-1.Vout 24))"
    )


def psu_fault_smt() -> str:
    return "(assert (= PSU-1.Vout 0))"


def psu_behavior_json() -> str:
    normal = {
        "id": "psu_normal",
        "name": "PSU 正常",
        "type": "normal",
        "probability": 0.0,
        "constraints": ["PSU-1.Vout=24V"],
    }
    fault = {
        "id": "psu_failure",
        "name": "PSU 输出失效",
        "type": "fault",
        "probability": 0.01,
        "constraints": ["PSU-1.Vout=0V"],
    }
    return compact_json({"normal": normal, "faults": [fault]})


def actuator_behavior_json() -> str:
    normal = {
        "id": "act_normal",
        "name": "执行器正常",
        "type": "normal",
        "probability": 0.0,
        "constraints": ["ACT-1.Pressure=8bar"],
    }
    fault = {
        "id": "act_stuck",
        "name": "执行器卡滞",
        "type": "fault",
        "probability": 0.02,
        "constraints": ["ACT-1.Pressure=0bar"],
    }
    return compact_json({"normal": normal, "faults": [fault]})


def subsystem_behavior_json() -> str:
    return compact_json({"normal": {"id": "sub_normal", "name": "子系统正常", "type": "normal", "probability": 0.0}, "faults": []})


def actuator_smt_variables() -> str:
    return (
        "(declare-fun ACT-1.Pressure () Real)\n"
        "(declare-fun ACT-1.Supply () Real)\n"
        "(declare-fun ACT-1.Command () Bool)"
    )


def actuator_normal_smt() -> str:
    return (
        "(assert (=> ACT-1.Command (= ACT-1.Pressure 8)))\n"
        "(assert (>= ACT-1.Supply 5))"
    )


def actuator_fault_smt() -> str:
    return "(assert (= ACT-1.Pressure 0))"


def container_ports_json(tag: str, out_port: str, out_category: str, in_port: str = "", in_category: str = "") -> str:
    ports = []
    if in_port:
        ports.append({"name": f"{tag}.{in_port}", "category": in_category, "direction": "input"})
    ports.append({"name": f"{tag}.{out_port}", "category": out_category, "direction": "output"})
    return compact_json(ports)


def demo_test_json_list() -> List[str]:
    def make_test(test_id: str, category: str, name: str, target: str, faults: Sequence[str], cost: float, duration: float, metrics=None):
        obj = {
            "id": test_id,
            "category": category,
            "name": name,
            "description": "演示生成的测试",
            "targetId": target,
            "detectableFaults": list(faults),
            "isolatableFaults": list(faults),
            "estimatedCost": cost,
            "estimatedDuration": duration,
        }
        if metrics:
            obj["metrics"] = metrics
        return obj

    signal_metrics = {"direction": "output", "unit": "V"}
    function_metrics = {"requiredInputs": ["PSU-1.Vout"], "actuators": ["ACT-1.Pressure"]}
    fault_metrics = {"sourceContainers": [4]}

    signal_test = make_test("signal:3:PSU-1.Vout", "signal", "PSU 输出电压检测", "PSU-1.Vout", ["psu_failure"], 1.0, 1.0, signal_metrics)
    function_test = make_test("function:4:DeliverPressure", "function", "DeliverPressure 功能测试", "DeliverPressure", ["psu_failure", "act_stuck"], 2.0, 2.0, function_metrics)
    fault_test = make_test("fault:4:act_stuck", "faultMode", "执行器卡滞诊断", "act_stuck", ["act_stuck"], 3.0, 3.0, fault_metrics)

    return [compact_json([signal_test]), compact_json([function_test, fault_test])]


def sync_rows(conn: sqlite3.Connection, table: str, columns: Sequence[str], rows: Sequence[Sequence], key_columns: Sequence[str]) -> None:
    if not rows:
        return
    key_indices = [columns.index(col) for col in key_columns]
    where_clause = " AND ".join(f'"{col}"=?' for col in key_columns)
    delete_sql = f"DELETE FROM {table} WHERE {where_clause}"
    for row in rows:
        conn.execute(delete_sql, [row[i] for i in key_indices])
    placeholders = ",".join(["?"] * len(columns))
    column_list = ",".join(f'"{col}"' for col in columns)
    insert_sql = f"INSERT INTO {table} ({column_list}) VALUES ({placeholders})"
    conn.executemany(insert_sql, rows)


def ensure_column(conn: sqlite3.Connection, table: str, column: str, definition: str) -> None:
    existing = {row[1] for row in conn.execute(f"PRAGMA table_info({table})")}
    if column not in existing:
        conn.execute(f"ALTER TABLE {table} ADD COLUMN {definition}")


def ensure_schema(conn: sqlite3.Connection) -> None:
    statements = [
        """
        CREATE TABLE IF NOT EXISTS FunctionDefineClass (
            FunctionDefineClass_ID INTEGER PRIMARY KEY,
            ParentNo INTEGER,
            Level INTEGER,
            Desc TEXT,
            _Order INTEGER,
            FunctionDefineName TEXT,
            FunctionDefineCode TEXT,
            DefaultSymbol TEXT,
            FuncType TEXT,
            TModel TEXT,
            TClassName TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS ProjectStructure (
            ProjectStructure_ID INTEGER PRIMARY KEY,
            Structure_ID TEXT,
            Structure_INT TEXT,
            Parent_ID INTEGER,
            Struct_Desc TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS Equipment (
            Equipment_ID INTEGER PRIMARY KEY,
            ProjectStructure_ID INTEGER,
            DT TEXT,
            Type TEXT,
            Eqpt_Category TEXT,
            Name TEXT,
            Desc TEXT,
            PartCode TEXT,
            SymbRemark TEXT,
            OrderNum TEXT,
            Factory TEXT,
            TVariable TEXT,
            TVariableLegacy TEXT,
            TModel TEXT,
            TModelLegacy TEXT,
            Structure TEXT,
            RepairInfo TEXT,
            Picture TEXT,
            MTBF TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS EquipmentDiagnosePara (
            DiagnoseParaID INTEGER PRIMARY KEY,
            Equipment_ID INTEGER,
            Name TEXT,
            Unit TEXT,
            CurValue TEXT,
            DefaultValue TEXT,
            Remark TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS Symbol (
            Symbol_ID INTEGER PRIMARY KEY,
            Equipment_ID INTEGER,
            Page_ID INTEGER,
            Symbol TEXT,
            Symbol_Category TEXT,
            Symbol_Desc TEXT,
            Designation TEXT,
            Symbol_Handle TEXT,
            Symbol_Remark TEXT,
            FunDefine TEXT,
            FuncType TEXT,
            SourceConn INTEGER,
            ExecConn INTEGER,
            SourcePrior INTEGER,
            InterConnect TEXT,
            Show_DT TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS Symb2TermInfo (
            Symb2TermInfo_ID INTEGER PRIMARY KEY,
            Symbol_ID INTEGER,
            ConnNum_Logic TEXT,
            ConnNum TEXT,
            ConnDirection TEXT,
            Internal INTEGER,
            ConnDesc TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS Function (
            FunctionID INTEGER PRIMARY KEY,
            FunctionName TEXT,
            ExecsList TEXT,
            CmdValList TEXT,
            UserTest TEXT,
            Remark TEXT,
            LinkText TEXT,
            ComponentDependency TEXT,
            AllComponents TEXT,
            FunctionDependency TEXT,
            PersistentFlag INTEGER,
            FaultProbability REAL
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS component_smt (
            component_id INTEGER NOT NULL,
            state_code TEXT NOT NULL,
            display_name TEXT,
            smt_script TEXT NOT NULL,
            metadata_json TEXT,
            PRIMARY KEY (component_id, state_code)
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS container (
            container_id INTEGER PRIMARY KEY AUTOINCREMENT,
            project_structure_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            level TEXT NOT NULL,
            source_equipment_id INTEGER,
            auto_generated INTEGER NOT NULL DEFAULT 0,
            description TEXT,
            fault_analysis_depth TEXT,
            inherits_from_container_id INTEGER
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS container_hierarchy (
            parent_id INTEGER NOT NULL,
            child_id INTEGER NOT NULL,
            relation_type TEXT DEFAULT 'contains',
            PRIMARY KEY (parent_id, child_id)
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS container_component (
            container_id INTEGER NOT NULL,
            equipment_id INTEGER NOT NULL,
            role TEXT,
            PRIMARY KEY (container_id, equipment_id)
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS container_interface (
            interface_id INTEGER PRIMARY KEY AUTOINCREMENT,
            container_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            direction TEXT NOT NULL,
            signal_category TEXT NOT NULL,
            signal_subtype TEXT,
            physical_domain TEXT,
            default_unit TEXT,
            description TEXT,
            inherits_interface_id INTEGER
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS container_interface_link (
            link_id INTEGER PRIMARY KEY AUTOINCREMENT,
            parent_container_id INTEGER NOT NULL,
            from_interface_id INTEGER NOT NULL,
            to_interface_id INTEGER NOT NULL,
            link_type TEXT DEFAULT 'connect',
            notes TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS container_state (
            state_id INTEGER PRIMARY KEY AUTOINCREMENT,
            container_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            state_type TEXT NOT NULL,
            derived_from_children INTEGER NOT NULL DEFAULT 0,
            probability REAL,
            rationale TEXT,
            analysis_scope TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS container_state_behavior (
            behavior_id INTEGER PRIMARY KEY AUTOINCREMENT,
            state_id INTEGER NOT NULL,
            representation TEXT NOT NULL,
            expression TEXT NOT NULL,
            notes TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS container_state_composition (
            composition_id INTEGER PRIMARY KEY AUTOINCREMENT,
            parent_state_id INTEGER NOT NULL,
            child_state_id INTEGER NOT NULL,
            relation TEXT DEFAULT 'includes'
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS container_state_interface (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            state_id INTEGER NOT NULL,
            interface_id INTEGER NOT NULL,
            constraint TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS function_definition (
            function_id INTEGER PRIMARY KEY AUTOINCREMENT,
            container_id INTEGER NOT NULL,
            parent_function_id INTEGER,
            name TEXT NOT NULL,
            description TEXT,
            requirement TEXT,
            expected_output TEXT,
            detection_rule TEXT,
            auto_generated INTEGER NOT NULL DEFAULT 0
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS function_io (
            io_id INTEGER PRIMARY KEY AUTOINCREMENT,
            function_id INTEGER NOT NULL,
            io_type TEXT NOT NULL,
            name TEXT NOT NULL,
            interface_id INTEGER,
            default_value TEXT,
            expression TEXT,
            description TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS function_dependency (
            function_id INTEGER NOT NULL,
            depends_on_function_id INTEGER NOT NULL,
            dependency_type TEXT DEFAULT 'requires',
            PRIMARY KEY (function_id, depends_on_function_id)
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS test_definition (
            test_id INTEGER PRIMARY KEY AUTOINCREMENT,
            container_id INTEGER,
            function_id INTEGER,
            related_state_id INTEGER,
            test_type TEXT NOT NULL,
            name TEXT NOT NULL,
            description TEXT,
            auto_generated INTEGER NOT NULL DEFAULT 1,
            interface_id INTEGER,
            signal_category TEXT,
            expected_result TEXT,
            complexity INTEGER,
            estimated_duration REAL,
            estimated_cost REAL
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS test_fault_coverage (
            test_id INTEGER NOT NULL,
            state_id INTEGER NOT NULL,
            coverage_type TEXT NOT NULL,
            confidence REAL,
            PRIMARY KEY (test_id, state_id, coverage_type)
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS test_constraint (
            constraint_id INTEGER PRIMARY KEY AUTOINCREMENT,
            test_id INTEGER NOT NULL,
            constraint_type TEXT NOT NULL,
            value TEXT,
            unit TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS analysis_requirement (
            requirement_id INTEGER PRIMARY KEY AUTOINCREMENT,
            container_id INTEGER NOT NULL,
            metric TEXT NOT NULL,
            target_value REAL NOT NULL,
            unit TEXT,
            notes TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS analysis_constraint (
            constraint_id INTEGER PRIMARY KEY AUTOINCREMENT,
            container_id INTEGER NOT NULL,
            constraint_type TEXT NOT NULL,
            value TEXT,
            unit TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS test_plan_candidate (
            candidate_id INTEGER PRIMARY KEY AUTOINCREMENT,
            container_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            description TEXT,
            total_cost REAL,
            total_duration REAL,
            selection_notes TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS test_plan_candidate_item (
            candidate_id INTEGER NOT NULL,
            test_id INTEGER NOT NULL,
            PRIMARY KEY (candidate_id, test_id)
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS diagnosis_fault (
            fault_id INTEGER PRIMARY KEY AUTOINCREMENT,
            state_id INTEGER NOT NULL,
            code TEXT NOT NULL,
            name TEXT,
            description TEXT,
            category TEXT,
            severity TEXT,
            metadata_json TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS diagnosis_test (
            diagnosis_test_id INTEGER PRIMARY KEY AUTOINCREMENT,
            test_id INTEGER NOT NULL,
            code TEXT NOT NULL,
            name TEXT,
            description TEXT,
            test_type TEXT,
            scope TEXT,
            metadata_json TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS diagnosis_matrix (
            matrix_id INTEGER PRIMARY KEY AUTOINCREMENT,
            container_id INTEGER NOT NULL,
            version TEXT,
            notes TEXT,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS diagnosis_matrix_entry (
            matrix_id INTEGER NOT NULL,
            diagnosis_test_id INTEGER NOT NULL,
            fault_id INTEGER NOT NULL,
            effect TEXT NOT NULL,
            weight REAL,
            evidence_type TEXT,
            notes TEXT,
            PRIMARY KEY (matrix_id, diagnosis_test_id, fault_id)
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS diagnosis_tree (
            tree_id INTEGER PRIMARY KEY AUTOINCREMENT,
            container_id INTEGER NOT NULL,
            name TEXT,
            description TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS diagnosis_tree_node (
            node_id INTEGER PRIMARY KEY AUTOINCREMENT,
            tree_id INTEGER NOT NULL,
            parent_node_id INTEGER,
            test_id INTEGER,
            state_id INTEGER,
            outcome TEXT,
            comment TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS containers (
            id INTEGER PRIMARY KEY,
            name TEXT,
            type INTEGER,
            parent_id INTEGER,
            order_index INTEGER,
            analysis_depth INTEGER,
            interface_json TEXT,
            behavior_smt TEXT,
            fault_modes_json TEXT,
            tests_json TEXT,
            analysis_json TEXT,
            equipment_id INTEGER,
            equipment_type TEXT,
            equipment_name TEXT
        )
        """,
        """
        CREATE TABLE IF NOT EXISTS equipment_containers (
            equipment_id INTEGER PRIMARY KEY,
            container_id INTEGER
        )
        """,
    ]
    for stmt in statements:
        conn.execute(stmt)
    ensure_column(conn, "Equipment", "TVariableLegacy", "TVariableLegacy TEXT")
    ensure_column(conn, "Equipment", "TModelLegacy", "TModelLegacy TEXT")
    ensure_column(conn, "Function", "LinkText", "LinkText TEXT")
    ensure_column(conn, "Function", "ComponentDependency", "ComponentDependency TEXT")
    ensure_column(conn, "Function", "AllComponents", "AllComponents TEXT")
    ensure_column(conn, "Function", "FunctionDependency", "FunctionDependency TEXT")
    ensure_column(conn, "Function", "PersistentFlag", "PersistentFlag INTEGER")
    ensure_column(conn, "Function", "FaultProbability", "FaultProbability REAL")
    ensure_column(conn, "container_state_interface", "constraint", '"constraint" TEXT')
    conn.execute(
        "CREATE INDEX IF NOT EXISTS idx_diag_entry_test ON diagnosis_matrix_entry(matrix_id, diagnosis_test_id)"
    )
    conn.execute(
        "CREATE INDEX IF NOT EXISTS idx_diag_entry_fault ON diagnosis_matrix_entry(matrix_id, fault_id)"
    )


def apply_demo_dataset(conn: sqlite3.Connection) -> None:
    tests_json = demo_test_json_list()

    sync_rows(
        conn,
        "FunctionDefineClass",
        [
            "FunctionDefineClass_ID",
            "ParentNo",
            "Level",
            "Desc",
            "_Order",
            "FunctionDefineName",
            "FunctionDefineCode",
            "DefaultSymbol",
            "FuncType",
            "TModel",
            "TClassName",
        ],
        [
            (1, 0, 0, "", 1, "电气工程", "", "", "", "", ""),
            (102, 1, 1, "", 2, "线圈,触点", "", "", "", "", ""),
            (10200, 102, 2, "", 1, "线圈", "200", "", "", "", ""),
            (1020001, 10200, 3, "", 1, "线圈,2 个连接点", "200.1", "", "接线端口", coil_base_tmodel(), "KA_xq"),
            (102000100, 1020001, 4, "", 1, "线圈,常规", "200.1.0", "SPS_M_K-1", "接线端口", coil_tmodel(), "NewKA_xq"),
            (117, 1, 1, "", 17, "elecPort", "", "", "", elec_port_tmodel(), "elecPort"),
            (107, 1, 1, "", 6, "电源,发电机", "", "", "", "", ""),
            (10700, 107, 2, "", 1, "电压源", "700", "", "", "", ""),
            (1070099, 10700, 3, "", 2, "电压源,可变", "700.99", "", "", "", ""),
            (107009901, 1070099, 4, "", 1, "电压源,可变", "700.99.1", "SPS_M_BAT-1", "", psu_tmodel(), "DC24VSource"),
        ],
        ["FunctionDefineClass_ID"],
    )

    sync_rows(
        conn,
        "ProjectStructure",
        ["ProjectStructure_ID", "Structure_ID", "Structure_INT", "Parent_ID", "Struct_Desc"],
        [
            (1001, "1", "Demo System", 0, "演示项目根节点"),
            (1002, "3", "Subsystem", 1001, "演示子系统"),
            (1003, "5", "Station 1", 1002, "演示位置"),
            (1004, "6", "Demo Diagram", 1003, "演示图纸分组"),
        ],
        ["ProjectStructure_ID"],
    )

    sync_rows(
        conn,
        "Equipment",
        [
            "Equipment_ID",
            "ProjectStructure_ID",
            "DT",
            "Type",
            "Eqpt_Category",
            "Name",
            "Desc",
            "PartCode",
            "SymbRemark",
            "OrderNum",
            "Factory",
            "TVariable",
            "TVariableLegacy",
            "TModel",
            "TModelLegacy",
            "Structure",
            "RepairInfo",
            "Picture",
            "MTBF",
        ],
        [
            (1, 1003, "PSU-1", "Power", "普通元件", "Power Supply", "提供24V稳压输出", "PSU001", "SPS_M_BAT-1", "1", "DemoWorks", psu_smt_variables(), "PSU-1.Cmd", "", psu_tmodel(), "107009901", "", "", "120000"),
            (2, 1003, "ACT-1", "Actuator", "普通元件", "Hydraulic Actuator", "输出8bar液压压力", "ACT001", "SPS_M_K-1", "2", "DemoWorks", actuator_smt_variables(), "ACT-1.Cmd", "", coil_tmodel(), "102000100", "", "", "90000"),
        ],
        ["Equipment_ID"],
    )

    sync_rows(
        conn,
        "component_smt",
        ["component_id", "state_code", "display_name", "smt_script", "metadata_json"],
        [
            (1, "variables", "PSU 变量", psu_smt_variables(), None),
            (1, "normal", "PSU 正常", psu_normal_smt(), None),
            (1, "fault_output_drop", "PSU 输出失效", psu_fault_smt(), None),
            (2, "variables", "执行器变量", actuator_smt_variables(), None),
            (2, "normal", "执行器正常", actuator_normal_smt(), None),
            (2, "fault_stuck", "执行器卡滞", actuator_fault_smt(), None),
        ],
        ["component_id", "state_code"],
    )

    sync_rows(
        conn,
        "EquipmentDiagnosePara",
        ["DiagnoseParaID", "Equipment_ID", "Name", "Unit", "CurValue", "DefaultValue", "Remark"],
        [
            (1, 1, "Vout", "V", "24", "24", "输出电压"),
            (2, 2, "Pressure", "bar", "8", "8", "输出压力"),
        ],
        ["DiagnoseParaID"],
    )

    alter_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    sync_rows(
        conn,
        "Page",
        [
            "Page_ID",
            "ProjectStructure_ID",
            "Page_Desc",
            "PageType",
            "PageNum",
            "PageName",
            "Scale",
            "Border",
            "Title",
            "AlterTime",
            "MD5Code",
        ],
        [
            (
                1,
                1004,
                "Demo diagram for workflow",
                "原理图",
                1,
                "D1",
                "1:1",
                "A3:420x297",
                "Demo Diagram",
                alter_time,
                None,
            )
        ],
        ["Page_ID"],
    )

    sync_rows(
        conn,
        "Symbol",
        [
            "Symbol_ID",
            "Equipment_ID",
            "Page_ID",
            "Symbol",
            "Symbol_Category",
            "Symbol_Desc",
            "Designation",
            "Symbol_Handle",
            "Symbol_Remark",
            "FunDefine",
            "FuncType",
            "SourceConn",
            "ExecConn",
            "SourcePrior",
            "InterConnect",
            "Show_DT",
        ],
        [
            (1, 1, 1, "SPS_M_BAT-1", "电源,发电机", "电压源子块", "PSU", "", "107009901", "电压源,可变", "source", 1, 0, 1, "", "PSU-1.Vout"),
            (2, 2, 1, "SPS_M_K-1", "线圈,触点", "执行器线圈子块", "ACT", "", "102000100", "线圈,常规", "actuator", 0, 1, 1, "", "ACT-1.Cmd"),
        ],
        ["Symbol_ID"],
    )

    sync_rows(
        conn,
        "Symb2TermInfo",
        ["Symb2TermInfo_ID", "Symbol_ID", "ConnNum_Logic", "ConnNum", "ConnDirection", "Internal", "ConnDesc"],
        [
            (1, 1, "1", "P", None, 0, "正极端子"),
            (2, 1, "2", "N", None, 0, "负极端子"),
            (3, 2, "1", "A1", None, 0, "线圈入口"),
            (4, 2, "2", "A2", None, 0, "线圈返回"),
        ],
        ["Symb2TermInfo_ID"],
    )

    sync_rows(
        conn,
        "Function",
        [
            "FunctionID",
            "FunctionName",
            "ExecsList",
            "CmdValList",
            "UserTest",
            "Remark",
            "LinkText",
            "ComponentDependency",
            "AllComponents",
            "FunctionDependency",
            "PersistentFlag",
            "FaultProbability",
        ],
        [
            (
                1,
                "DeliverPressure",
                "ACT-1.Pressure",
                "PSU-1.Vout=24V",
                "",
                "演示功能: 电源驱动执行器",
                "PSU-1.Vout,ACT-1.Pressure",
                "PSU-1,ACT-1",
                "PSU-1,ACT-1",
                "",
                1,
                0.01,
            )
        ],
        ["FunctionID"],
    )

    sync_rows(
        conn,
        "container",
        [
            "container_id",
            "project_structure_id",
            "name",
            "level",
            "source_equipment_id",
            "auto_generated",
            "description",
            "fault_analysis_depth",
            "inherits_from_container_id",
        ],
        [
            (1, 1001, "Demo System", "system", None, 0, "演示项目根节点", None, None),
            (2, 1002, "Subsystem", "subsystem", None, 0, "液压演示子系统", None, None),
            (3, 1003, "PSU-1", "component", 1, 1, "Power Supply container", None, None),
            (4, 1003, "ACT-1", "component", 2, 1, "Hydraulic actuator container", None, None),
        ],
        ["container_id"],
    )

    sync_rows(
        conn,
        "container_hierarchy",
        ["parent_id", "child_id", "relation_type"],
        [(1, 2, "contains"), (2, 3, "contains"), (2, 4, "contains")],
        ["parent_id", "child_id"],
    )

    sync_rows(
        conn,
        "container_component",
        ["container_id", "equipment_id", "role"],
        [(3, 1, "primary"), (4, 2, "primary")],
        ["container_id", "equipment_id"],
    )

    sync_rows(
        conn,
        "container_interface",
        [
            "interface_id",
            "container_id",
            "name",
            "direction",
            "signal_category",
            "signal_subtype",
            "physical_domain",
            "default_unit",
            "description",
            "inherits_interface_id",
        ],
        [
            (1, 3, "PSU-1.Vout", "out", "electric", "voltage", "electrical", "V", "PSU 输出电压", None),
            (2, 4, "ACT-1.Supply", "in", "hydraulic", "pressure", "hydraulic", "bar", "执行器端口供压", None),
            (3, 4, "ACT-1.Pressure", "out", "hydraulic", "pressure", "hydraulic", "bar", "执行器输出压力", None),
            (4, 2, "Subsystem.Pressure", "out", "hydraulic", "pressure", "hydraulic", "bar", "子系统对外输出", None),
        ],
        ["interface_id"],
    )

    sync_rows(
        conn,
        "container_interface_link",
        ["parent_container_id", "from_interface_id", "to_interface_id", "link_type", "notes"],
        [(2, 1, 2, "connect", None), (2, 3, 4, "connect", None)],
        ["parent_container_id", "from_interface_id", "to_interface_id"],
    )

    sync_rows(
        conn,
        "container_state",
        [
            "state_id",
            "container_id",
            "name",
            "state_type",
            "derived_from_children",
            "probability",
            "rationale",
            "analysis_scope",
        ],
        [
            (1, 3, "PSU 正常", "normal", 0, 0.99, "输出稳定在 24V", "code:normal"),
            (2, 3, "PSU 输出失效", "fault", 0, 0.01, "输出降为 0V", "code:fault_output_drop"),
            (3, 4, "执行器正常", "normal", 0, 0.98, "输出 8bar 压力", "code:normal"),
            (4, 4, "执行器卡滞", "fault", 0, 0.02, "输出压力为 0bar", "code:fault_stuck"),
            (5, 2, "子系统正常", "normal", 1, None, "聚合子级正常状态", "aggregate:normal"),
            (6, 2, "子系统故障", "fault", 1, None, "聚合子级故障状态", "aggregate:fault"),
        ],
        ["state_id"],
    )

    sync_rows(
        conn,
        "container_state_behavior",
        ["behavior_id", "state_id", "representation", "expression", "notes"],
        [
            (1, 1, "smt2", psu_normal_smt(), ""),
            (2, 2, "smt2", psu_fault_smt(), ""),
            (3, 3, "smt2", actuator_normal_smt(), ""),
            (4, 4, "smt2", actuator_fault_smt(), ""),
            (5, 5, "smt2", psu_normal_smt() + "\n" + actuator_normal_smt(), ""),
            (6, 6, "smt2", psu_fault_smt() + "\n" + actuator_fault_smt(), ""),
        ],
        ["behavior_id"],
    )

    sync_rows(
        conn,
        "container_state_composition",
        ["parent_state_id", "child_state_id", "relation"],
        [(5, 1, "includes"), (5, 3, "includes"), (6, 2, "includes"), (6, 4, "includes")],
        ["parent_state_id", "child_state_id"],
    )

    sync_rows(
        conn,
        "container_state_interface",
        ["id", "state_id", "interface_id", "constraint"],
        [
            (1, 1, 1, "PSU-1.Vout=24V"),
            (2, 2, 1, "PSU-1.Vout=0V"),
            (3, 3, 3, "ACT-1.Pressure=8bar"),
            (4, 4, 3, "ACT-1.Pressure=0bar"),
            (5, 5, 4, "Subsystem.Pressure=8bar"),
            (6, 6, 4, "Subsystem.Pressure=0bar"),
        ],
        ["id"],
    )

    sync_rows(
        conn,
        "function_definition",
        [
            "function_id",
            "container_id",
            "parent_function_id",
            "name",
            "description",
            "requirement",
            "expected_output",
            "detection_rule",
            "auto_generated",
        ],
        [(1, 2, None, "DeliverPressure", "PSU 提供能量驱动执行器输出压力", "当输入电压满足要求时，应输出 8bar 压力", "Subsystem.Pressure=8bar", "PSU-1.Vout >= 20", 0)],
        ["function_id"],
    )

    sync_rows(
        conn,
        "function_io",
        ["io_id", "function_id", "io_type", "name", "interface_id", "default_value", "expression", "description"],
        [
            (1, 1, "input", "SupplyVoltage", 1, None, "PSU-1.Vout", "来自电源的输出电压"),
            (2, 1, "output", "SubsystemPressure", 4, None, "Subsystem.Pressure", "期望输出压力"),
        ],
        ["io_id"],
    )

    sync_rows(
        conn,
        "test_definition",
        [
            "test_id",
            "container_id",
            "function_id",
            "related_state_id",
            "test_type",
            "name",
            "description",
            "auto_generated",
            "interface_id",
            "signal_category",
            "expected_result",
            "complexity",
            "estimated_duration",
            "estimated_cost",
        ],
        [
            (1, 3, None, None, "signal", "PSU 输出电压测试", "测量 PSU 输出是否稳定", 1, 1, "electric", "Vout=24V", 1, 0.5, 100.0),
            (2, 4, None, None, "signal", "执行器压力测试", "测量执行器输出压力", 1, 3, "hydraulic", "Pressure=8bar", 1, 0.7, 120.0),
            (3, 2, 1, None, "function", "系统功能验证", "验证 DeliverPressure 功能的整体表现", 1, None, "functional", "Subsystem.Pressure=8bar", 2, 1.5, 250.0),
            (4, 3, None, 2, "fault-mode", "PSU 故障诊断", "定位 PSU 输出失效", 0, 1, "electric", "输出降至 0V", 2, 0.9, 180.0),
        ],
        ["test_id"],
    )

    sync_rows(
        conn,
        "test_fault_coverage",
        ["test_id", "state_id", "coverage_type", "confidence"],
        [
            (1, 2, "detect", 0.95),
            (2, 4, "detect", 0.93),
            (3, 6, "detect", 0.90),
            (4, 2, "isolate", 0.80),
        ],
        ["test_id", "state_id", "coverage_type"],
    )

    sync_rows(
        conn,
        "diagnosis_fault",
        ["fault_id", "state_id", "code", "name", "description", "category", "severity", "metadata_json"],
        [
            (1, 2, "f01", "PSU 输出失效", "电源输出为零", "component", "major", None),
            (2, 4, "f02", "执行器卡滞", "执行器输出为零", "component", "major", None),
            (3, 6, "f03", "子系统故障", "子系统聚合故障", "system", "critical", None),
        ],
        ["fault_id"],
    )

    sync_rows(
        conn,
        "diagnosis_test",
        ["diagnosis_test_id", "test_id", "code", "name", "description", "test_type", "scope", "metadata_json"],
        [
            (1, 1, "t01", "PSU 输出电压测试", "测量 PSU 输出是否稳定", "signal", "component", None),
            (2, 2, "t02", "执行器压力测试", "测量执行器输出压力", "signal", "component", None),
            (3, 3, "t03", "系统功能验证", "验证 DeliverPressure 功能", "function", "subsystem", None),
            (4, 4, "t04", "PSU 故障诊断", "定位 PSU 输出失效", "fault-mode", "component", None),
        ],
        ["diagnosis_test_id"],
    )

    sync_rows(
        conn,
        "diagnosis_matrix",
        ["matrix_id", "container_id", "version", "notes"],
        [(1, 2, "v1", "DemoWorkflow 依赖矩阵")],
        ["matrix_id"],
    )

    sync_rows(
        conn,
        "diagnosis_matrix_entry",
        ["matrix_id", "diagnosis_test_id", "fault_id", "effect", "weight", "evidence_type", "notes"],
        [
            (1, 1, 1, "detect", 0.95, "analysis", ""),
            (1, 1, 3, "detect", 0.60, "analysis", "子系统故障包含 PSU 故障"),
            (1, 2, 2, "detect", 0.93, "analysis", ""),
            (1, 2, 3, "detect", 0.55, "analysis", "子系统故障包含执行器故障"),
            (1, 3, 1, "detect", 0.90, "analysis", ""),
            (1, 3, 2, "detect", 0.90, "analysis", ""),
            (1, 3, 3, "isolate", 0.80, "analysis", "聚合故障通过功能测试识别"),
            (1, 4, 1, "isolate", 0.90, "analysis", ""),
        ],
        ["matrix_id", "diagnosis_test_id", "fault_id"],
    )

    sync_rows(
        conn,
        "test_constraint",
        ["constraint_id", "test_id", "constraint_type", "value", "unit"],
        [
            (1, 1, "setup_time", "0.2", "hour"),
            (2, 2, "setup_time", "0.3", "hour"),
            (3, 3, "team_size", "3", "person"),
        ],
        ["constraint_id"],
    )

    sync_rows(
        conn,
        "analysis_requirement",
        ["requirement_id", "container_id", "metric", "target_value", "unit", "notes"],
        [
            (1, 2, "detection_rate", 0.90, "ratio", "子系统检测率目标"),
            (2, 2, "isolation_rate", 0.75, "ratio", "子系统隔离率目标"),
            (3, 1, "detection_rate", 0.85, "ratio", "系统层级检测率"),
        ],
        ["requirement_id"],
    )

    sync_rows(
        conn,
        "analysis_constraint",
        ["constraint_id", "container_id", "constraint_type", "value", "unit"],
        [
            (1, 2, "max_duration", "3", "hour"),
            (2, 2, "max_cost", "800", "USD"),
        ],
        ["constraint_id"],
    )

    sync_rows(
        conn,
        "test_plan_candidate",
        ["candidate_id", "container_id", "name", "description", "total_cost", "total_duration", "selection_notes"],
        [(1, 2, "基础测试方案", "满足指标的最小测试集合", 320.0, 2.0, "自动生成示例")],
        ["candidate_id"],
    )

    sync_rows(
        conn,
        "test_plan_candidate_item",
        ["candidate_id", "test_id"],
        [(1, 1), (1, 2), (1, 3)],
        ["candidate_id", "test_id"],
    )

    sync_rows(
        conn,
        "diagnosis_tree",
        ["tree_id", "container_id", "name", "description"],
        [(1, 2, "Demo Diagnosis Tree", "基于候选测试集生成的示例决策树")],
        ["tree_id"],
    )

    sync_rows(
        conn,
        "diagnosis_tree_node",
        ["node_id", "tree_id", "parent_node_id", "test_id", "state_id", "outcome", "comment"],
        [
            (1, 1, None, 1, None, "root", "执行 PSU 输出电压测试"),
            (2, 1, 1, None, 5, "pass", "测试通过，判定子系统正常"),
            (3, 1, 1, 4, None, "fail", "测试失败，执行故障模式测试"),
            (4, 1, 3, None, 2, "isolate", "确认 PSU 输出失效"),
        ],
        ["node_id"],
    )

    sync_rows(
        conn,
        "containers",
        [
            "id",
            "name",
            "type",
            "parent_id",
            "order_index",
            "analysis_depth",
            "interface_json",
            "behavior_smt",
            "fault_modes_json",
            "tests_json",
            "analysis_json",
            "equipment_id",
            "equipment_type",
            "equipment_name",
        ],
        [
            (1, "Demo System", 0, None, 0, 0, compact_json([]), "", subsystem_behavior_json(), "", "", None, "System", "Demo System"),
            (2, "Subsystem", 1, 1, 0, 0, compact_json([]), "", subsystem_behavior_json(), "", "", None, "Subsystem", "Hydraulics"),
            (3, "PSU-1", 6, 2, 0, 0, container_ports_json("PSU-1", "Vout", "power"), "", psu_behavior_json(), tests_json[0], "", 1, "Power", "Power Supply"),
            (4, "ACT-1", 6, 2, 1, 0, container_ports_json("ACT-1", "Pressure", "hydraulic", "Supply", "hydraulic"), "", actuator_behavior_json(), tests_json[1], "", 2, "Actuator", "Hydraulic Actuator"),
        ],
        ["id"],
    )

    sync_rows(
        conn,
        "equipment_containers",
        ["equipment_id", "container_id"],
        [(1, 3), (2, 4)],
        ["equipment_id"],
    )

    conn.commit()


def main() -> None:
    parser = argparse.ArgumentParser(description="Update DemoWorkflow project.db schema and sample data")
    parser.add_argument("db", type=Path, help="Path to project.db (e.g. MyProjects/DemoWorkflow/DemoWorkflow.db)")
    args = parser.parse_args()

    if not args.db.exists():
        raise SystemExit(f"Database file '{args.db}' does not exist")

    conn = sqlite3.connect(str(args.db))
    try:
        conn.execute("PRAGMA foreign_keys = ON")
        ensure_schema(conn)
        apply_demo_dataset(conn)
    finally:
        conn.close()

    print("Database updated with SMT components, aggregated states, and diagnosability matrix.")


if __name__ == "__main__":
    main()
