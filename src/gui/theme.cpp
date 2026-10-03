// SPDX-License-Identifier: GPL-3.0-or-later

#include "theme.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

#include "ui_configtabappearance.h"

#include "common/config.h"
#include "common/log.h"
#include "gui/iconfont.h"
#include "platform/platformnativeinterface.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QListView>
#include <QScreen>
#include <QSettings>
#include <QStyleFactory>

#if QT_VERSION >= QT_VERSION_CHECK(6,8,0)
#   include <QStyleHints>
#endif

#include <cmath>

namespace {

/// Up to this value of background lightness, window title style would be light.
const int lightThreshold = 100;

const QLatin1String defaultColorVarBase("default_bg");
const QLatin1String defaultColorVarText("default_text");
const QLatin1String defaultColorVarPlaceholderText("default_placeholder_text");
const QLatin1String defaultColorVarAlternateBase("default_alt_bg");
const QLatin1String defaultColorVarHighlight("default_highlight_bg");
const QLatin1String defaultColorVarHighlightText("default_highlight_text");
const QLatin1String defaultColorVarToolTipBase("default_tooltip_bg");
const QLatin1String defaultColorVarToolTipText("default_tooltip_text");
const QLatin1String defaultColorVarWindow("default_window");
const QLatin1String defaultColorVarWindowText("default_window_text");
const QLatin1String defaultColorVarButton("default_button");
const QLatin1String defaultColorVarButtonText("default_button_text");
const QLatin1String defaultColorVarBrightText("default_bright_text");
const QLatin1String defaultColorVarLight("default_light");
const QLatin1String defaultColorVarMidlight("default_midlight");
const QLatin1String defaultColorVarDark("default_dark");
const QLatin1String defaultColorVarMid("default_mid");
const QLatin1String defaultColorVarShadow("default_shadow");
const QLatin1String defaultColorLink("default_link");
const QLatin1String defaultColorLinkVisited("default_link_visited");

QPalette::ColorRole defaultColorVarToRole(const QString &varName)
{
    static QHash<QString, QPalette::ColorRole> map = {
        {defaultColorVarBase, QPalette::Base},
        {defaultColorVarText, QPalette::Text},
        {defaultColorVarPlaceholderText, QPalette::PlaceholderText},
        {defaultColorVarAlternateBase, QPalette::AlternateBase},
        {defaultColorVarHighlight, QPalette::Highlight},
        {defaultColorVarHighlightText, QPalette::HighlightedText},
        {defaultColorVarToolTipBase, QPalette::ToolTipBase},
        {defaultColorVarToolTipText, QPalette::ToolTipText},
        {defaultColorVarWindow, QPalette::Window},
        {defaultColorVarWindowText, QPalette::WindowText},
        {defaultColorVarButton, QPalette::Button},
        {defaultColorVarButtonText, QPalette::ButtonText},
        {defaultColorVarBrightText, QPalette::BrightText},
        {defaultColorVarLight, QPalette::Light},
        {defaultColorVarMidlight, QPalette::Midlight},
        {defaultColorVarDark, QPalette::Dark},
        {defaultColorVarMid, QPalette::Mid},
        {defaultColorVarShadow, QPalette::Shadow},
        {defaultColorLink, QPalette::Link},
        {defaultColorLinkVisited, QPalette::LinkVisited},
    };
    const auto it = map.find(varName);
    return (it == std::end(map)) ? QPalette::NoRole : it.value();
}

double normalizeFactor(double value)
{
    return qBound( 0.0, value, 1.0 );
}

int normalizeColorValue(int value)
{
    return qBound(0, value, 255);
}

/// Add RGB components properly.
int addColor(int c1, float multiply, int c2)
{
    const float f1 = static_cast<float>(c1);
    const float f2 = static_cast<float>(c2);
    return multiply > 0.0f
            ? static_cast<int>( std::sqrt(f1*f1 + multiply * f2*f2) )
            : c1 + static_cast<int>(multiply * f2);
}

void addColor(
        const QString &color, float multiply, int *r, int *g, int *b, int *a,
        const Theme &theme, const Values &values, int maxRecursion)
{
    if (color.isEmpty())
        return;

    QColor toAdd;
    float x = multiply;

    if (color.at(0).isDigit()) {
        bool ok;
        x = multiply * color.toFloat(&ok);
        if (!ok)
            return;
        toAdd = QColor(1, 1, 1);
    } else if ( color.startsWith('#') || color.startsWith("rgba(") ) {
        toAdd = deserializeColor(color);
        if ( !toAdd.isValid() )
            toAdd = QColor(Qt::black);
    } else if ( color.startsWith(QStringLiteral("default_")) ) {
        const QPalette::ColorRole role = defaultColorVarToRole(color);
        toAdd = QPalette().color(role);
    } else if (QColor col(color); col.isValid()) {
        toAdd = col;
    } else if (maxRecursion > 0) {
        const QVariant v = values.contains(color)
                ? values.value(color) : theme.value(color);
        toAdd = evalColor(v.toString(), theme, values, maxRecursion - 1);
    }

    *r = normalizeColorValue( addColor(*r, x, toAdd.red()) );
    *g = normalizeColorValue( addColor(*g, x, toAdd.green()) );
    *b = normalizeColorValue( addColor(*b, x, toAdd.blue()) );
    if (multiply > 0.0f) {
        const float fa = static_cast<float>(*a);
        const float fad = static_cast<float>(toAdd.alpha());
        *a = normalizeColorValue( static_cast<int>(fa + x * fad) );
    }
}

int fitFontWeight(int weight, int low, int high, int highCss)
{
    const int diffCss = 100;
    const int lowCss = highCss - diffCss;
    if (weight == low)
        return lowCss;

    const int diff = high - low;
    return lowCss + diffCss * weight / diff;
}

int getFontWeightForStyleSheet(int weight)
{
    if (weight < QFont::ExtraLight)
        return fitFontWeight(weight, QFont::Thin, QFont::ExtraLight, 200);
    if (weight < QFont::Light)
        return fitFontWeight(weight, QFont::ExtraLight, QFont::Light, 300);
    if (weight < QFont::Normal)
        return fitFontWeight(weight, QFont::Light, QFont::Normal, 400);
    if (weight < QFont::Medium)
        return fitFontWeight(weight, QFont::Normal, QFont::Medium, 500);
    if (weight < QFont::DemiBold)
        return fitFontWeight(weight, QFont::Medium, QFont::DemiBold, 600);
    if (weight < QFont::Bold)
        return fitFontWeight(weight, QFont::DemiBold, QFont::Bold, 700);
    if (weight < QFont::ExtraBold)
        return fitFontWeight(weight, QFont::Bold, QFont::ExtraBold, 800);
    if (weight < QFont::Black)
        return fitFontWeight(weight, QFont::ExtraBold, QFont::Black, 900);

    return 900;
}

QString getFontStyleSheet(const QString &fontString, double scale = 1.0)
{
    QFont font;
    if (!fontString.isEmpty())
        font.fromString(fontString);

    qreal size = font.pointSizeF() * scale;
    QString sizeUnits = "pt";
    if (size < 0.0) {
        size = font.pixelSize() * scale;
        sizeUnits = "px";
    }
    size = qBound(0.1, size, 50.0);

    QString result;
    result.append( QString::fromLatin1(";font-family: \"%1\"").arg(font.family()) );
    result.append( QString::fromLatin1(";font:%1 %2 %3%4")
                   .arg(font.style() == QFont::StyleItalic
                        ? "italic" : font.style() == QFont::StyleOblique ? "oblique" : "normal",
                        font.bold() ? "bold" : "")
                   .arg(size)
                   .arg(sizeUnits)
                   );
    result.append( QString::fromLatin1(";text-decoration:%1 %2 %3")
                   .arg(font.strikeOut() ? "line-through" : "",
                        font.underline() ? "underline" : "",
                        font.overline() ? "overline" : "")
                   );

    const int w = getFontWeightForStyleSheet( font.weight() );
    result.append( QString::fromLatin1(";font-weight:%1").arg(w) );

    result.append(";");

    return result;
}

QString themePrefix()
{
#ifdef COPYQ_THEME_PREFIX
    return adjustedInstallPath(QStringLiteral(COPYQ_THEME_PREFIX));
#else
    return platformNativeInterface()->themePrefix();
#endif
}

double getFactor(const QString &name, const QHash<QString, QVariant> &values)
{
    bool ok;
    const double scale = values.value(name).toDouble(&ok);
    return ok ? scale : 1.0;
}

} // namespace

