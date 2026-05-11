#pragma once
#include <arld/core/ClearanceEngine.h>
#include <arld/core/ProjectFile.h>
#include <QDockWidget>
#include <vector>

class QTableWidget;
class QPushButton;

namespace arld::ui {

class RampScene;
class RampView;

class ViolationsPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit ViolationsPanel(RampScene* scene, RampView* view, QWidget* parent = nullptr);
    void refresh(const std::vector<arld::core::ViolationResult>& violations);

private slots:
    void onRowClicked(int row, int col);
    void onOverrideClicked();

private:
    RampScene*    m_scene;
    RampView*     m_view;
    QTableWidget* m_table;
    QPushButton*  m_overrideBtn;
    std::vector<arld::core::ViolationResult> m_violations;
};

} // namespace arld::ui
