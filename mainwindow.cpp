#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <cmath>
#include <QMessageBox>
#include <QJSEngine>
#include <QJSValue>
#include <QKeyEvent>
#include <QTimer>
#include <QKeyEvent>
#include <QSettings>
#include <QList>
#include <QVariant>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    this->setStyleSheet(R"(
QWidget {
  background-color: #0F0F14;
  color: #EAEAF0;
  font-family: "Inter", "Segoe UI", sans-serif;
  font-size: 18px;
}

QLabel#LevelTitle {
  font-size: 40px;
  font-weight: 600;
  letter-spacing: 1px;
}

QLineEdit#EquationField {
  background: #17171F;
  border: 2px solid #1F1F2A;
  border-radius: 16px;
  padding: 14px 20px;
  font-family: "JetBrains Mono", monospace;
  font-size: 24px;
  selection-background-color: #8B5CF6;
  selection-color: #0F0F14;
}
QLineEdit#EquationField:focus {
  border-color: #8B5CF6;
  box-shadow: 0 0 0 4px rgba(139,92,246,0.25);
}

QPushButton[class~="Pad"] {
  background: #17171F;
  border: 1px solid #262636;
  border-radius: 16px;
  padding: 14px;
  min-width: 84px;
  min-height: 72px;
  transition: all 120ms ease;
}
QPushButton[class~="Pad"]:hover {
  border-color: #3B3B55;
}
QPushButton[class~="Pad"]:pressed {
  background: #13131A;
  transform: translateY(1px);
}

QPushButton[class~="PadAccent"] {
  border-color: rgba(139,92,246,0.6);
  color: #EAEAF0;
}

QPushButton#CheckBtn {
  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
    stop:0 #8B5CF6, stop:1 #EC4899);
  border: none;
  border-radius: 20px;
  padding: 18px 28px;
  font-size: 22px;
  font-weight: 700;
  letter-spacing: 0.5px;
}
QPushButton#CheckBtn:hover {
  filter: brightness(1.05);
}
QPushButton#CheckBtn:pressed {
  filter: brightness(0.95);
}

QPushButton#Restart, QPushButton#RestartThis {
  background: #17171F;
  border: 1px solid #262636;
  border-radius: 12px;
  padding: 12px 14px;
}
QPushButton#Restart:hover, QPushButton#RestartThis:hover {
  border-color: #8B5CF6;
}

