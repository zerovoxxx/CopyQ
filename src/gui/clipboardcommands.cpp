// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardcommands.h"
#include "common/common.h"
#include "common/mimetypes.h"
#include "gui/commandedit.h"
#include "gui/windowgeometryguard.h"
#include "item/itemfactory.h"
#include "platform/platformclipboard.h"
#include "platform/platformnativeinterface.h"
#include "scriptable/scriptvaluefactory.h"

#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QQmlEngine>
#include <QSaveFile>
#include <QScreen>
#include <QVBoxLayout>
#include <algorithm>

namespace {
struct Field { const char *name; const char *label; const char *kind; const char *section; };
const Field commandFields[] = {
    {"name", QT_TRANSLATE_NOOP("ClipboardCommands", "Name"), "text", "General"},
    {"enable", QT_TRANSLATE_NOOP("ClipboardCommands", "Enabled"), "bool", "General"},
    {"icon", QT_TRANSLATE_NOOP("ClipboardCommands", "Icon character or file"), "text", "General"},
    {"inMenu", QT_TRANSLATE_NOOP("ClipboardCommands", "Item menu"), "bool", "General"},
    {"automatic", QT_TRANSLATE_NOOP("ClipboardCommands", "Automatic"), "bool", "General"},
    {"display", QT_TRANSLATE_NOOP("ClipboardCommands", "Display"), "bool", "General"},
    {"isGlobalShortcut", QT_TRANSLATE_NOOP("ClipboardCommands", "Global shortcut"), "bool", "General"},
    {"isScript", QT_TRANSLATE_NOOP("ClipboardCommands", "Script override"), "bool", "General"},
    {"re", QT_TRANSLATE_NOOP("ClipboardCommands", "Content regular expression"), "regex", "Conditions"},
    {"wndre", QT_TRANSLATE_NOOP("ClipboardCommands", "Window regular expression"), "regex", "Conditions"},
    {"matchCmd", QT_TRANSLATE_NOOP("ClipboardCommands", "Match command"), "code", "Conditions"},
    {"input", QT_TRANSLATE_NOOP("ClipboardCommands", "Input MIME type"), "text", "Conditions"},
    {"cmd", QT_TRANSLATE_NOOP("ClipboardCommands", "Command"), "code", "Command"},
    {"output", QT_TRANSLATE_NOOP("ClipboardCommands", "Output MIME type"), "text", "Command"},
    {"sep", QT_TRANSLATE_NOOP("ClipboardCommands", "Output separator"), "text", "Command"},
    {"tab", QT_TRANSLATE_NOOP("ClipboardCommands", "Copy to collection"), "text", "Command"},
    {"outputTab", QT_TRANSLATE_NOOP("ClipboardCommands", "Output collection"), "text", "Command"},
    {"wait", QT_TRANSLATE_NOOP("ClipboardCommands", "Show action dialog"), "bool", "Behavior"},
    {"transform", QT_TRANSLATE_NOOP("ClipboardCommands", "Transform item"), "bool", "Behavior"},
    {"remove", QT_TRANSLATE_NOOP("ClipboardCommands", "Remove matching items"), "bool", "Behavior"},
    {"hideWindow", QT_TRANSLATE_NOOP("ClipboardCommands", "Hide after activation"), "bool", "Behavior"},
    {"shortcuts", QT_TRANSLATE_NOOP("ClipboardCommands", "Menu shortcuts (one per line)"), "list", "Shortcuts"},
    {"globalShortcuts", QT_TRANSLATE_NOOP("ClipboardCommands", "Global shortcuts (one per line)"), "list", "Shortcuts"}
};
}

ClipboardCommands::ClipboardCommands(const ClipboardBrowserSharedPtr &sharedData)
    : m_sharedData(sharedData), m_commands(loadAllCommands()), m_saved(m_commands)
    , m_templates(sharedData->itemFactory->commands())
{
    setObjectName(QStringLiteral("clipboard_commands"));
    setTitle(tr("QClip — Commands"));
    setMinimumSize(QSize(760, 520));
    setResizeMode(QQuickView::SizeRootObjectToView);
    if (!m_commands.isEmpty())
        m_current = 0;
}

ClipboardCommands::~ClipboardCommands() { QQuickView::setSource(QUrl()); }

