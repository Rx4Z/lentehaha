#include "Theme.h"

Theme& Theme::instance()
{
    static Theme inst;
    return inst;
}

Theme::Theme()
{
    m_colors["background"] = QColor("#1a1b26");
    m_colors["surface"] = QColor("#24283b");
    m_colors["surfaceAlt"] = QColor("#2f334d");
    m_colors["border"] = QColor("#3b3f5c");
    m_colors["primary"] = QColor("#7aa2f7");
    m_colors["primaryHover"] = QColor("#89b4fa");
    m_colors["success"] = QColor("#9ece6a");
    m_colors["warning"] = QColor("#e0af68");
    m_colors["error"] = QColor("#f7768e");
    m_colors["text"] = QColor("#c0caf5");
    m_colors["textMuted"] = QColor("#565f89");
    m_colors["accent"] = QColor("#bb9af7");
}

QColor Theme::background() const { return m_colors["background"]; }
QColor Theme::surface() const { return m_colors["surface"]; }
QColor Theme::surfaceAlt() const { return m_colors["surfaceAlt"]; }
QColor Theme::border() const { return m_colors["border"]; }
QColor Theme::primary() const { return m_colors["primary"]; }
QColor Theme::primaryHover() const { return m_colors["primaryHover"]; }
QColor Theme::success() const { return m_colors["success"]; }
QColor Theme::warning() const { return m_colors["warning"]; }
QColor Theme::error() const { return m_colors["error"]; }
QColor Theme::text() const { return m_colors["text"]; }
QColor Theme::textMuted() const { return m_colors["textMuted"]; }
QColor Theme::accent() const { return m_colors["accent"]; }

QString Theme::stylesheet() const
{
    return R"(
QMainWindow, QDialog {
    background-color: #1a1b26;
    color: #c0caf5;
}

QWidget {
    background-color: #1a1b26;
    color: #c0caf5;
    font-family: 'Segoe UI', 'Inter', sans-serif;
    font-size: 13px;
}

QPushButton {
    background-color: #24283b;
    color: #c0caf5;
    border: 1px solid #3b3f5c;
    border-radius: 6px;
    padding: 8px 16px;
    font-weight: 500;
}

QPushButton:hover {
    background-color: #2f334d;
    border-color: #7aa2f7;
}

QPushButton:pressed {
    background-color: #7aa2f7;
    color: #1a1b26;
}

QPushButton:disabled {
    background-color: #1a1b26;
    color: #565f89;
    border-color: #2f334d;
}

QPushButton#primaryButton {
    background-color: #7aa2f7;
    color: #1a1b26;
    border: none;
    font-weight: 600;
}

QPushButton#primaryButton:hover {
    background-color: #89b4fa;
}

QPushButton#dangerButton {
    background-color: #f7768e;
    color: #1a1b26;
    border: none;
    font-weight: 600;
}

QPushButton#dangerButton:hover {
    background-color: #ff9e64;
}

QLineEdit, QComboBox {
    background-color: #24283b;
    color: #c0caf5;
    border: 1px solid #3b3f5c;
    border-radius: 6px;
    padding: 6px 10px;
}

QLineEdit:focus, QComboBox:focus {
    border-color: #7aa2f7;
}

QComboBox::drop-down {
    border: none;
    width: 24px;
}

QComboBox QAbstractItemView {
    background-color: #24283b;
    color: #c0caf5;
    border: 1px solid #3b3f5c;
    selection-background-color: #7aa2f7;
    selection-color: #1a1b26;
}

QTreeWidget, QTreeView, QListWidget, QTableWidget {
    background-color: #24283b;
    color: #c0caf5;
    border: 1px solid #3b3f5c;
    border-radius: 6px;
    outline: none;
}

QTreeWidget::item, QTreeView::item, QListWidget::item {
    padding: 4px;
    border-radius: 4px;
}

QTreeWidget::item:selected, QTreeView::item:selected, QListWidget::item:selected,
QTableWidget::item:selected {
    background-color: #33467c;
    color: #c0caf5;
}

QTreeWidget::item:hover, QTreeView::item:hover, QListWidget::item:hover,
QTableWidget::item:hover {
    background-color: #2f334d;
}

QHeaderView::section {
    background-color: #2f334d;
    color: #c0caf5;
    border: none;
    border-bottom: 1px solid #3b3f5c;
    padding: 6px 6px 6px 3px;
    font-weight: 600;
}

QProgressBar {
    background-color: #24283b;
    border: 1px solid #3b3f5c;
    border-radius: 4px;
    text-align: center;
    color: #c0caf5;
}

QProgressBar::chunk {
    background-color: #7aa2f7;
    border-radius: 3px;
}

QProgressBar#successProgress::chunk {
    background-color: #9ece6a;
}

QProgressBar#errorProgress::chunk {
    background-color: #f7768e;
}

QProgressBar#warningProgress::chunk {
    background-color: #e0af68;
}

QScrollBar:vertical {
    background-color: #1a1b26;
    width: 10px;
    border-radius: 5px;
}

QScrollBar::handle:vertical {
    background-color: #3b3f5c;
    border-radius: 5px;
    min-height: 20px;
}