QStatusBar {
  background: #17171F;
  color: #A3A3B0;
  border-top: 1px solid #262636;
  font-size: 16px;
  padding: 6px 12px;
}
)");

    QSettings settings("Kacherga", "FourFoursGame");
    target = settings.value("target", 1).toInt(); // если нет сохранения, по умолчанию 9
    expr = QString("4_4_4_4=%1").arg(target);
    cursorPos = expr.length();
    updateUI();

    ui->EquationField->installEventFilter(this);
    keyToButton[Qt::Key_Plus]      = ui->btnPlus;
    keyToButton[Qt::Key_Minus]     = ui->btnMinus;
    keyToButton[Qt::Key_Asterisk]  = ui->btnMul;
    keyToButton[Qt::Key_Slash]     = ui->btnDiv;

    keyToButton[Qt::Key_Q]         = ui->btnPlus;
    keyToButton[Qt::Key_W]         = ui->btnMinus;
    keyToButton[Qt::Key_E]         = ui->btnMul;
    keyToButton[Qt::Key_R]         = ui->btnDiv;
    keyToButton[Qt::Key_T]         = ui->btnSCOB1;
    keyToButton[Qt::Key_A]         = ui->btnSqrt;
    keyToButton[Qt::Key_S]         = ui->btnFAC;
    keyToButton[Qt::Key_D]         = ui->btnDUBFAC;
    keyToButton[Qt::Key_F]         = ui->btnSUBFAC;
    keyToButton[Qt::Key_G]         = ui->btnSCOB2;

    keyToButton[Qt::Key_Enter]     = ui->CheckBtn;
    keyToButton[Qt::Key_Return]    = ui->CheckBtn;

    keyToButton[Qt::Key_Backspace] = ui->btnBack;

    backspaceTimer = new QTimer(this);
    backspaceTimer->setInterval(150); // интервал повторения в мс
    connect(backspaceTimer, &QTimer::timeout, this, [=](){
        backsp(); // функция удаления символа
    });

    ui->EquationField->installEventFilter(this);

    // === Инициализация ---
    expr = QString("4_4_4_4=%1").arg(target);
    cursorPos = expr.length();
    updateUI();

    // --- Подключение кнопок к слотам ---
    // Арифметические операции
    connect(ui->btnPlus,  &QPushButton::clicked, this, [=](){ insertBinaryOp('+'); });
    connect(ui->btnMinus, &QPushButton::clicked, this, [=](){ insertBinaryOp('-'); });
    connect(ui->btnMul,   &QPushButton::clicked, this, [=](){ insertBinaryOp('*'); });
    connect(ui->btnDiv,   &QPushButton::clicked, this, [=](){ insertBinaryOp('/'); });

    // Квадратный корень и факториал
    connect(ui->btnSqrt,   &QPushButton::clicked, this, [=](){ insertUnaryOp(1); });
    connect(ui->btnFAC,    &QPushButton::clicked, this, [=](){ insertUnaryOp(2); });
    connect(ui->btnSUBFAC, &QPushButton::clicked, this, [=](){ insertUnaryOp(3); });
    connect(ui->btnDUBFAC, &QPushButton::clicked, this, [=](){ insertUnaryOp(4); });

    // Скобки
    connect(ui->btnSCOB1, &QPushButton::clicked, this, [=](){ addBracket('('); });
    connect(ui->btnSCOB2, &QPushButton::clicked, this, [=](){ addBracket(')'); });

    connect(ui->btnBack, &QPushButton::clicked, this, [=](){ backsp(); });
    connect(ui->Restart, &QPushButton::clicked, this, [=](){ replace(1); });
    connect(ui->RestartThis, &QPushButton::clicked, this, [=](){ replace(2); });

    // Проверка результата
    connect(ui->CheckBtn, &QPushButton::clicked, this, &MainWindow::checkResult);

    // Отслеживание позиции курсора
    connect(ui->EquationField, &QLineEdit::cursorPositionChanged, this,
            [=](int oldPos, int newPos){ cursorPos = newPos;});

    QList<QPushButton*> buttons = this->findChildren<QPushButton*>();
    for (auto* button : buttons) {
        button->setFocusPolicy(Qt::NoFocus);
    }

    const QList<QPushButton*> padButtons = {
        ui->btnPlus, ui->btnMinus, ui->btnMul, ui->btnDiv,
        ui->btnSqrt, ui->btnFAC, ui->btnDUBFAC, ui->btnSUBFAC,
        ui->btnSCOB1, ui->btnSCOB2, ui->btnBack
    };
    for (auto* button : padButtons) {
        button->setProperty("class", QStringLiteral("Pad"));
    }

    const QList<QPushButton*> accentButtons = {
        ui->btnMul, ui->btnDiv,
        ui->btnFAC, ui->btnDUBFAC, ui->btnSUBFAC,
        ui->btnSCOB1, ui->btnSCOB2
    };
    for (auto* button : accentButtons) {
        button->setProperty("class", QStringLiteral("Pad PadAccent"));
    }
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QSettings settings("Kacherga", "FourFoursGame");
    settings.setValue("target", target);  // сохраняем текущий уровень
    QMainWindow::closeEvent(event);       // обязательно вызываем базовый метод
}

void MainWindow::replace(int a){
    if (a == 1) target = 1;
    expr = QString("4_4_4_4=%1").arg(target);
    cursorPos = expr.length();
    updateUI();
}

void MainWindow::showErrorHint(const QString &text, int time) {
    ui->statusbar->setStyleSheet(
        "QStatusBar {"
        "   background-color: #17171F;"
        "   color: #EF4444;"
        "   border-top: 1px solid #8B5CF6;"
        "   font-weight: 600;"
        "   font-size: 16px;"
        "   padding: 6px 12px;"
        "}"
        );

    ui->statusbar->showMessage(text);

    QTimer::singleShot(time, this, [this]() {
        // возвращаем обычный стиль через 3 секунды
        ui->statusbar->setStyleSheet(
            "QStatusBar {"
            "   background-color: #17171F;"
            "   color: #A3A3B0;"
            "   border-top: 1px solid #262636;"
            "   font-size: 16px;"
            "   font-weight: 500;"
            "   padding: 6px 12px;"
            "}"
            );
        ui->statusbar->clearMessage();
    });
}

