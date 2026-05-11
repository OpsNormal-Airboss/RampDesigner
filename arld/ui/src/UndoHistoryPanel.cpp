#include <arld/ui/UndoHistoryPanel.h>
#include <arld/ui/RampScene.h>
#include <arld/core/UndoStack.h>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

namespace arld::ui {

UndoHistoryPanel::UndoHistoryPanel(arld::ui::RampScene* scene, QWidget* parent)
    : QDockWidget(tr("Undo History"), parent)
    , m_scene(scene)
{
    setObjectName(QStringLiteral("UndoHistoryPanel"));

    auto* container = new QWidget(this);
    auto* vlay = new QVBoxLayout(container);
    vlay->setContentsMargins(4, 4, 4, 4);

    m_list = new QListWidget(container);
    m_list->setToolTip(tr("Click a command to jump to that history state"));
    vlay->addWidget(m_list);
    setWidget(container);

    connect(m_list, &QListWidget::itemClicked,
            this, &UndoHistoryPanel::onItemClicked);

    // Subscribe to undo stack changes
    auto& stack = m_scene->undoStack();
    auto prev = stack.onChanged;
    stack.onChanged = [this, prev]() {
        if (prev) prev();
        refresh();
    };

    refresh();
}

UndoHistoryPanel::~UndoHistoryPanel() {
    m_scene->undoStack().onChanged = nullptr;
}

void UndoHistoryPanel::refresh() {
    if (m_refreshing) return;
    m_refreshing = true;

    const auto& stack    = m_scene->undoStack();
    const auto  history  = stack.history();
    const int   current  = stack.currentIndex();  // -1 = nothing done
    const int   total    = static_cast<int>(history.size());

    // Show up to 20 most-recent commands (tail of history)
    constexpr int kMaxShow = 20;
    const int startIdx = std::max(0, total - kMaxShow);

    m_list->clear();
    for (int i = startIdx; i < total; ++i) {
        auto* item = new QListWidgetItem(
            QString::fromStdString(history[static_cast<size_t>(i)]), m_list);
        item->setData(Qt::UserRole, i);  // store absolute index

        if (i == current) {
            // Current command: bold
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
            item->setForeground(Qt::white);
        } else if (i > current) {
            // Future/undone commands: grayed out
            item->setForeground(QColor(120, 120, 120));
        }
    }

    m_refreshing = false;
}

void UndoHistoryPanel::onItemClicked(QListWidgetItem* item) {
    if (!item || m_refreshing) return;
    const int targetIndex = item->data(Qt::UserRole).toInt();
    // goToIndex uses 0-based executed index: index i means i+1 commands were executed.
    // We stored the stack index directly (0 = first command executed), so target = targetIndex.
    m_scene->undoStack().goToIndex(targetIndex);
}

} // namespace arld::ui
