/*
 *  Copyright© Florian Plesker <florian.plesker@web.de>
 */

#include "qlightterminal.h"

#include <QByteArray>
#include <QTextCursor>
#include <QPainter>
#include <QKeyEvent>
#include <QPoint>
#include <QLayout>
#include <QClipboard>
#include <QGuiApplication>
#include <QPointF>
#include <QFontMetricsF>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QXmlStreamReader>
#include <QDesktopServices>
#include <QRegularExpression>

QLightTerminal::QLightTerminal(QWidget *parent) : QWidget(parent), scrollbar(Qt::Orientation::Vertical),
    boxLayout(this), cursorTimer(this), selectionTimer(this),
    win{0, 0, 0, 0, 100, 10, 10, 1.25, 10, 8.42, 0, 8} {
    st = new SimpleTerminal();
    setAttribute(Qt::WA_StyledBackground, true);
    this->setFontSize(10, QFont::Normal);
    this->updateStyleSheet();

    boxLayout.setSpacing(0);
    boxLayout.setContentsMargins(0, 0, 0, 0);
    boxLayout.addWidget(&scrollbar);
    boxLayout.setAlignment(&scrollbar, Qt::AlignRight);

    connect(&scrollbar, &QScrollBar::valueChanged, this, &QLightTerminal::scrollX);

    win.viewPortHeight = win.height / win.lineheight;
    setupScrollbar();

    connect(st, &SimpleTerminal::s_error, this, [this](QString error) { emit s_error("Error from st: " + error); });
    connect(st, &SimpleTerminal::s_updateView, this, &QLightTerminal::updateTerminal);

    connect(&cursorTimer, &QTimer::timeout, this, [this]() {
        cursorVisible = !cursorVisible;
        update();
    });
    cursorTimer.start(750);

    connect(&selectionTimer, &QTimer::timeout, this, &QLightTerminal::updateSelection);
    connect(&resizeTimer, &QTimer::timeout, this, &QLightTerminal::resize);
    connect(st, &SimpleTerminal::s_closed, this, &QLightTerminal::close);
}

void QLightTerminal::close() {
    setDisabled(true);
    closed = true;
    scrollbar.setVisible(false);
    cursorTimer.stop();
    selectionTimer.stop();
    update();
    emit s_closed();
}

void QLightTerminal::updateTerminal(Term *) {
    cursorVisible = true;
    cursorTimer.start(750);
    if (st->term.histi != scrollbar.maximum()) {
        bool isMax = scrollbar.value() == scrollbar.maximum();
        scrollbar.setMaximum(st->term.histi * win.scrollMultiplier);
        if (isMax) scrollbar.setValue(scrollbar.maximum());
        scrollbar.setVisible(scrollbar.maximum() != 0);
    }
    update();
}

void QLightTerminal::scrollX(int n) {
    int scroll = (st->term.scr - (scrollbar.maximum() - scrollbar.value()) / win.scrollMultiplier);
    if (scroll < 0) st->kscrollup(-scroll);
    else st->kscrolldown(scroll);
    update();
}

void QLightTerminal::setFontSize(int size, int weight) {
    QFont mono = QFont(m_fontFamily, size, weight);
    mono.setFixedPitch(true);
    mono.setStyleHint(QFont::Monospace);
    setFont(mono);
    QFontMetricsF metric = QFontMetricsF(mono);
    win.lineheight = metric.lineSpacing() * win.lineHeightScale;
    win.fontSize = size;
    auto improvedRect = metric.boundingRect(metric.boundingRect("a"), 0, "a");
    win.charWith = improvedRect.width();
    win.charHeight = improvedRect.height();
    update();
}

void QLightTerminal::setBackground(QColor color) {
    colors[defaultBackground] = color;
    updateStyleSheet();
}

void QLightTerminal::setLineHeightScale(double scale) {
    QFontMetricsF metric = QFontMetricsF(font());
    win.lineheight = metric.lineSpacing() * scale;
    win.lineHeightScale = scale;
    update();
}

void QLightTerminal::setPadding(double vertical, double horizontal) {
    win.hPadding = horizontal;
    win.vPadding = vertical;
    update();
}