void MainWindow::backsp() {
    int ind = cursorPos - 1;
    if (cursorPos <=0 || cursorPos > expr.indexOf('=') || expr[ind] == '4' || expr[ind] == '_') {
        return;
    }

    if (expr[ind] == '+' ||  expr[ind] == '-' || expr[ind] == '/' || expr[ind] == '*') {
        expr[ind] = '_';
    } else if (expr[ind] == '(') {
        int b = 1;
        for (int i = ind + 1; i < expr.indexOf('='); i++) {
            if (expr[i] == '(') b++;
            if (expr[i] == ')') b--;
            if (b == 0) {
                if (expr[i+1] == '!') {
                    if (expr[i+2] == '!') {
                        expr.remove(ind, 1);
                        expr.remove(i-1, 3);
                        cursorPos --;
                    } else {
                        expr.remove(ind, 1);
                        expr.remove(i-1, 2);
                        cursorPos --;
                    }
                } else if (expr[ind-1] == '!' || expr[ind-1] == "√") {
                    expr.remove(ind-1, 2);
                    expr.remove(i-2, 1);
                    cursorPos -= 2;
                } else {
                    expr.remove(ind, 1);
                    expr.remove(i-1, 1);
                    cursorPos--;
                }
                updateUI();
                return;
            }
        }
        expr.remove(ind, 1);
        cursorPos--;
    } else if (expr[ind] == ')') {
        int b = 1;
        for (int i = ind - 1; i >= 0; i--) {
            if (expr[i] == '(') b--;
            if (expr[i] == ')') b++;
            if (b == 0) {
                if (expr[i-1] == '!' || expr [i-1] == "√") {
                    expr.remove(ind, 1);
                    expr.remove(i, 1);
                    expr.remove(i-1, 1);
                    cursorPos -= 3;
                } else if (expr[ind+1] == '!') {
                    if (expr[ind+2] == '!') {
                        expr.remove(ind, 3);
                        expr.remove(i, 1);
                        cursorPos -= 2;
                    } else {
                        expr.remove(ind, 2);
                        expr.remove(i, 1);
                        cursorPos -= 2;
                    }
                } else {
                    expr.remove(ind, 1);
                    expr.remove(i, 1);
                    cursorPos -= 2;
                }
                updateUI();
                return;
            }
        }
        expr.remove(ind, 1);
        cursorPos--;
    } else if (expr[ind] == '!' && expr[ind-1] == '!') {
        expr.remove(ind - 1, 2);
        cursorPos -= 2;
    } else {
        expr.remove(ind, 1);
        cursorPos--;
    }
    updateUI();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();

        // Игнорируем авто-повтор (кроме стрелок)
        if (keyEvent->isAutoRepeat() && key != Qt::Key_Left && key != Qt::Key_Right)
            return true;

        // === Backspace ===
        if (key == Qt::Key_Backspace) {
            if (!keyEvent->isAutoRepeat()) {
                backsp();            // сразу удаляем
                backspaceTimer->start(); // запускаем повторение
            }
            QPushButton* btn = keyToButton[key];

            // Сохраняем стиль
            QString normalStyle = btn->styleSheet();

            // Визуальный эффект нажатия
            btn->setDown(true);
            btn->setStyleSheet(
                "background-color: #13131A;"
                "border: 1px solid #3B3B55;"
                "border-radius: 16px;"
                "color: #EAEAF0;"
                );
            btn->setProperty("normalStyle", normalStyle);

            btn->click();
            return true;
            return true;
        }

        // === Пробел блокируем ===
        if (key == Qt::Key_Space)
            return true;

        // === Клавиши, связанные с кнопками ===
        if (keyToButton.contains(key)) {
            QPushButton* btn = keyToButton[key];

            // Сохраняем стиль
            QString normalStyle = btn->styleSheet();

            // Визуальный эффект нажатия
            btn->setDown(true);
            btn->setStyleSheet(
                "background-color: #13131A;"
                "border: 1px solid #3B3B55;"
                "border-radius: 16px;"
                "color: #EAEAF0;"
                );
            btn->setProperty("normalStyle", normalStyle);

            btn->click();
            return true;
        }

        // === Разрешаем только стрелки и Home/End ===
        if (obj == ui->EquationField) {
            switch (key) {
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_Home:
            case Qt::Key_End:
                return QMainWindow::eventFilter(obj, event);
            default:
                return true; // блокируем все остальные клавиши
            }
        }
    }
    else if (event->type() == QEvent::KeyRelease) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();

        if (keyEvent->isAutoRepeat())
            return true;

        // === Остановка Backspace ===
        if (key == Qt::Key_Backspace) {
            backspaceTimer->stop();
            QPushButton* btn = keyToButton[key];
            btn->setDown(false);
            if (btn->property("normalStyle").isValid()) {
                btn->setStyleSheet(btn->property("normalStyle").toString());
                btn->setProperty("normalStyle", QVariant());
            }
            return true;
            return true;
        }

        // === Отпускание кнопок ===
        if (keyToButton.contains(key)) {
            QPushButton* btn = keyToButton[key];
            btn->setDown(false);
            if (btn->property("normalStyle").isValid()) {
                btn->setStyleSheet(btn->property("normalStyle").toString());
                btn->setProperty("normalStyle", QVariant());
            }
            return true;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}








