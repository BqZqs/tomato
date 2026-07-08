#pragma once

// ---------------------------------------------------------------------------
// shared theme constants – single source of truth for all widget styling
// ---------------------------------------------------------------------------

namespace Theme {

// ── Color palette ────────────────────────────────────────────────────

inline constexpr auto kTextPrimary    = "#1A1A2E";
inline constexpr auto kTextSecondary  = "#7B7D8C";
inline constexpr auto kTextDisabled   = "#B0B3BF";
inline constexpr auto kSurfaceWhite   = "#FFFFFF";
inline constexpr auto kSurfaceLight   = "#F8F9FB";
inline constexpr auto kPageBg         = "#F0F2F5";
inline constexpr auto kBorder         = "#D1D5DB";
inline constexpr auto kBorderHover    = "#9CA3AF";
inline constexpr auto kHoverBg        = "#F3F4F6";
inline constexpr auto kPressedBg      = "#E5E7EB";
inline constexpr auto kAccent         = "#6C63FF";
inline constexpr auto kAccentBg       = "#EEF2FF";
inline constexpr auto kSuccess        = "#22C55E";
inline constexpr auto kSuccessHover   = "#16A34A";
inline constexpr auto kError          = "#EF4444";
inline constexpr auto kErrorHover     = "#DC2626";
inline constexpr auto kWarning        = "#F59E0B";
inline constexpr auto kDisabledBg     = "#F9FAFB";
inline constexpr auto kSeparator      = "#E5E7EB";
inline constexpr auto kSplitterHandle = "rgba(108,99,255,0.12)";

// ── Shared stylesheets (reused by taskwidget & daily_task_widget) ─────

inline constexpr auto kBtnStyleSheet = R"(
QPushButton {
  color:#1A1A2E;
  background:#FFFFFF;
  border:1px solid #D1D5DB;
  border-radius:6px;
  padding:6px 12px;
  min-height:32px;
}
QPushButton:hover   { background:#F3F4F6; border-color:#9CA3AF; }
QPushButton:pressed { background:#E5E7EB; }
QPushButton:disabled { color:#B0B3BF; background:#F9FAFB; border-color:#E5E7EB; }
)";

inline constexpr auto kListStyleSheet = R"(
QListWidget {
  color:#1A1A2E;
  background:transparent;
  border:1px solid #D1D5DB;
  border-radius:6px;
  padding:6px;
}
QListWidget::item { padding:2px 0px; border-bottom:1px solid #E5E7EB; }
QListWidget::item:selected { background:#F3F4F6; }
)";

inline constexpr auto kNavBtnStyleSheet = R"(
QPushButton {
  font-weight:bold;
  color:#1A1A2E;
  background:#FFFFFF;
  border:1px solid #D1D5DB;
  border-radius:6px;
  padding:4px 10px;
  min-height:28px;
  min-width:36px;
}
QPushButton:hover   { background:#F3F4F6; border-color:#9CA3AF; }
QPushButton:pressed { background:#E5E7EB; }
QPushButton:disabled { color:#B0B3BF; background:#F9FAFB; border-color:#E5E7EB; }
)";

inline constexpr auto kDateEditStyleSheet = R"(
QDateEdit {
  font-weight:bold;
  color:#1A1A2E;
  background:#FFFFFF;
  border:1px solid #D1D5DB;
  border-radius:6px;
  padding:4px 8px;
}
)";

inline constexpr auto kComboStyleSheet = R"(
QComboBox {
  font-weight:bold;
  color:#1A1A2E;
  background:#FFFFFF;
  border:1px solid #D1D5DB;
  border-radius:6px;
  padding:4px 8px;
}
QComboBox:hover { border-color:#9CA3AF; }
QComboBox QAbstractItemView { background:#FFFFFF; selection-background-color:#F3F4F6; }
QComboBox::drop-down { width:20px; border:none; }
QComboBox::down-arrow { image:none; border-left:4px solid transparent;
 border-right:4px solid transparent; border-top:5px solid #7B7D8C;
 margin-right:4px; }
)";

inline constexpr auto kSplitterStyleSheet = R"(
QSplitter::handle { background:transparent; }
QSplitter::handle:vertical { height:10px; }
QSplitter::handle:horizontal { width:10px; }
QSplitter::handle:hover { background:rgba(108,99,255,0.12); border-radius:4px; }
)";

inline constexpr auto kTopBarStyleSheet = R"(
background:#FFFFFF; border-radius:10px;
)";

inline constexpr auto kPanelStyleSheet = R"(
QFrame { background:#FFFFFF; border:none; border-radius:10px; }
)";

} // namespace Theme
