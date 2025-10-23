#!/usr/bin/env python3
"""Normalize DemoWorkflow project database schema to match demo_projectbuilder.cpp."""

import argparse
import sqlite3
from pathlib import Path
from typing import List, Tuple

TABLE_DEFINITIONS: Tuple[Tuple[str, str], ...] = (
    (
        "FunctionDefineClass",
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
    ),
    (
        "UserTest",
        """
        CREATE TABLE IF NOT EXISTS UserTest (
            UserTest_ID INTEGER PRIMARY KEY,
            FunctionID INTEGER,
            Name TEXT
        )
        """,
    ),
    (
        "function_bindings",
        """
        CREATE TABLE IF NOT EXISTS function_bindings (
            function_id INTEGER PRIMARY KEY,
            symbol_id INTEGER
        )
        """,
    ),
    (
        "containers",
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
    ),
    (
        "equipment_containers",
        """
        CREATE TABLE IF NOT EXISTS equipment_containers (
            equipment_id INTEGER PRIMARY KEY,
            container_id INTEGER
        )
        """,
    ),
    (
        "container_interface_link",
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
    ),
    (
        "container_state_composition",
        """
        CREATE TABLE IF NOT EXISTS container_state_composition (
            composition_id INTEGER PRIMARY KEY AUTOINCREMENT,
            parent_state_id INTEGER NOT NULL,
            child_state_id INTEGER NOT NULL,
            relation TEXT DEFAULT 'includes'
        )
        """,
    ),
    (
        "component_smt",
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
    ),
    (
        "diagnosis_fault",
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
    ),
    (
        "diagnosis_test",
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
    ),
    (
        "diagnosis_matrix",
        """
        CREATE TABLE IF NOT EXISTS diagnosis_matrix (
            matrix_id INTEGER PRIMARY KEY AUTOINCREMENT,
            container_id INTEGER NOT NULL,
            version TEXT,
            notes TEXT,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
        """,
    ),
    (
        "diagnosis_matrix_entry",
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
    ),
)

COLUMN_DEFINITIONS: Tuple[Tuple[str, str, str], ...] = (
    ("Equipment", "TVariableLegacy", "TVariableLegacy TEXT"),
    ("Equipment", "TModelLegacy", "TModelLegacy TEXT"),
    ("Function", "LinkText", "LinkText TEXT"),
    ("Function", "ComponentDependency", "ComponentDependency TEXT"),
    ("Function", "AllComponents", "AllComponents TEXT"),
    ("Function", "FunctionDependency", "FunctionDependency TEXT"),
    ("Function", "PersistentFlag", "PersistentFlag INTEGER DEFAULT 0"),
    ("Function", "FaultProbability", "FaultProbability REAL"),
    ("container_state_interface", "constraint", '"constraint" TEXT'),
)

INDEX_DEFINITIONS: Tuple[Tuple[str, str], ...] = (
    (
        "idx_diag_entry_test",
        "CREATE INDEX IF NOT EXISTS idx_diag_entry_test ON diagnosis_matrix_entry(matrix_id, diagnosis_test_id)",
    ),
    (
        "idx_diag_entry_fault",
        "CREATE INDEX IF NOT EXISTS idx_diag_entry_fault ON diagnosis_matrix_entry(matrix_id, fault_id)",
    ),
)


def table_exists(conn: sqlite3.Connection, name: str) -> bool:
    cur = conn.execute(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=? LIMIT 1",
        (name,),
    )
    return cur.fetchone() is not None


def get_columns(conn: sqlite3.Connection, table: str) -> List[str]:
    cur = conn.execute(f"PRAGMA table_info({table})")
    return [row[1] for row in cur.fetchall()]


def ensure_table(conn: sqlite3.Connection, name: str, create_sql: str, actions: List[str]) -> None:
    existed = table_exists(conn, name)
    conn.execute(create_sql)
    if not existed:
        actions.append(f"created table {name}")


def ensure_column(conn: sqlite3.Connection, table: str, column: str, definition: str, actions: List[str]) -> None:
    existing = set(get_columns(conn, table))
    if column not in existing:
        conn.execute(f"ALTER TABLE {table} ADD COLUMN {definition}")
        actions.append(f"added column {table}.{column}")


def ensure_index(conn: sqlite3.Connection, name: str, create_sql: str, actions: List[str]) -> None:
    cur = conn.execute(
        "SELECT 1 FROM sqlite_master WHERE type='index' AND name=? LIMIT 1",
        (name,),
    )
    if cur.fetchone() is None:
        conn.execute(create_sql)
        actions.append(f"created index {name}")


def apply_schema(conn: sqlite3.Connection) -> List[str]:
    actions: List[str] = []
    for name, create_sql in TABLE_DEFINITIONS:
        ensure_table(conn, name, create_sql, actions)
    for table, column, definition in COLUMN_DEFINITIONS:
        ensure_column(conn, table, column, definition, actions)
    for name, create_sql in INDEX_DEFINITIONS:
        ensure_index(conn, name, create_sql, actions)
    return actions


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Update DemoWorkflow project database schema to match demo_projectbuilder.cpp"
    )
    parser.add_argument("db", type=Path, help="Path to DemoWorkflow.db or project database file")
    args = parser.parse_args()

    if not args.db.exists():
        raise SystemExit(f"Database file not found: {args.db}")

    conn = sqlite3.connect(args.db)
    try:
        conn.execute("PRAGMA foreign_keys=OFF")
        actions = apply_schema(conn)
        conn.commit()
    finally:
        conn.close()

    if actions:
        print("Schema updated:")
        for action in actions:
            print(f" - {action}")
    else:
        print("Schema already up to date.")


if __name__ == "__main__":
    main()