Theme::Theme(const QSettings &settings)
{
    loadTheme(settings);
}

Theme::Theme(Ui::ConfigTabAppearance *ui)
    : ui(ui)
{
}

void Theme::loadTheme(const QSettings &settings)
{
    resetTheme();

    for ( auto key = m_theme.keyBegin(); key != m_theme.keyEnd(); ++key ) {
        const auto value = settings.value(*key);
        if ( value.isValid() )
            m_theme[*key].setValue(value);
    }

    updateTheme();
}

void Theme::saveTheme(QSettings *settings) const
{
    QStringList keys = m_theme.keys();
    keys.sort();

    for (const auto &key : keys)
        settings->setValue( key, value(key) );
}

QVariant Theme::value(const QString &name) const
{
    return m_theme.value(name).value();
}

QColor Theme::color(const QString &name) const
{
    return evalColor( value(name).toString(), *this );
}

QFont Theme::font(const QString &name) const
{
    return themeFontFromString( value(name).toString() );
}

QColor Theme::evalColorExpression(const QString &expr) const
{
    return evalColor( expr, *this );
}

void Theme::decorateBrowser(QListView *c) const
{
    QAbstractScrollArea *c2 = c;
    decorateBrowser(c2);

    bool ok;
    const int itemSpacing = value("item_spacing").toInt(&ok);
    c->setSpacing( ok ? itemSpacing : c->fontMetrics().lineSpacing() / 6 );
}