void QLightTerminal::zoomIn() { setFontSize(win.fontSize + 1); }
void QLightTerminal::zoomOut() { if (win.fontSize > 1) setFontSize(win.fontSize - 1); }
void QLightTerminal::resetZoom() { setFontSize(10); }

char* QLightTerminal::getSel() { return st->getsel(); }

void QLightTerminal::pasteText(const QString& text) {
    QByteArray d = text.toUtf8();
    st->ttywrite(d.constData(), d.size(), 1);
}

void QLightTerminal::selectAll() {
    st->selstart(0, 0, 0);
    st->selextend(st->term.col - 1, st->term.row - 1, SEL_REGULAR, 1);
    update();
}

void QLightTerminal::clearScreen() {
    st->ttywrite("\033[2J\033[H", 8, 1);
}


void QLightTerminal::loadColorScheme(const QString& path) {
    if (path.endsWith(".colorscheme"))
        loadKonsoleScheme(path);
    else if (path.endsWith(".itermcolors"))
        loadIterm2Scheme(path);
    else if (path.endsWith(".json"))
        loadVSCodeScheme(path);
    else if (path.contains("Xdefaults") || path.contains("Xresources"))
        loadXdefaultsScheme(path);
}

bool QLightTerminal::loadKonsoleScheme(const QString& path) {
    QSettings s(path, QSettings::IniFormat);
    auto load = [&](const char* group, int idx) {
        s.beginGroup(group);
        if (s.contains("Color")) {
            QStringList rgb = s.value("Color").toStringList();
            if (rgb.size() >= 3)
                colors[idx] = QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt());
        }
        s.endGroup();
    };
    load("Background", defaultBackground);
    load("Foreground", 258);
    for (int i = 0; i < 16; ++i)
        load(qPrintable(QString("Color%1").arg(i)), i);
    load("Color0Intense", 8);
    load("Color1Intense", 9);
    load("Color2Intense", 10);
    load("Color3Intense", 11);
    load("Color4Intense", 12);
    load("Color5Intense", 13);
    load("Color6Intense", 14);
    load("Color7Intense", 15);
    updateStyleSheet();
    update();
    return true;
}

bool QLightTerminal::loadWindowsTerminalScheme(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    QJsonObject obj = doc.object();
    struct { const char* key; int idx; } map[] = {
        {"background", defaultBackground}, {"foreground", 258},
        {"black", 0}, {"red", 1}, {"green", 2}, {"yellow", 3},
        {"blue", 4}, {"magenta", 5}, {"cyan", 6}, {"white", 7},
        {"brightBlack", 8}, {"brightRed", 9}, {"brightGreen", 10}, {"brightYellow", 11},
        {"brightBlue", 12}, {"brightMagenta", 13}, {"brightCyan", 14}, {"brightWhite", 15}
    };
    for (auto& m : map) {
        if (obj.contains(m.key))
            colors[m.idx] = QColor(obj[m.key].toString());
    }
    updateStyleSheet();
    update();
    return true;
}

bool QLightTerminal::loadVSCodeScheme(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    QJsonObject root = doc.object();
    QJsonObject colorsObj = root.value("colors").toObject();
    struct { const char* key; int idx; } map[] = {
        {"editor.background", defaultBackground}, {"terminal.foreground", 258},
        {"terminal.ansiBlack", 0}, {"terminal.ansiRed", 1},
        {"terminal.ansiGreen", 2}, {"terminal.ansiYellow", 3},
        {"terminal.ansiBlue", 4}, {"terminal.ansiMagenta", 5},
        {"terminal.ansiCyan", 6}, {"terminal.ansiWhite", 7},
        {"terminal.ansiBrightBlack", 8}, {"terminal.ansiBrightRed", 9},
        {"terminal.ansiBrightGreen", 10}, {"terminal.ansiBrightYellow", 11},
        {"terminal.ansiBrightBlue", 12}, {"terminal.ansiBrightMagenta", 13},
        {"terminal.ansiBrightCyan", 14}, {"terminal.ansiBrightWhite", 15}
    };
    bool loaded = false;
    for (auto& m : map) {
        if (colorsObj.contains(m.key)) {
            colors[m.idx] = QColor(colorsObj[m.key].toString());
            loaded = true;
        }
    }
    if (!loaded) return loadWindowsTerminalScheme(path);
    updateStyleSheet();
    update();
    return true;
}