void ClipboardCommands::open()
{
    if (status() != QQuickView::Ready) {
        setInitialProperties({{QStringLiteral("controller"), QVariant::fromValue(this)}});
        QQuickView::setSource(QUrl(QStringLiteral("qrc:/copyq/QClip/gui/qml/Commands.qml")));
    }
    if (status() != QQuickView::Ready) {
        m_error = tr("Unable to load commands.");
        emit fieldsChanged();
        return;
    }
    if (!isVisible() && screen()) {
        const auto area = screen()->availableGeometry();
        const QSize size(qMin(1100, area.width()), qMin(760, area.height()));
        setGeometry(QRect(area.center() - QPoint(size.width()/2, size.height()/2), size));
    }
    show(); raise(); requestActivate();
}

QVariantList ClipboardCommands::commands() const
{
    QVariantList result;
    for (const auto &command : m_commands)
        result.append(QVariantMap{{QStringLiteral("name"), command.localizedName()},
            {QStringLiteral("enabled"), command.enable}, {QStringLiteral("type"), command.type()}});
    return result;
}

QVariantList ClipboardCommands::templates() const
{
    QVariantList result;
    for (const auto &command : m_templates)
        result.append(command.localizedName());
    return result;
}

QVariantList ClipboardCommands::fields() const
{
    if (m_current < 0 || m_current >= m_commands.size())
        return {};
    const auto &command = m_commands[m_current];
    auto object = ::toScriptValue(command, engine());
    QVariantList result;
    for (const auto &field : commandFields) {
        const auto name = QLatin1String(field.name);
        QVariant value = object.property(name).toVariant();
        if (name == QLatin1String("re")) value = command.re.pattern();
        if (name == QLatin1String("wndre")) value = command.wndre.pattern();
        result.append(QVariantMap{{QStringLiteral("name"), name}, {QStringLiteral("label"), tr(field.label)},
            {QStringLiteral("kind"), QLatin1String(field.kind)}, {QStringLiteral("section"), QLatin1String(field.section)},
            {QStringLiteral("value"), value}});
    }
    return result;
}

QVariantMap ClipboardCommands::theme() const { return m_sharedData->theme.quickTheme(); }

void ClipboardCommands::select(int row)
{
    if (row < -1 || row >= m_commands.size()) return;
    m_current = row;
    m_error.clear();
    emit fieldsChanged();
}

bool ClipboardCommands::setField(const QString &name, const QVariant &value)
{
    if (m_current < 0 || m_current >= m_commands.size()) return false;
    const auto field = std::find_if(std::begin(commandFields), std::end(commandFields),
        [&name](const Field &field) { return name == QLatin1String(field.name); });
    if (field == std::end(commandFields)) return false;
    auto object = ::toScriptValue(m_commands[m_current], engine());
    if (QLatin1String(field->kind) == QLatin1String("list")) {
        const auto values = value.typeId() == QMetaType::QStringList ? value.toStringList()
            : value.toString().split('\n', Qt::SkipEmptyParts);
        object.setProperty(name, ::toScriptValue(values, engine()));
    } else if (QLatin1String(field->kind) == QLatin1String("bool")) {
        object.setProperty(name, value.toBool());
    } else {
        object.setProperty(name, value.toString());
    }
    auto updated = ::fromScriptValue<Command>(object, engine());
    // Keep Qt regex options which JavaScript's RegExp cannot represent.
    updated.re = m_commands[m_current].re;
    updated.wndre = m_commands[m_current].wndre;
    if (name == QLatin1String("re")) updated.re.setPattern(value.toString());
    if (name == QLatin1String("wndre")) updated.wndre.setPattern(value.toString());
    m_commands[m_current] = updated;
    m_error.clear();
    emit commandsChanged();
    return true;
}

void ClipboardCommands::create() { Command command; command.name = tr("New command"); addCommands({command}); }
void ClipboardCommands::addTemplate(int row) { if (row >= 0 && row < m_templates.size()) addCommands({m_templates[row]}); }
void ClipboardCommands::addCommands(const Commands &commands)
{
    if (commands.isEmpty()) return;
    m_current = m_commands.size();
    m_commands.append(commands);
    emit commandsChanged(); emit fieldsChanged();
}

void ClipboardCommands::remove(const QList<int> &rows)
{
    auto sorted = rows;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    for (int row : sorted) if (row >= 0 && row < m_commands.size()) m_commands.removeAt(row);
    m_current = qMin(m_current, int(m_commands.size()) - 1);
    emit commandsChanged(); emit fieldsChanged();
}

void ClipboardCommands::move(int row, int step)
{
    const int destination = row + step;
    if (row < 0 || row >= m_commands.size() || destination < 0 || destination >= m_commands.size()) return;
    m_commands.move(row, destination);
    m_current = destination;
    emit commandsChanged(); emit fieldsChanged();
}