void Theme::decorateMainWindow(QWidget *mainWindow) const
{
    QPalette palette = QApplication::palette();

    // This seems to properly reset icon colors.
    mainWindow->setStyleSheet(QString());
    mainWindow->setPalette(palette);

    const auto bg = color("bg");

#if QT_VERSION >= QT_VERSION_CHECK(6,8,0)
    // Change window dark/light title scheme if supported.
    QStyleHints *styleHints = QGuiApplication::styleHints();
    if ( !isMainWindowThemeEnabled() )
        styleHints->unsetColorScheme();
    else if (bg.lightness() < lightThreshold)
        styleHints->setColorScheme(Qt::ColorScheme::Dark);
    else
        styleHints->setColorScheme(Qt::ColorScheme::Light);
#endif

    const auto quick = quickTheme();
    if (!quick.value(QStringLiteral("custom_style")).toBool()) {
        const auto surface = quick.value(QStringLiteral("bg")).value<QColor>();
        const auto foreground = quick.value(QStringLiteral("fg")).value<QColor>();
        const auto accent = quick.value(QStringLiteral("sel_bg")).value<QColor>();
        const bool dark = surface.lightnessF() < 0.5;
        const auto field = dark ? QStringLiteral("#30323a") : QStringLiteral("#ffffff");
        const auto border = dark ? QStringLiteral("#454750") : QStringLiteral("#dfe2e9");
        palette.setColor(QPalette::Window, surface);
        palette.setColor(QPalette::WindowText, foreground);
        palette.setColor(QPalette::Base, QColor(field));
        palette.setColor(QPalette::Text, foreground);
        palette.setColor(QPalette::ButtonText, foreground);
        palette.setColor(QPalette::Highlight, accent);
        palette.setColor(QPalette::HighlightedText, Qt::white);
        mainWindow->setPalette(palette);
        mainWindow->setStyleSheet(QStringLiteral(
            "QPushButton, QToolButton { color: %1; background: %2; border: 1px solid %3; border-radius: 6px; padding: 4px 10px; }"
            "QPushButton:hover, QToolButton:hover { border-color: %4; }"
            "QPushButton:default { background: %4; color: white; border-color: %4; }"
            "QLineEdit, QComboBox, QSpinBox, QPlainTextEdit, QTextEdit { color: %1; background: %2; border: 1px solid %3; border-radius: 6px; padding: 4px; }"
            "QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus { border-color: %4; }"
            "QGroupBox { border: 1px solid %3; border-radius: 8px; margin-top: 12px; padding-top: 12px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 12px; }"
            "QTabWidget::pane { border: 1px solid %3; border-radius: 8px; }"
            "QTabBar::tab { color: %1; padding: 5px 10px; border: 0; }"
            "QTabBar::tab:selected { color: %4; }"
            "QScrollBar:vertical { background: transparent; width: 8px; margin: 2px; }"
            "QScrollBar::handle:vertical { background: %3; border-radius: 3px; min-height: 24px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }")
            .arg(foreground.name(), field, border, accent.name()));
        return;
    }
    if ( !isMainWindowThemeEnabled() ) {
        const QString cssTemplate = QStringLiteral("main_window_simple");
        mainWindow->setStyleSheet(getStyleSheet(cssTemplate));
        return;
    }

    const auto fg = color("fg");
    palette.setColor( QPalette::Base, bg );
    palette.setColor( QPalette::AlternateBase, color("alt_bg") );
    palette.setColor( QPalette::Text, fg );
    palette.setColor( QPalette::Window, bg );
    palette.setColor( QPalette::WindowText, fg );
    palette.setColor( QPalette::Button, bg );
    palette.setColor( QPalette::ButtonText, fg );
    palette.setColor( QPalette::Highlight, color("sel_bg") );
    palette.setColor( QPalette::HighlightedText, color("sel_fg") );

    mainWindow->setPalette(palette);
    const QString cssTemplate = value("css_template_main_window").toString();
    mainWindow->setStyleSheet(getStyleSheet(cssTemplate));
}