bool QLightTerminal::loadIterm2Scheme(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QXmlStreamReader xml(&f);
    struct { const char* name; int idx; } map[] = {
        {"Ansi 0 Color", 0}, {"Ansi 1 Color", 1}, {"Ansi 2 Color", 2}, {"Ansi 3 Color", 3},
        {"Ansi 4 Color", 4}, {"Ansi 5 Color", 5}, {"Ansi 6 Color", 6}, {"Ansi 7 Color", 7},
        {"Ansi 8 Color", 8}, {"Ansi 9 Color", 9}, {"Ansi 10 Color", 10}, {"Ansi 11 Color", 11},
        {"Ansi 12 Color", 12}, {"Ansi 13 Color", 13}, {"Ansi 14 Color", 14}, {"Ansi 15 Color", 15},
        {"Background Color", defaultBackground}, {"Foreground Color", 258}
    };
    QString currentKey;
    int r = 0, g = 0, b = 0, component = -1;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == QStringLiteral("key"))
                currentKey = xml.readElementText();
            else if (xml.name() == QStringLiteral("real")) {
                double val = xml.readElementText().toDouble();
                if (component == 0) r = qBound(0, (int)(val * 255), 255);
                else if (component == 1) g = qBound(0, (int)(val * 255), 255);
                else if (component == 2) b = qBound(0, (int)(val * 255), 255);
                component++;
            }
        } else if (xml.isEndElement() && xml.name() == QStringLiteral("dict")) {
            for (auto& m : map) {
                if (currentKey == m.name) { colors[m.idx] = QColor(r, g, b); break; }
            }
            component = -1; r = g = b = 0;
        }
    }
    f.close();
    updateStyleSheet();
    update();
    return true;
}

bool QLightTerminal::loadXdefaultsScheme(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QString content = QString::fromUtf8(f.readAll());
    f.close();
    QRegularExpression re(R"(\*\.?(color[0-9]{1,2}|foreground|background):\s*(#[0-9a-fA-F]{6}))");
    auto it = re.globalMatch(content);
    while (it.hasNext()) {
        auto match = it.next();
        QString name = match.captured(1);
        QColor color(match.captured(2));
        if (name == "foreground") colors[258] = color;
        else if (name == "background") colors[defaultBackground] = color;
        else if (name.startsWith("color")) {
            int idx = name.mid(5).toInt();
            if (idx >= 0 && idx < 16) colors[idx] = color;
        }
    }
    updateStyleSheet();
    update();
    return true;
}


void QLightTerminal::buildContextMenu(QMenu* menu) {

    char* sel = st->getsel();
    bool hasSel = (sel != nullptr);
    if (sel) free(sel);

    bool hasClip = !QGuiApplication::clipboard()->text().isEmpty();

    QAction* copyAct = menu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy), "Copy");
    copyAct->setEnabled(hasSel);
    QObject::connect(copyAct, &QAction::triggered, [this]() {
        char* s = st->getsel();
        if (s) { QGuiApplication::clipboard()->setText(QString::fromUtf8(s)); free(s); }
    });

    QAction* pasteAct = menu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::EditPaste), "Paste");
    pasteAct->setEnabled(hasClip);
    QObject::connect(pasteAct, &QAction::triggered, [this]() {
        QByteArray data = QGuiApplication::clipboard()->text().toUtf8();
        st->ttywrite(data.constData(), data.size(), 1);
    });

    menu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::EditSelectAll), "Select All", [this]() { selectAll(); });
    menu->addSeparator();

    QAction* searchAct = menu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::EditFind), "Search Online");
    searchAct->setEnabled(hasSel);
    QObject::connect(searchAct, &QAction::triggered, [this]() {
        char* s = st->getsel();
        if (s) {
            QString q = QString::fromUtf8(s).trimmed().left(200);
            free(s);
            if (!q.isEmpty()) QDesktopServices::openUrl(QUrl("https://www.ecosia.org/search?q=" + q));
        }
    });

    menu->addSeparator();
    menu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::EditClear), "Clear", [this]() { clearScreen(); });
    menu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::ZoomIn), "Zoom In", [this]() { zoomIn(); });
    menu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::ZoomOut), "Zoom Out", [this]() { zoomOut(); });
    menu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::ZoomFitBest), "Reset Zoom", [this]() { resetZoom(); });
}



