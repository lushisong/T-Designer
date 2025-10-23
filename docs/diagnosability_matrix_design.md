# Diagnosability (D-Matrix) Design Notes

## 1. Conceptual Model
- **Tests (`T`)**: any verification action (signal measurement, functional validation,
  or virtual fault-mode probe) that produces evidence about system health.
- **Faults (`F`)**: analysable failure states corresponding to `container_state`
  entries where `state_type='fault'`. Each fault belongs to a container and may be
  derived from one or more child component fault states.
- **Matrix Entry (`D_{ij}`)**: qualitative relationship between test `T_i` and fault
  `F_j`. Typical labels:
  - `0`: no effect (test is indifferent to this fault).
  - `1`: detection (test fails when the fault is present).
  - `2`: isolation (test response uniquely identifies the fault or a fault set).
  - Optionally store additional metadata such as confidence, cost impact, or
    conditional applicability.

## 2. Storage Strategy
Given the expected sparsity (large matrices with relatively few non-zero entries),
we store the matrix in compressed form.

### 2.1 Tables
1. `diagnosis_fault`
   - `fault_id` (PK)
   - `container_state_id` (FK → `container_state.state_id`)
   - `code` (short token, e.g., `f12`)
   - `name`, `description`
   - `severity`, `category`
   - `metadata_json` (optional attributes, e.g., MTBF, related functions)
2. `diagnosis_test`
   - `test_id` (PK)
   - `test_definition_id` (FK → `test_definition.test_id`)
   - `code` (short token, e.g., `t05`)
   - `name`, `description`
   - `test_type`, `scope`
   - `metadata_json`
3. `diagnosis_matrix`
   - `matrix_id` (PK)
   - `container_id` (analysis scope)
   - `version` (timestamp or semantic version)
   - `notes`
4. `diagnosis_matrix_entry`
   - `matrix_id` (FK)
  - `test_id` (FK)
   - `fault_id` (FK)
   - `effect` (enum: `none`, `detect`, `isolate`, `ambiguity`, ...)
   - `weight` (REAL; e.g., detection probability)
   - `evidence_type` (string; optional tags such as `simulated`, `analysis`)
   - `notes`
   - Composite PK: (`matrix_id`, `test_id`, `fault_id`).

Indices:
- `CREATE INDEX idx_diag_entry_test ON diagnosis_matrix_entry(matrix_id, test_id);`
- `CREATE INDEX idx_diag_entry_fault ON diagnosis_matrix_entry(matrix_id, fault_id);`

### 2.2 JSON Metadata Conventions
Store per-test/fault alias sets and UI hints:
```json
{
  "aliases": ["HydraulicPressureSensor"],
  "tooltip": "执行器压力信号测试",
  "severity": "major"
}
```

## 3. In-Memory Representation
Define the following DO classes (in `DO/diagnosis/`):

```cpp
struct DiagnosabilityTest {
    int id;                 // surrogate key
    QString code;           // tXX label
    QString name;
    QString description;
    QString type;           // signal/function/fault-mode
    QJsonObject metadata;
};

struct DiagnosabilityFault {
    int id;
    QString code;           // fXX label
    QString name;
    QString description;
    QString category;
    QJsonObject metadata;
};

struct DiagnosabilityCell {
    int testId;
    int faultId;
    QString effect;         // "none"/"detect"/"isolate"/...
    double weight;          // detection probability or ranking score
    QString evidenceType;
};

class DiagnosabilityMatrix {
public:
    int matrixId;
    int containerId;
    QVector<DiagnosabilityTest> tests;
    QVector<DiagnosabilityFault> faults;
    QHash<int, QVector<DiagnosabilityCell>> columns;   // keyed by testId
    QHash<int, QVector<DiagnosabilityCell>> rows;      // keyed by faultId
    // helper maps from testId/faultId to column/row index for quick lookup
    QHash<int, int> testIndexMap;
    QHash<int, int> faultIndexMap;
};
```

### Access Patterns
- **Get column**: return `columns.value(testId)`; build lazily by querying
  `diagnosis_matrix_entry` when missing.
- **Get row**: analogous using `rows` map.
- **Slice**: Provide `QVector<DiagnosabilityCell> getSliceByTests(const QList<int>& testIds)`.
- **Statistics**: functions returning detection/isolation coverage percentages.

## 4. UI Model Considerations
- `DiagnosabilityMatrixModel` exposes `rowCount = faults.size()` and
  `columnCount = tests.size()`.
- For each cell, look up the pre-fetched `DiagnosabilityCell` entry; if absent,
  return `""` (interpreted as `0`).
- Tooltips: use `Qt::ToolTipRole` to provide `test.name`/`fault.name` metadata
  on headers and cells (for cell tooltips include `effect` + `weight`).
- Implement `fetchMore()` to request additional batches of faults/tests for
  very large matrices (e.g., load 200 rows at a time).
- Provide `QSortFilterProxyModel` with regex filter to isolate a single test or
  fault; pair with search line edits in the dialog.

## 5. Generation Workflow
1. Determine analysis scope container.
2. Enumerate tests (auto-generated + user-defined) relevant to that container.
3. Enumerate candidate fault states within the scope (and optionally imported
   from child containers if deeper analysis is requested).
4. For each test/fault pair, evaluate SMT-based coverage heuristics or reuse
   curated sample data; persist as `diagnosis_matrix_entry` records.
5. Update UI caches when the matrix is regenerated.

## 6. Performance Targets
- Support up to 300 tests × 10,000 faults (~3 million cells). Sparse storage ensures
  actual entries remain manageable (e.g., < 100k non-zero).
- UI refresh should stay below 200 ms when switching focus between tests or faults.
- Database queries should leverage indices to avoid full scans; use pagination for
  row loading.