Commands ClipboardCommands::selectedCommands(const QList<int> &rows) const
{
    Commands result;
    for (int row = 0; row < m_commands.size(); ++row)
        if (rows.contains(row)) result.append(m_commands[row]);
    return result;
}
QString ClipboardCommands::exportSelected(const QList<int> &rows) const { return exportCommands(selectedCommands(rows)); }

bool ClipboardCommands::importText(const QString &text)
{
    const auto commands = importCommandsFromText(text);
    if (commands.isEmpty()) { m_error = tr("No commands found in this text."); emit fieldsChanged(); return false; }
    addCommands(commands);
    return true;
}

void ClipboardCommands::importFile()
{
    const auto file = QFileDialog::getOpenFileName(nullptr, tr("Import commands"), QString(), tr("Commands (*.ini);;All files (*)"));
    if (!file.isEmpty()) {
        const auto commands = importCommandsFromFile(file);
        if (commands.isEmpty()) { m_error = tr("No commands found in this file."); emit fieldsChanged(); }
        else addCommands(commands);
    }
}

void ClipboardCommands::exportFile(const QList<int> &rows)
{
    const auto path = QFileDialog::getSaveFileName(nullptr, tr("Export commands"), QString(), tr("Commands (*.ini);;All files (*)"));
    if (path.isEmpty()) return;
    QSaveFile file(path);
    const auto text = exportSelected(rows).toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(text) != text.size() || !file.commit()) {
        m_error = tr("Unable to save commands: %1").arg(file.errorString()); emit fieldsChanged();
    }
}

void ClipboardCommands::copy(const QList<int> &rows) { emit clipboardRequested({{mimeText, exportSelected(rows).toUtf8()}}); }
void ClipboardCommands::paste()
{
    const auto clipboard = platformNativeInterface()->clipboard();
    const auto data = clipboard->mimeData(ClipboardMode::Clipboard);
    if (data) importText(data->text());
}

void ClipboardCommands::editCode(const QString &field)
{
    if (m_current < 0 || (field != QLatin1String("cmd") && field != QLatin1String("matchCmd"))) return;
    const int row = m_current;
    auto dialog = new QDialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->setWindowTitle(tr("QClip — Edit command"));
    dialog->resize(960, 700);
    auto layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->addWidget(new QLabel(tr("Script editor — completion and syntax highlighting"), dialog));
    auto edit = new CommandEdit(dialog);
    edit->setCommand(field == QLatin1String("cmd") ? m_commands[row].cmd : m_commands[row].matchCmd);
    layout->addWidget(edit, 1);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, dialog, [this, dialog, edit, field, row]() {
        if (!CommandEdit::scriptError(edit->command()).isEmpty()) return;
        if (m_current == row) { setField(field, edit->command()); emit fieldsChanged(); }
        dialog->accept();
    });
    connect(this, &QObject::destroyed, dialog, &QWidget::close);
    m_sharedData->theme.decorateMainWindow(dialog);
    dialog->show(); raiseWindow(dialog); edit->setFocus();
}

bool ClipboardCommands::apply(bool close)
{
    m_error.clear();
    for (int row = 0; row < m_commands.size(); ++row) {
        const auto &command = m_commands[row];
        if (!command.enable) continue;
        m_error = !command.re.isValid() ? command.re.errorString()
            : !command.wndre.isValid() ? command.wndre.errorString()
            : CommandEdit::scriptError(command.cmd);
        if (m_error.isEmpty()) m_error = CommandEdit::scriptError(command.matchCmd);
        if (!m_error.isEmpty()) { m_current = row; emit fieldsChanged(); return false; }
    }
    saveCommands(m_commands);
    m_saved = m_commands;
    emit commandsSaved(); emit commandsChanged(); emit fieldsChanged();
    if (close) { hide(); emit finished(); }
    return true;
}

void ClipboardCommands::cancel() { hide(); emit finished(); }
bool ClipboardCommands::maybeClose(QWidget *parent)
{
    if (modified()) {
        const auto answer = QMessageBox::question(parent, tr("Unsaved commands"), tr("Save changes to commands?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (answer == QMessageBox::Cancel || (answer == QMessageBox::Save && !apply())) return false;
    }
    cancel(); return true;
}

bool ClipboardCommands::event(QEvent *event)
{
    if (event->type() == QEvent::Close && !maybeClose(nullptr)) {
        static_cast<QCloseEvent*>(event)->ignore();
        return true;
    }
    return ClipboardWindow::event(event);
}