void QLightTerminal::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        QMenu menu;
        buildContextMenu(&menu);
        emit contextMenuRequested(&menu);
        menu.exec(event->globalPosition().toPoint());
        return;
    }
    setFocus();
    mouseDown = true;
    lastMousePos = event->pos();
    st->selclear();
    update();
    if (QDateTime::currentMSecsSinceEpoch() - lastClick < 500) {
        lastClick = 0;
        QPointF pos = event->position();
        int col = (pos.x() - win.hPadding) / win.charWith;
        int row = (pos.y() - win.vPadding) / win.lineheight;
        st->selstart(col, row, SNAP_LINE);
    }
    cursorVisible = true;
    update();
    cursorTimer.start(750);
}

void QLightTerminal::mouseReleaseEvent(QMouseEvent *event) {
    mouseDown = false;
    if (selectionStarted) {
        QPointF pos = event->position();
        int col = (pos.x() - win.hPadding) / win.charWith;
        int row = (pos.y() - win.vPadding) / win.lineheight;
        col = MIN(col, win.viewPortWidth - 1);
        row = MIN(row, win.viewPortHeight - 1);
        st->selextend(col, row, SEL_REGULAR, 1);
        selectionStarted = false;
        selectionTimer.stop();
        update();
    }
}

void QLightTerminal::mouseMoveEvent(QMouseEvent *event) {
    if (mouseDown) {
        lastMousePos = event->position();
        if (!selectionStarted) {
            QPointF pos = event->position();
            int col = (pos.x() - win.hPadding) / win.charWith;
            double row = (pos.y() - win.vPadding) / win.lineheight;
            if (row >= win.viewPortHeight) return;
            st->selstart(col, row, 0);
            selectionTimer.start(100);
            selectionStarted = true;
        }
    }
}

void QLightTerminal::mouseDoubleClickEvent(QMouseEvent *event) {
    QPointF pos = event->position();
    int col = (pos.x() - win.hPadding) / win.charWith;
    int row = (pos.y() - win.vPadding) / win.lineheight;
    if (row >= win.viewPortHeight || row < 0) return;
    if (col >= win.viewPortWidth || col < 0) return;
    st->selclear();
    st->selstart(col, row, SNAP_WORD);
    lastClick = QDateTime::currentMSecsSinceEpoch();
    update();
}

bool QLightTerminal::focusNextPrevChild(bool) { return false; }

void QLightTerminal::updateStyleSheet() {
    setStyleSheet("background-color:" + colors[defaultBackground].name() + ";");
    update();
}

void QLightTerminal::updateSelection() {
    if (selectionStarted) {
        int col = (lastMousePos.x() - win.hPadding) / win.charWith;
        double row = (lastMousePos.y() - win.vPadding) / win.lineheight;
        if (row < 0.4) {
            double scroll = MIN(scrollbar.value() / win.lineheight, (row - 0.4) * -2);
            scrollbar.setValue(scrollbar.value() - scroll * win.scrollMultiplier);
        }
        if (row > win.viewPortHeight - 0.4) {
            double scroll = MIN((scrollbar.maximum() - scrollbar.value()) / win.lineheight,
                                (row - win.viewPortHeight + 0.4) * 2);
            scrollbar.setValue(scrollbar.value() + scroll * win.scrollMultiplier);
        }
        col = MIN(col, win.viewPortWidth - 1);
        row = MIN(row, win.viewPortHeight - 1);
        st->selextend(col, row, SEL_REGULAR, 0);
        update();
    }
}

void QLightTerminal::resizeEvent(QResizeEvent *event) {
    if (closed) return;
    win.height = event->size().height();
    win.width = event->size().width();
    if (resizeTimer.isActive()) { resizeTimer.start(500); return; }
    resizeTimer.start(500);
    event->accept();
}

