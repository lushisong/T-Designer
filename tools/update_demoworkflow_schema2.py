#!/usr/bin/env python3
"""Update DemoWorkflow SQLite schema to match demo_projectbuilder.cpp expectations."""

import argparse
import sqlite3
from pathlib import Path
from typing import Iterable

SCHEMA_TABLES = [
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
    CREATE TABLE IF NOT EXISTS Page (
        Page_ID INTEGER PRIMARY KEY,
        ProjectStructure_ID INTEGER,
        Page_Desc TEXT,
        PageType TEXT,
        PageNum INTEGER,
        PageName TEXT,
        Scale TEXT,
        Border TEXT,
        Title TEXT,
        AlterTime TEXT,
        MD5Code TEXT
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
    CREATE TABLE IF NOT EXISTS function_bindings (
        function_id INTEGER PRIMARY KEY,
        symbol_id INTEGER
    )
    """,
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
    CREATE TABLE IF NOT EXISTS UserTest (
        UserTest_ID INTEGER PRIMARY KEY,
        FunctionID INTEGER,
        Name TEXT
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
        inherits_from_container_id INTEGER,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        updated_at TEXT DEFAULT CURRENT_TIMESTAMP
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
    CREATE TABLE IF NOT EXISTS container_interface_binding (
        binding_id INTEGER PRIMARY KEY AUTOINCREMENT,
        interface_id INTEGER NOT NULL,
        equipment_id INTEGER,
        terminal_id INTEGER,
        connect_line_id INTEGER,
        binding_role TEXT
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
    CREATE TABLE IF NOT EXISTS container_state_interface (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        state_id INTEGER NOT NULL,
        interface_id INTEGER NOT NULL,
        "constraint" TEXT
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
        unit TEXT DEFAULT 'ratio',
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
    """
]

COLUMN_UPDATES = [
    ("Equipment", "TVariableLegacy", "TVariableLegacy TEXT"),
    ("Equipment", "TModelLegacy", "TModelLegacy TEXT"),
    ("Function", "LinkText", "LinkText TEXT"),
    ("Function", "ComponentDependency", "ComponentDependency TEXT"),
    ("Function", "AllComponents", "AllComponents TEXT"),
    ("Function", "FunctionDependency", "FunctionDependency TEXT"),
    ("Function", "PersistentFlag", "PersistentFlag INTEGER"),
    ("Function", "FaultProbability", "FaultProbability REAL"),
    ("container_state_interface", "constraint", '"constraint" TEXT'),
]

INDEX_STATEMENTS = [
    "CREATE INDEX IF NOT EXISTS idx_diag_entry_test ON diagnosis_matrix_entry(matrix_id, diagnosis_test_id)",
    "CREATE INDEX IF NOT EXISTS idx_diag_entry_fault ON diagnosis_matrix_entry(matrix_id, fault_id)",
]


def ensure_table(conn: sqlite3.Connection, ddl: str) -> None:
    conn.execute(ddl)


def column_exists(conn: sqlite3.Connection, table: str, column: str) -> bool:
    cursor = conn.execute(f"PRAGMA table_info({table})")
    return any(row[1].lower() == column.lower() for row in cursor.fetchall())


def ensure_column(conn: sqlite3.Connection, table: str, column: str, definition: str) -> None:
    if not column_exists(conn, table, column):
        conn.execute(f"ALTER TABLE {table} ADD COLUMN {definition}")


def apply_schema(conn: sqlite3.Connection) -> None:
    for ddl in SCHEMA_TABLES:
        ensure_table(conn, ddl)
    for table, column, definition in COLUMN_UPDATES:
        ensure_column(conn, table, column, definition)
    for ddl in INDEX_STATEMENTS:
        conn.execute(ddl)


def main() -> None:
    parser = argparse.ArgumentParser(description="Update DemoWorkflow SQLite schema to match demo_projectbuilder expectations")
    parser.add_argument("db", type=Path, help="Path to DemoWorkflow database")
    args = parser.parse_args()

    db_path = args.db
    if not db_path.exists():
        raise SystemExit(f"Database {db_path} does not exist")

    conn = sqlite3.connect(str(db_path))
    try:
        apply_schema(conn)
        conn.commit()
    finally:
        conn.close()

    print(f"Schema updated for {db_path}")


if __name__ == "__main__":
    main()