// --- Обновление интерфейса ---
void MainWindow::updateUI() {
    ui->LevelTitle->setText(QString("LEVEL %1").arg(target));
    ui->EquationField->blockSignals(true);
    ui->EquationField->setText(expr);
    ui->EquationField->blockSignals(false);
    ui->EquationField->setCursorPosition(cursorPos);
}

// --- Добавление скобок ---
void MainWindow::addBracket(char brac) {
    if((cursorPos > expr.length()-2 || cursorPos == 0) && brac == ')') return;
    if(cursorPos > expr.length()-3 && brac == '(') return;

    // Подсчитываем общий баланс скобок по всем значениям
    int totalOpen = expr.count('(') - expr.count(')');
    if ((brac == '(' && totalOpen >= 1) || (brac == ')' && totalOpen <= -1) ||
        (brac == '(' && (expr.left(cursorPos).count(')') - expr.left(cursorPos).count('(')) > 0) ||
        (brac == ')' && (expr.mid(cursorPos).count('(') - expr.mid(cursorPos).count(')')))) {
        showErrorHint("Сначала закрой предыдущие скобки!", 3000);
        return;
    }

    if (brac == '(') {
        int ind = cursorPos;
        if (expr[ind] == ')' || expr[ind] == '+' || expr[ind] == '-' || expr[ind] == '*' || expr[ind] == '/' || expr[ind] == '_' ||
            (expr[ind] == '!' && expr[ind+1] != '4' && expr[ind+1] != '(')) {
            return;
        }
    } else {
        int ind = cursorPos - 1;
        if (expr[ind] == '(' || expr[ind] == '+' || expr[ind] == '-' || expr[ind] == '*' || expr[ind] == '/' || expr[ind] == '_' || expr[ind] == "√" ||
            (expr[ind] == '!' && expr[ind-1] != '4' && expr[ind-1] != ')' && expr[ind-1] != '!')) {
            return;
        }
    }
    expr.insert(cursorPos, brac);
    cursorPos++;

    updateUI();
}

// --- Применение квадратного корня или факториала ---
void MainWindow::insertUnaryOp(int type) {

    int totalOpen = expr.count('(') - expr.count(')');
    if (abs(totalOpen) >= 1) {
        showErrorHint("⚠ Сначала закрой предыдущие скобки!", 3000);
        return;
    }

    switch (type) {
    case 1:
        applyUnaryOperation1("√", "");
        break;
    case 2:
        applyUnaryOperation1("", "!");
        break;
    case 3:
        applyUnaryOperation1("!", "");
        break;
    case 4:
        applyUnaryOperation1("", "!!");
        break;
    default:
        break;
    }

    updateUI();
}

