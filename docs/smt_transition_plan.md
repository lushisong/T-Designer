# SMT-Based DemoWorkflow Transition Plan

## 1. Background & Goals
The current DemoWorkflow showcase project still relies on Livingstone-style T-language
snippets that were embedded inside the `Equipment` table (`TModel`, `TVariable`),
and the higher-level container descriptions hard-code simple algebraic expressions.
The new development cycle requires us to:

1. Express each physical component (元件) in SMT-LIB 2 format and migrate DemoWorkflow to
   use the common Z3-based solver stack.
2. Automatically aggregate container-level SMT behavior (including connections between
   child component ports) so that the solver operates at the analysis scope selected by
   the user.
3. Introduce a diagnosability D-matrix model with scalable data structures,
   persistent storage, and UI visualisation (supporting hundreds of tests and thousands
   of faults).
4. Update the database schema and runtime services so the demonstration workflow in
   `docs/本开发周期的功能总要求.txt` can be exercised end-to-end.

## 2. Current State Summary
- DemoWorkflow database structure is seeded by `DemoProjectBuilder::populateProjectDatabase`.
  Component state behaviors are stored as plain expressions (`representation='expr'`).
- Component definitions under `Equipment.TModel` are still in the bespoke Livingstone
  T-language format.
- There is no persisted D-matrix representation; `test_fault_coverage` only stores
  aggregated coverage percentages.
- UI: `TestManagementDialog` has a placeholder "依赖矩阵" tab but no scalable model/view
  implementation.

## 3. Target Architecture
### 3.1 Component-Level SMT Representation
- Introduce a dedicated table `component_behavior_smt` (component_id, state_code,
  smt_script, metadata JSON) or reuse `container_state_behavior` with a structured
  `representation` value (`'smt2'`) for individual components.
- Populate component SMT definitions by importing `Model.db` entries (variable
  declarations, normal mode assertions, fault modes).
- Retain backwards compatibility by storing the original T-language in archived fields
  (`Equipment.TModelLegacy`).

### 3.2 Container SMT Aggregation
- Store explicit port definitions in `container_interface` (already present) and add
  a `container_interface_link` table describing connections between interfaces.
- Aggregation workflow:
  1. Load SMT scripts for each child component within a container.
  2. Instantiate component-level scripts by alpha-renaming symbols to ensure
     uniqueness per placement (e.g., `%MD%` → `%Subsystem.MD1%`).
  3. Translate connection definitions into equality constraints tying corresponding
     child interface variables.
  4. For containers with derived states, synthesise aggregated SMT scripts by
     conjoining the instantiated child scripts plus connection constraints;
     export the result into `container_state_behavior` with `representation='smt2'`.
  5. Persist metadata describing which child states contribute to each aggregated
     state (`container_state_composition` table) for traceability.
- Provide BO services to (re)generate container SMT on demand whenever wiring or
  child assignments change.

### 3.3 D-Matrix Data Model
- Logical model: sparse matrix mapping tests × fault states with qualitative
  outcomes (`detect`, `isolate`, weight/probability).
- In-memory structure: `DiagnosabilityMatrix` class encapsulating:
  - `QVector<TestDescriptor>` and `QVector<FaultDescriptor>` (metadata).
  - `QHash<int, QVector<CellEntry>>` or compressed sparse column representation.
  - Efficient slicing APIs to retrieve a test column or fault row.
- Database schema additions:
  - `diagnosis_fault` capturing each analysable fault state (ties to
    `container_state` when `state_type='fault'`).
  - `diagnosis_test` enumerating candidate tests (ties to `test_definition`).
  - `diagnosis_matrix`/`diagnosis_matrix_entry` storing sparse D-matrix entries
    (`matrix_id`, `test_id`, `fault_id`, `effect` enum, `weight`, `notes`).
  - Indices on (`test_id`, `fault_id`) to support fast lookups.

### 3.4 UI Updates — Dependency Matrix Tab
- Replace the placeholder widget with a `QTableView` bound to a custom
  `DiagnosabilityMatrixModel` (derived from `QAbstractTableModel`).
- Implement:
  - Lazy data access via the sparse matrix structure to handle large matrices.
  - Header delegates showing `t{index}` and `f{index}` labels with hover tooltips.
  - Filtering UI (search fields) to focus on a single test column or fault row.
  - Virtual scrolling with chunked data retrieval to maintain responsiveness.
