#ifndef DMATRIXVIEWERDIALOG_H
#define DMATRIXVIEWERDIALOG_H

#include <QDialog>

#include "testability/testability_types.h"

class DMatrixModel;

namespace Ui {
class DMatrixViewerDialog;
}

class DMatrixViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DMatrixViewerDialog(QWidget *parent = nullptr);
    ~DMatrixViewerDialog() override;

    void setMatrix(const testability::DMatrixBuildResult &result,
                   const testability::DMatrixBuildOptions &options,
                   const QString &csvPath,
                   const QString &metadataPath);

private slots:
    void onExportClicked();
    void onShowFaultNamesChanged(int state);
    void onShowTestNamesChanged(int state);
    void onSelectTests();
    void onSelectFaults();
    void onSaveClicked();
    void onSaveAsClicked();
    void onCellDoubleClicked(const QModelIndex &index);

private:
    void updateSummary();
    void applyVisibility();
    bool exportCsv(const QString &filePath) const;
    bool saveMetadataToPath(const QString &path);
    void openSelectionDialog(bool forTests);
    void showCellDetails(const QModelIndex &index);

    Ui::DMatrixViewerDialog *ui = nullptr;
    DMatrixModel *model = nullptr;
    testability::DMatrixBuildOptions currentOptions;
    QString csvPath;
    QString metadataPath;
};

#endif // DMATRIXVIEWERDIALOG_H