void Theme::decorateScrollArea(QAbstractScrollArea *scrollArea) const
{
    const auto scrollbarPolicy = this->scrollbarPolicy();
    scrollArea->setVerticalScrollBarPolicy(scrollbarPolicy);
    scrollArea->setHorizontalScrollBarPolicy(scrollbarPolicy);
}

QVariantMap Theme::quickTheme() const
{
    if (m_theme.isEmpty()) {
        Theme defaults;
        defaults.resetTheme();
        defaults.updateTheme();
        return defaults.quickTheme();
    }
    QVariantMap result;
    for (const auto name : {"bg", "fg", "alt_bg", "sel_bg", "sel_fg", "edit_bg", "edit_fg",
            "find_bg", "find_fg", "notes_bg", "notes_fg"})
        result.insert(QLatin1String(name), color(QLatin1String(name)));
    const bool dark = QApplication::palette().color(QPalette::Window).lightnessF() < 0.5;
    for (const auto &entry : QList<QPair<QString, QString>>{
            {QStringLiteral("bg"), dark ? QStringLiteral("#25272d") : QStringLiteral("#f4f5f7")},
            {QStringLiteral("fg"), dark ? QStringLiteral("#f0f1f5") : QStringLiteral("#252832")},
            {QStringLiteral("alt_bg"), dark ? QStringLiteral("#34363e") : QStringLiteral("#e9ebf0")},
            {QStringLiteral("sel_bg"), QStringLiteral("#3975ed")}}) {
        if (value(entry.first).toString().startsWith(QLatin1String("default_")))
            result.insert(entry.first, QColor(entry.second));
    }
    if (value(QStringLiteral("find_fg")).toString() == QLatin1String("black"))
        result.insert(QStringLiteral("find_fg"), result.value(QStringLiteral("fg")));
    result.insert(QStringLiteral("custom_style"), isCustomStyleEnabled());
    result.insert(QStringLiteral("font"), font(QStringLiteral("font")));
    result.insert(QStringLiteral("edit_font"), editorFont());
    result.insert(QStringLiteral("find_font"), searchFont());
    result.insert(QStringLiteral("num_font"), rowNumberFont());
    result.insert(QStringLiteral("num_fg"), color(QStringLiteral("num_fg")));
    result.insert(QStringLiteral("show_number"), showRowNumber());
    result.insert(QStringLiteral("show_scrollbars"), value(QStringLiteral("show_scrollbars")));
    for (const auto name : {"quick_spacing", "quick_margin", "quick_row_height", "quick_radius"})
        result.insert(QLatin1String(name), value(QLatin1String(name)));
    result.insert(QStringLiteral("main_css"), getStyleSheet(isMainWindowThemeEnabled()
        ? value(QStringLiteral("css_template_main_window")).toString() : QStringLiteral("main_window_simple")));
    result.insert(QStringLiteral("items_css"), getStyleSheet(value(QStringLiteral("css_template_items")).toString()));
    result.insert(QStringLiteral("menu_css"), getMenuStyleSheet());
    return result;
}

void Theme::decorateDialog(QWidget *dialog, const QString &title, const QString &description) const
{
    decorateMainWindow(dialog);
    auto layout = dialog->layout();
    if (!layout || dialog->findChild<QWidget *>(QStringLiteral("dialog_heading"))) return;
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);
    auto header = new QWidget(dialog);
    header->setObjectName(QStringLiteral("dialog_heading"));
    auto headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 8);
    auto heading = new QLabel(title, header);
    auto headingFont = font(QStringLiteral("font"));
    headingFont.setBold(true);
    headingFont.setPointSizeF(qMax(14.0, headingFont.pointSizeF() + 4));
    heading->setFont(headingFont);
    headerLayout->addWidget(heading);
    auto detail = new QLabel(description, header);
    detail->setWordWrap(true);
    headerLayout->addWidget(detail);
    if (auto box = qobject_cast<QBoxLayout *>(layout)) box->insertWidget(0, header);
    else if (auto grid = qobject_cast<QGridLayout *>(layout)) {
        struct Cell { QLayoutItem *item; int row; int column; int rows; int columns; };
        QList<Cell> cells;
        while (grid->count()) {
            Cell cell{};
            grid->getItemPosition(0, &cell.row, &cell.column, &cell.rows, &cell.columns);
            cell.item = grid->takeAt(0);
            cells.append(cell);
        }
        for (const auto &cell : cells)
            grid->addItem(cell.item, cell.row + 1, cell.column, cell.rows, cell.columns);
        grid->addWidget(header, 0, 0, 1, qMax(1, grid->columnCount()));
    }
}