- Provide context actions to export slices or copy selected rows/columns.

### 3.5 Workflow Integration
- Extend BO services so that:
  - Selecting an analysis scope triggers regeneration/loading of the corresponding
    container-level SMT and D-matrix slice.
  - Test creation flows update both the test catalog and the D-matrix entries.
- Update DemoProjectBuilder to seed the database with coherent sample data covering
  component SMT, aggregated container behavior, and a non-trivial D-matrix.

## 4. Detailed Implementation Breakdown
### 4.1 Schema Migration & Seed Data
1. Draft SQL migration scripts introducing new tables:
   - `component_smt`, `container_interface_link`, `container_state_composition`,
     `diagnosis_fault`, `diagnosis_test`, `diagnosis_matrix`,
     `diagnosis_matrix_entry`.
   - Add archival columns (`Equipment.TModelLegacy`, `Equipment.TVariableLegacy`).
2. Update `DemoProjectBuilder::populateProjectDatabase` to execute the schema
   migration when seeding DemoWorkflow.
3. Provide companion updates for the template database (`templete/project.db`).

### 4.2 Component SMT Import Pipeline
1. Implement `ComponentSmtDefinition` DO struct capturing variables, parameters,
   states, and SMT assertions.
2. Write importer in `DemoProjectBuilder` (or dedicated tool) that reads
   `ref/Model.db` and maps each component type to DemoWorkflow equipment entries.
3. Persist SMT scripts into `component_smt` and populate new JSON metadata fields
   describing ports, parameters, and failure modes.
4. Archive legacy T-language strings to the new `Equipment` legacy columns.

### 4.3 Container Aggregation Engine
1. Model wiring information using `container_interface_link` records generated from
   the existing connection tables (`ConnectLine`, `Wires`).
2. Implement BO service `SmtAggregationService` with responsibilities:
   - Retrieve child component SMT definitions and rename symbols per instance.
   - Assemble connection constraints from interface links.
   - For each parent container state, compute the conjunction of relevant child
     states and persist aggregated SMT (`container_state_behavior`).
3. Provide a recomputation command usable both during demo project generation and at
   runtime when users modify connections.

### 4.4 Diagnosability Matrix Domain Layer
1. Create DO classes (`DiagnosabilityTest`, `DiagnosabilityFault`,
   `DiagnosabilityMatrix`, `DiagnosabilityCell`).
2. Implement repository/BO (`DiagnosabilityMatrixRepository`) handling CRUD over the
   new diagnosis tables, leveraging sparse storage and pagination queries.
3. Provide helper methods to extract a single test column / fault row for UI access.

### 4.5 UI Integration
1. Develop `DiagnosabilityMatrixModel` (inherits `QAbstractTableModel`) that binds to
   the BO service and exposes matrix data in a paged manner.
2. Update `TestManagementDialog` to embed a `QTableView`, search/filter controls, and
   tooltip-enabled headers.
3. Implement interactive features (row/column focus, export) and connect them to the
   repository helper methods.

### 4.6 Demo Data Authoring
1. Compose representative SMT definitions for DemoWorkflow components (PSU, actuator,
   hydraulic subsystem) based on `ref/Model.db`.
2. Define container-level wiring and generate aggregated SMT scripts using the new
   aggregation service.
3. Seed a sparse D-matrix example with ~5 tests × ~8 faults to validate the flow.

### 4.7 Validation & Testing
1. Add Qt unit tests covering:
   - Component SMT import correctness (variable and assertion counts match reference).
   - Aggregated SMT generation (connection constraints present, alpha-renaming works).
   - D-matrix repository slicing logic.
2. Provide UI smoke test verifying that the matrix model reports correct dimensions
   and tooltips for the seeded dataset.
3. Update CI/test docs to include commands for running the new tests.

## 5. Open Questions & Research Items
- Component parameter instantiation strategy: need to confirm how `%Resistance%`
  placeholders in `Model.db` map to actual parameter values stored in project
  equipment records.
- Fault mode aggregation semantics: whether child fault combinations should produce
  distinct parent fault states or collapsed "generic fault" states.
- D-matrix entry semantics for different test types (signal/function/fault-mode)
  — require alignment with `docs/本开发周期的功能总要求.txt` definitions.