void QLightTerminal::resize() {
    resizeTimer.stop();
    int paddingBottom = 4;
    int rows = (win.height - win.vPadding * 2 - paddingBottom) / win.lineheight;
    int scrollbarPadding = 5;
    int cols = (win.width - 2 * win.hPadding - scrollbarPadding) / win.charWith;
    cols = MAX(1, cols);
    if (cols == win.viewPortWidth && rows == win.viewPortHeight) return;
    win.viewPortWidth = cols;
    win.viewPortHeight = rows;
    st->tresize(cols, win.viewPortHeight);
    st->ttyresize(cols * 8.5, win.viewPortHeight * win.lineheight);
}

void QLightTerminal::wheelEvent(QWheelEvent *event) {
    event->accept();
    QPoint numPixels = event->pixelDelta();
    if (!numPixels.isNull()) { scrollbar.setValue(scrollbar.value() - numPixels.y() * 2); return; }
    QPoint numDegrees = event->angleDelta();
    if (!numDegrees.isNull()) scrollbar.setValue(scrollbar.value() - numDegrees.y() * 2);
}

void QLightTerminal::focusOutEvent(QFocusEvent*) {
    cursorTimer.stop();
    cursorVisible = false;
    update();
}

void QLightTerminal::setupScrollbar() {
    scrollbar.setMaximum(0);
    scrollbar.setValue(0);
    scrollbar.setPageStep(100 * win.scrollMultiplier);
    scrollbar.setVisible(false);
    scrollbar.setStyleSheet(R"(
        QScrollBar::sub-line:vertical, QScrollBar::add-line:vertical { height: 0; }
        QScrollBar::vertical { margin: 0; width: 9px; padding-right: 1px; }
        QScrollBar::handle { background: #aaaaaa; border-radius: 4px; }
    )");
}


void QLightTerminal::keyPressEvent(QKeyEvent *e) {
    e->accept();
    QString input = e->text();
    Qt::KeyboardModifiers mods = e->modifiers();
    int key = e->key();

    if (key == Qt::Key_Backspace) {
        st->ttywrite(mods.testFlag(Qt::AltModifier) ? "\033\177" : "\177", mods.testFlag(Qt::AltModifier) ? 2 : 1, 1);
        return;
    }

    if (key == 86 && mods.testFlag(Qt::ControlModifier) && mods.testFlag(Qt::ShiftModifier)) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QByteArray data = clipboard->text().toLocal8Bit();
        st->ttywrite(data.data(), data.size(), 1);
        return;
    }

    if (key == 67 && mods.testFlag(Qt::ControlModifier) && mods.testFlag(Qt::ShiftModifier)) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        char* sel = st->getsel();
        if (sel) { clipboard->setText(QString::fromUtf8(sel)); free(sel); }
        return;
    }

    if (!input.isEmpty()) {
        QByteArray text;
        if (input.contains('\n'))
            text = input.left(input.indexOf('\n') + 1).toUtf8();
        else
            text = e->text().toUtf8();
        st->ttywrite(text, text.size(), 1);
    } else {
        int end = 24;
        if (key > keys[end].key || key < keys[0].key) return;
        for (int i = 0; i < end;) {
            int nextKey = keys[i].nextKey;
            if (key == keys[i].key) {
                for (int j = i; j < i + nextKey; j++) {
                    if (mods.testFlag(keys[j].mods)) {
                        st->ttywrite(keys[j].cmd, keys[j].cmd_size, 1);
                        return;
                    }
                }
            }
            i += nextKey;
        }
    }
}