void Theme::decorateItemPreview(QAbstractScrollArea *itemPreview) const
{
    decorateBrowser(itemPreview);
}

QString Theme::getMenuStyleSheet() const
{
    if (!isCustomStyleEnabled()) {
        const bool dark = color(QStringLiteral("bg")).lightnessF() < 0.5;
        return QStringLiteral(
            "QMenu { background: %1; color: %2; border: 1px solid %3; border-radius: 10px; padding: 6px; }"
            "QMenu::item { padding: 8px 28px; border-radius: 6px; }"
            "QMenu::item:selected { background: %4; color: white; }"
            "QMenu::item:disabled { color: %3; }"
            "QMenu::separator { height: 1px; background: %3; margin: 5px 8px; }")
            .arg(dark ? QStringLiteral("#30323a") : QStringLiteral("#ffffff"),
                color(QStringLiteral("fg")).name(), dark ? QStringLiteral("#454750") : QStringLiteral("#dfe2e9"),
                color(QStringLiteral("sel_bg")).name());
    }
    const QString cssTemplate = value("css_template_menu").toString();
    return getStyleSheet(cssTemplate);
}

bool Theme::isCustomStyleEnabled() const
{
    if (isMainWindowThemeEnabled()) return true;
    Theme defaults;
    defaults.resetTheme();
    for (auto key = m_theme.keyBegin(); key != m_theme.keyEnd(); ++key) {
        if ((key->endsWith(QLatin1String("_css")) || key->startsWith(QLatin1String("css"))
                || key->startsWith(QLatin1String("search_bar"))) && value(*key) != defaults.value(*key))
            return true;
    }
    return false;
}

QString Theme::getNotificationStyleSheet() const
{
    const QString cssTemplate = value("css_template_notification").toString();
    return getStyleSheet(cssTemplate);
}

Qt::ScrollBarPolicy Theme::scrollbarPolicy() const
{
    return value("show_scrollbars").toBool()
            ? Qt::ScrollBarAsNeeded
            : Qt::ScrollBarAlwaysOff;
}

bool Theme::useSystemIcons() const
{
    return value("use_system_icons").toBool();
}

QFont Theme::themeFontFromString(const QString &fontString) const
{
    QFont font;
    if ( !fontString.isEmpty() )
        font.fromString(fontString);
    if ( !isAntialiasingEnabled() )
        font.setStyleStrategy(QFont::NoAntialias);
    return font;
}

bool Theme::isAntialiasingEnabled() const
{
    return m_antialiasing;
}