QScrollBar::handle:vertical:hover {
    background-color: #565f89;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}

QScrollBar:horizontal {
    background-color: #1a1b26;
    height: 10px;
    border-radius: 5px;
}

QScrollBar::handle:horizontal {
    background-color: #3b3f5c;
    border-radius: 5px;
    min-width: 20px;
}

QScrollBar::handle:horizontal:hover {
    background-color: #565f89;
}

QTabWidget::pane {
    border: 1px solid #3b3f5c;
    border-radius: 6px;
    background-color: #24283b;
}

QTabBar::tab {
    background-color: #1a1b26;
    color: #565f89;
    padding: 8px 16px;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
}

QTabBar::tab:selected {
    background-color: #24283b;
    color: #c0caf5;
    border-bottom: 2px solid #7aa2f7;
}

QTabBar::tab:hover:!selected {
    color: #c0caf5;
}

QCheckBox {
    color: #c0caf5;
    spacing: 6px;
}

QCheckBox::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid #3b3f5c;
    border-radius: 3px;
    background-color: #24283b;
}

QCheckBox::indicator:hover {
    border-color: #7aa2f7;
}

QCheckBox::indicator:checked {
    background-color: #7aa2f7;
    border-color: #7aa2f7;
}

QCheckBox::indicator:checked:hover {
    background-color: #89b4fa;
    border-color: #89b4fa;
}

QRadioButton {
    color: #c0caf5;
    spacing: 6px;
}

QRadioButton::indicator {
    width: 15px;
    height: 15px;
    border: 1px solid #3b3f5c;
    border-radius: 8px;
    background-color: #24283b;
}

QRadioButton::indicator:hover {
    border-color: #7aa2f7;
}

QRadioButton::indicator:checked {
    background-color: #7aa2f7;
    border-color: #7aa2f7;
}

QRadioButton::indicator:checked:hover {
    background-color: #89b4fa;
    border-color: #89b4fa;
}

QGroupBox {
    border: 1px solid #3b3f5c;
    border-radius: 6px;
    margin-top: 10px;
    padding-top: 6px;
    color: #565f89;
    font-weight: 600;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 10px;
    padding: 0 4px;
}

QTreeWidget::indicator {
    width: 15px;
    height: 15px;
    border: 1px solid #3b3f5c;
    border-radius: 3px;
    background-color: #24283b;
    margin-right: 2px;
}

/* Selection must not turn an unchecked box into a dark square: give it the
   selected row's background so it reads the same as on a normal row. */
QTreeWidget::indicator:selected {
    background-color: #33467c;
}

QTreeWidget::indicator:hover {
    border-color: #7aa2f7;
}

QTreeWidget::indicator:checked {
    background-color: #7aa2f7;
    border-color: #7aa2f7;
}

QTreeWidget::indicator:checked:selected {
    background-color: #7aa2f7;
    border-color: #7aa2f7;
}

QTreeWidget::indicator:checked:hover {
    background-color: #89b4fa;
    border-color: #89b4fa;
}

QTreeWidget::indicator:indeterminate {
    background-color: #24283b;
    border-color: #7aa2f7;
    image: none;
}

QTreeWidget::indicator:indeterminate:selected {
    background-color: #33467c;
}

QProgressBar#capacityMeter {
    background-color: #24283b;
    border: 1px solid #3b3f5c;
    border-radius: 4px;
    text-align: center;
    color: #c0caf5;
    min-height: 18px;
}

QProgressBar#capacityMeter::chunk {
    background-color: #e0af68;
    border-radius: 3px;
}

QLabel#warningLabel {
    background-color: #2f334d;
    border: 1px solid #3b3f5c;
    border-left: 3px solid #e0af68;
    border-radius: 6px;
    padding: 8px 12px;
    color: #e0af68;
}

QLabel#warningLabel[blocking="true"] {
    border-left: 3px solid #f7768e;
    color: #f7768e;
}

QSplitter::handle {
    background-color: transparent;
}

QSplitter::handle:horizontal {
    width: 16px;
}

QSplitter::handle:vertical {
    height: 16px;
}

QMenuBar {
    background-color: #1a1b26;
    color: #c0caf5;
}

QMenuBar::item:selected {
    background-color: #2f334d;
}

QMenu {
    background-color: #24283b;
    color: #c0caf5;
    border: 1px solid #3b3f5c;
    border-radius: 6px;
}

QMenu::item:selected {
    background-color: #7aa2f7;
    color: #1a1b26;
}

QStatusBar {
    background-color: #1a1b26;
    color: #565f89;
    border-top: 1px solid #3b3f5c;
}

QToolTip {
    background-color: #24283b;
    color: #c0caf5;
    border: 1px solid #3b3f5c;
    border-radius: 4px;
    padding: 4px;
}

QLabel {
    background: transparent;
    border: none;
}

QLabel#titleLabel {
    font-size: 18px;
    font-weight: 700;
    color: #c0caf5;
    background: transparent;
}

QLabel#subtitleLabel {
    font-size: 12px;
    color: #565f89;
    background: transparent;
    padding: 0;
}

QLabel#statusLabel {
    font-size: 11px;
    color: #565f89;
    background: transparent;
}

QFrame#cardFrame {
    background-color: transparent;
    border: 1px solid #3b3f5c;
    border-radius: 8px;
}

QFrame#dividerFrame {
    background-color: #3b3f5c;
    max-height: 1px;
    min-height: 1px;
    border: none;
}
)";
}