void MainWindow::applyUnaryOperation1(const QString &start, const QString &end) {
    if (cursorPos < 0 || cursorPos > expr.length()) {
        showErrorHint("⚠ Нельзя ставить сюда этот знак!", 3000);
        return;
    }

    QString before = expr.left(cursorPos);
    QString after  = expr.mid(cursorPos);

    // Префиксные операции (√ и субфакториал !)
    if (start == "√" || start == "!") {
        if(before.back() == "√" || before.back() == "!") return;
        if (!after.isEmpty() && after[0] == '4') {
            if (after[1] == '!') {
                if (after[2] == '!') {
                    expr = before + start + '(' + after.left(3) + ')' + after.mid(3);
                } else {
                    expr = before + start + '(' + after.left(2) + ')' + after.mid(2);
                }
            } else {
                expr = before + start + after;
            }
            cursorPos += start.length();
        } else if (!after.isEmpty() && (after[0] == '(' || ((after[0] == "√" || after[0] == '!') && after[1] == '('))) {
            // Найти конец скобочного блока
            int b = 1;
            int i = (after[0] == "√" || after[0] == '!') ? 2:1;
            while (i < after.length() && b > 0) {
                if (after[i] == '(') b++;
                else if (after[i] == ')') b--;
                i++;
            }

            // Проверяем постфиксные операции после блока
            int j = i;
            while (j < after.length() && (after[j] == '!' )) j++;

            // Оборачиваем всё вместе
            if (j == i && after[0] != "√" && after[0] != '!') {
                expr = before + start + after;
            } else {
                expr = before + start + '(' + after.left(j) + ')' + after.mid(j);
            }
            cursorPos += start.length(); // после start + '('
        } else if (after[0] == "√" || after[0] == '!') {
            expr = before + start + '(' + after.left(2) + ')' + after.mid(2);
            cursorPos += start.length();
        }
    } else {
        if (after[0] == '!') return;
        QString endbef = before.right(2);
        int len = before.length() - 1;
        if (!before.isEmpty() && before.back() == '4') {
            if (before[len-1] == "√" || before[len-1] == "!") {
                expr = before.left(len-1) + '(' + before.mid(len-1) + ')' + end + after;
            } else {
                expr = before + end + after;
            }
            cursorPos += end.length();
        } else if (!before.isEmpty() && (before.back() == ')' || (before.back() == '!' && (before[len-1] == ')' || (endbef == "!!" && before[len-2] == ')'))))) {
            int b = 1;
            int i = (before.back() == '!') ? ((endbef == "!!") ? (len - 3):(len - 2)):len-1;
            while (i >= 0 && b > 0) {
                if (before[i] == ')') b++;
                else if (before[i] == '(') b--;
                i--;
            }

            int j = i;
            while (j >= 0 && (before[j] == '!' || before[j] == "√")) j--;

            if (j == i && before.back() != '!' && endbef != "!!") {
                expr = before + end + after;
                cursorPos += end.length();
            } else {
                expr = before.left(j+1) + '(' + before.mid(j+1) + ')' + end + after;
                cursorPos += end.length() + 2;
            }

        } else if (before.back() == '!' && (before[len-2] == ')' || before[len-3] == ')' || before[len-2] == '4' || before[len-3] == '4')) {
            if (endbef == "!!") {
                expr = before.left(len-3) + '(' + before.mid(len-3) + ')' + end + after;
            } else {
                expr = before.left(len-1) + '(' + before.mid(len-1) + ')' + end + after;
            }
            cursorPos += end.length() + 2;
        }
    }

    updateUI();
}




// --- Установка арифметической операции ---
void MainWindow::insertBinaryOp(char op) {
    int totalOpen = expr.count('(') - expr.count(')');
    if (abs(totalOpen) >= 1) {
        showErrorHint("Сначала закрой предыдущие скобки!", 3000);
        return;
    }

    if(expr[cursorPos] == '+' || expr[cursorPos] == '-' || expr[cursorPos] == '*' || expr[cursorPos] == '/' || expr[cursorPos] == '_') {
        expr[cursorPos] = op;
        cursorPos++;
    } else if (expr[cursorPos - 1] == '+' || expr[cursorPos - 1] == '-' || expr[cursorPos - 1] == '*' || expr[cursorPos - 1] == '/' || expr[cursorPos - 1] == '_') {
        expr[cursorPos - 1] = op;
    } else {
        showErrorHint("Нельзя ставить операцию сюда", 3000);
        return;
    }

    updateUI();
}