void Theme::resetTheme()
{
    m_theme["bg"]        = Option(defaultColorVarBase, "VALUE", ui ? ui->pushButtonColorBg : nullptr);
    m_theme["edit_bg"]   = Option(defaultColorVarBase, "VALUE", ui ? ui->pushButtonColorEditorBg : nullptr);
    m_theme["fg"]        = Option(defaultColorVarText, "VALUE", ui ? ui->pushButtonColorFg : nullptr);
    m_theme["edit_fg"]   = Option(defaultColorVarText, "VALUE", ui ? ui->pushButtonColorEditorFg : nullptr);
    m_theme["num_fg"]    = Option(defaultColorVarPlaceholderText, "VALUE", ui ? ui->pushButtonColorNumberFg : nullptr);
    m_theme["alt_bg"]    = Option(defaultColorVarAlternateBase, "VALUE", ui ? ui->pushButtonColorAltBg : nullptr);
    m_theme["sel_bg"]    = Option(defaultColorVarHighlight, "VALUE", ui ? ui->pushButtonColorSelBg : nullptr);
    m_theme["sel_fg"]    = Option(defaultColorVarHighlightText, "VALUE", ui ? ui->pushButtonColorSelFg : nullptr);
    m_theme["find_bg"]   = Option("yellow", "VALUE", ui ? ui->pushButtonColorFoundBg : nullptr);
    m_theme["find_fg"]   = Option("black", "VALUE", ui ? ui->pushButtonColorFoundFg : nullptr);
    m_theme["notes_bg"]  = Option(defaultColorVarToolTipBase, "VALUE", ui ? ui->pushButtonColorNotesBg : nullptr);
    m_theme["notes_fg"]  = Option(defaultColorVarToolTipText, "VALUE", ui ? ui->pushButtonColorNotesFg : nullptr);
    m_theme["notification_bg"]  = Option("#333", "VALUE", ui ? ui->pushButtonColorNotificationBg : nullptr);
    m_theme["notification_fg"]  = Option("#ddd", "VALUE", ui ? ui->pushButtonColorNotificationFg : nullptr);

    m_theme["font"]        = Option("", "VALUE", ui ? ui->pushButtonFont : nullptr);
    m_theme["edit_font"]   = Option("", "VALUE", ui ? ui->pushButtonEditorFont : nullptr);
    m_theme["find_font"]   = Option("", "VALUE", ui ? ui->pushButtonFoundFont : nullptr);
    m_theme["num_font"]    = Option("", "VALUE", ui ? ui->pushButtonNumberFont : nullptr);
    m_theme["notes_font"]  = Option("", "VALUE", ui ? ui->pushButtonNotesFont : nullptr);
    m_theme["notification_font"]  = Option("", "VALUE", ui ? ui->pushButtonNotificationFont : nullptr);
    m_theme["show_number"] = Option(true, "checked", ui ? ui->checkBoxShowNumber : nullptr);
    m_theme["show_scrollbars"] = Option(true, "checked", ui ? ui->checkBoxScrollbars : nullptr);

    m_theme["css"] = Option("");
    m_theme["menu_css"] = Option(
                "\n    ;border: 1px solid ${sel_bg}"
                "\n    ;background: ${bg}"
                "\n    ;color: ${fg}"
                );
    m_theme["menu_bar_css"] = Option(
                "\n    ;background: ${bg}"
                "\n    ;color: ${fg}"
                );
    m_theme["menu_bar_selected_css"] = Option(
                "\n    ;background: ${sel_bg}"
                "\n    ;color: ${sel_fg}"
                );
    m_theme["menu_bar_disabled_css"] = Option(
                "\n    ;color: ${bg - #666}"
                );

    m_theme["item_css"] = Option("");
    m_theme["alt_item_css"] = Option("");
    m_theme["sel_item_css"] = Option("");
    m_theme["hover_item_css"] = Option("");
    m_theme["cur_item_css"] = Option(
                "\n    ;border: 0.1em solid ${sel_bg}"
                );
    m_theme["item_spacing"] = Option("");
    m_theme["notes_css"] = Option("");

    m_theme["tab_bar_css"] = Option(
                "\n    ;background: ${bg - #222}"
                );
    m_theme["tab_bar_tab_selected_css"] = Option(
                "\n    ;padding: 0.5em"
                "\n    ;background: ${bg}"
                "\n    ;border: 0.05em solid ${bg}"
                "\n    ;color: ${fg}"
                );
    m_theme["tab_bar_tab_unselected_css"] = Option(
                "\n    ;border: 0.05em solid ${bg}"
                "\n    ;padding: 0.5em"
                "\n    ;background: ${bg - #222}"
                "\n    ;color: ${fg - #333}"
                );
    m_theme["tab_bar_scroll_buttons_css"] = Option(
                "\n    ;background: ${bg - #222}"
                "\n    ;color: ${fg}"
                "\n    ;border: 0"
                );
    m_theme["tab_bar_item_counter"] = Option(
                "\n    ;color: ${fg - #044 + #400}"
                "\n    ;font-size: 6pt"
                );
    m_theme["tab_bar_sel_item_counter"] = Option(
                "\n    ;color: ${sel_bg - #044 + #400}"
                );

    m_theme["tab_tree_css"] = Option(
                "\n    ;color: ${fg}"
                "\n    ;background-color: ${bg}"
                );
    m_theme["tab_tree_sel_item_css"] = Option(
                "\n    ;color: ${sel_fg}"
                "\n    ;background-color: ${sel_bg}"
                "\n    ;border-radius: 2px"
                );
    m_theme["tab_tree_item_counter"] = Option(
                "\n    ;color: ${fg - #044 + #400}"
                "\n    ;font-size: 6pt"
                );
    m_theme["tab_tree_sel_item_counter"] = Option(
                "\n    ;color: ${sel_fg - #044 + #400}"
                );

    m_theme["tool_bar_css"] = Option(
                "\n    ;color: ${fg}"
                "\n    ;background-color: ${bg}"
                "\n    ;border: 0"
                );
    m_theme["tool_button_css"] = Option(
                "\n    ;color: ${fg}"
                "\n    ;background: ${bg}"
                "\n    ;border: 0"
                "\n    ;border-radius: 2px"
                );
    m_theme["tool_button_selected_css"] = Option(
                "\n    ;background: ${sel_bg - #222}"
                "\n    ;color: ${sel_fg}"
                "\n    ;border: 1px solid ${sel_bg}"
                );
    m_theme["tool_button_pressed_css"] = Option(
                "\n    ;background: ${sel_bg}"
                );

    m_theme["search_bar"] = Option(
                "\n    ;background: ${edit_bg}"
                "\n    ;color: ${edit_fg}"
                "\n    ;border: 1px solid ${alt_bg}"
                "\n    ;margin: 2px"
                );
    m_theme["search_bar_focused"] = Option(
                "\n    ;border: 1px solid ${sel_bg}"
                );

    m_theme["use_system_icons"] = Option(false, "checked", ui ? ui->checkBoxSystemIcons : nullptr);
    m_theme["font_antialiasing"] = Option(true, "checked", ui ? ui->checkBoxAntialias : nullptr);
    m_theme["style_main_window"] = Option(false, "checked", ui ? ui->checkBoxStyleMainWindow : nullptr);
    for (const auto &field : QList<QPair<QString,int>>{
            {QStringLiteral("quick_spacing"), 8}, {QStringLiteral("quick_margin"), 12},
            {QStringLiteral("quick_row_height"), 48}, {QStringLiteral("quick_radius"), 8}})
        m_theme[field.first] = Option(field.second, "value",
            ui ? ui->scrollAreaThemeContents->findChild<QSpinBox *>(field.first) : nullptr);

    const int iconSize = iconFontSizePixels();
    m_theme["icon_size"] = Option( QString::number(iconSize) );

    m_theme["css_template_items"] = Option("items");
    m_theme["css_template_main_window"] = Option("main_window");
    m_theme["css_template_notification"] = Option("notification");
    m_theme["css_template_menu"] = Option("menu");

    m_theme["num_margin"] = Option(2);
    m_theme["num_sel_fg"] = Option(QString());
}

