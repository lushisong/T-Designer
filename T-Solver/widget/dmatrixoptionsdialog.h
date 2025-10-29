#ifndef DMATRIXOPTIONSDIALOG_H
#define DMATRIXOPTIONSDIALOG_H

#include <QDialog>

class QLineEdit;

#include "testability/testability_types.h"

namespace Ui {
class DMatrixOptionsDialog;
}

class DMatrixOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DMatrixOptionsDialog(QWidget *parent = nullptr);
    ~DMatrixOptionsDialog() override;

    void setOptions(const testability::DMatrixBuildOptions &options);
    testability::DMatrixBuildOptions options() const;

private:
    Ui::DMatrixOptionsDialog *ui = nullptr;
    QLineEdit *outputDirEdit = nullptr;
};

#endif // DMATRIXOPTIONSDIALOG_H