int MainWindow::calculateResult() {
    expr = expr.left(expr.indexOf('='));
    // ==============================
    // Обработка квадратного корня √
    // ==============================
    int pos = expr.indexOf("√");
    while (pos != -1) {
        if (pos + 1 < expr.length()) {
            if (expr[pos + 1] != '(') {
                QString number;
                int i = pos + 1;
                while (i < expr.length() && (expr[i].isDigit() || expr[i] == '.')) {
                    number += expr[i];
                    i++;
                }
                expr = expr.left(pos + 1) + "(" + number + ")" + expr.mid(i);
            }
            expr.replace(pos, 1, "Math.sqrt");
        }
        pos = expr.indexOf("√", pos + 1);
    }

    // ==============================
    // Обработка двойного факториала !!
    // ==============================
    pos = expr.indexOf("!!");
    while (pos != -1) {
        QString number;
        int i = pos - 1;

        // --- если перед !! идёт закрывающая скобка ---
        if (i >= 0 && expr[i] == ')') {
            int balance = 1;
            number = ")";
            i--;
            while (i >= 0 && balance > 0) {
                number = expr[i] + number;
                if (expr[i] == ')') balance++;
                else if (expr[i] == '(') balance--;
                i--;
            }
        }
        // --- иначе собираем число ---
        else {
            while (i >= 0 && (expr[i].isDigit() || expr[i] == '.')) {
                number = expr[i] + number;
                i--;
            }
        }

        expr = expr.left(i + 1) + "dubfactorial(" + number + ")" + expr.mid(pos + 2);
        pos = expr.indexOf("!!", i + 1);
    }

    pos = expr.indexOf("!");
    while (pos != -1) {
        // если ! стоит в начале или перед числом/скобкой
        if (pos == 0 || (!expr[pos - 1].isDigit() && expr[pos - 1] != ')')) {
            QString number;
            int i = pos + 1;

            // если после ! идёт скобка — находим конец выражения
            if (i < expr.length() && expr[i] == '(') {
                int balance = 1;
                number += '(';
                i++;
                while (i < expr.length() && balance > 0) {
                    number += expr[i];
                    if (expr[i] == '(') balance++;
                    else if (expr[i] == ')') balance--;
                    i++;
                }
            } else {
                // собираем число
                while (i < expr.length() && (expr[i].isDigit() || expr[i] == '.')) {
                    number += expr[i];
                    i++;
                }
            }

            expr = expr.left(pos) + "subfactorial(" + number + ")" + expr.mid(i);
        }
        pos = expr.indexOf("!", pos + 1);
    }

    // ==============================
    // Обработка факториала !
    // ==============================
    pos = expr.indexOf("!");
    while (pos != -1) {
        if (pos > 0) {
            if (expr[pos - 1] == ')') {
                int balance = 1;
                int j = pos - 2;
                while (j >= 0 && balance > 0) {
                    if (expr[j] == ')') balance++;
                    else if (expr[j] == '(') balance--;
                    j--;
                }

                if (balance == 0) {
                    expr = expr.left(j + 1) + "factorial" + expr.mid(j + 1, pos - j - 1) + expr.mid(pos + 1);
                    int insertPos = expr.indexOf("factorial", j + 1) + 9;
                    expr.insert(insertPos, "(");
                    int closePos = expr.indexOf(")", insertPos);
                    expr.insert(closePos + 1, ")");
                }
            } else {
                QString number;
                int i = pos - 1;
                while (i >= 0 && (expr[i].isDigit() || expr[i] == '.')) {
                    number = expr[i] + number;
                    i--;
                }
                expr = expr.left(i + 1) + "factorial(" + number + ")" + expr.mid(pos + 1);
            }
        }
        pos = expr.indexOf("!", pos + 1);
    }

    // ==============================
    // Определение функций в JS
    // ==============================
    QJSEngine engine;

    engine.evaluate(R"(
        function factorial(n) {
            if (n < 0 || n != Math.floor(n)) return NaN;
            if (n === 0) return 1;
            let r = 1;
            for (let i = 1; i <= n; i++) r *= i;
            return r;
        }

        function subfactorial(n) {
            if (n < 0 || n != Math.floor(n)) return NaN;
            if (n === 0) return 1;
            if (n === 1) return 0;

            let sum = 0;
            for (let k = 0; k <= n; k++) {
                sum += ((k % 2 === 0 ? 1 : -1) / factorial(k));
            }
            return Math.round(factorial(n) * sum);
        }

        function dubfactorial(n) {
            if (n < 0 || n != Math.floor(n)) return NaN;
            if (n === 0 || n === 1) return 1;
            let r = 1;
            for (let i = n; i > 1; i -= 2) r *= i;
            return r;
        }
    )");

    QJSValue result = engine.evaluate("(" + expr + ")");

    if (result.isError() || !result.isNumber()) {
        QMessageBox::warning(this, "Ошибка", "Неверное выражение!");
        return -9999;
    }

    double value = result.toNumber();
    if (std::floor(value) != value) {
        QMessageBox::warning(this, "Ошибка", "Результат не является целым числом!");
        return -9999;
    }

    return static_cast<int>(value);
}


// --- Проверка результата ---
void MainWindow::checkResult() {
    int result = calculateResult();
    if (result == target) {
        QMessageBox::information(this, "Result", "🎉 YOU WIN!");
        target++;
    } else {
        if (result == -9999) {
            QMessageBox::warning(this, "Result", QString("💀 YOU LOSE..."));

        } else {
            QMessageBox::warning(this, "Result", QString("💀 YOU LOSE... Result = %1").arg(result));
        }
    }

    // Сброс для нового уровня
    expr = QString("4_4_4_4=%1").arg(target);
    cursorPos = expr.length();
    updateUI();
}