void Theme::updateTheme()
{
    m_margins = QSize(2, 2);

    // search style
    m_searchPalette.setColor(QPalette::Base, color("find_bg"));
    m_searchPalette.setColor(QPalette::Text, color("find_fg"));
    m_searchFont = font("find_font");

    // editor style
    m_editorPalette.setColor(QPalette::Base, color("edit_bg"));
    m_editorPalette.setColor(QPalette::Text, color("edit_fg"));
    m_editorFont = font("edit_font");

    // number style
    m_showRowNumber = value("show_number").toBool();
    m_rowNumberPalette.setColor(QPalette::Text, color("num_fg"));
    const QString numSelFgValue = value("num_sel_fg").toString();
    const QColor numSelFg =
        numSelFgValue.isEmpty() ? color("sel_fg") : evalColor(numSelFgValue, *this);
    m_rowNumberPalette.setColor(QPalette::HighlightedText, numSelFg);
    m_rowNumberFont = font("num_font");
    m_rowNumberFontMetrics = QFontMetrics(m_rowNumberFont);
    const QVariant rowNumberMargin = value("num_margin");
    m_rowNumberMargin = rowNumberMargin.canConvert<int>() ? rowNumberMargin.toInt() : 2;

    m_antialiasing = value("font_antialiasing").toBool();
}

QSize Theme::rowNumberSize(int n) const
{
    if (!m_showRowNumber)
        return QSize(0, 0);

    const auto number = QString::number(n + m_rowIndexFromOne);
    return m_rowNumberFontMetrics.boundingRect(number).size()
        + m_margins + QSize(m_rowNumberMargin, 0);
}

void Theme::decorateBrowser(QAbstractScrollArea *c) const
{
    decorateScrollArea(c);
    const QString cssTemplate = value("css_template_items").toString();
    c->setStyleSheet(getStyleSheet(cssTemplate));
}

bool Theme::isMainWindowThemeEnabled() const
{
    return m_theme.value("style_main_window").value().toBool();
}

QString Theme::themeColorString(const QString &name) const
{
    return serializeColor( color(name) );
}

QString Theme::getStyleSheet(const QString &name, Values values, int maxRecursion) const
{
    const QString fileName = findThemeFile(name + ".css");

    if ( fileName.isEmpty() )
        return QString();

    QFile file(fileName);
    if ( !file.open(QIODevice::ReadOnly) ) {
        log( QString("Failed to open stylesheet \"%1\": %2")
             .arg(fileName, file.errorString()), LogError );
        return QString();
    }

    const auto css = QString::fromUtf8( file.readAll() );
    return parseStyleSheet(css, values, maxRecursion - 1);
}

