#pragma once
#include <QDockWidget>

class QListWidget;
class QListWidgetItem;

namespace arld::ui {

class RampScene;

class UndoHistoryPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit UndoHistoryPanel(arld::ui::RampScene* scene, QWidget* parent = nullptr);
    void refresh();

private slots:
    void onItemClicked(QListWidgetItem* item);

private:
    arld::ui::RampScene* m_scene;
    QListWidget*         m_list;
    bool                 m_refreshing = false;  // guard against recursive refresh
};

} // namespace arld::ui