void QLightTerminal::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setBackgroundMode(Qt::BGMode::OpaqueMode);

    if (closed) {
        painter.drawText(QPointF(win.hPadding, win.lineheight + win.vPadding), "Terminal is closed.");
        return;
    }

    QFont font;
    QString line;
    uint32_t fgColor = 0, bgColor = 0, cfgColor = 0, cbgColor = 0;
    ushort mode = -1;
    double offset;
    bool changed = false;

    int drawOffset = MAX(event->rect().y() - win.vPadding, 0) / win.lineheight;
    int drawHeight = (event->rect().height()) / win.lineheight;
    int drawEnd = drawOffset + drawHeight;
    int i = MIN(drawEnd, win.viewPortHeight);
    int stop = MAX(i - drawHeight, 0);
    double yPos = i * win.lineheight + win.vPadding;
    int temp;

    while (i > stop) {
        i--;
        offset = win.hPadding;
        line = QString();

        Glyph *tLine = ((i) < st->term.scr ? st->term.hist[((i) + st->term.histi - \
                                                                                   st->term.scr + HISTSIZE + 1) % HISTSIZE] : \
                                             st->term.line[(i) - st->term.scr]);

        for (int j = 0; j < st->term.col; j++) {
            Glyph g = tLine[j];
            if (g.mode == ATTR_WDUMMY) continue;

            if (cfgColor != g.fg) { fgColor = g.fg; cfgColor = g.fg; changed = true; }
            if (cbgColor != g.bg) { bgColor = g.bg; cbgColor = g.bg; changed = true; }
            if (st->selected(j, i)) g.mode ^= ATTR_REVERSE;

            if (mode != g.mode) {
                mode = g.mode; changed = true;
                fgColor = cfgColor; bgColor = cbgColor;

                if ((g.mode & ATTR_BOLD_FAINT) == ATTR_FAINT) painter.setOpacity(0.5);
                else painter.setOpacity(1);

                if (g.mode & ATTR_REVERSE) { temp = fgColor; fgColor = bgColor; bgColor = temp; }
                if (g.mode & ATTR_INVISIBLE) fgColor = bgColor;

                font = painter.font();
                if ((g.mode & ATTR_BOLD) != font.bold()) font.setBold(g.mode & ATTR_BOLD);
                if ((g.mode & ATTR_ITALIC) != font.italic()) font.setItalic(g.mode & ATTR_ITALIC);
                if ((g.mode & ATTR_UNDERLINE) != font.underline()) font.setUnderline(g.mode & ATTR_UNDERLINE);
                if ((g.mode & ATTR_STRUCK) != font.strikeOut()) font.setStrikeOut(g.mode & ATTR_STRUCK);
            }

            if (changed) {
                painter.drawText(QPointF(offset, yPos), line);
                offset += line.size() * win.charWith;

                if (IS_TRUECOL(fgColor))
                    painter.setPen(QColor(RED_FROM_TRUE(fgColor), GREEN_FROM_TRUE(fgColor), BLUE_FROM_TRUE(fgColor)));
                else
                    painter.setPen(colors[fgColor]);

                if (IS_TRUECOL(bgColor))
                    painter.setBackground(QBrush(QColor(RED_FROM_TRUE(bgColor), GREEN_FROM_TRUE(bgColor), BLUE_FROM_TRUE(bgColor))));
                else
                    painter.setBackground(QBrush(colors[bgColor]));

                line = QString();
                changed = false;
                painter.setFont(font);
            }

            if (0xffff < g.u) line += QStringView(QChar::fromUcs4(g.u));
            else line += QChar(g.u);
        }
        painter.drawText(QPointF(offset, yPos), line);
        yPos -= win.lineheight;
    }

    if (st->term.scr != 0) return;

    fgColor = st->term.c.attr.bg;
    if (IS_TRUECOL(fgColor))
        painter.setPen(QColor(RED_FROM_TRUE(fgColor), GREEN_FROM_TRUE(fgColor), BLUE_FROM_TRUE(fgColor)));
    else
        painter.setPen(colors[fgColor]);

    bgColor = st->term.c.attr.fg;
    if (IS_TRUECOL(bgColor))
        painter.setBackground(QBrush(QColor(RED_FROM_TRUE(bgColor), GREEN_FROM_TRUE(bgColor), BLUE_FROM_TRUE(bgColor))));
    else
        painter.setBackground(QBrush(colors[bgColor]));

    line = QString();
    for (int k = 0; k < st->term.c.x; k++) {
        auto rune = st->term.line[st->term.c.y][k].u;
        if (0xffff < rune) line += QStringView(QChar::fromUcs4(rune));
        else line += QChar(rune);
    }
    int cursorOffset = line.size() * win.charWith;
    double cursorPosVert = MIN(st->term.c.y + 1, win.viewPortHeight);
    auto cursorPos = QPointF(cursorOffset + win.hPadding, cursorPosVert * win.lineheight + win.vPadding);

    if (!cursorVisible) return;

    auto runeAtCursor = st->term.line[st->term.c.y][st->term.c.x].u;
    if (0xffff < runeAtCursor) painter.drawText(cursorPos, QString(QChar::fromUcs4(runeAtCursor)));
    else painter.drawText(cursorPos, QChar(runeAtCursor));
}