QString Theme::parseStyleSheet(const QString &css, Values values, int maxRecursion) const
{
    QString output;
    const QString variableBegin("${");
    const QString variableEnd("}");
    for ( int i = 0; i < css.size(); ++i ) {
        const int a = css.indexOf(variableBegin, i);
        if (a == -1) {
            output.append(css.mid(i));
            break;
        }

        const int b = css.indexOf(variableEnd, a + variableBegin.size());
        if (b == -1) {
            output.append(css.mid(i));
            break;
        }

        output.append(css.mid(i, a - i));
        i = b + variableEnd.size() - 1;

        const QString name = css
                .mid(a + variableBegin.size(), b - a - variableBegin.size())
                .trimmed();
        const QString result = parsePlaceholder(name, &values, maxRecursion);
        output.append(result);
    }

    return output;
}

QString Theme::parsePlaceholder(const QString &name, Values *values, int maxRecursion) const
{
    if (maxRecursion < 0) {
        log("Max recursion reached when parsing style sheet", LogError);
        return QString();
    }

    if ( name.contains("=") ) {
        const int i = name.indexOf('=');
        const QString k = name.mid(0, i).trimmed();
        const QString v = name.mid(i + 1).trimmed();
        if (k == "scale")
            (*values)[k] = 1.0;
        (*values)[k] = parsePlaceholder(v, values, maxRecursion - 1);
        return QString();
    }

    if ( name.startsWith("css:") )
        return getStyleSheet(name.mid(4), *values, maxRecursion - 1);

    const double scale = getFactor("scale", *values);

    const QString value = this->value(name).toString();
    if (name.contains("font"))
        return getFontStyleSheet(value, scale);

    if (name.contains("css") || value.contains(":"))
        return parseStyleSheet(value, *values, maxRecursion - 1);

    {
        bool ok;
        const double v = name.toDouble(&ok);
        if (ok)
            return QString::number(v * scale);
    }

    {
        bool ok;
        const double v = value.toDouble(&ok);
        if (ok)
            return QString::number(v * scale);
    }

    QColor color = evalColor(name, *this, *values);
    const double saturationF = getFactor("hsv_saturation_factor", *values);
    const double valueF = getFactor("hsv_value_factor", *values);
    if ( !qFuzzyCompare(saturationF, 1.0) || !qFuzzyCompare(valueF, 1.0) ) {
        color.setHsvF(
            color.hueF(),
            normalizeFactor(color.saturationF() * saturationF),
            normalizeFactor(color.valueF() * valueF),
            color.alphaF()
        );
    }
    return serializeColor(color);
}

QString serializeColor(const QColor &color)
{
    if (color.alpha() == 255)
        return color.name();

    return QString::fromLatin1("rgba(%1,%2,%3,%4)")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(color.alpha());
}

QColor deserializeColor(const QString &colorName)
{
    QColor color;

    if ( colorName.startsWith(QStringLiteral("rgba(")) ) {
        QStringList list = colorName.mid(5, colorName.indexOf(')') - 5).split(',');
        int r = list.value(0).toInt();
        int g = list.value(1).toInt();
        int b = list.value(2).toInt();
        int a = static_cast<int>( list.value(3).toDouble() * 255 );

        color = QColor(r, g, b, a > 255 ? a / 255 : a);
    } else if ( colorName.startsWith(QStringLiteral("default_")) ) {
        const QPalette::ColorRole role = defaultColorVarToRole(colorName);
        color = QPalette().color(role);
    } else {
        color = QColor(colorName);
    }

    if ( !color.isValid() )
        log( QString("Failed to deserialize color \"%1\"").arg(colorName) );

    return color;
}

QColor evalColor(const QString &expression, const Theme &theme, const Values &values, int maxRecursion)
{
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;

    QStringList addList = QString(expression).remove(' ').split('+');
    for (const auto &add : addList) {
        QStringList subList = add.split('-');
        float multiply = 1;
        for (const auto &sub : subList) {
            addColor(sub, multiply, &r, &g, &b, &a, theme, values, maxRecursion);
            multiply = -1;
        }
    }

    return QColor(r, g, b, a);
}

QString defaultUserThemePath()
{
    return QDir::cleanPath(settingsDirectoryPath() + "/themes");
}

QStringList themePaths()
{
    QStringList paths;

    const QString customThemsPath = qEnvironmentVariable("COPYQ_THEME_PREFIX");
    if ( !customThemsPath.isEmpty() )
        paths.append(customThemsPath);

    const QString userThemesPath = defaultUserThemePath();
    QDir themesDir(userThemesPath);
    if ( themesDir.mkpath(".") )
        paths.append(userThemesPath);

    const QString themesPath = themePrefix();
    if ( !themesPath.isEmpty() )
        paths.append(themesPath);

    return paths;
}

QString findThemeFile(const QString &fileName)
{
    for (const QString &path : themePaths()) {
        const QDir dir(path);
        if ( dir.exists(fileName) )
            return dir.absoluteFilePath(fileName);
    }
    return QString();
}
